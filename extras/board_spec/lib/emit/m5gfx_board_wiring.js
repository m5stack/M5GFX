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
    options: source.options ?? [],
  };
}

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
    if (display?.bus === bus[1] && ["sclk", "mosi", "miso"].includes(bus[2])) fields.push(`display_${bus[2]}`);
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
  const display = Object.fromEntries(["sclk", "mosi", "miso", "dc", "cs", "rst", "busy"].map((name) => [name, value(`display_${name}`)]));
  const displayDevice = Object.values(board.devices ?? {}).find((device) => device.kind === "display");
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
  let resetGpio = -1;
  if (mapping.reset === "display_rst") {
    if (display.rst < 0) throw new Error(`${board.id}: reset uses display_rst but the display reset GPIO is unavailable`);
    resetGpio = display.rst;
  }
  const hold = sharedSd ? [sharedSd.sd_cs, display.cs] : [display.cs];
  return {
    mapping,
    display,
    sharedSd,
    i2c,
    resetGpio,
    powerGpio: value("power_gpio"),
    backlightGpio: value("backlight_gpio"),
    hold: hold.filter((pin) => pin >= 0),
  };
}

function constant(name, value) {
  return `constexpr std::int8_t ${name} = ${value};`;
}

export function renderM5GFXWiringHeader(entries) {
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
    const fields = new Set(value.mapping.fields);
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
    if (fields.has("hold")) lines.push(`  constexpr std::int8_t hold[] = { ${value.hold.join(", ")} };`);
    lines.push(`} // namespace ${value.mapping.cppNamespace}`);
  }
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
