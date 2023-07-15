#include <M5GFX.h>
#if defined ( SDL_h_ )

int main_func(bool* running);
void setup(void);
void loop(void);

__attribute__((weak))
int main_func(bool* running)
{
  // SDL_SetThreadPriority(SDL_ThreadPriority::SDL_THREAD_PRIORITY_HIGH);
  setup();
  do
  {
    loop();
  } while (*running);
  return 0;
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
  /// loopThreadの動作用フラグ
  bool running = true;

  /// loopThreadを起動
  auto thread = SDL_CreateThread((SDL_ThreadFunction)main_func, "main_func", &running);

  /// SDLの準備
  if (0 != lgfx::Panel_sdl::setup()) { return 1; }

  /// 全部のウィンドウが閉じられるまでSDLのイベント・描画処理を継続
  while (0 == lgfx::Panel_sdl::loop()) {};

  /// main_funcを終了する
  running = false;
  SDL_WaitThread(thread, nullptr);

  /// SDLを終了する
  return lgfx::Panel_sdl::close();
}
#endif
