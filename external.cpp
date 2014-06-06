#include "MediaExternal.h"
#include "MediaRunnable.h"
#include "MediaThread.h"
#include "MediaTimer.h"
#include "MediaDNS.h"
#include "mozilla/RefPtr.h"
#include "stdio.h"
#include "prthread.h"

using namespace media;

class Lookup : public DNSListener {
MEDIA_REF_COUNT_INLINE
public:
  virtual nsresult OnLookupComplete(Cancelable *request, DNSRecord *record, nsresult status);
};

nsresult
Lookup::OnLookupComplete(Cancelable *request, DNSRecord *record, nsresult status)
{
  printf("Got DNS callback. %p\n", PR_GetCurrentThread());
  PRNetAddr na;
  if (NS_SUCCEEDED(record->GetNextAddr(1, &na))) {
    char buff[128];
    PR_NetAddrToString(&na, buff, 128);
    printf("IP Address: %u %s\n", na.inet.ip, buff);
  }
  else {
    printf("No Address found!\n");
  }
  return NS_OK;
}

class TickQuit : public TimerCallback {
MEDIA_DECL_THREADSAFE_REFCOUNTING
public:
  TickQuit() : mCount(0) {}
  int mCount;
  virtual nsresult Notify(Timer *timer)
  {
    mCount++;
    printf("Quit tick %d: %p\n", mCount, PR_GetCurrentThread());
    return NS_OK;
  }
  bool Done() { return mCount > 4; }
};

NS_IMPL_ISUPPORTS0(TickQuit)

class Tick : public TimerCallback {
MEDIA_DECL_THREADSAFE_REFCOUNTING
public:
  Tick(const char* aMsg) : msg(aMsg) {}
  const char* msg;
  virtual nsresult Notify(Timer *timer)
  {
    printf("%s: %p\n", msg, PR_GetCurrentThread());
    timer->Release();
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS0(Tick)

class Message : public Runnable {
public:
  virtual nsresult Run(void)
  {
    printf("Message sent to: %p\n", PR_GetCurrentThread());
    return NS_OK;
  }
};

class ThreadTest : public Runnable {
public:
  virtual nsresult Run(void)
  {
    printf("Running in Thread: %p\n", PR_GetCurrentThread());
    mozilla::RefPtr<Message> msg = new Message;
    NS_DispatchToMainThread(msg, MEDIA_DISPATCH_NORMAL);
    return NS_OK;
  }
protected:
};



int
main(int argc, char *argv[])
{
  media::Initialize();

  printf("Main Thread: %p\n", PR_GetCurrentThread());

  mozilla::RefPtr<DNS> dns = GetDNSService();
  mozilla::RefPtr<Lookup> lookup = new Lookup;
  dns->AsyncResolve("mozilla.org", DNS::RESOLVE_DISABLE_IPV6, lookup, nullptr, nullptr);

  mozilla::RefPtr<Thread> thread = NS_NewThread(new ThreadTest());

  mozilla::RefPtr<TickQuit> quit = new TickQuit;
  mozilla::RefPtr<Timer> timer = CreateTimer();
  timer->InitWithCallback(quit, 1000, Timer::TYPE_REPEATING_PRECISE);
  mozilla::RefPtr<Timer> oneShot = CreateTimer();
  oneShot->InitWithCallback(new Tick("One Shot"), 0, Timer::TYPE_ONE_SHOT);
  oneShot->AddRef();
  oneShot = nullptr;

  while(!quit->Done()) {
    media::ProcessNextEvent(true);
    printf("Process Event\n");
  }

  timer = nullptr;
  oneShot = nullptr;

  media::Shutdown();

  return 0;
}
