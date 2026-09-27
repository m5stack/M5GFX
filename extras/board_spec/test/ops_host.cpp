#include "board_detect/ops.hpp"
#include "board_detect/dedicated_release_probe.hpp"
#include "board_detect/m5/pmic_ops.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <utility>
#include <vector>

namespace bdops = m5gfx::board_detect::ops;
namespace pmicops = m5gfx::board_detect::m5::pmic_ops;
namespace bdetect = m5gfx::board_detect;

struct fake_write_t
{
  std::uint16_t addr;
  std::uint16_t reg;
  std::uint8_t old_value;
  std::uint8_t new_value;
};

struct fake_gpio_t
{
  std::uint16_t pin;
  int value;
};

struct fake_t
{
  std::map<std::pair<std::uint16_t, std::uint16_t>, std::uint8_t> registers;
  std::vector<fake_write_t> writes;
  std::vector<fake_gpio_t> gpios;
  std::uint32_t now = 0;
  int reads = 0;
  int writes_attempted = 0;
  int read_failures = 0;
  std::vector<int> read_nacks;
  int write_failures = 0;
  int ready_failures = 0;
  int ready_attempts = 0;
  std::uint32_t ready_attempt_ms = 0;
};

static bool read8(void* context, const bdops::i2c_device_t& device,
                  std::uint16_t reg, std::uint8_t* value)
{
  auto& fake = *static_cast<fake_t*>(context);
  ++fake.reads;
  if (fake.read_failures-- > 0) { return false; }
  for (auto attempt : fake.read_nacks)
  {
    if (attempt == fake.reads) { return false; }
  }
  *value = fake.registers[std::make_pair(device.addr, reg)];
  return true;
}

static bool write8(void* context, const bdops::i2c_device_t& device,
                   std::uint16_t reg, std::uint8_t value)
{
  auto& fake = *static_cast<fake_t*>(context);
  ++fake.writes_attempted;
  if (fake.write_failures-- > 0) { return false; }
  const auto key = std::make_pair(device.addr, reg);
  const auto old_value = fake.registers[key];
  fake.registers[key] = value;
  fake.writes.push_back({ device.addr, reg, old_value, value });
  return true;
}

static bool ready(void* context, const bdops::i2c_device_t&, std::uint32_t remaining_ms)
{
  auto& fake = *static_cast<fake_t*>(context);
  if (remaining_ms == 0) { return false; }
  ++fake.ready_attempts;
  fake.now += fake.ready_attempt_ms;
  return fake.ready_failures-- <= 0;
}

static bool gpio_mode(void* context, std::uint16_t pin, bdops::gpio_mode_t mode)
{
  static_cast<fake_t*>(context)->gpios.push_back({ pin, 100 + static_cast<int>(mode) });
  return true;
}

static bool gpio_write(void* context, std::uint16_t pin, bool high)
{
  static_cast<fake_t*>(context)->gpios.push_back({ pin, high ? 1 : 0 });
  return true;
}

static void delay(void* context, std::uint32_t ms) { static_cast<fake_t*>(context)->now += ms; }
static std::uint32_t millis(void* context) { return static_cast<fake_t*>(context)->now; }

static bdops::backend_t backend(fake_t& fake)
{
  return { read8, write8, ready, gpio_mode, gpio_write, delay, millis, &fake };
}

static const bdops::i2c_device_t devices[] = {
  { 400000, 0, 0x34, 0, 1, 0 },
};
static const std::int8_t allowed_pins[] = { 4, 5 };
static const bdops::gpio_scope_t gpio_scope = { 40, allowed_pins, 2 };

struct legacy_write_t
{
  std::uint8_t addr;
  std::uint8_t reg;
  std::uint8_t value;
  std::uint8_t keep;
};

