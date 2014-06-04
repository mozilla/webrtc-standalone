#include "WebRTCCall.h"

#include <queue>
#include <string>
#include <sstream>

#include <errno.h>
#include <fcntl.h>           // For O_* constants
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>        // For mode constants
#include <unistd.h>

#include "FakeMediaStreams.h"
//#include "FakeMediaStreamsImpl.h"
#include "MediaExternal.h"
#include "MediaMutex.h"
#include "MediaRunnable.h"
#include "MediaThread.h"
#include "MediaTimer.h"
#include "MediaSegmentExternal.h"
#include "nss.h"
#include "PeerConnectionImpl.h"
#include "PeerConnectionCtx.h"
#include "ssl.h"
#include "prthread.h"

namespace {
class PCObserver;
typedef media::MutexAutoLock MutexAutoLock;
}

class WebRTCCall::State {
public:
  State();
  ~State();

  void AddEvent(WebRTCCallEvent* event)
  {
    if (event) {
      MutexAutoLock lock(mEventsLock);
      mEvents.push(event);
    }
    if (mSystem && event) {
      mSystem->NotifyNextEventReady();
    }
  }

  void NextEvent()
  {
    WebRTCCallEvent* event = nullptr;
    {
      MutexAutoLock lock(mEventsLock);
      event = mEvents.front();
      if (event) { mEvents.pop(); }
    }
    if (event) {
      event->Run();
      delete event;
    }
  }

  bool mRunThread;

  WebRTCCallSystem* mSystem;
  WebRTCCallObserver* mCallObserver;
  mozilla::RefPtr<nsIDOMMediaStream> mStream;
  mozilla::RefPtr<media::Thread> mThread;
  mozilla::RefPtr<media::Timer> mTimer;
  mozilla::RefPtr<sipcc::PeerConnectionImpl> mPeerConnection;
  mozilla::RefPtr<PCObserver> mPeerConnectionObserver;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(State);

protected:
  static void* WorkFunc(void*);
  media::Mutex mEventsLock;
  std::queue<WebRTCCallEvent*> mEvents;
};


namespace {

static const int32_t PCOFFER = 0;
static const int32_t PCANSWER = 1;

class CallEvent : public WebRTCCallEvent {
public:
  virtual ~CallEvent() {}
protected:
  CallEvent(const mozilla::RefPtr<WebRTCCall::State>& aState) : mState(aState) {}

  mozilla::RefPtr<WebRTCCall::State> mState;
};

class ReceiveAnswerEvent : public CallEvent {
public:
  ReceiveAnswerEvent(const mozilla::RefPtr<WebRTCCall::State>& aState, const std::string& aAnswer)
      : CallEvent(aState)
      , mAnswer(aAnswer) {}

  virtual void Run()
  {
    if (mState.get() && mState->mCallObserver) {
      mState->mCallObserver->ReceiveAnswer(mAnswer);
    }
  }
protected:
  std::string mAnswer;
};

class ReceiveNextVideoFrameEvent : public CallEvent {
public:
  ReceiveNextVideoFrameEvent(const mozilla::RefPtr<WebRTCCall::State>& aState) : CallEvent(aState),  mBuffer(nullptr), mBufferSize(0) {}

  virtual ~ReceiveNextVideoFrameEvent()
  {
    delete []mBuffer;
    mBuffer = 0;
  }

  ReceiveNextVideoFrameEvent(
      const mozilla::RefPtr<WebRTCCall::State>& aState,
      const unsigned char* aBuffer,
      const int32_t aBufferSize)
      : CallEvent(aState)
      , mBuffer(nullptr)
      , mBufferSize(aBufferSize)
  {
    if (aBufferSize && aBuffer) {
      mBuffer = new unsigned char[aBufferSize];
      if (mBuffer) {
        memcpy((void*)mBuffer, (const void*)aBuffer, aBufferSize);
      }
    }
  }

