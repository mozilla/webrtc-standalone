#include <stdio.h>

namespace render {


struct State {
  FILE* Out;
  State() : Out(nullptr) {}
  ~State() {
    if (Out) { fclose(Out); }
  }
};

static State* sState = nullptr;

void
Initialize()
{
  if (sState) {
    return;
  }
//  fprintf(stderr, "render::Initialize\n");
  sState = new State;
}

void
Shutdown()
{
  delete sState;
  sState = 0;
}

void
Draw(const unsigned char* aImage, int size, int aWidth, int aHeight)
{
  //play with "mplayer -demuxer rawvideo -rawvideo w=640:h=480:format=i420 video.yuv"
  if (!sState->Out) { sState->Out = fopen("video.yuv", "w"); }
  fprintf(stderr, "Got image: %d %dx%d\n", size, aWidth, aHeight);
  if (sState && sState->Out && aImage && size) { fwrite(aImage, size, 1, sState->Out); }

#if 0
  static int count = 0;
  count++;
  if (count == 16) {
    FILE* tmp = fopen("frame.yuv", "w");
    if (tmp && aImage && size) {
       fwrite(aImage, size, 1, tmp);
       fclose(tmp); tmp = 0;
    }
  }
#endif // 0
}

bool
KeepRunning()
{
  bool result = true;

  return result;
}

} // namespace standalone