static const legacy_write_t legacy_power192[] = {
  { 0x34, 0x95, 0x84, 0x72 }, { 0x34, 0x28, 0xF0, 0x0F },
  { 0x34, 0x12, 0x04, 0xFF }, { 0x34, 0x92, 0x00, 0xF8 },
  { 0x34, 0x96, 0x02, 0xFF }, { 0x34, 0x94, 0x02, 0xFF },
};
static const legacy_write_t legacy_power2101[] = {
  { 0x34, 0x90, 0x08, 0xF7 }, { 0x34, 0x80, 0x05, 0xFF },
  { 0x34, 0x82, 0x12, 0x00 }, { 0x34, 0x84, 0x6A, 0x00 },
  { 0x34, 0x90, 0x02, 0xFD },
};
static const legacy_write_t legacy_reset192[] = {
  { 0x34, 0x96, 0x00, 0xFD }, { 0x34, 0x94, 0x00, 0xFD },
};
static const legacy_write_t legacy_release192[] = {
  { 0x34, 0x96, 0x02, 0xFF }, { 0x34, 0x94, 0x02, 0xFF },
};
static const legacy_write_t legacy_reset2101[] = { { 0x34, 0x90, 0x00, 0xFD } };
static const legacy_write_t legacy_release2101[] = { { 0x34, 0x90, 0x02, 0xFF } };
static const legacy_write_t legacy_sticks3[] = {
  { 0x6E, 0x09, 0x00, 0x00 }, { 0x6E, 0x16, 0x00, 0xFB },
  { 0x6E, 0x10, 0x04, 0xFF }, { 0x6E, 0x13, 0x00, 0xFB },
  { 0x6E, 0x11, 0x04, 0xFF },
};

static void test_validation()
{
  {
    auto operation = bdops::delay_ms(1);
    std::uint8_t invalid = 0xFF;
    std::memcpy(&operation, &invalid, 1);
    assert(bdops::validate_ops(devices, 1, &operation, 1, gpio_scope).status
           == bdops::op_status_t::invalid_op);
  }
  {
    const auto operation = bdops::i2c_write8(1, 0, 0);
    assert(bdops::validate_ops(devices, 1, &operation, 1, gpio_scope).status
           == bdops::op_status_t::invalid_device);
  }
  {
    const bdops::i2c_device_t invalid_device[] = { { 400000, 0, 0x34, 0, 2, 0 } };
    const auto operation = bdops::i2c_write8(0, 0, 0);
    assert(bdops::validate_ops(invalid_device, 1, &operation, 1, gpio_scope).status
           == bdops::op_status_t::invalid_device);
  }
  {
    const auto zero = bdops::i2c_masked8(0, 0, 0, 0);
    const auto outside = bdops::i2c_masked8(0, 0, 0x80, 0x01);
    assert(bdops::validate_ops(devices, 1, &zero, 1, gpio_scope).status
           == bdops::op_status_t::invalid_mask);
    assert(bdops::validate_ops(devices, 1, &outside, 1, gpio_scope).status
           == bdops::op_status_t::invalid_mask);
  }
  {
    const auto outside = bdops::gpio_write_high(40);
    const auto absent = bdops::gpio_write_high(6);
    assert(bdops::validate_ops(devices, 1, &outside, 1, gpio_scope).status
           == bdops::op_status_t::invalid_gpio);
    assert(bdops::validate_ops(devices, 1, &absent, 1, gpio_scope).status
           == bdops::op_status_t::invalid_gpio);
  }
  {
    const auto operation = bdops::i2c_transfer(0, 0, 1);
    assert(bdops::validate_ops(devices, 1, &operation, 1, gpio_scope).status
           == bdops::op_status_t::unsupported);
  }
}

