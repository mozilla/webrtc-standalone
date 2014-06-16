#include <queue>
#include <string>
#include <sstream>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>        // For mode constants
#include <unistd.h>

#include "mozilla/RefPtr.h"
#include "FakeMediaStreams.h"

#include "MediaExternal.h"
#include "MediaMutex.h"
#include "MediaRefCount.h"
#include "MediaRunnable.h"
#include "MediaSegmentExternal.h"
#include "MediaThread.h"
#include "MediaTimer.h"

#include "PeerConnectionCtx.h"
#include "PeerConnectionImpl.h"

#include "nss.h"
#include "ssl.h"

#include "prthread.h"
#include "prerror.h"
#include "prio.h"

#include "render.h"

#define LOG(format, ...) fprintf(stderr, format, ##__VA_ARGS__);

namespace {

class PCObserver;
typedef media::MutexAutoLock MutexAutoLock;

struct State {
  mozilla::RefPtr<nsIDOMMediaStream> mStream;
  mozilla::RefPtr<sipcc::PeerConnectionImpl> mPeerConnection;
  mozilla::RefPtr<PCObserver> mPeerConnectionObserver;
  PRFileDesc* mSock;
  State() : mSock(nullptr) {}
  MEDIA_REF_COUNT_INLINE
};

static const int32_t PCOFFER = 0;
static const int32_t PCANSWER = 1;

class VideoSink : public Fake_VideoSink {
public:
  VideoSink(const mozilla::RefPtr<State>& aState) : mState(aState) {}
  virtual ~VideoSink() {}

  virtual void SegmentReady( media::MediaSegment* aSegment)
  {
    media::VideoSegment* segment = reinterpret_cast<media::VideoSegment*>(aSegment);
    if (segment && mState) {
      const media::VideoFrame *frame = segment->GetLastFrame();
      unsigned int size;
      const unsigned char *image = frame->GetImage(&size);
      if (size > 0) {
LOG("Got IMAGE!\n");
        render::Draw(image, size, 640, 480);
      }
    }
  }
protected:
  mozilla::RefPtr<State> mState;
};

class PCObserver : public PeerConnectionObserverExternal, public media::TimerCallback
{
MEDIA_REF_COUNT_INLINE
public:
  PCObserver(const mozilla::RefPtr<State>& aState) : mState(aState) {}

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
  mozilla::RefPtr<State> mState;
};

NS_IMETHODIMP
PCObserver::OnCreateAnswerSuccess(const char* answer, ER&)
{
  if (answer && mState.get() && mState->mSock && mState->mPeerConnection.get()) {
    mState->mPeerConnection->SetLocalDescription(PCANSWER, answer);
    std::string out(answer);
    PR_Send(mState->mSock, answer, strlen(answer), 0, PR_INTERVAL_NO_TIMEOUT);
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

bool
CheckPRError(PRStatus result)
{
  if (result != PR_SUCCESS) {
    PRInt32 len = PR_GetErrorTextLength() + 1;
    if (len > 0) {
      char* buf = new char[len];
      memset(buf, 0, len);
      PR_GetErrorText(buf);
      fprintf(stderr, "Error: %s\n", buf);
      delete []buf;
    }
    else {
      fprintf(stderr, "Error number: %d\n", (int)PR_GetError());
    }
    return false;
  }
  return true;
}

int
main(int argc, char* argv[])
{
  media::Initialize();
  NSS_NoDB_Init(nullptr);
  NSS_SetDomesticPolicy();
  mozilla::RefPtr<State> state = new State;

  PRNetAddr addr;
  memset(&addr, 0, sizeof(addr));
  PR_SetNetAddr(PR_IpAddrAny, PR_AF_INET, 8088, &addr);
  PRFileDesc* sock = PR_OpenTCPSocket(PR_AF_INET);

  if (!sock) {
    fprintf(stderr, "Failed to create socket\n.");
  }

  PRSocketOptionData opt;

  opt.option = PR_SockOpt_Reuseaddr;
  opt.value.reuse_addr = true;
  PR_SetSocketOption(sock, &opt);

//  opt.option = PR_SockOpt_Blocking;
//  opt.value.non_blocking = true;
//  PR_SetSocketOption(sock, &opt);

  CheckPRError(PR_Bind(sock, &addr));

  if (CheckPRError(PR_Listen(sock, 5))) {
//    memset(&addr, 0, sizeof(addr));
    state->mSock = PR_Accept(sock, &addr, PR_INTERVAL_NO_TIMEOUT);
    PR_Shutdown(sock, PR_SHUTDOWN_BOTH);
    PR_Close(sock);
    sock = nullptr;
  }
  else {
    exit(-1);
  }

  const int len = 2048;
  char offer[len];
  memset(offer, 0, len);

  if (state->mSock) {
    PRInt32 read = PR_Recv(state->mSock, offer, len, 0, PR_INTERVAL_NO_TIMEOUT);
    if (read > 0) {
      fprintf(stderr, "Received ->\n%s\n", offer);
    }
    else {
      fprintf(stderr, "Failed to read data\n");
      exit(-1);
    }
  }
  else {
    LOG("Failed to get socket\n");
    exit(-1);
  }
  sipcc::IceConfiguration cfg;

  state->mPeerConnection = sipcc::PeerConnectionImpl::CreatePeerConnection();
  state->mPeerConnectionObserver = new PCObserver(state);
  state->mPeerConnection->Initialize(*(state->mPeerConnectionObserver), nullptr, cfg, NS_GetCurrentThread());

  mozilla::RefPtr<media::Timer> timer = media::CreateTimer();;

  timer->InitWithCallback(
    state->mPeerConnectionObserver,
    PR_MillisecondsToInterval(30),
    media::Timer::TYPE_REPEATING_PRECISE);

  state->mPeerConnection->SetRemoteDescription(PCOFFER, offer);
  sipcc::MediaConstraintsExternal constraints;
  state->mPeerConnection->CreateAnswer(constraints);

  render::Initialize();
  while (render::KeepRunning()) { NS_ProcessNextEvent(nullptr, true); }

  render::Shutdown();
  media::Shutdown();

  return 0;
}
