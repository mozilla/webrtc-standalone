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
#include "FakeMediaStreamsImpl.h"
#include "FakePCObserver.h"

#include "prio.h"
#include "nsASocketHandler.h"
#include "mozilla/Mutex.h"
#include "nsRefPtr.h"
#include "nsIRunnable.h"
#include "VideoSegment.h"
#include "nsISocketTransportService.h"
#include "nsIThread.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"

#include "PeerConnectionCtx.h"
#include "PeerConnectionImpl.h"

#include "XPCOMRTInit.h"

#include "nss.h"
#include "ssl.h"

#include "prthread.h"
#include "prerror.h"

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
typedef mozilla::MutexAutoLock MutexAutoLock;

struct State {
  mozilla::RefPtr<mozilla::DOMMediaStream> mStream;
  mozilla::RefPtr<mozilla::PeerConnectionImpl> mPeerConnection;
  mozilla::RefPtr<PCObserver> mPeerConnectionObserver;
  PRFileDesc* mSocket;
  State() : mSocket(nullptr) {}
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(State)
protected:
  ~State(){}
};

static const int32_t PCOFFER = 0;
static const int32_t PCANSWER = 1;

class VideoSink : public Fake_VideoSink {
public:
  VideoSink(const mozilla::RefPtr<State>& aState) : mState(aState) {}
  virtual ~VideoSink() {}

  virtual void SegmentReady(mozilla::MediaSegment* aSegment)
  {
    mozilla::VideoSegment* segment = reinterpret_cast<mozilla::VideoSegment*>(aSegment);
    if (segment && mState) {
      const mozilla::VideoFrame *frame = segment->GetLastFrame();
      unsigned int size;
      nsRefPtr<mozilla::SimpleImageBuffer> buf = frame->GetImage();
      const unsigned char *image = buf->GetImage(&size);
      if (size > 0) {
        int width = 0, height = 0;
        buf->GetWidthAndHeight(&width, &height);
        render::Draw(image, size, width, height);
      }
    }
  }
protected:
  mozilla::RefPtr<State> mState;
};

static const int sBufferLength = 2048;

class SocketHandler : public nsASocketHandler {
NS_DECL_THREADSAFE_ISUPPORTS
public:
  SocketHandler(mozilla::RefPtr<State>& aState) : mState(aState), mSent(0), mReceived(0) {
    mPollFlags = PR_POLL_READ;
  }
  virtual void OnSocketReady(PRFileDesc *fd, int16_t outFlags);
  virtual void OnSocketDetached(PRFileDesc *fd);
  virtual void IsLocal(bool *aIsLocal) { if (aIsLocal) { *aIsLocal = false; } }
  virtual uint64_t ByteCountSent() { return mSent; }
  virtual uint64_t ByteCountReceived() { return mReceived; }

protected:
  virtual ~SocketHandler() {}
  char mBuffer[sBufferLength];
  std::string mMessage;
  mozilla::RefPtr<State> mState;
  uint64_t mSent;
  uint64_t mReceived;
};

NS_IMPL_ISUPPORTS0(SocketHandler)

class PCObserver : public test::AFakePCObserver
{
NS_DECL_THREADSAFE_ISUPPORTS
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
  NS_IMETHODIMP OnAddStream(mozilla::DOMMediaStream *stream, ER&);
  NS_IMETHODIMP OnRemoveStream(ER&);
  NS_IMETHODIMP OnReplaceTrackSuccess(ER&) { return NS_OK; }
  NS_IMETHODIMP OnReplaceTrackError(uint32_t code, const char *msg, ER&) { return NS_OK; }
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
  virtual ~PCObserver() {}
  mozilla::RefPtr<State> mState;
};

NS_IMPL_ISUPPORTS(PCObserver, nsISupportsWeakReference)

class PullTimer : public nsITimerCallback
{
NS_DECL_THREADSAFE_ISUPPORTS
public:
  PullTimer(const mozilla::RefPtr<State>& aState) : mState(aState) {}
  NS_IMETHOD Notify(nsITimer *timer);
protected:
  virtual ~PullTimer() {}
  mozilla::RefPtr<State> mState;
};

NS_IMPL_ISUPPORTS(PullTimer, nsITimerCallback)

class ProcessMessage : public nsIRunnable {
NS_DECL_THREADSAFE_ISUPPORTS
NS_DECL_NSIRUNNABLE
public:
  ProcessMessage(mozilla::RefPtr<State>& aState) :
    mState(aState) {}
  void addMessage(const std::string& aMessage)
  {
    mMessageList.push_back(aMessage);
  }
protected:
  virtual ~ProcessMessage(){}
  mozilla::RefPtr<State> mState;
  std::vector<std::string> mMessageList;
};

NS_IMPL_ISUPPORTS(ProcessMessage, nsIRunnable)

class DispatchSocketHandler : public nsIRunnable {
NS_DECL_THREADSAFE_ISUPPORTS
NS_DECL_NSIRUNNABLE
public:
  DispatchSocketHandler(PRFileDesc* aSocket, mozilla::RefPtr<SocketHandler>& aHandler) :
    mSocket(aSocket),
    mHandler(aHandler) {}
protected:
  virtual ~DispatchSocketHandler() {}
  PRFileDesc* mSocket;
  mozilla::RefPtr<SocketHandler> mHandler;
};

