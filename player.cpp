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

#include "MediaASocketHandler.h"
#include "MediaExternal.h"
#include "MediaMutex.h"
#include "MediaRefCount.h"
#include "MediaRunnable.h"
#include "MediaSegmentExternal.h"
#include "MediaSocketTransportService.h"
#include "MediaThread.h"
#include "MediaTimer.h"

#include "PeerConnectionCtx.h"
#include "PeerConnectionImpl.h"

#include "nss.h"
#include "ssl.h"

#include "prthread.h"
#include "prerror.h"
#include "prio.h"

#include "json.h"
#include "render.h"

#define LOG(format, ...) fprintf(stderr, format, ##__VA_ARGS__);

const char JSONTerminator[] = "\r\n";
const int JSONTerminatorSize = sizeof(JSONTerminator) - 1;

namespace {

void
LogPRError()
{
  PRInt32 len = PR_GetErrorTextLength() + 1;
  if (len > 0) {
    char* buf = new char[len];
    memset(buf, 0, len);
    PR_GetErrorText(buf);
    LOG("PR Error: %s\n", buf);
    delete []buf;
  }
  else {
    LOG("PR Error number: %d\n", (int)PR_GetError());
  }
}

class PCObserver;
typedef media::MutexAutoLock MutexAutoLock;

struct State {
  mozilla::RefPtr<nsIDOMMediaStream> mStream;
  mozilla::RefPtr<sipcc::PeerConnectionImpl> mPeerConnection;
  mozilla::RefPtr<PCObserver> mPeerConnectionObserver;
  PRFileDesc* mSocket;
  State() : mSocket(nullptr) {}
  MEDIA_REF_COUNT_INLINE
};

static const int32_t PCOFFER = 0;
static const int32_t PCANSWER = 1;

class VideoSink : public Fake_VideoSink {
public:
  VideoSink(const mozilla::RefPtr<State>& aState) : mState(aState) {}
  virtual ~VideoSink() {}

  virtual void SegmentReady(media::MediaSegment* aSegment)
  {
    media::VideoSegment* segment = reinterpret_cast<media::VideoSegment*>(aSegment);
    if (segment && mState) {
      const media::VideoFrame *frame = segment->GetLastFrame();
      unsigned int size;
      const unsigned char *image = frame->GetImage(&size);
      if (size > 0) {
        int width = 0, height = 0;
        frame->GetWidthAndHeight(&width, &height);
        render::Draw(image, size, width, height);
      }
    }
  }
protected:
  mozilla::RefPtr<State> mState;
};

static const int sBufferLength = 2048;

class SocketHandler : public media::ASocketHandler {
MEDIA_REF_COUNT_INLINE
public:
  SocketHandler(mozilla::RefPtr<State>& aState) : mState(aState) {
    mPollFlags = PR_POLL_READ;
  }
  virtual void OnSocketReady(PRFileDesc *fd, int16_t outFlags);
  virtual void OnSocketDetached(PRFileDesc *fd);

protected:
  char mBuffer[sBufferLength];
  std::string mMessage;
  mozilla::RefPtr<State> mState;
};


class PCObserver : public PeerConnectionObserverExternal
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
  NS_IMETHODIMP OnAddIceCandidateSuccess(ER&)
  {
    LOG("OnAddIceCandidateSuccessn\n");
    return NS_OK;
  }
  NS_IMETHODIMP OnAddIceCandidateError(uint32_t code, const char *msg, ER&)
  {
    LOG("OnAddIceCandidateError: %u %s\n", code, msg);
    return NS_OK;
  }
  NS_IMETHODIMP OnIceCandidate(uint16_t level, const char *mid, const char *cand, ER&);

protected:
  mozilla::RefPtr<State> mState;
};

class PullTimer : public media::TimerCallback
{
MEDIA_REF_COUNT_INLINE
public:
  PullTimer(const mozilla::RefPtr<State>& aState) : mState(aState) {}
  // media::TimerCallback
  NS_IMETHOD Notify(media::Timer *timer);
protected:
  mozilla::RefPtr<State> mState;
};

class ProcessMessage : public media::Runnable {
public:
  ProcessMessage(mozilla::RefPtr<State>& aState) :
    mState(aState) {}
  virtual nsresult Run();
  void addMessage(const std::string& aMessage)
  {
    mMessageList.push_back(aMessage);
  }
protected:
  mozilla::RefPtr<State> mState;
  std::vector<std::string> mMessageList;
};

