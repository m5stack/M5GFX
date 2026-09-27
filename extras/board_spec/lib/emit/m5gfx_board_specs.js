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

function panel(spec, defaults, label) {
  const width = integer(spec.width, `${label}.width`);
  const height = integer(spec.height, `${label}.height`);
  return {
    width,
    height,
    memoryWidth: optionalInteger(spec.memory_width ?? defaults?.memory_width, `${label}.memory_width`),
    memoryHeight: optionalInteger(spec.memory_height ?? defaults?.memory_height, `${label}.memory_height`),
    offsetX: optionalInteger(spec.offset_x ?? defaults?.offset_x, `${label}.offset_x`),
    offsetY: optionalInteger(spec.offset_y ?? defaults?.offset_y, `${label}.offset_y`),
    rotationOffset: optionalInteger(spec.rotation_offset ?? defaults?.rotation_offset, `${label}.rotation_offset`),
    invert: optionalBoolean(spec.invert ?? defaults?.invert, `${label}.invert`),
    readable: optionalBoolean(spec.readable ?? defaults?.readable, `${label}.readable`),
  };
}

export function emitM5GFXSpecs(board, resolvedVariants, parts) {
  if (board.id !== "m5atoms3" && board.id !== "m5sticks3") return null;
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
    panels[name] = panel(device.spec ?? {}, parts[name]?.spec, `${board.id}.${name}`);
    if (parts[name]?.id_probe) probes[name] = probe(parts[name], name);
  }
  const pmicDevice = Object.values(board.devices ?? {}).find((device) => device.kind === "pmic");
  const pmicPart = parts[pmicDevice?.part];
  const pmicBus = board.buses?.[pmicDevice?.bus];
  const pmic = pmicDevice ? {
    i2cAddr: hex(pmicPart?.i2c_addr?.[0], `${board.id}.${pmicDevice.part}.i2c_addr`),
    i2cFreq: integer(pmicBus?.freq, `${board.id}.${pmicDevice.bus}.freq`),
    idReg: hex(pmicPart?.id_probe?.reg, `${board.id}.${pmicDevice.part}.id_probe.reg`),
    // StickS3 deliberately matches the PMIC by ACK only. Keep the catalog's
    // documented ID value, but do not generate a constant that detection ignores.
    ...(board.id === "m5sticks3" ? {} : {
      idValue: hex(pmicPart?.id_probe?.value, `${board.id}.${pmicDevice.part}.id_probe.value`),
    }),
  } : null;
  return {
    namespace: board.id === "m5atoms3" ? "atoms3" : "sticks3",
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
    "#pragma once",
    "",
    "#include <cstdint>",
  ];
  for (const entry of entries) {
    lines.push("", `namespace m5gfx { namespace board_detect { namespace m5 { namespace specs { namespace ${entry.namespace} {`,
      `constexpr int bus_host = ${entry.bus.hostSymbol ?? entry.bus.host};`,
      `constexpr std::uint32_t bus_freq_write = ${entry.bus.freqWrite};`,
      `constexpr std::uint32_t bus_freq_read = ${entry.bus.freqRead};`,
      `constexpr bool bus_three_wire = ${boolean(entry.bus.threeWire)};`);
    for (const [name, value] of Object.entries(entry.panels)) {
      lines.push("", `namespace panel_${name} {`,
      `  constexpr int width = ${value.width};`,
      `  constexpr int height = ${value.height};`,
      `  constexpr int memory_width = ${value.memoryWidth ?? 0};`,
      `  constexpr int memory_height = ${value.memoryHeight ?? 0};`,
      `  constexpr int offset_x = ${value.offsetX ?? -32768};`,
      `  constexpr int offset_y = ${value.offsetY ?? -32768};`,
      `  constexpr int rotation_offset = ${value.rotationOffset ?? "0xFF"};`,
      `  constexpr int invert = ${optionalFlag(value.invert)};`,
      `  constexpr int readable = ${optionalFlag(value.readable)};`,
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
