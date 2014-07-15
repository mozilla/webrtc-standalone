#include <string.h>

#include "SDL.h"

#include <stdio.h>
#define LOG(format, ...) fprintf(stderr, format, ##__VA_ARGS__);

static void
quit(int rc)
{
  SDL_Quit();
  exit(rc);
}

namespace render {

struct State {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  State() : window(0), renderer(0), texture(0) {}
};

static State* sState = nullptr;

void
Initialize()
{
  if (sState) {
    return;
  }

  sState = new State;

  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
    return;
  }

  sState->window = SDL_CreateWindow("WebRTC Player",
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    //640, 480,
                                    320, 240,
                                    0);
                                    // SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  if (!sState->window) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't set create window: %s\n", SDL_GetError());
    quit(3);
  }

  sState->renderer = SDL_CreateRenderer(sState->window, -1, 0);
  if (!sState->renderer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't set create renderer: %s\n", SDL_GetError());
    quit(4);
  }

  sState->texture = SDL_CreateTexture(sState->renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, 320, 240);
  if (!sState->texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't set create texture: %s\n", SDL_GetError());
    quit(5);
  }
LOG("Initialized\n");
}

void
Shutdown()
{
  SDL_DestroyRenderer(sState->renderer);
  delete sState;
  sState = 0;
}

void
Draw(const unsigned char* aImage, int size, int aWidth, int aHeight)
{
  if (!sState) {
    return;
  }
  Uint8* dst = nullptr;
  void* pixels = nullptr;
  int pitch = 0;

  if (SDL_LockTexture(sState->texture, NULL, &pixels, &pitch) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't lock texture: %s\n", SDL_GetError());
    quit(5);
  }

  dst = (Uint8*)pixels;
fprintf(stderr, "Got Image: %d %d x %d\n",(int)size, (int)aWidth,(int)aHeight);
  memcpy(dst, aImage, size);
  SDL_UnlockTexture(sState->texture);

  SDL_RenderClear(sState->renderer);
  SDL_RenderCopy(sState->renderer, sState->texture, NULL, NULL);
  SDL_RenderPresent(sState->renderer);
}

bool
KeepRunning()
{
  bool result = true;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
LOG("SDL_PollEvent\n");
    switch (event.type) {
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          result = false;
        }
      break;
      case SDL_QUIT:
        result = false;
      break;
    }
  }
LOG("SDL_PollEvent DONE\n");
  return result;
}

} // namespace standalone
