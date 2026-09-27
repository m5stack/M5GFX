import { assertM5GFXSentinelTypes, DEVICE_KIND_SPEC_KEYS } from "../parts.js";

const SPI_HOSTS = { SPI1_HOST: 0, SPI2_HOST: 1, SPI3_HOST: 2 };

function integer(value, label) {
  if (!Number.isInteger(value)) throw new Error(`${label} must be an integer`);
  return value;
}

function host(value) {
  if (Number.isInteger(value)) return value;
  if (Object.prototype.hasOwnProperty.call(SPI_HOSTS, value)) return SPI_HOSTS[value];
  throw new Error(`unsupported SPI host ${value}`);
}

function hex(value, label) {
  if (typeof value !== "string" || !/^0x[0-9a-f]+$/i.test(value)) throw new Error(`${label} must be hexadecimal`);
  return Number.parseInt(value.slice(2), 16);
}

function probe(part, label) {
  const source = part?.id_probe;
  if (!source) throw new Error(`${label} has no id_probe`);
  const values = source.values ?? (source.value === undefined ? [] : [source.value]);
  if (!values.length) throw new Error(`${label} id_probe has no values`);
  return {
    cmd: hex(source.reg, `${label}.id_probe.reg`),
    mask: hex(source.mask, `${label}.id_probe.mask`),
    values: values.map((value) => hex(value, `${label}.id_probe.value`)),
    ...(source.dummy_bits === undefined ? {} : {
      dummyBits: integer(source.dummy_bits, `${label}.id_probe.dummy_bits`),
    }),
  };
}

function optionalInteger(value, label) {
  return value === undefined ? null : integer(value, label);
}

function optionalBoolean(value, label) {
  if (value === undefined) return null;
  if (typeof value !== "boolean") throw new Error(`${label} must be a boolean`);
  return value;
}

function partDefault(definitions, key) {
  return definitions?.[key]?.default;
}

function panel(spec, definitions, label) {
  assertM5GFXSentinelTypes(definitions, label);
  const width = integer(spec.width, `${label}.width`);
  const height = integer(spec.height, `${label}.height`);
  const result = {
    width,
    height,
    memoryWidth: optionalInteger(spec.memory_width ?? partDefault(definitions, "memory_width"), `${label}.memory_width`),
    memoryHeight: optionalInteger(spec.memory_height ?? partDefault(definitions, "memory_height"), `${label}.memory_height`),
    offsetX: optionalInteger(spec.offset_x ?? partDefault(definitions, "offset_x"), `${label}.offset_x`),
    offsetY: optionalInteger(spec.offset_y ?? partDefault(definitions, "offset_y"), `${label}.offset_y`),
    rotationOffset: optionalInteger(spec.rotation_offset ?? partDefault(definitions, "rotation_offset"), `${label}.rotation_offset`),
    invert: optionalBoolean(spec.invert ?? partDefault(definitions, "invert"), `${label}.invert`),
    readable: optionalBoolean(spec.readable ?? partDefault(definitions, "readable"), `${label}.readable`),
    ...(spec.rgb_order === undefined && partDefault(definitions, "rgb_order") === undefined ? {} : {
      rgbOrder: optionalBoolean(spec.rgb_order ?? partDefault(definitions, "rgb_order"), `${label}.rgb_order`),
    }),
    ...(spec.line_padding === undefined && partDefault(definitions, "line_padding") === undefined ? {} : {
      linePadding: optionalInteger(spec.line_padding ?? partDefault(definitions, "line_padding"), `${label}.line_padding`),
    }),
  };
  Object.defineProperty(result, "cppTypes", {
    value: Object.fromEntries(Object.entries(definitions ?? {}).map(([key, definition]) => [key, definition["x-cpp-type"] ?? "int"])),
  });
  return result;
}

