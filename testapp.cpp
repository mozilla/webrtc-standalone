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

#define LOG(format, ...) fprintf(stderr, format, ##__VA_ARGS__);
#define REPLY(format, ...) \
  fprintf(stdout, format, ##__VA_ARGS__); \
  fprintf(stdout, "\n"); \
  fflush(stdout);

namespace {

enum ActionType { OfferAction, AnswerAction, UnknownAction };
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
         if (*ptr == 'n') { value += '\n'; }
         else if (*ptr == 'r') { value += '\r'; }
         else if (*ptr == 't') { value += '\t'; }
         else if (*ptr == '\\') { value += '\\'; }
         else if (*ptr == '\"') { value += '\"'; }
       }
       else if (*ptr == '\\') { escape = true; }
       else { value += *ptr; }

       ptr++;
    }
    result = true;
  }
  return result;
}

static const int BufferSize = 2048;

class OfferEvent : public WebRTCCallEvent {
public:
  OfferEvent(WebRTCCall& aCall, const std::string& aOffer)
      : mCall(aCall)
      , mOffer(aOffer) {}

  virtual void Run() { mCall.SetOffer(mOffer); }
protected:
  WebRTCCall& mCall;
  std::string mOffer;
};

void*
ReadStdin(void* ptr)
{
  WebRTCCall* self = static_cast<WebRTCCall*>(ptr);

  static char buf[BufferSize];
  bool done = false;
  while (!done) {
    fgets(buf, BufferSize, stdin);
    ActionType type = UnknownAction;
    std::string description;
    parse(buf, BufferSize, type, description);

    if (type == OfferAction) {
      self->AddEvent(new OfferEvent(*self, description));
    }
    else if (!strncmp(buf, "EXIT", 4)) { done = true; LOG("EXIT THREAD!"); }
    else { REPLY("{\"type\":\"error\",\"message\":\"Unknown payload\"}"); }
  }
  return nullptr;
}

class TestWebRTCCallObserver : public WebRTCCallObserver {
public:
  TestWebRTCCallObserver() : mFile(nullptr)
  {
    //play with "mplayer -demuxer rawvideo -rawvideo w=640:h=480:format=i420 video.yuv"
    mFile = fopen("video.yuv", "w");
  }
  virtual ~TestWebRTCCallObserver()
  {
    if (mFile) { fclose(mFile); }
  }

  void ReceiveAnswer(const std::string& aAnswer)
  {
    std::string out("{\"type\":\"answer\",\"sdp\":\"");
    const char* ptr = aAnswer.c_str();
    while (*ptr) {
      if (*ptr == '\n') { out += "\\n"; }
      else if (*ptr == '\r') { out += "\\r"; }
      else { out += *ptr; }
      ptr++;
    }
    out += "\"}";
LOG("REPLY: %s\n", out.c_str());
    REPLY("%s", out.c_str());
  }

  void ReceiveNextVideoFrame(const unsigned char* aBuffer, const int32_t aBufferSize)
  {
    if (mFile && aBuffer && aBufferSize) { fwrite(aBuffer, aBufferSize, 1, mFile); }
  }
protected:
  FILE* mFile;
};

class TestWebRTCCallSystem : public WebRTCCallSystem {
public:
  TestWebRTCCallSystem(sem_t* aSema) : mSema(aSema) {}
  virtual ~TestWebRTCCallSystem() {}
  void NotifyNextEventReady() { if (mSema) { sem_post(mSema); } }
protected:
  sem_t* mSema;
};

} // namespace

int
main(int argc, char *argv[])
{
  std::string name("/testapp-semaphore-");
  std::stringstream out;
  out << getpid();
  name += out.str();
  LOG("Semaphore name: %s\n", name.c_str());
  sem_t* sema = sem_open(name.c_str(), O_CREAT, S_IRUSR | S_IWUSR, 0);

  TestWebRTCCallSystem system(sema);
  TestWebRTCCallObserver obs;
  WebRTCCall call(system, obs);

  pthread_t t;
  pthread_create(&t, nullptr, ReadStdin, static_cast<void*>(&call));

  while(true) {
    if (sem_wait(sema) < 0) {
       switch (errno) {
         case EAGAIN:  LOG("The semaphore is already locked.\n"); break;
         case EDEADLK: LOG("A deadlock was detected.\n"); break;
         case EINTR:   LOG("The call was interrupted by a signal.\n"); break;
         case EINVAL:  LOG("sem is not a valid semaphore descriptor.\n"); break;
         default: LOG("UNKNOWN Error\n");
       }
    }
    else { call.ProcessNextEvent(); }
  }

  sem_close(sema);
  sem_unlink(name.c_str());
  sema = nullptr;
  return 0;
}
