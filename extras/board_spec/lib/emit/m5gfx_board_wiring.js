export function m5gfxBoardMapping(target, boardId) {
  const source = target.boards?.[boardId];
  if (!source?.wiring_output) return null;
  return {
    boardId,
    chip: source.chip,
    cppNamespace: source.cpp_namespace,
    boardEnum: source.board_enum,
    descName: source.desc_name,
    fields: source.wiring_fields ?? [],
    reset: source.reset,
    wiringOutput: source.wiring_output,
    specsOutput: source.specs_output,
    probeParts: source.probe_parts ?? null,
    options: source.options ?? [],
    descInternalI2c: source.desc_internal_i2c ?? false,
    cardputerSubdivision: source.cardputer_subdivision ?? null,
    identityDevices: source.identity_devices ?? [],
    releaseProbe: source.release_probe ?? null,
  };
}

export const PARALLEL_EPD_SIGNALS = ["data0", "data1", "data2", "data3", "data4", "data5", "data6", "data7", "pwr", "spv", "ckv", "sph", "oe", "le", "cl"];

// M5GFX board detection intentionally consumes only the base-board wiring.
// Accessory/composition wiring is not an input to these board descriptors.

function stripWiringRoleSource(role) {
  const slash = role.indexOf("/");
  return slash === -1 ? role : role.slice(slash + 1);
}

function requireWiringParts(parts) {
  if (!parts || typeof parts !== "object" || Array.isArray(parts)) throw new TypeError("M5GFX wiring generation requires the parts catalog");
  return parts;
}

// This is the single role-to-wiring-field mapping used by emission and validation.
export function wiringFieldsForRole(board, sourceRole, parts) {
  requireWiringParts(parts);
  const role = stripWiringRoleSource(sourceRole);
  const fields = [];
  const bus = /^bus:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
  if (bus) {
    const display = Object.values(board.devices ?? {}).find((device) => device.kind === "display");
    const sd = Object.values(board.devices ?? {}).find((device) => device.kind === "sd");
    if (display?.bus === bus[1] && ["sclk", "mosi", "miso", "io0", "io1", "io2", "io3"].includes(bus[2])) fields.push(`display_${bus[2]}`);
    if (display?.bus === bus[1] && board.buses?.[bus[1]]?.kind === "parallel_epd") fields.push(`display_${bus[2]}`);
    if (sd?.bus === display?.bus && sd.bus === bus[1]) {
      const name = { sclk: "shared_sd_sclk", mosi: "shared_sd_mosi", miso: "shared_sd_miso" }[bus[2]];
      if (name) fields.push(name);
    }
    if (bus[1] === "internal_i2c" && (bus[2] === "sda" || bus[2] === "scl")) fields.push(`internal_i2c_${bus[2]}`);
  }

  const deviceRole = /^dev:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
  const displayId = Object.entries(board.devices ?? {}).find(([, device]) => device.kind === "display")?.[0];
  const sdId = Object.entries(board.devices ?? {}).find(([, device]) => device.kind === "sd")?.[0];
  const kind = deviceRole && board.devices?.[deviceRole[1]]?.kind;
  if (kind === "display" && deviceRole[1] === displayId) {
    const name = { dc: "display_dc", cs: "display_cs", rst: "display_rst", busy: "display_busy" }[deviceRole[2]];
    if (name) fields.push(name);
  }
  if (kind === "sd" && deviceRole[1] === sdId && board.devices[sdId].bus === board.devices[displayId]?.bus) {
    const spiSignal = parts[board.devices[sdId].part]?.spi_alias?.[deviceRole[2]];
    const name = { sclk: "shared_sd_sclk", mosi: "shared_sd_mosi", miso: "shared_sd_miso", cs: "shared_sd_sd_cs" }[spiSignal];
    if (name) fields.push(name);
  }
  if (kind === "power_hold" && deviceRole[2] === "enable") fields.push("power_gpio");
  if (kind === "backlight" && deviceRole[2] === "pwm") fields.push("backlight_gpio");
  if (kind === "touch" && deviceRole[2] === "int") fields.push("touch_int");
  if (kind === "camera") fields.push(`camera_${deviceRole[2]}`);
  return fields;
}

export function wiringAssignments(board, parts) {
  requireWiringParts(parts);
  const values = {};
  for (const [gpio, pin] of Object.entries(board.pins ?? {})) for (const role of pin.roles ?? []) {
    for (const field of wiringFieldsForRole(board, role, parts)) {
      if (values[field] !== undefined && values[field] !== Number(gpio)) {
        throw new Error(`${board.id}: ${field} is assigned to both GPIO ${values[field]} and GPIO ${gpio}`);
      }
      values[field] = Number(gpio);
    }
  }
  return values;
}

