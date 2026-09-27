export const PIN_NAMES = [
  "in_i2c_scl", "in_i2c_sda",
  "port_a_pin1", "port_a_pin2", "port_b_pin1", "port_b_pin2",
  "port_c_pin1", "port_c_pin2", "port_d_pin1", "port_d_pin2",
  "port_e_pin1", "port_e_pin2",
  "sd_mmc_clk", "sd_mmc_cmd", "sd_mmc_d0", "sd_mmc_d1", "sd_mmc_d2", "sd_mmc_d3",
  "rgb_led", "power_hold",
  ...Array.from({ length: 30 }, (_, index) => `mbus_pin${index + 1}`),
];

export const PIN_TABLE_GROUPS = [
  { name: "_pin_table_i2c_ex_in", pins: PIN_NAMES.slice(0, 4), unknown: [22, 21, 33, 32] },
  { name: "_pin_table_port_bc", pins: PIN_NAMES.slice(4, 8), unknown: [255, 255, 255, 255] },
  { name: "_pin_table_port_de", pins: PIN_NAMES.slice(8, 12), unknown: [255, 255, 255, 255] },
  { name: "_pin_table_sd", pins: PIN_NAMES.slice(12, 18), unknown: [255, 255, 255, 255, 255, 255] },
  { name: "_pin_table_other0", pins: ["rgb_led"], unknown: [255] },
  { name: "_pin_table_other1", pins: ["power_hold"], unknown: [255] },
  { name: "_pin_table_mbus", pins: PIN_NAMES.slice(20), unknown: Array(30).fill(255) },
];

export const PIN_TABLE_TARGETS = {
  esp32: { soc: "esp32", define: "CONFIG_IDF_TARGET_ESP32", unknown: {} },
  esp32s3: {
    soc: "esp32s3",
    define: "CONFIG_IDF_TARGET_ESP32S3",
    unknown: { _pin_table_i2c_ex_in: [39, 38, 1, 2] },
  },
};

const CPP_BOARD_NAMES = {
  m5stack: "board_M5Stack",
  m5stack_core2: "board_M5StackCore2",
  m5tough: "board_M5Tough",
  m5station: "board_M5Station",
  m5paper: "board_M5Paper",
  m5timercam: "board_M5TimerCam",
  m5atoms3: "board_M5AtomS3",
  m5sticks3: "board_M5StickS3",
};

export function stripRoleSource(role) {
  const slash = role.indexOf("/");
  return slash === -1 ? role : role.slice(slash + 1);
}

// This is the single role-to-pin_name_t mapping used by emission and validation.
export function pinNameForRole(board, sourceRole) {
  const role = stripRoleSource(sourceRole);
  const bus = /^bus:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
  if (bus?.[1] === "internal_i2c" && (bus[2] === "scl" || bus[2] === "sda")) return `in_i2c_${bus[2]}`;
  if (bus?.[1] === "main_spi" && Object.values(board.devices ?? {}).some((device) => device.kind === "sd" && device.bus === "main_spi")) {
    return { sclk: "sd_mmc_clk", mosi: "sd_mmc_cmd", miso: "sd_mmc_d0" }[bus[2]] ?? null;
  }

  const connector = /^conn:(port_[a-e]|port_b2|port_c2)\.([12])$/.exec(role);
  if (connector) {
    const port = { port_b2: "port_d", port_c2: "port_e" }[connector[1]] ?? connector[1];
    return `${port}_pin${connector[2]}`;
  }
  const mbus = /^conn:mbus\.([1-9]|[12][0-9]|30)$/.exec(role);
  if (mbus) return `mbus_pin${mbus[1]}`;

  const device = /^dev:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
  const kind = device && board.devices?.[device[1]]?.kind;
  if (kind === "sd") return {
    clk: "sd_mmc_clk", cmd: "sd_mmc_cmd", d0: "sd_mmc_d0",
    d1: "sd_mmc_d1", d2: "sd_mmc_d2", d3: "sd_mmc_d3",
  }[device[2]] ?? null;
  if (kind === "led_strip" && device[2] === "data") return "rgb_led";
  if (kind === "power_hold" && device[2] === "enable") return "power_hold";
  return null;
}

export function consumesPinTableRole(board, role) {
  return pinNameForRole(board, role) !== null;
}

export function pinTableAssignments(board) {
  const assignments = {};
  for (const [gpio, pin] of Object.entries(board.pins ?? {})) for (const role of pin.roles ?? []) {
    const name = pinNameForRole(board, role);
    if (name) assignments[name] = Number(gpio);
  }
  return assignments;
}

export function emitPinTable(board, target = {}) {
  const assignments = pinTableAssignments(board);
  const values = Object.fromEntries(PIN_NAMES.map((name) => [name, assignments[name] ?? 255]));
  const overrides = target.overrides?.[board.id] ?? {};
  for (const [name, override] of Object.entries(overrides)) {
    if (!PIN_NAMES.includes(name)) throw new Error(`unknown pin-table override ${board.id}.${name}`);
    if (!Number.isInteger(override?.value) || override.value < 0 || override.value > 255 || typeof override.reason !== "string" || !override.reason) {
      throw new Error(`invalid pin-table override ${board.id}.${name}`);
    }
    values[name] = override.value;
  }
  return { values, overrides };
}

function sameValues(left, right) {
  return left.length === right.length && left.every((value, index) => value === right[index]);
}

export function renderPinTableJson(entries) {
  return `${JSON.stringify(Object.fromEntries(entries.map(({ board, emitted }) => [board.id, emitted.values])), null, 2)}\n`;
}

export function renderPinTableInl(entries, targetId = "esp32") {
  const target = PIN_TABLE_TARGETS[targetId];
  if (!target) throw new Error(`unknown pin-table target ${targetId}`);
  const lines = ["// Generated from extras/board_spec; do not edit."];
  for (const group of PIN_TABLE_GROUPS) {
    const unknown = target.unknown[group.name] ?? group.unknown;
    lines.push("", `// ${group.name}[][${group.pins.length + 1}]`);
    for (const { board, emitted } of entries) {
      const values = group.pins.map((name) => emitted.values[name]);
      if (sameValues(values, unknown)) continue;
      const comments = group.pins
        .filter((name) => emitted.overrides[name])
        .map((name) => `${name}: ${emitted.overrides[name].reason}`);
      const suffix = comments.length ? ` // override: ${comments.join("; ")}` : "";
      lines.push(`{ board_t::${CPP_BOARD_NAMES[board.id]}, ${values.join(", ")} },${suffix}`);
    }
    lines.push(`{ board_t::board_unknown, ${unknown.join(", ")} },`);
  }
  return `${lines.join("\n")}\n`;
}
