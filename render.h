#ifndef media_render_dot_h_
#define media_render_dot_h_

namespace render {

void Initialize();
void Shutdown();
void Draw(const unsigned char* aImage, int size, int aWidth, int aHeight);
bool KeepRunning();

} // namespace standalone
#endif // ifndef media_render_dot_h_