export function emitM5GFXWiring(board, parts, target) {
  requireWiringParts(parts);
  const mapping = m5gfxBoardMapping(target, board.id);
  if (!mapping) return null;
  return emitM5GFXWiringForMapping(board, parts, mapping);
}

export function emitM5GFXWiringForMapping(board, parts, mapping) {
  requireWiringParts(parts);
  const assigned = wiringAssignments(board, parts);
  const value = (name) => assigned[name] ?? -1;
  const quadNames = ["io0", "io1", "io2", "io3"];
  const hasQuad = quadNames.some((name) => value(`display_${name}`) >= 0);
  const displayDevice = Object.values(board.devices ?? {}).find((device) => device.kind === "display");
  const displayBus = board.buses?.[displayDevice?.bus];
  // A parallel EPD has no SPI display pins; its wiring is the bus signal list.
  const displayNames = displayBus?.kind === "parallel_epd"
    ? PARALLEL_EPD_SIGNALS
    : ["sclk", "mosi", "miso", ...(hasQuad ? quadNames : []), "dc", "cs", "rst", "busy"];
  const display = Object.fromEntries(displayNames.map((name) => [name, value(`display_${name}`)]));
  const sdDevice = Object.values(board.devices ?? {}).find((device) => device.kind === "sd");
  const sharesSdBus = sdDevice && sdDevice.bus === displayDevice?.bus;
  if (sharesSdBus && !sdDevice.derived?.modes?.includes("spi")) {
    throw new Error(`${board.id}: SD shares the display bus but does not support SPI`);
  }
  const sharedSd = sharesSdBus ? {
    sclk: value("shared_sd_sclk"), mosi: value("shared_sd_mosi"), miso: value("shared_sd_miso"),
    sd_cs: value("shared_sd_sd_cs"), other_cs: display.cs,
  } : null;
  const preferredHost = board.buses?.internal_i2c?.preferred_host;
  if (mapping.fields.includes("i2c") && (!Number.isInteger(preferredHost) || preferredHost < 0 || preferredHost > 127)) {
    throw new Error(`${board.id}: internal_i2c.preferred_host must be an integer from 0 to 127`);
  }
  const i2c = board.buses?.internal_i2c ? {
    sda: value("internal_i2c_sda"), scl: value("internal_i2c_scl"), port: preferredHost ?? -1,
  } : null;
  const camera = Object.fromEntries(Object.entries(assigned)
    .filter(([name]) => name.startsWith("camera_"))
    .map(([name, pin]) => [name.slice("camera_".length), pin]));
  let resetGpio = -1;
  if (mapping.reset === "display_rst") {
    if (display.rst < 0) throw new Error(`${board.id}: reset uses display_rst but the display reset GPIO is unavailable`);
    resetGpio = display.rst;
  }
  const hold = sharedSd ? [sharedSd.sd_cs, display.cs] : [display.cs ?? -1];
  return {
    mapping,
    display,
    sharedSd,
    i2c,
    resetGpio,
    powerGpio: value("power_gpio"),
    backlightGpio: value("backlight_gpio"),
    touchInt: value("touch_int"),
    camera,
    hold: hold.filter((pin) => pin >= 0),
  };
}

function constant(name, value) {
  return `constexpr std::int8_t ${name} = ${value};`;
}

function cardputerSubdivisionForEntry(entry, entries) {
  const config = entry.emitted.mapping.cardputerSubdivision;
  if (!config) return null;
  const byBoard = new Map(entries.map((item) => [item.board.id, item]));
  const sensePins = [];
  for (const boardId of config.sense_i2c_from_boards ?? []) {
    const source = byBoard.get(boardId);
    if (!source?.emitted.i2c) throw new Error(`${entry.board.id}: Cardputer subdivision source ${boardId} needs internal I2C wiring`);
    sensePins.push(source.emitted.i2c.sda, source.emitted.i2c.scl);
  }
  const i2cSource = byBoard.get(config.vameter_board);
  if (!i2cSource?.emitted.i2c) throw new Error(`${entry.board.id}: Cardputer VAMeter source ${config.vameter_board} needs internal I2C wiring`);
  const addresses = (config.vameter_i2c_devices ?? []).map((deviceId) => {
    const device = i2cSource.board.devices?.[deviceId];
    if (device?.bus !== "internal_i2c" || !device.i2c_addr) {
      throw new Error(`${entry.board.id}: Cardputer VAMeter device ${deviceId} needs an internal I2C address`);
    }
    return Number.parseInt(device.i2c_addr.slice(2), 16);
  });
  if ([...new Set(sensePins)].length !== 4 || [...new Set(addresses)].length !== 2) {
    throw new Error(`${entry.board.id}: Cardputer subdivision needs four sense pins and two I2C addresses`);
  }
  return {
    sensePins: [...new Set(sensePins)].sort((left, right) => left - right),
    i2cAddresses: [...new Set(addresses)],
    i2cSda: i2cSource.emitted.i2c.sda,
    i2cScl: i2cSource.emitted.i2c.scl,
  };
}