  virtual void Run()
  {
    if (mState.get() && mState->mCallObserver) {
      mState->mCallObserver->ReceiveNextVideoFrame(mBuffer, mBufferSize);
    }
  }
protected:
  unsigned char* mBuffer;
  int32_t mBufferSize;
};

class StopThreadEvent : public media::Runnable {
MEDIA_REF_COUNT_INLINE
public:

  StopThreadEvent(const mozilla::RefPtr<WebRTCCall::State>& aState) : mState(aState) {}

  NS_IMETHOD Run()
  {
    mState->mRunThread = false;
    return NS_OK;
  }

  virtual ~StopThreadEvent() {}
protected:
  mozilla::RefPtr<WebRTCCall::State> mState;
};


class RemoteDescriptionEvent : public media::Runnable {
MEDIA_REF_COUNT_INLINE
public:

  RemoteDescriptionEvent(
      mozilla::RefPtr<WebRTCCall::State>& aState,
      const std::string& aDescription,
      const int aAction)
      : mState(aState)
      , mDescription(aDescription)
      , mAction(aAction) {}

  NS_IMETHOD Run()
  {
    if (mState.get() && mState->mPeerConnection.get()) {
      mState->mPeerConnection->SetRemoteDescription(mAction, mDescription.c_str());
      if (mAction == PCOFFER) {
        sipcc::MediaConstraintsExternal constraints;
        mState->mPeerConnection->CreateAnswer(constraints);
      }
    }
    return NS_OK;
  }

  virtual ~RemoteDescriptionEvent() {}
protected:
  mozilla::RefPtr<WebRTCCall::State> mState;
  std::string mDescription;
  int mAction;
};

class VideoSink : public Fake_VideoSink {
public:
  VideoSink(const mozilla::RefPtr<WebRTCCall::State>& aState) : mState(aState) {}
  virtual ~VideoSink() {}

  virtual void SegmentReady( media::MediaSegment* aSegment)
  {
    media::VideoSegment* segment = reinterpret_cast<media::VideoSegment*>(aSegment);
    if (segment && mState) {
      const media::VideoFrame *frame = segment->GetLastFrame();
      unsigned int size;
      const unsigned char *image = frame->GetImage(&size);
      if (size > 0) {
          mState->AddEvent(new ReceiveNextVideoFrameEvent(mState, image, size));
      }
    }
  }
protected:
  mozilla::RefPtr<WebRTCCall::State> mState;
};

class PCObserver : public PeerConnectionObserverExternal, public media::TimerCallback
{
MEDIA_REF_COUNT_INLINE
public:
  PCObserver(const mozilla::RefPtr<WebRTCCall::State>& aState) : mState(aState) {}

  // PeerConnectionObserverExternal
  NS_IMETHODIMP OnCreateOfferSuccess(const char* offer, ER&) { return NS_OK; }
  NS_IMETHODIMP OnCreateOfferError(uint32_t code, const char *msg, ER&) { return NS_OK; }
  NS_IMETHODIMP OnCreateAnswerSuccess(const char* answer, ER&);
  NS_IMETHODIMP OnCreateAnswerError(uint32_t code, const char *msg, ER&);
  NS_IMETHODIMP OnSetLocalDescriptionSuccess(ER&) { return NS_OK; }
  NS_IMETHODIMP OnSetRemoteDescriptionSuccess(ER&) { return NS_OK; }
  NS_IMETHODIMP OnSetLocalDescriptionError(uint32_t code, const char *msg, ER&) { return NS_OK; }
  NS_IMETHODIMP OnSetRemoteDescriptionError(uint32_t code, const char *msg, ER&) { return NS_OK; }
  NS_IMETHODIMP NotifyConnection(ER&) { return NS_OK; }
  NS_IMETHODIMP NotifyClosedConnection(ER&) { return NS_OK; }
  NS_IMETHODIMP NotifyDataChannel(nsIDOMDataChannel *channel, ER&) { return NS_OK; }
  NS_IMETHODIMP OnStateChange(mozilla::dom::PCObserverStateType state_type, ER&, void*);
  NS_IMETHODIMP OnAddStream(nsIDOMMediaStream *stream, ER&);
  NS_IMETHODIMP OnRemoveStream(ER&);
  NS_IMETHODIMP OnAddTrack(ER&) { return NS_OK; }
  NS_IMETHODIMP OnRemoveTrack(ER&) { return NS_OK; }
  NS_IMETHODIMP OnAddIceCandidateSuccess(ER&) { return NS_OK; }
  NS_IMETHODIMP OnAddIceCandidateError(uint32_t code, const char *msg, ER&) { return NS_OK; }
  NS_IMETHODIMP OnIceCandidate(uint16_t level, const char *mid, const char *cand, ER&) { return NS_OK; }

