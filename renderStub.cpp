namespace render {

struct State {
};

static State* sState = nullptr;

void
Initialize()
{
  if (sState) {
    return;
  }

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
}

bool
KeepRunning()
{
  bool result = true;

  return result;
}

} // namespace standalone