class DispatchSocketHandler : public media::Runnable {
public:
  DispatchSocketHandler(PRFileDesc* aSocket, mozilla::RefPtr<SocketHandler>& aHandler) :
    mSocket(aSocket),
    mHandler(aHandler) {}
  virtual nsresult Run(void)
  {
    mozilla::RefPtr<media::SocketTransportService> sts = media::GetSocketTransportService();
    sts->AttachSocket(mSocket, mHandler);
    return NS_OK;
  }
protected:
  PRFileDesc* mSocket;
  mozilla::RefPtr<SocketHandler> mHandler;
};

void
SocketHandler::OnSocketReady(PRFileDesc *fd, int16_t outFlags)
{
  static const std::string Term(JSONTerminator);
  if (outFlags & PR_POLL_READ) {
    memset(mBuffer, 0, sBufferLength);
    PRInt32 read = PR_Recv(fd, mBuffer, sBufferLength, 0, PR_INTERVAL_NO_WAIT);
    if (read > 0) {
      LOG("Received ->\n%s\n", mBuffer);
      mozilla::RefPtr<ProcessMessage> pmsg = new ProcessMessage(mState);
      mMessage += mBuffer;

      size_t start = 0;
      size_t found = mMessage.find(Term);
      if (found != std::string::npos) {
        while (found != std::string::npos) {
          pmsg->addMessage(mMessage.substr(start, found - start));
          start = found + JSONTerminatorSize;
          found = mMessage.find(Term, start);
        }
        // Save any partial messages until the rest is received.
        if (start < mMessage.length()) {
          std::string remainder = mMessage.substr(start);
          mMessage = remainder;
          LOG("Saved: '%s'\n", mMessage.c_str());
        }
        else {
          mMessage.clear();
        }

        NS_DispatchToMainThread(pmsg);
      }
    }
    else {
      static bool failed = false;
      if (!failed) {
        LOG("Socket failed to read data\n");
        failed = true;
      }
    }
  }
  else {
    LOG("Unknown outFlags: %d\n", (int)outFlags);
  }
}

void
SocketHandler::OnSocketDetached(PRFileDesc *fd)
{

}

