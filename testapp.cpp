#include <iostream>
#include <map>
#include <algorithm>
#include <string>
#include <unistd.h>

#include "base/basictypes.h"
#include "logging.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"
#include "gtest_utils.h"

#include "nspr.h"
#include "nss.h"
#include "ssl.h"
#include "prthread.h"

#include "FakeMediaStreams.h"
#include "FakeMediaStreamsImpl.h"
#include "cpr_stdlib.h"
#include "PeerConnectionImpl.h"
#include "PeerConnectionCtx.h"
#include "PeerConnectionMedia.h"
#include "MediaPipeline.h"
#include "runnable_utils.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"
#include "base/message_loop.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsNetUtil.h"
#include "nsIIOService.h"
#include "nsIDNSService.h"
#include "nsITimer.h"
#include "nsWeakReference.h"
#include "nsXPCOM.h"
#include "nsXULAppAPI.h"
#include "nricectx.h"
#include "rlogringbuffer.h"
#include "mozilla/SyncRunnable.h"
#include "logging.h"
#include "stunserver.h"
#include "stunserver.cpp"

#include <stdio.h>


#define LOG(format, ...) fprintf(stderr, format, ##__VA_ARGS__);
#define REPLY(format, ...) \
  fprintf(stdout, format, ##__VA_ARGS__); \
  fprintf(stdout, "\n"); \
  fflush(stdout);

class PCObserver : public PeerConnectionObserverExternal, public nsITimerCallback
{
public:
  PCObserver(sipcc::PeerConnectionImpl* aPc) : mPc(aPc), mStream(nullptr) {}

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_IMETHODIMP OnCreateOfferSuccess(const char* offer, ER&);
  NS_IMETHODIMP OnCreateOfferError(uint32_t code, const char *msg, ER&);
  NS_IMETHODIMP OnCreateAnswerSuccess(const char* answer, ER&);
  NS_IMETHODIMP OnCreateAnswerError(uint32_t code, const char *msg, ER&);
  NS_IMETHODIMP OnSetLocalDescriptionSuccess(ER&);
  NS_IMETHODIMP OnSetRemoteDescriptionSuccess(ER&);
  NS_IMETHODIMP OnSetLocalDescriptionError(uint32_t code, const char *msg, ER&);
  NS_IMETHODIMP OnSetRemoteDescriptionError(uint32_t code, const char *msg, ER&);
  NS_IMETHODIMP NotifyConnection(ER&);
  NS_IMETHODIMP NotifyClosedConnection(ER&);
  NS_IMETHODIMP NotifyDataChannel(nsIDOMDataChannel *channel, ER&);
  NS_IMETHODIMP OnStateChange(mozilla::dom::PCObserverStateType state_type, ER&, void*);
  NS_IMETHODIMP OnAddStream(nsIDOMMediaStream *stream, ER&);
  NS_IMETHODIMP OnRemoveStream(ER&);
  NS_IMETHODIMP OnAddTrack(ER&);
  NS_IMETHODIMP OnRemoveTrack(ER&);
  NS_IMETHODIMP OnAddIceCandidateSuccess(ER&);
  NS_IMETHODIMP OnAddIceCandidateError(uint32_t code, const char *msg, ER&);
  NS_IMETHODIMP OnIceCandidate(uint16_t level, const char *mid, const char *cand, ER&);

  // nsITimerCallback
  NS_IMETHOD Notify(nsITimer *timer);

protected:
  sipcc::PeerConnectionImpl* mPc;
  nsIDOMMediaStream* mStream;
};

NS_IMPL_ISUPPORTS2(PCObserver, nsISupportsWeakReference, nsITimerCallback)

NS_IMETHODIMP
PCObserver::OnCreateOfferSuccess(const char* offer, ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnCreateOfferError(uint32_t code, const char *message, ER&)
{
  return NS_OK;
}

namespace {
static const int32_t PCOFFER = 0;
static const int32_t PCANSWER = 1;
}

NS_IMETHODIMP
PCObserver::OnCreateAnswerSuccess(const char* answer, ER&)
{
  LOG("OnCreateAnswerSuccess:\n[%s]\n", answer);
  mPc->SetLocalDescription(PCANSWER, answer);
  std::string out("{\"type\":\"answer\",\"sdp\":\"");
  const char* ptr = answer;
  while (*ptr) {
    if (*ptr == '\n') { out += "\\n"; }
    else if (*ptr == '\r') { out += "\\r"; }
    else { out += *ptr; }
    ptr++;
  }
  out += "\"}";
  LOG("formatted: %s", out.c_str());
  REPLY("%s", out.c_str());
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnCreateAnswerError(uint32_t code, const char *message, ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnSetLocalDescriptionSuccess(ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnSetRemoteDescriptionSuccess(ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnSetLocalDescriptionError(uint32_t code, const char *message, ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnSetRemoteDescriptionError(uint32_t code, const char *message, ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::NotifyConnection(ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::NotifyClosedConnection(ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::NotifyDataChannel(nsIDOMDataChannel *channel, ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnStateChange(mozilla::dom::PCObserverStateType state_type, ER&, void*)
{
  nsresult rv;
  mozilla::dom::PCImplReadyState gotready;
  mozilla::dom::PCImplIceConnectionState gotice;
  mozilla::dom::PCImplIceGatheringState goticegathering;
  mozilla::dom::PCImplSipccState gotsipcc;
  mozilla::dom::PCImplSignalingState gotsignaling;

  switch (state_type)
  {
  case mozilla::dom::PCObserverStateType::ReadyState:
    rv = mPc->ReadyState(&gotready);
    NS_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::IceConnectionState:
    rv = mPc->IceConnectionState(&gotice);
    NS_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::IceGatheringState:
    rv = mPc->IceGatheringState(&goticegathering);
    NS_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::SdpState:
    // NS_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::SipccState:
    rv = mPc->SipccState(&gotsipcc);
    NS_ENSURE_SUCCESS(rv, rv);
    break;
  case mozilla::dom::PCObserverStateType::SignalingState:
    rv = mPc->SignalingState(&gotsignaling);
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
PCObserver::OnAddStream(nsIDOMMediaStream *stream, ER&)
{
  LOG("Add stream %p\n", (void*)stream);
  mStream = stream;
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnRemoveStream(ER&)
{
  LOG("Remove stream %p\n", (void*)mStream);
  mStream = 0;
  return NS_OK;
}
NS_IMETHODIMP
PCObserver::OnAddTrack(ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnRemoveTrack(ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnIceCandidate(uint16_t level,
                           const char * mid,
                           const char * candidate, ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnAddIceCandidateSuccess(ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::OnAddIceCandidateError(uint32_t code, const char *message, ER&)
{
  return NS_OK;
}

NS_IMETHODIMP
PCObserver::Notify(nsITimer *timer)
{
  Fake_DOMMediaStream* fake = reinterpret_cast<Fake_DOMMediaStream*>(mStream);
  if (fake) {
    Fake_MediaStream* ms = reinterpret_cast<Fake_MediaStream*>(fake->GetStream());
    static uint64_t value = 0;
    if (ms) {
      ms->NotifyPull(nullptr, value++);
    }
  }
  return NS_OK;
}

enum ActionType { OfferAction, AnswerAction, UnknownAction };
static const int BufferSize = 2048;
static char sOffer[] = "\"type\":\"offer";
static char sAnswer[] = "\"type\":\"answer";
static char sSpd[] = "\"sdp\":\"";

static bool
parse(char *sample, const int size, ActionType& type, std::string& value)
{
  bool result = false;
  type = UnknownAction;
  if (strstr(sample, sOffer)) { type = OfferAction; }
  else if (strstr(sample, sAnswer)) { type = AnswerAction; }

  char *ptr = strstr(sample, sSpd);

  if (ptr) {
    ptr += strlen(sSpd);
    bool escape = false;
    while (*ptr != '\"' && *ptr != '\0') {
       if (escape) {
         escape = false;
         if (*ptr == 'n') {
           value += '\n';
         }
         else if (*ptr == 'r') {
           value += '\r';
         }
         else if (*ptr == 't') {
           value += '\t';
         }
         else if (*ptr == '\\') {
           value += '\\';
         }
         else if (*ptr == '\"') {
           value += '\"';
         }
       }
       else if (*ptr == '\\') {
         escape = true;
       }
       else {
         value += *ptr;
       }

       ptr++;
    }
    result = true;
  }
  return result;
}

class RemoteDescriptionEvent : public nsIRunnable {
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  std::string mDescription;
  int mAction;

  RemoteDescriptionEvent(mozilla::RefPtr<sipcc::PeerConnectionImpl>& aPc) :
                         mAction(OfferAction),
                         mPc(aPc) {}

  NS_IMETHOD Run() {
    mPc->SetRemoteDescription(mAction, mDescription.c_str());
    if (mAction == OfferAction) {
      LOG("dispatch thread %p\n", (void *)pthread_self());
      sipcc::MediaConstraintsExternal c;
      mPc->CreateAnswer(c);
    }
    return NS_OK;
  }

  virtual ~RemoteDescriptionEvent() {}
protected:
  mozilla::RefPtr<sipcc::PeerConnectionImpl> mPc;
};

NS_IMPL_ISUPPORTS1(RemoteDescriptionEvent, nsIRunnable);

class nsReadStdin : public nsIRunnable {
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  nsReadStdin(nsCOMPtr<nsIThread>& aTarget,
              mozilla::RefPtr<sipcc::PeerConnectionImpl>& aPc) :
              mTarget(aTarget),
              mPc(aPc) {}

  NS_IMETHOD Run() {

    nsCOMPtr<nsIThread> thread;
    nsresult rv = NS_GetCurrentThread(getter_AddRefs(thread));
    if (NS_FAILED(rv)) {
      LOG("failed to get current thread\n");
      return NS_OK;
    }
    LOG("running on read stdin thread %p %p\n", (void *)pthread_self(), (void *)thread.get());

    nsCOMPtr<RemoteDescriptionEvent> event = new RemoteDescriptionEvent(mPc);

    static char buf[BufferSize];
    bool done = false;
    while (!done) {
      fgets(buf, BufferSize, stdin);
      ActionType type = UnknownAction;
      parse(buf, BufferSize, type, event->mDescription);
      LOG("-=-=-=-START-=-=-=-\n");
      LOG("%s\n", event->mDescription.c_str());
      LOG("-=-=-=-STOP -=-=-=-\n");

      if (type != UnknownAction) {
        event->mAction = type;
        mTarget->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
      }
      else {
        if (!strncmp(buf, "EXIT", 4)) { done = true; LOG("EXIT THREAD!"); }
        REPLY("{\"type\":\"error\",\"message\":\"Unknown payload\"}");
      }
    }
    return NS_OK;
  }

  virtual ~nsReadStdin() {}
protected:
  nsCOMPtr<nsIThread> mTarget;
  mozilla::RefPtr<sipcc::PeerConnectionImpl> mPc;
};

NS_IMPL_ISUPPORTS1(nsReadStdin, nsIRunnable)


int
main(int argc, char *argv[]) {

  NS_InitXPCOM2(nullptr, nullptr, nullptr);
  NSS_NoDB_Init(nullptr);
  NSS_SetDomesticPolicy();
  nsresult rv;

  nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    LOG("failed to create timer\n");
    return 0;
  }

  sipcc::IceConfiguration cfg;

  nsCOMPtr<nsIThread> thread;
  rv = NS_GetCurrentThread(getter_AddRefs(thread));
  if (NS_FAILED(rv)) {
    LOG("failed to get current thread\n");
    return 0;
  }
  LOG("running on thread %p\n", (void *)thread.get());
  LOG("main thread %p\n", (void *)pthread_self());

  mozilla::RefPtr<sipcc::PeerConnectionImpl> pc = sipcc::PeerConnectionImpl::CreatePeerConnection();
  nsRefPtr<PCObserver> pco = new PCObserver(pc.get());
  pc->Initialize(*pco, nullptr, cfg, thread.get());

  nsCOMPtr<nsIRunnable> event = new nsReadStdin(thread, pc);

  nsCOMPtr<nsIThread> runner;

  rv = NS_NewThread(getter_AddRefs(runner), event);
  if (NS_FAILED(rv)) {
    LOG("failed to create thread\n");
    return 0;
  }

  timer->InitWithCallback(pco, PR_MillisecondsToInterval(30),
                               nsITimer::TYPE_REPEATING_PRECISE);

  //XRE_RunAppShell();
  // while (NS_ProcessNextEvent(nullptr, false)) { LOG("ProcessNextEvent\n"); }
  while (true) { NS_ProcessNextEvent(nullptr, true); } // LOG("ProcessNextEvent\n"); }

  return 0;
}
