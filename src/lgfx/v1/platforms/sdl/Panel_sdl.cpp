/*----------------------------------------------------------------------------/
  Lovyan GFX - Graphics library for embedded devices.

Original Source:
 https://github.com/lovyan03/LovyanGFX/

Licence:
 [FreeBSD](https://github.com/lovyan03/LovyanGFX/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)

Contributors:
 [ciniml](https://github.com/ciniml)
 [mongonta0716](https://github.com/mongonta0716)
 [tobozo](https://github.com/tobozo)

Porting for SDL:
 [imliubo](https://github.com/imliubo)
/----------------------------------------------------------------------------*/
#include "Panel_sdl.hpp"

#if defined ( SDL_h_ )

#include "../common.hpp"
#include "../../misc/common_function.hpp"
#include "../../Bus.hpp"

#include <list>

namespace lgfx
{
 inline namespace v1
 {
  static SDL_semaphore *_update_in_semaphore = nullptr;
  static SDL_semaphore *_update_out_semaphore = nullptr;
  static bool _inited = false;

  static std::list<monitor_t*> _list_monitor;

  static monitor_t* const getMonitorByWindowID(uint32_t windowID)
  {
    for (auto& m : _list_monitor)
    {
      if (SDL_GetWindowID(m->window) == windowID) { return m; }
    }
    return nullptr;
  }
//----------------------------------------------------------------------------

  void Panel_sdl::_event_proc(void)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if ((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP))
      {
        int gpio = -1;
        switch (event.key.keysym.sym)
        { /// M5StackのBtnA～BtnCのエミュレート;
        case SDLK_LEFT:  gpio = 39; break;
        case SDLK_DOWN:  gpio = 38; break;
        case SDLK_RIGHT: gpio = 37; break;
        case SDLK_UP:    gpio = 36; break;
        default: continue;
        }
        if (event.type == SDL_KEYDOWN) {
          gpio_lo(gpio);
        } else {
          gpio_hi(gpio);
        }
      }
      else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION)
      {
        auto mon = getMonitorByWindowID(event.button.windowID);
        if (mon != nullptr)
        {
          int x, y, w, h;
          SDL_GetWindowSize(mon->window, &w, &h);
          SDL_GetMouseState(&x, &y);
          mon->touch_x = x * mon->panel->config().panel_width / w;
          mon->touch_y = y * mon->panel->config().panel_height / h;
          if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
          {
            mon->touched = true;
          }
          if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
          {
            mon->touched = false;
          }
        }
      }
      else
      if (event.type == SDL_WINDOWEVENT
      || event.type == SDL_QUIT) {
        auto monitor = getMonitorByWindowID(event.window.windowID);
        if (monitor) {
          if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
            monitor->closing = true;
          }
          else
          {
            monitor->panel->sdl_invalidate();
          }
        }
      }
    }
  }

  bool Panel_sdl::_update_proc(void)
  {
    uint32_t msec = millis();
    for (auto it = _list_monitor.begin(); it != _list_monitor.end(); )
    {
      if ((*it)->closing) {
        SDL_DestroyTexture((*it)->texture);
        SDL_DestroyRenderer((*it)->renderer);
        SDL_DestroyWindow((*it)->window);
        _list_monitor.erase(it++);
        if (_list_monitor.empty()) {
          return true;
        }
        continue;
      }
      uint32_t lm = (*it)->panel->_last_msec;
      if (lm == 0 || (msec - lm) >= 16)
      {
        (*it)->panel->_last_msec = msec;
        (*it)->panel->sdl_update();
      }
      ++it;
    }
    return false;
  }

  int Panel_sdl::setup(void)
  {
    if (_inited) return 1;
    _inited = true;

    _update_in_semaphore = SDL_CreateSemaphore(0);
    _update_out_semaphore = SDL_CreateSemaphore(0);
    for (size_t pin = 0; pin < EMULATED_GPIO_MAX; ++pin) { gpio_hi(pin); }
    /*Initialize the SDL*/
    SDL_Init(SDL_INIT_VIDEO);
    SDL_StartTextInput();
    // SDL_SetThreadPriority(SDL_ThreadPriority::SDL_THREAD_PRIORITY_HIGH);
    return 0;
  }

  int Panel_sdl::loop(void)
  {
    if (!_inited) return 1;
    _event_proc();
    int wait = SDL_SemWaitTimeout(_update_in_semaphore, 8);
    if (_update_proc()) { return 1; }

    if (wait != SDL_MUTEX_TIMEDOUT && SDL_SemValue(_update_out_semaphore) < 0) {
      SDL_SemPost(_update_out_semaphore);
    }
    return 0;
  }

  int Panel_sdl::close(void)
  {
    if (!_inited) return 1;
    _inited = false;

    SDL_StopTextInput();
    SDL_DestroySemaphore(_update_in_semaphore);
    SDL_DestroySemaphore(_update_out_semaphore);
    SDL_Quit();
    return 0;
  }

  void Panel_sdl::setScaling(uint_fast8_t scaling_x, uint_fast8_t scaling_y)
  {
    monitor.scaling_x = scaling_x;
    monitor.scaling_y = scaling_y;
  }

  Panel_sdl::~Panel_sdl(void)
  {
    _list_monitor.remove(&monitor);
  }

  Panel_sdl::Panel_sdl(void) : Panel_FrameBufferBase()
  {
    _auto_display = true;
    monitor.panel = this;
  }

  bool Panel_sdl::init(bool use_reset)
  {
    initFrameBuffer(_cfg.panel_width * 4, _cfg.panel_height);
    bool res = Panel_FrameBufferBase::init(use_reset);

    _list_monitor.push_back(&monitor);

    return res;
  }

  color_depth_t Panel_sdl::setColorDepth(color_depth_t depth)
  {
    // if (depth != color_depth_t::grayscale_8bit) {
    //   depth = ((depth & color_depth_t::bit_mask) > 16) ? rgb888_3Byte : rgb565_2Byte;
    // }
    _write_depth = depth;
    _read_depth = depth;

    return depth;
  }

  void Panel_sdl::display(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h)
  {
    _last_msec = 0;
    SDL_SemPost(_update_in_semaphore);
    SDL_SemWaitTimeout(_update_out_semaphore, 1);
  }

  uint_fast8_t Panel_sdl::getTouchRaw(touch_point_t* tp, uint_fast8_t count)
  {
    tp->x = monitor.touch_x;
    tp->y = monitor.touch_y;
    tp->size = monitor.touched ? 1 : 0;
    tp->id = 0;
    return monitor.touched;
  }

  void Panel_sdl::setWindowTitle(const char* title)
  {
    _window_title = title;
    if (monitor.window) {
      SDL_SetWindowTitle(monitor.window, _window_title);
    }
  }
  
  void Panel_sdl::sdl_create(monitor_t * m)
  {
    int flag = SDL_WINDOW_RESIZABLE;
#if SDL_FULLSCREEN
    flag |= SDL_WINDOW_FULLSCREEN;
#endif
    m->window = SDL_CreateWindow(_window_title,
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              _cfg.panel_width * m->scaling_x, _cfg.panel_height * m->scaling_y, flag);       /*last param. SDL_WINDOW_BORDERLESS to hide borders*/

    m->renderer = SDL_CreateRenderer(m->window, -1, SDL_RENDERER_ACCELERATED);

    m->texture = SDL_CreateTexture(m->renderer, SDL_PIXELFORMAT_RGB24,
                     SDL_TEXTUREACCESS_STATIC, _cfg.panel_width, _cfg.panel_height);
    SDL_SetTextureBlendMode(m->texture, SDL_BLENDMODE_NONE);
  }

  void Panel_sdl::sdl_update(void)
  {
    if (monitor.renderer == nullptr)
    {
      sdl_create(&monitor);
    }

    auto array = (rgb888_t*)alloca((_cfg.panel_width + 1) * 4);
    pixelcopy_t pc(nullptr, color_depth_t::rgb888_3Byte, _write_depth, false);

    SDL_Rect r = {0, 0, _cfg.panel_width, 1};
    for (int y = 0; y < _cfg.panel_height; ++y)
    {
      r.y = y;
      pc.src_x32 = 0;
      pc.src_data = _lines_buffer[y];
      pc.fp_copy(array, 0, _cfg.panel_width, &pc);
      SDL_UpdateTexture(monitor.texture, &r, array, _cfg.panel_width * _write_bits >> 3);
    }
    { /*Update the renderer with the texture containing the rendered image*/
      if (0 == SDL_RenderCopy(monitor.renderer, monitor.texture, NULL, NULL))
      {
        SDL_RenderPresent(monitor.renderer);
      }
    }
  }

  bool Panel_sdl::initFrameBuffer(size_t width, size_t height)
  {
    uint8_t** lineArray = (uint8_t**)heap_alloc_dma(height * sizeof(uint8_t*));
    if ( nullptr == lineArray ) { return false; }

    /// 8byte alignment;
    width = (width + 7) & ~7u;

    _lines_buffer = lineArray;
    memset(lineArray, 0, height * sizeof(uint8_t*));

    uint8_t* framebuffer = (uint8_t*)heap_alloc_dma(width * height + 16);

    size_t alloc_idx = 0;
    auto fb = framebuffer;
    {
      for (int y = 0; y < height; ++y)
      {
        lineArray[y] = fb;
        fb += width;
      }
    }
    return true;
  }

  void Panel_sdl::deinitFrameBuffer(void)
  {
    auto lines = _lines_buffer;
    _lines_buffer = nullptr;
    if (lines != nullptr)
    {
      heap_free(lines[0]);
      heap_free(lines);
    }
  }

//----------------------------------------------------------------------------
 }
}

#endif