static void test_i2c_execution()
{
  {
    fake_t fake;
    fake.registers[std::make_pair(0x34, 0x10)] = 0xA0;
    const bdops::op_t operations[] = { bdops::i2c_masked8(0, 0x10, 0x05, 0x0F) };
    const auto result = bdops::run_ops(backend(fake), devices, 1, operations, 1, gpio_scope);
    assert(result.status == bdops::op_status_t::ok && result.failed_index == 1);
    assert(fake.reads == 1 && fake.writes.size() == 1);
    assert(fake.writes[0].old_value == 0xA0 && fake.writes[0].new_value == 0xA5);
  }
  {
    fake_t fake;
    const bdops::op_t operations[] = { bdops::i2c_masked8(0, 0x10, 0xA5, 0xFF) };
    assert(bdops::run_ops(backend(fake), devices, 1, operations, 1, gpio_scope).status
           == bdops::op_status_t::ok);
    assert(fake.reads == 0 && fake.writes.size() == 1);
  }
  {
    fake_t fake;
    fake.read_failures = 1;
    const bdops::op_t operations[] = { bdops::i2c_bit_on(0, 0x10, 1) };
    const auto result = bdops::run_ops(backend(fake), devices, 1, operations, 1, gpio_scope);
    assert(result.status == bdops::op_status_t::i2c_nack && result.failed_index == 0);
    assert(fake.writes_attempted == 0);
  }
  {
    fake_t fake;
    fake.write_failures = 1;
    const bdops::op_t operations[] = {
      bdops::i2c_write8(0, 0x10, 1), bdops::i2c_write8(0, 0x11, 2),
    };
    const auto result = bdops::run_ops(backend(fake), devices, 1, operations, 2, gpio_scope);
    assert(result.status == bdops::op_status_t::i2c_nack && result.failed_index == 0);
    assert(fake.writes_attempted == 1 && fake.writes.empty());
  }
}

static void test_wait_and_gpio()
{
  {
    fake_t fake;
    fake.ready_failures = 2;
    const bdops::op_t operations[] = { bdops::i2c_wait_ready(0, 10, 3) };
    assert(bdops::run_ops(backend(fake), devices, 1, operations, 1, gpio_scope).status
           == bdops::op_status_t::ok);
    assert(fake.ready_attempts == 3 && fake.now == 6);
  }
  {
    fake_t fake;
    fake.ready_failures = 10;
    const bdops::op_t operations[] = { bdops::i2c_wait_ready(0, 5, 3) };
    assert(bdops::run_ops(backend(fake), devices, 1, operations, 1, gpio_scope).status
           == bdops::op_status_t::timeout);
    assert(fake.now == 5 && fake.ready_attempts == 2);
  }
  {
    fake_t fake;
    fake.ready_failures = 1;
    const bdops::op_t operations[] = { bdops::i2c_wait_ready(0, 0) };
    assert(bdops::run_ops(backend(fake), devices, 1, operations, 1, gpio_scope).status
           == bdops::op_status_t::timeout);
    assert(fake.ready_attempts == 0 && fake.now == 0);
  }
  {
    fake_t fake;
    fake.ready_failures = 1;
    fake.ready_attempt_ms = 26;
    const bdops::op_t operations[] = { bdops::i2c_wait_ready(0, 5) };
    assert(bdops::run_ops(backend(fake), devices, 1, operations, 1, gpio_scope).status
           == bdops::op_status_t::timeout);
    assert(fake.ready_attempts == 1 && fake.now == 26);
  }
  {
    fake_t fake;
    const bdops::op_t operations[] = {
      bdops::gpio_write_high(4), bdops::gpio_set_mode(4, bdops::gpio_mode_t::output),
      bdops::gpio_write_low(5), bdops::delay_ms(7),
    };
    assert(bdops::run_ops(backend(fake), devices, 1, operations, 4, gpio_scope).status
           == bdops::op_status_t::ok);
    assert(fake.gpios.size() == 3 && fake.now == 7);
    assert(fake.gpios[0].pin == 4 && fake.gpios[0].value == 1);
    assert(fake.gpios[1].pin == 4 && fake.gpios[1].value == 100);
    assert(fake.gpios[2].pin == 5 && fake.gpios[2].value == 0);
  }
}

