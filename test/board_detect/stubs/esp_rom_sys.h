#pragma once
#include <lgfx/v1/platforms/esp32/common.hpp>
inline void esp_rom_delay_us(unsigned us) { lgfx::delayMicroseconds(us); }