NS_IMPL_ISUPPORTS(DispatchSocketHandler, nsIRunnable)

nsresult
DispatchSocketHandler::Run(void)
{
  nsresult rv;
  nsCOMPtr<nsISocketTransportService> sts = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  sts->AttachSocket(mSocket, mHandler);
  return NS_OK;
}

void
SocketHandler::OnSocketReady(PRFileDesc *fd, int16_t outFlags)
{
  static const std::string Term(JSONTerminator);
  if (outFlags & PR_POLL_READ) {
    memset(mBuffer, 0, sBufferLength);
    PRInt32 read = PR_Recv(fd, mBuffer, sBufferLength, 0, PR_INTERVAL_NO_WAIT);
    if (read > 0) {
      mReceived += read;
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
      mCondition = NS_BASE_STREAM_CLOSED;
      static bool failed = false;
      if (!failed) {
        LOG("No data on socket ready. Assuming connection lost.\n");
        failed = true;
      }
    }
  }
  if (outFlags & PR_POLL_WRITE) {
     LOG("\n*** PR_POLL_WRITE\n");
  }
  if (outFlags & PR_POLL_EXCEPT) {
     LOG("\n*** PR_POLL_EXCEPT\n");
  }
  if (outFlags & PR_POLL_ERR) {
     LOG("\n*** PR_POLL_ERR\n");
  }
  if (outFlags & PR_POLL_NVAL) {
     LOG("\n*** PR_POLL_NVAL\n");
  }
  if (outFlags & PR_POLL_HUP) {
     LOG("\n*** PR_POLL_HUP\n");
  }
}

void
SocketHandler::OnSocketDetached(PRFileDesc *fd)
{
  LOG("Socket Detached!\n");
  if (fd == mState->mSocket) {
    PR_Close(fd);
    mState->mSocket = 0;
  }
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
  mozilla::dom::PCImplSignalingState gotsignaling;

  switch (state_type) {
  case mozilla::dom::PCObserverStateType::IceConnectionState:
    rv = mState->mPeerConnection->IceConnectionState(&gotice);
    NS_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::IceGatheringState:
    rv = mState->mPeerConnection->IceGatheringState(&goticegathering);
    NS_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::SdpState:
    // NS_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::SignalingState:
    rv = mState->mPeerConnection->SignalingState(&gotsignaling);
    NS_ENSURE_SUCCESS(rv, rv);
    break;
  default:
    // Unknown State
    MOZ_CRASH("Unknown state change type.");
    break;
  }
  return NS_OK;
}


NS_IMETHODIMP
PCObserver::OnAddStream(mozilla::DOMMediaStream *stream, ER&)
{
  mState->mStream = stream;

  Fake_DOMMediaStream* fake = reinterpret_cast<Fake_DOMMediaStream*>(mState->mStream.get());
  if (fake) {
    Fake_MediaStream* ms = reinterpret_cast<Fake_MediaStream*>(fake->GetStream());
    Fake_SourceMediaStream* sms = ms->AsSourceStream();
    if (sms) {
      nsRefPtr<Fake_VideoSink> sink = new VideoSink(mState);
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
    gen.addPair("sdpMLineIndex", (int)level);
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
PullTimer::Notify(nsITimer *timer)
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
          mState->mPeerConnection->AddIceCandidate(candidate.c_str(), mid.c_str(), (unsigned short)index);
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
  NS_InitXPCOMRT();
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

  mozilla::IceConfiguration cfg;

  state->mPeerConnection = mozilla::PeerConnectionImpl::CreatePeerConnection();
  state->mPeerConnectionObserver = new PCObserver(state);
  state->mPeerConnection->Initialize(*(state->mPeerConnectionObserver), nullptr, cfg, NS_GetCurrentThread());

  nsresult rv;

  nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  mozilla::RefPtr<PullTimer> pull = new PullTimer(state);

  timer->InitWithCallback(
    pull,
    PR_MillisecondsToInterval(16),
    nsITimer::TYPE_REPEATING_PRECISE);

  mozilla::RefPtr<SocketHandler> handler = new SocketHandler(state);
  mozilla::RefPtr<DispatchSocketHandler> dispatch = new DispatchSocketHandler(state->mSocket, handler);
  nsCOMPtr<nsISocketTransportService> sts = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  nsCOMPtr<nsIEventTarget> ststhread = do_QueryInterface(sts, &rv);
  ststhread->Dispatch(dispatch, NS_DISPATCH_NORMAL);

  render::Initialize();
  while (render::KeepRunning()) { NS_ProcessNextEvent(nullptr, true); }

  if (state->mStream) { state->mStream->GetStream()->AsSourceStream()->StopStream(); }
  state->mPeerConnection->CloseStreams();
  state->mPeerConnection->Close();
  state->mPeerConnection = nullptr;

  render::Shutdown();
  NS_ShutdownXPCOMRT();

  return 0;
}