function descriptorPins(emitted) {
  // Keep the transaction and conditional-pin metadata conservative for every
  // part described to board detection. Target metadata distinguishes an I2C
  // bus that the generated wiring exposes from one that the descriptor opens.
  const fields = new Set(emitted.mapping.fields);
  const values = [];
  if (fields.has("display")) values.push(...Object.values(emitted.display));
  if (fields.has("shared_sd") && emitted.sharedSd) values.push(...Object.values(emitted.sharedSd));
  if (fields.has("i2c") && emitted.i2c && emitted.mapping.descInternalI2c) {
    values.push(emitted.i2c.sda, emitted.i2c.scl);
  }
  if (emitted.mapping.reset) values.push(emitted.resetGpio);
  if (fields.has("power")) values.push(emitted.powerGpio);
  if (fields.has("backlight")) values.push(emitted.backlightGpio);
  if (fields.has("touch")) values.push(emitted.touchInt);
  if (fields.has("camera")) values.push(...Object.values(emitted.camera));
  if (fields.has("hold")) values.push(...emitted.hold);
  return [...new Set(values.filter((pin) => Number.isInteger(pin) && pin >= 0))].sort((left, right) => left - right);
}

export function detectionPinsForEntries(entries) {
  return [...new Set(entries.flatMap((entry) => {
    const subdivision = cardputerSubdivisionForEntry(entry, entries);
    return [...descriptorPins(entry.emitted), ...(subdivision?.sensePins ?? [])];
  }))].sort((left, right) => left - right);
}

export function partitionDetectionPins(entries, chip) {
  const conditional = Object.fromEntries(Object.keys(chip.reserved_conditional ?? {}).map((mode) => [mode, []]));
  const unconditional = [];
  for (const pin of detectionPinsForEntries(entries)) {
    const modes = Object.entries(chip.reserved_conditional ?? {})
      .filter(([, pins]) => pins.includes(pin)).map(([mode]) => mode);
    if (modes.length) for (const mode of modes) conditional[mode].push(pin);
    else unconditional.push(pin);
  }
  return { unconditional, conditional };
}

export function validateDetectionPins(entries, chip, pins = detectionPinsForEntries(entries)) {
  const available = new Set(pins);
  for (const entry of entries) {
    const { board, emitted } = entry;
    const subdivision = cardputerSubdivisionForEntry(entry, entries);
    for (const pin of [...descriptorPins(emitted), ...(subdivision?.sensePins ?? [])]) {
      if (!available.has(pin)) throw new Error(`${board.id}: descriptor GPIO ${pin} is absent from the detection pin set`);
      if (chip.absent?.includes(pin)) throw new Error(`${board.id}: detection GPIO ${pin} does not exist on ${chip.id}`);
      if (chip.reserved?.includes(pin)) throw new Error(`${board.id}: detection GPIO ${pin} is reserved by ${chip.id}`);
      const conditional = chip.reserved_conditional?.[board.spec?.storage?.psram_mode] ?? [];
      if (conditional.includes(pin)) throw new Error(`${board.id}: detection GPIO ${pin} is reserved when PSRAM mode is ${board.spec.storage.psram_mode}`);
      if (pin === chip.usb?.dn || pin === chip.usb?.dp) throw new Error(`${board.id}: detection GPIO ${pin} is reserved for native USB`);
    }
  }
  return pins;
}

