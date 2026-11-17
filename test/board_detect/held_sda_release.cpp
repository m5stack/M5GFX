// release_held_sda() against a peripheral left in the middle of a byte by a reset.
#define CONFIG_IDF_TARGET_ESP32S3 1
#define CONFIG_IDF_TARGET "esp32s3"
#include <cassert>
#include <cstdio>
#include <vector>
#include "board_detect/board_detect.inl"
using namespace m5gfx::board_detect;

static constexpr int sda = 4;
static constexpr int scl = 5;
static unsigned cases;

// A peripheral on an open drain bus, replayed from the GPIO events of the master. It drives
// `bits` (index 8 of every byte is the acknowledge slot), moves to the next bit on each falling
// SCL edge and lets go of SDA after a STOP. A transmitter also stops after a NACK.
struct peripheral_t
{
  std::vector<bool> bits;  // the level the peripheral puts on SDA for each bit position
  std::size_t start;       // bit position at the moment of the reset
  bool transmitter;        // sending a read (true) or receiving a write (false)
  bool scl_stuck_low;
};

static bool sda_level(const peripheral_t& p)
{
  bool master_sda = true, master_scl = true, active = true;
  std::size_t position = p.start;
  auto peripheral_sda = [&]() { return !active || p.bits[position % p.bits.size()]; };
  for (const auto& e : host_hardware::events)
  {
    if (e.kind != 4) { continue; }
    if (e.pin == scl)
    {
      const bool rising = !master_scl && e.value;
      const bool falling = master_scl && !e.value;
      master_scl = e.value;
      if (rising && active && p.transmitter && position % 9 == 8 && master_sda)
      { active = false; }  // NACK from the master: the transmitter stops
      if (falling && active) { ++position; }
    }
    else if (e.pin == sda)
    {
      const bool line_was_low = !(master_sda && peripheral_sda());
      master_sda = e.value;
      if (master_scl && line_was_low && master_sda && peripheral_sda()) { active = false; }  // STOP
    }
  }
  return master_sda && peripheral_sda();
}

// The master's own drive of SCL: released (high) until it writes a level.
static bool master_scl_level()
{
  bool level = true;
  for (const auto& e : host_hardware::events) { if (e.kind == 4 && e.pin == scl) { level = e.value; } }
  return level;
}

static bool run(const peripheral_t& p, unsigned* scl_pulses)
{
  host_hardware::reset();
  host_hardware::input = [&](int pin, int, bool latch) {
    if (pin == scl) { return !p.scl_stuck_low && master_scl_level(); }
    if (pin == sda) { return sda_level(p); }
    return latch;
  };
  const bool released = release_held_sda(sda, scl);
  unsigned pulses = 0;
  for (const auto& e : host_hardware::events) { pulses += e.kind == 4 && e.pin == scl && e.value == 0; }
  *scl_pulses = pulses;
  ++cases;
  return released;
}

// One byte as the peripheral sends it, MSB first, then the acknowledge slot (released).
static std::vector<bool> byte_bits(const std::uint8_t value)
{
  std::vector<bool> bits;
  for (int bit = 7; bit >= 0; --bit) { bits.push_back((value >> bit) & 1); }
  bits.push_back(true);
  return bits;
}

int main()
{
  unsigned pulses = 0;
  // Reset while sending 0x2D, at its first 0 bit: SDA goes high on the next 1 bit and low again
  // on the bit after it. Stopping at the first high SDA leaves the bus held.
  assert(run({byte_bits(0x2D), 0, true, false}, &pulses));
  assert(pulses <= 10);
  // Every reset position inside the bytes the UnitNFC returns in the ToughC5 reproduction.
  for (const std::uint8_t value : {0x2D, 0x93, 0x00, 0x01, 0x80, 0x55, 0xAA, 0xFE})
  {
    const auto bits = byte_bits(value);
    for (std::size_t start = 0; start < 8; ++start)
    {
      if (bits[start]) { continue; }  // SDA is high there: nothing to release
      assert(run({bits, start, true, false}, &pulses));
      assert(pulses <= 10);
    }
  }
  // A receiver holding SDA in its acknowledge slot.
  {
    std::vector<bool> bits(9, true);
    bits[8] = false;
    assert(run({bits, 8, false, false}, &pulses));
    assert(pulses <= 10);
  }
  // A peripheral that never lets go of SDA, not even for a STOP: the clocking is bounded.
  assert(!run({std::vector<bool>(9, false), 0, false, false}, &pulses));
  assert(pulses == 10);
  // SCL held low by someone else: give up instead of clocking.
  assert(!run({byte_bits(0x00), 0, true, true}, &pulses));
  assert(pulses == 0);
  // Nothing held: no clocks at all.
  assert(run({std::vector<bool>(9, true), 0, true, false}, &pulses));
  assert(pulses == 0);
  std::printf("held SDA release: %u cases passed\n", cases);
  return 0;
}