template <std::size_t LegacyN, std::size_t NewN, std::size_t DeviceN>
static void assert_same_sequence(const legacy_write_t (&legacy)[LegacyN],
                                 const bdops::op_t (&converted)[NewN],
                                 const bdops::i2c_device_t (&device_table)[DeviceN],
                                 std::uint32_t frequency)
{
  static_assert(LegacyN == NewN, "operation count changed during conversion");
  for (std::size_t i = 0; i < LegacyN; ++i)
  {
    bdops::decoded_register_write_t write;
    assert(bdops::decode_register_write(converted[i], &write));
    assert(write.dev == 0 && write.dev < DeviceN);
    const auto write_mask = static_cast<std::uint8_t>(~legacy[i].keep) | legacy[i].value;
    assert(device_table[write.dev].addr == legacy[i].addr);
    assert(device_table[write.dev].freq_hz == frequency);
    assert(write.reg == legacy[i].reg);
    assert(write.value == (legacy[i].value & write_mask));
    assert(write.mask == write_mask);
  }
}

static void test_legacy_conversion()
{
  assert_same_sequence(legacy_power192, pmicops::power192,
                       pmicops::core2_devices, 400000);
  assert_same_sequence(legacy_power2101, pmicops::power2101,
                       pmicops::core2_devices, 400000);
  assert_same_sequence(legacy_reset192, pmicops::reset192,
                       pmicops::core2_devices, 400000);
  assert_same_sequence(legacy_release192, pmicops::release192,
                       pmicops::core2_devices, 400000);
  assert_same_sequence(legacy_reset2101, pmicops::reset2101,
                       pmicops::core2_devices, 400000);
  assert_same_sequence(legacy_release2101, pmicops::release2101,
                       pmicops::core2_devices, 400000);
  assert_same_sequence(legacy_sticks3, pmicops::sticks3_power_on,
                       pmicops::pm1_devices, 100000);
}

template <std::size_t N>
static std::uint8_t converted_mask(const bdops::op_t (&operations)[N], std::uint16_t reg)
{
  std::uint8_t mask = 0;
  for (std::size_t i = 0; i < N; ++i)
  {
    bdops::decoded_register_write_t write;
    if (bdops::decode_register_write(operations[i], &write) && write.reg == reg)
    {
      mask |= write.mask;
    }
  }
  return mask;
}

static void test_core2_restore_coverage()
{
  static const std::uint16_t restore192[] = { 0x12, 0x28, 0x92, 0x95, 0x96, 0x94 };
  static const std::uint8_t expected192[] = { 0x04, 0xF0, 0x07, 0x8D, 0x02, 0x02 };
  static const std::uint16_t restore2101[] = { 0x80, 0x82, 0x84, 0x90 };
  static const std::uint8_t expected2101[] = { 0x05, 0xFF, 0xFF, 0x0A };
  for (std::size_t i = 0; i < sizeof(restore192) / sizeof(restore192[0]); ++i)
  {
    const auto mask = converted_mask(pmicops::power192, restore192[i])
                    | converted_mask(pmicops::reset192, restore192[i])
                    | converted_mask(pmicops::release192, restore192[i]);
    assert(mask == expected192[i]);
  }
  for (std::size_t i = 0; i < sizeof(restore2101) / sizeof(restore2101[0]); ++i)
  {
    const auto mask = converted_mask(pmicops::power2101, restore2101[i])
                    | converted_mask(pmicops::reset2101, restore2101[i])
                    | converted_mask(pmicops::release2101, restore2101[i]);
    assert(mask == expected2101[i]);
  }
}

static bdops::op_status_t run_legacy_sticks3(fake_t& fake, std::uint32_t deadline)
{
  const auto& device = pmicops::pm1_devices[0];
  for (const auto& write : legacy_sticks3)
  {
    for (;;)
    {
      bool ok = false;
      if (write.keep == 0)
      {
        ok = write8(&fake, device, write.reg, write.value);
      }
      else
      {
        std::uint8_t old_value = 0;
        if (read8(&fake, device, write.reg, &old_value))
        {
          const auto new_value = static_cast<std::uint8_t>(
            (old_value & write.keep) | write.value);
          ok = write8(&fake, device, write.reg, new_value);
        }
      }
      if (ok) { break; }
      if (static_cast<std::int32_t>(fake.now - deadline) >= 0)
      {
        return bdops::op_status_t::timeout;
      }
      delay(&fake, 1);
    }
  }
  return bdops::op_status_t::ok;
}