export function emitM5GFXSpecs(board, resolvedVariants, parts, mapping) {
  if (!mapping?.specsOutput) return null;
  const display = Object.values(board.devices ?? {}).find((device) => device.kind === "display");
  const bus = board.buses?.[display?.bus];
  const backlightDevice = board.devices?.backlight;
  const backlightBus = board.buses?.[backlightDevice?.bus];
  const backlight = backlightBus?.kind === "i2c" ? null : backlightDevice?.spec;
  const backlightI2c = backlightBus?.kind === "i2c" ? {
    i2cAddr: hex(backlightDevice?.i2c_addr, `${board.id}.backlight.i2c_addr`),
  } : null;
  if (!bus || !display) throw new Error(`${board.id} specs are incomplete`);
  const panels = {};
  const probes = {};
  for (const variant of resolvedVariants) {
    const device = Object.values(variant.devices ?? {}).find((item) => item.kind === "display");
    const name = device?.part;
    if (!name || panels[name]) continue;
    panels[name] = panel(device.spec ?? {}, parts[name]?.spec_keys, `${board.id}.${name}`);
    if (parts[name]?.id_probe) probes[name] = probe(parts[name], name);
  }
  const pmicDevice = Object.values(board.devices ?? {}).find((device) => device.kind === "pmic");
  const pmicPart = parts[pmicDevice?.part];
  const pmicBus = board.buses?.[pmicDevice?.bus];
  const pmic = pmicDevice ? {
    i2cAddr: hex(pmicPart?.i2c_addr?.[0], `${board.id}.${pmicDevice.part}.i2c_addr`),
    i2cFreq: integer(pmicBus?.freq, `${board.id}.${pmicDevice.bus}.freq`),
    idReg: hex(pmicPart?.id_probe?.reg, `${board.id}.${pmicDevice.part}.id_probe.reg`),
    ...(pmicPart?.id_probe?.match === "ack_only" ? {} : {
      idValue: hex(pmicPart?.id_probe?.value, `${board.id}.${pmicDevice.part}.id_probe.value`),
    }),
  } : null;
  const touchDevice = Object.values(board.devices ?? {}).find((device) => device.kind === "touch" && board.buses?.[device.bus]?.kind === "i2c");
  const touchPart = parts[touchDevice?.part];
  const touchBus = board.buses?.[touchDevice?.bus];
  const touchSpec = touchDevice?.spec ?? {};
  if (touchDevice) assertM5GFXSentinelTypes(touchPart?.spec_keys, `${board.id}.${touchDevice.part}`);
  // A part with several addresses and no board value leaves the address to
  // the touch driver's own default/probe order (i2cAddr === null).
  const touchAddr = touchDevice?.i2c_addr ?? (touchPart?.i2c_addr?.length === 1 ? touchPart.i2c_addr[0] : undefined);
  const touch = touchDevice ? {
    i2cAddr: touchAddr === undefined ? null : hex(touchAddr, `${board.id}.${touchDevice.part}.i2c_addr`),
    i2cFreq: integer(touchBus?.freq, `${board.id}.${touchDevice.bus}.freq`),
    xMin: integer(touchSpec.x_min ?? partDefault(touchPart?.spec_keys, "x_min"), `${board.id}.${touchDevice.part}.x_min`),
    xMax: integer(touchSpec.x_max, `${board.id}.${touchDevice.part}.x_max`),
    yMin: integer(touchSpec.y_min ?? partDefault(touchPart?.spec_keys, "y_min"), `${board.id}.${touchDevice.part}.y_min`),
    yMax: integer(touchSpec.y_max, `${board.id}.${touchDevice.part}.y_max`),
    rotationOffset: optionalInteger(touchSpec.rotation_offset, `${board.id}.${touchDevice.part}.rotation_offset`),
  } : null;
  const powerHold = Object.values(board.devices ?? {}).find((device) => device.kind === "power_hold");
  const identityDevices = Object.fromEntries((mapping.identityDevices ?? []).map((deviceId) => {
    const device = board.devices?.[deviceId];
    const part = parts[device?.part];
    const bus = board.buses?.[device?.bus];
    if (!device || !part?.id_probe || bus?.kind !== "i2c") {
      throw new Error(`${board.id}.${deviceId} is not an identifiable I2C device`);
    }
    const address = device.i2c_addr ?? part.i2c_addr?.[0];
    return [deviceId, {
      i2cAddr: hex(address, `${board.id}.${deviceId}.i2c_addr`),
      i2cFreq: integer(device.spec?.probe_freq ?? bus.freq, `${board.id}.${deviceId}.probe_freq`),
      idReg: hex(part.id_probe.reg, `${board.id}.${deviceId}.id_probe.reg`),
      ...(part.id_probe.match === "ack_only" ? {} : {
        idValue: hex(part.id_probe.value, `${board.id}.${deviceId}.id_probe.value`),
      }),
      ...(device.spec?.firmware_reg === undefined ? {} : {
        firmwareReg: integer(device.spec.firmware_reg, `${board.id}.${deviceId}.firmware_reg`),
        firmwareMin: integer(device.spec.firmware_min, `${board.id}.${deviceId}.firmware_min`),
      }),
    }];
  }));
  const assignments = Object.fromEntries(Object.entries(board.pins ?? {}).flatMap(([pin, row]) =>
    (row.roles ?? []).map((role) => [role, Number(pin)])));
  const releaseProbe = mapping.releaseProbe ? {
    pins: mapping.releaseProbe.pins.map((role) => {
      if (!Object.prototype.hasOwnProperty.call(assignments, role)) throw new Error(`${board.id}: release probe role ${role} has no GPIO`);
      return assignments[role];
    }),
    reads: integer(mapping.releaseProbe.reads, `${board.id}.release_probe.reads`),
    samples: integer(mapping.releaseProbe.samples, `${board.id}.release_probe.samples`),
    settleUs: integer(mapping.releaseProbe.settle_us, `${board.id}.release_probe.settle_us`),
    shortMaxNs: integer(mapping.releaseProbe.short_max_ns, `${board.id}.release_probe.short_max_ns`),
    longMinNs: integer(mapping.releaseProbe.long_min_ns, `${board.id}.release_probe.long_min_ns`),
  } : null;
  return {
    namespace: mapping.cppNamespace,
    bus: bus.kind === "parallel_epd" ? {
      kind: "parallel_epd",
      speed: integer(bus.freq, `${board.id}.${display.bus}.freq`),
      width: (bus.signals ?? []).filter((signal) => /^data\d+$/.test(signal)).length,
    } : bus.kind === "i2c" ? {
      kind: "i2c",
      port: integer(bus.preferred_host, `${board.id}.${display.bus}.preferred_host`),
      freqWrite: integer(bus.freq, `${board.id}.${display.bus}.freq`),
      freqRead: integer(bus.freq_read ?? bus.freq, `${board.id}.${display.bus}.freq_read`),
      addr: hex(display.i2c_addr, `${board.id}.${display.part}.i2c_addr`),
      prefixLen: 0,
    } : {
      host: host(bus.preferred_host),
      hostSymbol: typeof bus.preferred_host === "string" ? bus.preferred_host : null,
      freqWrite: integer(bus.freq, `${board.id}.${display.bus}.freq`),
      freqRead: integer(bus.freq_read, `${board.id}.${display.bus}.freq_read`),
      threeWire: display.spec?.three_wire ?? !(bus.signals ?? []).includes("miso"),
    },
    // Emitted only when the board states the level; boards without it keep
    // the active-high default and their generated header unchanged.
    powerHold: powerHold?.spec?.active_low === undefined ? null : {
      activeLow: optionalBoolean(powerHold.spec.active_low, `${board.id}.power_hold.active_low`),
    },
    panels,
    probes,
    backlight: backlight ? {
      freq: integer(backlight.freq, `${board.id}.backlight.freq`),
      channel: integer(backlight.channel, `${board.id}.backlight.channel`),
      invert: optionalBoolean(backlight.invert ?? partDefault(DEVICE_KIND_SPEC_KEYS.backlight, "invert"), `${board.id}.backlight.invert`),
      offset: integer(backlight.offset ?? partDefault(DEVICE_KIND_SPEC_KEYS.backlight, "offset"), `${board.id}.backlight.offset`),
    } : null,
    backlightI2c,
    pmic,
    touch,
    identityDevices,
    releaseProbe,
  };
}

