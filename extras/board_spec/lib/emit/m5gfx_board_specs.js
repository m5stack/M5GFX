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
  };
  Object.defineProperty(result, "cppTypes", {
    value: Object.fromEntries(Object.entries(definitions ?? {}).map(([key, definition]) => [key, definition["x-cpp-type"] ?? "int"])),
  });
  return result;
}

export function emitM5GFXSpecs(board, resolvedVariants, parts, mapping) {
  if (!mapping?.specsOutput) return null;
  const bus = board.buses?.main_spi;
  const display = board.devices?.lcd;
  const backlight = board.devices?.backlight?.spec;
  if (!bus || !display || !backlight) throw new Error(`${board.id} specs are incomplete`);
  const panels = {};
  const probes = {};
  for (const variant of resolvedVariants) {
    const device = variant.devices?.lcd;
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
  return {
    namespace: mapping.cppNamespace,
    bus: {
      host: host(bus.preferred_host),
      hostSymbol: typeof bus.preferred_host === "string" ? bus.preferred_host : null,
      freqWrite: integer(bus.freq, `${board.id}.main_spi.freq`),
      freqRead: integer(bus.freq_read, `${board.id}.main_spi.freq_read`),
      threeWire: !(bus.signals ?? []).includes("miso"),
    },
    panels,
    probes,
    backlight: {
      freq: integer(backlight.freq, `${board.id}.backlight.freq`),
      channel: integer(backlight.channel, `${board.id}.backlight.channel`),
      invert: optionalBoolean(backlight.invert, `${board.id}.backlight.invert`),
      offset: integer(backlight.offset, `${board.id}.backlight.offset`),
    },
    pmic,
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
    lines.push("", `namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace ${entry.namespace} {`,
      `constexpr int bus_host = ${entry.bus.hostSymbol ?? entry.bus.host};`,
      `constexpr std::uint32_t bus_freq_write = ${entry.bus.freqWrite};`,
      `constexpr std::uint32_t bus_freq_read = ${entry.bus.freqRead};`,
      `constexpr bool bus_three_wire = ${boolean(entry.bus.threeWire)};`);
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
        `} // namespace panel_${name}`);
    }
    for (const [name, value] of Object.entries(entry.probes)) {
      lines.push("", `namespace probe_${name} {`,
      `  constexpr std::uint8_t cmd = ${uint(value.cmd)};`,
      `  constexpr std::uint32_t mask = ${uint(value.mask)};`,
      `  constexpr std::uint32_t values[] = { ${value.values.map(uint).join(", ")} };`,
        `} // namespace probe_${name}`);
    }
    lines.push("", "namespace backlight {",
      `  constexpr std::uint32_t freq = ${entry.backlight.freq};`,
      `  constexpr std::uint8_t channel = ${entry.backlight.channel};`,
      `  constexpr bool invert = ${boolean(entry.backlight.invert)};`,
      `  constexpr std::uint8_t offset = ${entry.backlight.offset};`,
      "} // namespace backlight");
    if (entry.pmic) {
      lines.push("", "namespace pmic {",
        `  constexpr std::uint8_t i2c_addr = ${uint(entry.pmic.i2cAddr)};`,
        `  constexpr std::uint32_t i2c_freq = ${entry.pmic.i2cFreq};`,
        `  constexpr std::uint8_t id_reg = ${uint(entry.pmic.idReg)};`,
        ...(entry.pmic.idValue === undefined ? [] :
          [`  constexpr std::uint8_t id_value = ${uint(entry.pmic.idValue)};`]),
        "} // namespace pmic");
    }
    lines.push("", `} } } } } // namespace m5gfx::board_detect::m5::specs::${entry.namespace}`);
  }
  lines.push("");
  return lines.join("\n");
}