static bool run_sticks3_reads(fake_t& fake, std::uint32_t deadline)
{
  const auto& device = pmicops::pm1_devices[0];
  const std::uint16_t registers[] = { 0x00, 0x11 };
  for (auto reg : registers)
  {
    for (;;)
    {
      std::uint8_t value = 0;
      if (read8(&fake, device, reg, &value)) { break; }
      if (static_cast<std::int32_t>(fake.now - deadline) >= 0) { return false; }
      delay(&fake, 1);
    }
  }
  return true;
}

static void assert_sticks3_retry_equivalent(int write_failures,
                                             const std::vector<int>& read_nacks)
{
  fake_t legacy;
  fake_t converted;
  legacy.write_failures = write_failures;
  converted.write_failures = write_failures;
  legacy.read_nacks = read_nacks;
  converted.read_nacks = read_nacks;
  const auto legacy_status = run_sticks3_reads(legacy, 200)
                           ? run_legacy_sticks3(legacy, 200)
                           : bdops::op_status_t::timeout;
  bdops::op_status_t converted_status = bdops::op_status_t::timeout;
  if (run_sticks3_reads(converted, 200))
  {
    const bdops::retry_policy_t policy { 200, 1 };
    converted_status = bdops::run_ops(
      backend(converted), pmicops::pm1_devices, 1,
      pmicops::sticks3_power_on,
      sizeof(pmicops::sticks3_power_on) / sizeof(pmicops::sticks3_power_on[0]),
      gpio_scope, &policy).status;
  }
  assert(converted_status == legacy_status);
  assert(converted.now == legacy.now);
  assert(converted.reads == legacy.reads);
  assert(converted.writes_attempted == legacy.writes_attempted);
  assert(converted.registers == legacy.registers);
  assert(converted.writes.size() == legacy.writes.size());
  for (std::size_t i = 0; i < legacy.writes.size(); ++i)
  {
    assert(converted.writes[i].addr == legacy.writes[i].addr);
    assert(converted.writes[i].reg == legacy.writes[i].reg);
    assert(converted.writes[i].old_value == legacy.writes[i].old_value);
    assert(converted.writes[i].new_value == legacy.writes[i].new_value);
  }
}

static void test_sticks3_retry_equivalence()
{
  const int failure_counts[] = { 0, 3, 200, 201 };
  for (auto failures : failure_counts)
  {
    assert_sticks3_retry_equivalent(failures, {});
  }
  // Attempts 1/2 exercise ID-read NACKs; attempts 2/3 exercise state-read NACKs.
  assert_sticks3_retry_equivalent(0, { 1, 2 });
  assert_sticks3_retry_equivalent(0, { 2, 3 });
}

static void test_pm1_family_sequences()
{
  {
    fake_t fake;
    static const std::int8_t pins[] = { 39 };
    const bdops::gpio_scope_t scope = { 49, pins, 1 };
    const auto result = bdops::run_ops(
      backend(fake), pmicops::pm1_family_devices, 2,
      pmicops::stopwatch_power_on,
      sizeof(pmicops::stopwatch_power_on) / sizeof(pmicops::stopwatch_power_on[0]), scope);
    assert(result.status == bdops::op_status_t::ok);
    assert(fake.writes.size() == 12 && fake.gpios.size() == 2 && fake.now == 20);
    assert(fake.writes[0].addr == 0x6E && fake.writes[0].reg == 0x09);
    assert(fake.writes[3].addr == 0x4F && fake.writes[3].reg == 0x23);
    assert(fake.writes.back().addr == 0x4F && fake.writes.back().reg == 0x06);
    assert(fake.gpios[0].pin == 39 && fake.gpios[0].value == 100);
    assert(fake.gpios[1].pin == 39 && fake.gpios[1].value == 1);
  }
  {
    fake_t fake;
    static const std::int8_t pins[] = { 16 };
    const bdops::gpio_scope_t scope = { 49, pins, 1 };
    const auto result = bdops::run_ops(
      backend(fake), pmicops::pm1_family_devices, 2,
      pmicops::papermono_power_on,
      sizeof(pmicops::papermono_power_on) / sizeof(pmicops::papermono_power_on[0]), scope);
    assert(result.status == bdops::op_status_t::ok);
    assert(fake.writes.size() == 11 && fake.gpios.size() == 2 && fake.now == 10);
    assert(fake.writes[0].addr == 0x6E && fake.writes[0].reg == 0x09);
    assert(fake.writes[3].addr == 0x4F && fake.writes[3].reg == 0x03);
    assert(fake.writes.back().addr == 0x4F && fake.writes.back().reg == 0x05);
    assert(fake.gpios[0].pin == 16 && fake.gpios[0].value == 100);
    assert(fake.gpios[1].pin == 16 && fake.gpios[1].value == 1);
  }
}

