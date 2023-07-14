#if __has_include(<lgfx/v1/platforms/sdl/Panel_sdl.hpp>)
#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>
#if defined ( SDL_h_ )

void setup(void);
void loop(void);

static int loopThread(void* args)
{
  for (;;)
  {
    loop();
  }
}

#if __has_include(<windows.h>)
#include <windows.h>
#include <tchar.h>

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#else
__attribute__((weak))
int main(int, char**)
#endif
{
  setup();
  SDL_CreateThread(loopThread, "lt", nullptr);
  for (;;)
  {
    SDL_Delay(1);
    lgfx::Panel_sdl::sdl_event_handler();
  }
  return 0;
}
#endif
#endif