NS_IMETHODIMP
PCObserver::OnCreateAnswerSuccess(const char* answer, ER&)
{
  if (answer && mState.get() && mState->mSocket && mState->mPeerConnection.get()) {
    mState->mPeerConnection->SetLocalDescription(PCANSWER, answer);
    LOG("Answer ->\n%s\n", answer);
    JSONGenerator gen;
    gen.openMap();
    gen.addPair("type", std::string("answer"));
    gen.addPair("sdp", std::string(answer));
    gen.closeMap();
    std::string value;
    if (gen.getJSON(value)) {
      PR_Send(mState->mSocket, value.c_str(), value.length(), 0, PR_INTERVAL_NO_TIMEOUT);
      PR_Send(mState->mSocket, JSONTerminator, JSONTerminatorSize, 0, PR_INTERVAL_NO_TIMEOUT);
    }
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
  mozilla::dom::PCImplIceConnectionState gotice;
  mozilla::dom::PCImplIceGatheringState goticegathering;
  mozilla::dom::PCImplSipccState gotsipcc;
  mozilla::dom::PCImplSignalingState gotsignaling;

  switch (state_type) {
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
PCObserver::OnIceCandidate(uint16_t level, const char *mid, const char *cand, ER&) {
  if (cand && (cand[0] != '\0')) {
    LOG("OnIceCandidate: candidate: %s mid: %s level: %d\n", cand, mid, (int)level);
    JSONGenerator gen;
    gen.openMap();
    gen.addPair("candidate", std::string(cand));
    gen.addPair("sdpMid", std::string(mid));
    gen.addPair("sdpMLineIndex", (int)level - 1);
    gen.closeMap();
    std::string value;
    if (gen.getJSON(value)) {
      LOG("Sending candidate JSON: %s\n", value.c_str());
      PRInt32 amount = PR_Send(mState->mSocket, value.c_str(), value.length(), 0, PR_INTERVAL_NO_TIMEOUT);
      if (amount < (PRInt32)value.length()) {
        LogPRError();
      }
      amount = PR_Send(mState->mSocket, JSONTerminator, JSONTerminatorSize, 0, PR_INTERVAL_NO_TIMEOUT);
      if (amount < JSONTerminatorSize) {
        LogPRError();
      }
    }
  }
  else {
    LOG("OnIceCandidate ignoring null ice candidate\n");
  }
  return NS_OK;
}

NS_IMETHODIMP
PullTimer::Notify(media::Timer *timer)
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

typedef std::vector<std::string>::size_type vsize_t;

nsresult
ProcessMessage::Run()
{
  if (NS_IsMainThread() == false) {
    LOG("ProcessMessage must be run on the main thread.\n");
    return NS_ERROR_FAILURE;
  }

  const vsize_t size = mMessageList.size();
  for (vsize_t ix = 0; ix < size; ix++) {
    std::string message = mMessageList[ix];
    JSONParser parse(message.c_str());
    std::string type;
    if (parse.find("type", type)) {
      std::string sdp;
      if ((type == "offer") && parse.find("sdp", sdp)) {
        mState->mPeerConnection->SetRemoteDescription(PCOFFER, sdp.c_str());
        mState->mPeerConnection->CreateAnswer();
      }
      else {
        LOG("ERROR: Failed to parse offer:\n%s\n", message.c_str());
      }
    }
    else {
      std::string candidate, mid;
      int index = 0;
      if (parse.find("candidate", candidate) && parse.find("sdpMid", mid) && parse.find("sdpMLineIndex", index)) {
        if (candidate[0] != '\0') {
          mState->mPeerConnection->AddIceCandidate(candidate.c_str(), mid.c_str(), (unsigned short)index + 1);
        }
        else {
          LOG("ERROR: Received NULL ice candidate:\n%s\n", message.c_str());
        }
      }
      else {
        LOG("ERROR: Ice candidate failed to parse: %s candidate:%s sdpMid:%s sdpMLineIndex:%s\n", message.c_str(), (parse.find("candidate", candidate) ? "True" : "False"), (parse.find("sdpMid", mid) ? "True" : "False"), (parse.find("sdpMLineIndex", index) ? "True" : "False"));
      }
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
      LOG("PR Error: %s\n", buf);
      delete []buf;
    }
    else {
      LOG("PR Error number: %d\n", (int)PR_GetError());
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
  PR_SetNetAddr(PR_IpAddrAny, PR_AF_INET, 8011, &addr);
  PRFileDesc* sock = PR_OpenTCPSocket(PR_AF_INET);

  if (!sock) {
    LOG("ERROR: Failed to create socket\n.");
  }

  PRSocketOptionData opt;

  opt.option = PR_SockOpt_Reuseaddr;
  opt.value.reuse_addr = true;
  PR_SetSocketOption(sock, &opt);

  CheckPRError(PR_Bind(sock, &addr));

  if (CheckPRError(PR_Listen(sock, 5))) {
    state->mSocket = PR_Accept(sock, &addr, PR_INTERVAL_NO_TIMEOUT);
    PR_Shutdown(sock, PR_SHUTDOWN_BOTH);
    PR_Close(sock);
    sock = nullptr;
  }
  else {
    exit(-1);
  }

  if (!state->mSocket) {
    LOG("ERROR: Failed to create socket\n");
    exit(-1);
  }

  sipcc::IceConfiguration cfg;

  state->mPeerConnection = sipcc::PeerConnectionImpl::CreatePeerConnection();
  state->mPeerConnectionObserver = new PCObserver(state);
  state->mPeerConnection->Initialize(*(state->mPeerConnectionObserver), nullptr, cfg, NS_GetCurrentThread());

  mozilla::RefPtr<media::Timer> timer = media::CreateTimer();;
  mozilla::RefPtr<PullTimer> pull = new PullTimer(state);

  timer->InitWithCallback(
    pull,
    PR_MillisecondsToInterval(16),
    media::Timer::TYPE_REPEATING_PRECISE);

  mozilla::RefPtr<SocketHandler> handler = new SocketHandler(state);
  mozilla::RefPtr<DispatchSocketHandler> dispatch = new DispatchSocketHandler(state->mSocket, handler);
  mozilla::RefPtr<media::EventTarget> sts = media::GetSocketTransportServiceTarget();
  sts->Dispatch(dispatch, MEDIA_DISPATCH_NORMAL);

  render::Initialize();
  while (render::KeepRunning()) { NS_ProcessNextEvent(nullptr, true); }

  if (state->mStream) { state->mStream->GetStream()->AsSourceStream()->StopStream(); }
  state->mPeerConnection->CloseStreams();
  state->mPeerConnection->Close();
  state->mPeerConnection = nullptr;

  render::Shutdown();
  media::Shutdown();

  return 0;
}