static void test_pm1_ext_family_sequences()
{
  {
    fake_t fake;
    const bdops::gpio_scope_t scope = { 49, nullptr, 0 };
    assert(bdops::run_ops(
      backend(fake), pmicops::pm1_family_devices, 2,
      pmicops::chaincaptain_power_on,
      sizeof(pmicops::chaincaptain_power_on) / sizeof(pmicops::chaincaptain_power_on[0]),
      scope).status == bdops::op_status_t::ok);
    assert(fake.writes.size() == 8);
    assert(fake.writes[0].addr == 0x6E && fake.writes[0].reg == 0x09);
    assert(fake.writes[2].addr == 0x4F && fake.writes[2].reg == 0x23);
    assert(fake.writes.back().addr == 0x4F && fake.writes.back().reg == 0x03);
    assert(bdops::run_ops(
      backend(fake), pmicops::pm1_family_devices, 2,
      pmicops::chaincaptain_reset_assert, 1, scope).status == bdops::op_status_t::ok);
    delay(&fake, 10);
    assert(bdops::run_ops(
      backend(fake), pmicops::pm1_family_devices, 2,
      pmicops::chaincaptain_reset_release, 1, scope).status == bdops::op_status_t::ok);
    delay(&fake, 20);
    assert(fake.writes[8].reg == 0x05 && fake.writes[8].new_value == 0);
    assert(fake.writes[9].reg == 0x05 && fake.writes[9].new_value == 1);
    assert(fake.now == 30);

    fake_t skipped;
    assert(bdops::run_ops(
      backend(skipped), pmicops::pm1_family_devices, 2,
      pmicops::chaincaptain_reset_release, 1, scope).status == bdops::op_status_t::ok);
    delay(&skipped, 20);
    assert(skipped.writes.size() == 1 && skipped.writes[0].reg == 0x05);
    assert(skipped.writes[0].new_value == 1 && skipped.now == 20);
  }
  {
    fake_t fake;
    static const std::int8_t pins[] = { 44 };
    const bdops::gpio_scope_t scope = { 49, pins, 1 };
    assert(bdops::run_ops(
      backend(fake), pmicops::pm1_devices, 1,
      pmicops::papercolor_power_on,
      sizeof(pmicops::papercolor_power_on) / sizeof(pmicops::papercolor_power_on[0]),
      scope).status == bdops::op_status_t::ok);
    assert(fake.writes.size() == 6 && fake.now == 100 && fake.gpios.size() == 2);
    assert(fake.writes[1].reg == 0x16 && fake.writes[1].new_value == 0);
    assert(fake.writes[2].reg == 0x10 && fake.writes[2].new_value == 0x09);
    assert(fake.writes[3].reg == 0x13 && fake.writes[3].new_value == 0);
    assert(fake.writes[4].reg == 0x11 && fake.writes[4].new_value == 0x09);
    assert(fake.gpios[0].pin == 44 && fake.gpios[0].value == 100);
    assert(fake.gpios[1].pin == 44 && fake.gpios[1].value == 1);
  }
}