  // media::TimerCallback
  NS_IMETHOD Notify(media::Timer *timer);

protected:
  mozilla::RefPtr<WebRTCCall::State> mState;
};

NS_IMETHODIMP
PCObserver::OnCreateAnswerSuccess(const char* answer, ER&)
{
  if (mState.get() && mState->mPeerConnection.get()) {
    mState->mPeerConnection->SetLocalDescription(PCANSWER, answer);
    std::string out(answer);
    mState->AddEvent(new ReceiveAnswerEvent(mState, out));
  }

  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnCreateAnswerError(uint32_t code, const char *message, ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnStateChange(mozilla::dom::PCObserverStateType state_type, ER&, void*)
{
  if (!mState || mState->mPeerConnection.get()) {
    return NS_OK;
  }

  nsresult rv;
  mozilla::dom::PCImplReadyState gotready;
  mozilla::dom::PCImplIceConnectionState gotice;
  mozilla::dom::PCImplIceGatheringState goticegathering;
  mozilla::dom::PCImplSipccState gotsipcc;
  mozilla::dom::PCImplSignalingState gotsignaling;

  switch (state_type) {
  case mozilla::dom::PCObserverStateType::ReadyState:
    rv = mState->mPeerConnection->ReadyState(&gotready);
    MEDIA_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::IceConnectionState:
    rv = mState->mPeerConnection->IceConnectionState(&gotice);
    MEDIA_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::IceGatheringState:
    rv = mState->mPeerConnection->IceGatheringState(&goticegathering);
    MEDIA_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::SdpState:
    // MEDIA_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::SipccState:
    rv = mState->mPeerConnection->SipccState(&gotsipcc);
    MEDIA_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::SignalingState:
    rv = mState->mPeerConnection->SignalingState(&gotsignaling);
    MEDIA_ENSURE_SUCCESS(rv, rv);
    break;
  default:
    // Unknown State
    MOZ_CRASH("Unknown state change type.");
    break;
  }
  return NS_OK;
}


NS_IMETHODIMP
PCObserver::OnAddStream(nsIDOMMediaStream *stream, ER&)
{
  mState->mStream = stream;

  Fake_DOMMediaStream* fake = reinterpret_cast<Fake_DOMMediaStream*>(mState->mStream.get());
  if (fake) {
    Fake_MediaStream* ms = reinterpret_cast<Fake_MediaStream*>(fake->GetStream());
    Fake_SourceMediaStream* sms = ms->AsSourceStream();
    if (sms) {
      mozilla::RefPtr<Fake_VideoSink> sink = new VideoSink(mState);
      sms->AddVideoSink(sink);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnRemoveStream(ER&)
{
  // Not being called?
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::Notify(media::Timer *timer)
{
  if (mState.get()) {
    Fake_DOMMediaStream* fake = reinterpret_cast<Fake_DOMMediaStream*>(mState->mStream.get());
    if (fake) {
      Fake_MediaStream* ms = reinterpret_cast<Fake_MediaStream*>(fake->GetStream());
      static uint64_t value = 0;
      if (ms) { ms->NotifyPull(nullptr, value++); }
    }
  }
  return NS_OK;
}

} // namespace

WebRTCCall::WebRTCCall() : mState(new State)
{
  if (mState) { mState->AddRef(); }
}

WebRTCCall::WebRTCCall(WebRTCCallSystem& aSystem, WebRTCCallObserver& aObserver)
    : mState(new State)
{
  if (mState) {
    mState->AddRef();
    mState->mSystem = &aSystem;
    mState->mCallObserver = &aObserver;
  }
}

WebRTCCall::~WebRTCCall()
{
  if (mState) {
    if (mState->mThread.get()) {
      mozilla::RefPtr<StopThreadEvent> event = new StopThreadEvent(mState);
      mState->mThread->Dispatch(event, media::EventTarget::DISPATCH_NORMAL);
    }
    mState->Release();
    mState = nullptr;
  }
}

void
WebRTCCall::AddEvent(WebRTCCallEvent* event)
{
  if (mState) { mState->AddEvent(event); }
  else { delete event; }
}

void
WebRTCCall::ProcessNextEvent()
{
  if (mState) { mState->NextEvent(); }
}

void
WebRTCCall::RegisterSystem(WebRTCCallSystem& aSystem)
{
  if (mState && !mState->mSystem) { mState->mSystem = &aSystem; }
}

void
WebRTCCall::ReleaseSystem(WebRTCCallSystem& aSystem)
{
  if (mState && (mState->mSystem == &aSystem)) { mState->mSystem = nullptr; }
}

void
WebRTCCall::RegisterObserver(WebRTCCallObserver& aObserver)
{
  if (mState && !mState->mCallObserver) { mState->mCallObserver = &aObserver; }
}

void
WebRTCCall::ReleaseObserver(WebRTCCallObserver& aObserver)
{
  if (mState && (mState->mCallObserver == &aObserver)) { mState->mCallObserver = nullptr; }
}

void
WebRTCCall::SetOffer(const std::string& aOffer)
{
  if (mState && mState->mThread.get()) {
    mozilla::RefPtr<State> pState(mState);
    mozilla::RefPtr<RemoteDescriptionEvent> event = new RemoteDescriptionEvent(pState, aOffer, PCOFFER);
    mState->mThread->Dispatch(event, media::EventTarget::DISPATCH_NORMAL);
  }
}

WebRTCCall::State::State()
    : mRunThread(true)
    , mSystem(nullptr)
    , mCallObserver(nullptr)
    , mStream(nullptr)
    , mEventsLock("")
{
  pthread_t t;
  pthread_create(&t, nullptr, WorkFunc, static_cast<void*>(this));
}

WebRTCCall::State::~State()
{
  // No need to grab mutex since no one else should be accessing the State.
  while(!mEvents.empty()) {
    WebRTCCallEvent* event = mEvents.front();
    mEvents.pop();
    delete event;
  }
}

void*
WebRTCCall::State::WorkFunc(void* ptr)
{
  mozilla::RefPtr<State> self = static_cast<State*>(ptr);

//  NS_InitXPCOM2(nullptr, nullptr, nullptr);
  media::Initialize();
  NSS_NoDB_Init(nullptr);
  NSS_SetDomesticPolicy();

  self->mThread = NS_GetCurrentThread();

  sipcc::IceConfiguration cfg;

  self->mPeerConnection = sipcc::PeerConnectionImpl::CreatePeerConnection();
  self->mPeerConnectionObserver = new PCObserver(self);
  self->mPeerConnection->Initialize(*(self->mPeerConnectionObserver), nullptr, cfg, self->mThread.get());

  self->mTimer = media::CreateTimer();;

  self->mTimer->InitWithCallback(
      self->mPeerConnectionObserver,
      PR_MillisecondsToInterval(30),
      media::Timer::TYPE_REPEATING_PRECISE);

  while (self->mRunThread) { NS_ProcessNextEvent(nullptr, true); }

  media::Shutdown();

  return nullptr;
}