export function renderM5GFXWiringHeader(entries) {
  const chips = new Map(entries.map((entry) => [entry.chip?.id, entry.chip]));
  if (chips.size !== 1 || chips.has(undefined)) throw new Error("M5GFX wiring output requires exactly one chip catalog");
  const chip = chips.values().next().value;
  validateDetectionPins(entries, chip);
  const partitioned = partitionDetectionPins(entries, chip);
  const lines = [
    "// Generated from extras/board_spec; do not edit by hand.",
    "// Unspecified values use target-specific unknown or sentinel values.",
    "// Precedence: board value > part default > panel-class default.",
    "#pragma once",
    "",
    "#include <cstdint>",
    "",
    "namespace m5gfx { namespace board_detect { namespace m5 { namespace wiring {",
  ];
  for (const entry of entries) {
    const value = entry.emitted;
    const subdivision = cardputerSubdivisionForEntry(entry, entries);
    const fields = new Set(value.mapping.fields);
    const boardPins = [
      ...descriptorPins(value),
      ...(subdivision?.sensePins ?? []),
    ];
    lines.push("", `namespace ${value.mapping.cppNamespace} {`);
    if (fields.has("display")) for (const [name, pin] of Object.entries(value.display)) lines.push(`  ${constant(`display_${name}`, pin)}`);
    if (fields.has("shared_sd") && value.sharedSd) for (const [name, pin] of Object.entries(value.sharedSd)) lines.push(`  ${constant(`shared_sd_${name}`, pin)}`);
    if (fields.has("i2c") && value.i2c) {
      lines.push(`  ${constant("internal_i2c_sda", value.i2c.sda)}`);
      lines.push(`  ${constant("internal_i2c_scl", value.i2c.scl)}`);
      lines.push(`  ${constant("internal_i2c_port", value.i2c.port)}`);
    }
    if (value.mapping.reset) lines.push(`  ${constant("reset_gpio", value.resetGpio)}`);
    if (fields.has("power")) lines.push(`  ${constant("power_gpio", value.powerGpio)}`);
    if (fields.has("backlight")) lines.push(`  ${constant("backlight_gpio", value.backlightGpio)}`);
    if (fields.has("touch")) lines.push(`  ${constant("touch_int", value.touchInt)}`);
    if (fields.has("camera")) for (const [name, pin] of Object.entries(value.camera)) lines.push(`  ${constant(`camera_${name}`, pin)}`);
    if (fields.has("hold")) lines.push(`  constexpr std::int8_t hold[] = { ${value.hold.join(", ")} };`);
    for (const [mode, pins] of Object.entries(chip.reserved_conditional ?? {})) {
      lines.push(`  constexpr bool touches_${mode}_pins = ${boardPins.some((pin) => pins.includes(pin))};`);
    }
    if (subdivision) {
      lines.push("  namespace cardputer_subdivision {");
      lines.push(`    constexpr std::int8_t sense_pins[] = { ${subdivision.sensePins.join(", ")} };`);
      lines.push(`    constexpr std::uint8_t vameter_i2c_addrs[] = { ${subdivision.i2cAddresses.map((value) => `0x${value.toString(16).toUpperCase()}`).join(", ")} };`);
      lines.push(`    constexpr std::int8_t vameter_i2c_sda = ${subdivision.i2cSda};`);
      lines.push(`    constexpr std::int8_t vameter_i2c_scl = ${subdivision.i2cScl};`);
      lines.push("  } // namespace cardputer_subdivision");
    }
    lines.push(`} // namespace ${value.mapping.cppNamespace}`);
  }
  lines.push("", "namespace detection {",
    `  constexpr std::int8_t unconditional_pins[] = { ${partitioned.unconditional.join(", ")} };`);
  for (const [mode, pins] of Object.entries(partitioned.conditional)) {
    lines.push(`  constexpr std::int8_t ${mode}_pins[] = { ${pins.join(", ")} };`);
  }
  lines.push("} // namespace detection");
  lines.push("", "} // namespace wiring");
  const optionEntries = entries.filter(({ emitted }) => emitted.mapping.options.length);
  if (optionEntries.length) {
    lines.push("", "namespace generated_options {");
    for (const { emitted } of optionEntries) {
      const { cppNamespace } = emitted.mapping;
      const options = [...emitted.mapping.options].sort((left, right) => left.bit - right.bit);
      lines.push(`namespace ${cppNamespace} {`);
      for (const option of options) lines.push(`  constexpr std::uint32_t ${option.name} = 1u << ${option.bit};`);
      lines.push(`  static const char* const names[] = { ${options.map((option) => `"${option.name}"`).join(", ")} };`);
      lines.push(`} // namespace ${cppNamespace}`);
    }
    lines.push("} // namespace generated_options");
  }
  lines.push("", "} } } // namespace m5gfx::board_detect::m5", "");
  return lines.join("\n");
}

export function selectM5GFXWiringBoards(boards, mappings) {
  return mappings.map((mapping) => {
    const matches = boards.filter((board) => board.id === mapping.boardId);
    if (matches.length !== 1) throw new Error(`M5GFX wiring board ${mapping.boardId} must appear exactly once; found ${matches.length}`);
    return matches[0];
  });
}