static void test_paper_family_sequences()
{
  fake_t fake;
  const bdops::gpio_scope_t scope = { 49, nullptr, 0 };
  assert(bdops::run_ops(
    backend(fake), pmicops::pm1_devices, 1,
    pmicops::paperdiy_power_on,
    sizeof(pmicops::paperdiy_power_on) / sizeof(pmicops::paperdiy_power_on[0]),
    scope).status == bdops::op_status_t::ok);
  // Legacy order: 0x09, 0x0A full writes, then GPIO2 function/direction/
  // push-pull/output bits, then a 10 ms settle; no SoC GPIO is touched.
  assert(fake.writes.size() == 6 && fake.gpios.empty() && fake.now == 10);
  assert(fake.writes[0].addr == 0x6E && fake.writes[0].reg == 0x09 && fake.writes[0].new_value == 0);
  assert(fake.writes[1].reg == 0x0A && fake.writes[1].new_value == 0);
  assert(fake.writes[2].reg == 0x16 && fake.writes[2].new_value == 0);
  assert(fake.writes[3].reg == 0x10 && fake.writes[3].new_value == 0x04);
  assert(fake.writes[4].reg == 0x13 && fake.writes[4].new_value == 0);
  assert(fake.writes[5].reg == 0x11 && fake.writes[5].new_value == 0x04);
}

static void test_dedicated_release_summary()
{
  const std::uint16_t samples[] = { 3, 4, 0, 3, bdetect::dedicated_release_no_high };
  std::uint8_t valid = 0;
  assert(bdetect::median_dedicated_release_samples(samples, 5, 256, &valid)
         == bdetect::dedicated_release_no_high);
  assert(valid == 4);
  const std::uint16_t complete[] = { 13, 15, 11, 14, 12 };
  assert(bdetect::median_dedicated_release_samples(complete, 5, 256, &valid) == 13);
  assert(valid == 5);

  bdetect::dedicated_release_result_t result;
  result.available = true;
  result.pin_count = 8;
  result.requested_samples = 9;
  result.reads = 256;
  result.cpu_hz = 240000000;
  result.total_cycles = 18524;  // 9 passes at about 8.04 cycles/read.
  const auto set_values = [&](std::uint16_t value)
  {
    for (std::uint8_t index = 0; index < result.pin_count; ++index)
    {
      result.valid_samples[index] = result.requested_samples;
      result.median_first_high[index] = value;
    }
  };
  set_values(6);
  auto summary = bdetect::summarize_dedicated_release(result, 260, 330);
  assert(summary.average_ns == 201 && summary.valid_pins == 8 && summary.pin_count == 8);
  assert(summary.band == bdetect::pin_release_band_t::short_release);

  set_values(10);
  summary = bdetect::summarize_dedicated_release(result, 260, 330);
  assert(summary.average_ns == 335 && summary.band == bdetect::pin_release_band_t::long_release);

  set_values(8);
  summary = bdetect::summarize_dedicated_release(result, 260, 330);
  assert(summary.average_ns == 268 && summary.band == bdetect::pin_release_band_t::ambiguous);

  result.valid_samples[3] = 8;
  summary = bdetect::summarize_dedicated_release(result, 260, 330);
  assert(summary.valid_pins == 7 && summary.band == bdetect::pin_release_band_t::ambiguous);
  result.valid_samples[3] = 9;
  summary = bdetect::summarize_dedicated_release(result, 330, 260);
  assert(summary.band == bdetect::pin_release_band_t::ambiguous);
}

int main()
{
  static_assert(sizeof(bdops::op_t) <= 16, "source IR operation grew beyond its ROM budget");
  test_validation();
  test_i2c_execution();
  test_wait_and_gpio();
  test_legacy_conversion();
  test_core2_restore_coverage();
  test_sticks3_retry_equivalence();
  test_pm1_family_sequences();
  test_pm1_ext_family_sequences();
  test_paper_family_sequences();
  test_dedicated_release_summary();
  return 0;
}
