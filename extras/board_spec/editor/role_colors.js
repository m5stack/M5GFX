const FIXED_ROLE_COLORS = {
  mbus: "#f6d6a8",
  port_a: "#cdeccf",
  port_b: "#cfe4ff",
  port_c: "#ead7ff",
  port_d: "#ffd8e4",
  port_e: "#d4ece9",
  main_spi: "#ffd1b8",
  internal_i2c: "#c7e8f3",
  lcd: "#f8d0c8",
  sd: "#d5e8b7",
  touch: "#e2d2f4",
};

function hslToHex(hue, saturation, lightness) {
  const saturationRatio = saturation / 100;
  const lightnessRatio = lightness / 100;
  const chroma = (1 - Math.abs(2 * lightnessRatio - 1)) * saturationRatio;
  const component = ((hue / 60) % 2 + 2) % 2;
  const second = chroma * (1 - Math.abs(component - 1));
  const [red, green, blue] = hue < 60 ? [chroma, second, 0]
    : hue < 120 ? [second, chroma, 0]
      : hue < 180 ? [0, chroma, second]
        : hue < 240 ? [0, second, chroma]
          : hue < 300 ? [second, 0, chroma]
            : [chroma, 0, second];
  const match = lightnessRatio - chroma / 2;
  return `#${[red, green, blue].map((value) => Math.round((value + match) * 255).toString(16).padStart(2, "0")).join("")}`;
}

export function objectRoleOwner(kind, id) {
  return { kind, id, key: `${kind}:${id}` };
}

export function roleOwner(role) {
  const plain = String(role ?? "").split("/").at(-1);
  const object = /^(bus|dev|conn):([a-z][a-z0-9_]*)\./.exec(plain);
  if (object) return objectRoleOwner(object[1], object[2]);
  const power = /^pwr:([a-z][a-z0-9_]*)$/.exec(plain);
  return power ? objectRoleOwner("pwr", power[1]) : null;
}

export function defaultRoleColor(id) {
  if (FIXED_ROLE_COLORS[id]) return FIXED_ROLE_COLORS[id];
  let hash = 2166136261;
  for (const character of String(id)) {
    hash ^= character.codePointAt(0);
    hash = Math.imul(hash, 16777619);
  }
  return hslToHex((hash >>> 0) % 360, 58, 86);
}

export function roleContrastColor(background) {
  const match = /^#([0-9a-f]{6})$/i.exec(background);
  if (!match) return "#17212b";
  const value = Number.parseInt(match[1], 16);
  const red = (value >> 16) & 0xff;
  const green = (value >> 8) & 0xff;
  const blue = value & 0xff;
  const luminance = (0.2126 * red + 0.7152 * green + 0.0722 * blue) / 255;
  return luminance > 0.56 ? "#17212b" : "#ffffff";
}

export function roleColor(owner, overrides = {}) {
  const background = overrides[owner?.key] ?? defaultRoleColor(owner?.id ?? "unknown");
  return { background, foreground: roleContrastColor(background) };
}

function connectorOwnerId(role) {
  const plain = String(role ?? "").split("/").at(-1);
  return /^conn:([a-z][a-z0-9_]*)\./.exec(plain)?.[1] ?? null;
}

export function assignConnectorLanes(pins = {}) {
  const gpiosByConnector = new Map();
  for (const [gpio, pin] of Object.entries(pins)) {
    for (const role of pin.roles ?? []) {
      const id = connectorOwnerId(role);
      if (!id) continue;
      if (!gpiosByConnector.has(id)) gpiosByConnector.set(id, new Set());
      gpiosByConnector.get(id).add(String(gpio));
    }
  }

  const ordered = [...gpiosByConnector].sort(([leftId, leftPins], [rightId, rightPins]) => (
    rightPins.size - leftPins.size || leftId.localeCompare(rightId)
  ));
  const lanes = [];
  const laneByConnector = {};
  for (const [id, gpios] of ordered) {
    let laneIndex = lanes.findIndex((lane) => lane.every((member) => {
      const memberGPIOs = gpiosByConnector.get(member);
      return [...gpios].every((gpio) => !memberGPIOs.has(gpio));
    }));
    if (laneIndex === -1) {
      laneIndex = lanes.length;
      lanes.push([]);
    }
    lanes[laneIndex].push(id);
    laneByConnector[id] = laneIndex;
  }
  return { lanes, laneByConnector };
}

export function connectorRolesByLane(roles, assignment) {
  const byLane = Array.from({ length: assignment.lanes.length }, () => []);
  for (const role of roles ?? []) {
    const id = connectorOwnerId(role);
    if (id !== null) byLane[assignment.laneByConnector[id]].push(role);
  }
  return byLane;
}
