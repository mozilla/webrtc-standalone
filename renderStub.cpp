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
  fprintf(stderr, "render::Initialize\n");
  sState = new State;
  sState->Out = fopen("video.yuv", "w");
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
  fprintf(stderr, "Got image: %d %dx%d\n", size, aWidth, aHeight);
  if (sState && sState->Out && aImage && size) { fwrite(aImage, size, 1, sState->Out); }
}

bool
KeepRunning()
{
  bool result = true;
  fprintf(stderr,"KeepRunning\n");

  return result;
}

} // namespace standalone