function boolean(value) { return value ? "true" : "false"; }
function optionalFlag(value) { return value === null ? "-1" : (value ? "1" : "0"); }
function uint(value) { return `0x${value.toString(16).toUpperCase()}`; }

export function renderM5GFXSpecsHeader(specs) {
  const entries = Array.isArray(specs) ? specs : [specs];
  const lines = [
    "// Generated from extras/board_spec; do not edit by hand.",
    "// Unspecified values use target-specific unknown or sentinel values.",
    "// Precedence: board value > part default > panel-class default.",
    "#pragma once",
    "",
    "#include <cstdint>",
    '#include "../setup_sentinels.hpp"',
  ];
  for (const entry of entries) {
    lines.push("", `namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace ${entry.namespace} {`);
    if (entry.bus.kind === "parallel_epd") {
      lines.push(`constexpr std::uint32_t bus_speed = ${entry.bus.speed};`,
        `constexpr std::uint8_t bus_width = ${entry.bus.width};`);
    } else if (entry.bus.kind === "i2c") {
      lines.push(`constexpr int bus_port = ${entry.bus.port};`,
        `constexpr std::uint32_t bus_freq_write = ${entry.bus.freqWrite};`,
        `constexpr std::uint32_t bus_freq_read = ${entry.bus.freqRead};`,
        `constexpr std::uint8_t bus_i2c_addr = ${uint(entry.bus.addr)};`,
        `constexpr std::uint8_t bus_prefix_len = ${entry.bus.prefixLen};`);
    } else {
      lines.push(`constexpr int bus_host = ${entry.bus.hostSymbol ?? entry.bus.host};`,
        `constexpr std::uint32_t bus_freq_write = ${entry.bus.freqWrite};`,
        `constexpr std::uint32_t bus_freq_read = ${entry.bus.freqRead};`,
        `constexpr bool bus_three_wire = ${boolean(entry.bus.threeWire)};`);
    }
    for (const [name, value] of Object.entries(entry.panels)) {
      const cpp = (key) => value.cppTypes[key] ?? "int";
      lines.push("", `namespace panel_${name} {`,
      `  constexpr ${cpp("width")} width = ${value.width};`,
      `  constexpr ${cpp("height")} height = ${value.height};`,
      `  constexpr ${cpp("memory_width")} memory_width = ${value.memoryWidth ?? "setup_sentinel::keep_dimension"};`,
      `  constexpr ${cpp("memory_height")} memory_height = ${value.memoryHeight ?? "setup_sentinel::keep_dimension"};`,
      `  constexpr ${cpp("offset_x")} offset_x = ${value.offsetX ?? "setup_sentinel::keep_offset"};`,
      `  constexpr ${cpp("offset_y")} offset_y = ${value.offsetY ?? "setup_sentinel::keep_offset"};`,
      `  constexpr ${cpp("rotation_offset")} rotation_offset = ${value.rotationOffset ?? "setup_sentinel::keep_u8"};`,
      `  constexpr ${cpp("invert")} invert = ${value.invert === null ? "setup_sentinel::keep_i8" : optionalFlag(value.invert)};`,
      `  constexpr ${cpp("readable")} readable = ${value.readable === null ? "setup_sentinel::keep_i8" : optionalFlag(value.readable)};`,
      ...(value.rgbOrder === undefined || value.rgbOrder === null ? [] :
        [`  constexpr ${cpp("rgb_order")} rgb_order = ${optionalFlag(value.rgbOrder)};`]),
      ...(value.linePadding === undefined || value.linePadding === null ? [] :
        [`  constexpr ${cpp("line_padding")} line_padding = ${value.linePadding};`]),
        `} // namespace panel_${name}`);
    }
    for (const [name, value] of Object.entries(entry.probes)) {
      lines.push("", `namespace probe_${name} {`,
      `  constexpr std::uint8_t cmd = ${uint(value.cmd)};`,
      ...(value.dummyBits === undefined ? [] :
        [`  constexpr std::uint8_t dummy_bits = ${value.dummyBits};`]),
      `  constexpr std::uint32_t mask = ${uint(value.mask)};`,
      `  constexpr std::uint32_t values[] = { ${value.values.map(uint).join(", ")} };`,
        `} // namespace probe_${name}`);
    }
    if (entry.backlight) {
      lines.push("", "namespace backlight {",
        `  constexpr std::uint32_t freq = ${entry.backlight.freq};`,
        `  constexpr std::uint8_t channel = ${entry.backlight.channel};`,
        `  constexpr bool invert = ${boolean(entry.backlight.invert)};`,
        `  constexpr std::uint8_t offset = ${entry.backlight.offset};`,
        "} // namespace backlight");
    }
    if (entry.backlightI2c) {
      lines.push("", "namespace backlight_i2c {",
        `  constexpr std::uint8_t i2c_addr = ${uint(entry.backlightI2c.i2cAddr)};`,
        "} // namespace backlight_i2c");
    }
    if (entry.pmic) {
      lines.push("", "namespace pmic {",
        `  constexpr std::uint8_t i2c_addr = ${uint(entry.pmic.i2cAddr)};`,
        `  constexpr std::uint32_t i2c_freq = ${entry.pmic.i2cFreq};`,
        `  constexpr std::uint8_t id_reg = ${uint(entry.pmic.idReg)};`,
        ...(entry.pmic.idValue === undefined ? [] :
          [`  constexpr std::uint8_t id_value = ${uint(entry.pmic.idValue)};`]),
        "} // namespace pmic");
    }
    if (entry.powerHold) {
      lines.push("", "namespace power_hold {",
        `  constexpr bool active_low = ${boolean(entry.powerHold.activeLow)};`,
        "} // namespace power_hold");
    }
    if (entry.touch) {
      lines.push("", "namespace touch {",
        ...(entry.touch.i2cAddr === null ? [] :
          [`  constexpr std::uint8_t i2c_addr = ${uint(entry.touch.i2cAddr)};`]),
        `  constexpr std::uint32_t i2c_freq = ${entry.touch.i2cFreq};`,
        `  constexpr int x_min = ${entry.touch.xMin};`,
        `  constexpr int x_max = ${entry.touch.xMax};`,
        `  constexpr int y_min = ${entry.touch.yMin};`,
        `  constexpr int y_max = ${entry.touch.yMax};`,
        `  constexpr int rotation_offset = ${entry.touch.rotationOffset ?? "setup_sentinel::keep_u8"};`,
        "} // namespace touch");
    }
    for (const [name, value] of Object.entries(entry.identityDevices ?? {})) {
      lines.push("", `namespace i2c_${name} {`,
        `  constexpr std::uint8_t i2c_addr = ${uint(value.i2cAddr)};`,
        `  constexpr std::uint32_t i2c_freq = ${value.i2cFreq};`,
        `  constexpr std::uint8_t id_reg = ${uint(value.idReg)};`,
        ...(value.idValue === undefined ? [] : [`  constexpr std::uint8_t id_value = ${uint(value.idValue)};`]),
        ...(value.firmwareReg === undefined ? [] : [
          `  constexpr std::uint8_t firmware_reg = ${uint(value.firmwareReg)};`,
          `  constexpr std::uint8_t firmware_min = ${uint(value.firmwareMin)};`,
        ]),
        `} // namespace i2c_${name}`);
    }
    if (entry.releaseProbe) {
      lines.push("", "namespace release_probe {",
        `  constexpr std::int8_t pins[] = { ${entry.releaseProbe.pins.join(", ")} };`,
        `  constexpr std::uint16_t reads = ${entry.releaseProbe.reads};`,
        `  constexpr std::uint8_t samples = ${entry.releaseProbe.samples};`,
        `  constexpr std::uint32_t settle_us = ${entry.releaseProbe.settleUs};`,
        `  constexpr std::uint32_t short_max_ns = ${entry.releaseProbe.shortMaxNs};`,
        `  constexpr std::uint32_t long_min_ns = ${entry.releaseProbe.longMinNs};`,
        "} // namespace release_probe");
    }
    lines.push("", `} } } } } // namespace m5gfx::board_detect::m5::specs::${entry.namespace}`);
  }
  lines.push("");
  return lines.join("\n");
}
