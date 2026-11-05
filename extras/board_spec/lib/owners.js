import { stripSource } from "./pintable_roles.js";

const outputCaps = new Set(["output", "pwm", "rail", "ldo"]);

export function parseRole(role) {
  role = stripSource(role);
  const match = /^(bus|dev|conn|pwr):([a-z0-9_]+)(?:\.([a-z0-9_]+))?$/.exec(role);
  if (!match || (match[1] !== "pwr" && !match[3]) || (match[1] === "pwr" && match[3])) return null;
  return { type: match[1], id: match[2], signal: match[3] };
}

export function ownerRoleEntries(board) {
  const entries = [];
  for (const [pin, row] of Object.entries(board.pins ?? {})) for (const role of row.roles ?? []) {
    entries.push({ owner: "soc", pin, row, role, parsed: parseRole(role), path: `/pins/${pin}/roles`, endpoint: `gpio:${pin}` });
  }
  for (const [owner, device] of Object.entries(board.devices ?? {})) for (const [pin, row] of Object.entries(device.pins ?? {})) for (const role of row.roles ?? []) {
    entries.push({ owner, pin, row, role, parsed: parseRole(role), path: `/devices/${owner}/pins/${pin}/roles`, endpoint: `pin:${owner}.${pin}` });
  }
  return entries;
}

export function deriveDeviceEndpoints(board) {
  for (const device of Object.values(board.devices ?? {})) {
    delete device.signals;
    delete device.enable;
  }
  for (const entry of ownerRoleEntries(board)) {
    if (entry.parsed?.type !== "dev") continue;
    const device = board.devices?.[entry.parsed.id];
    if (!device) continue;
    if (device.kind === "speaker" && entry.parsed.signal === "enable") device.enable = entry.endpoint;
    else {
      if (!device.signals) device.signals = {};
      device.signals[entry.parsed.signal] = entry.endpoint;
    }
  }
  return board;
}

export function validateOwners(board, parts) {
  const issues = [];
  const issue = (id, path, message) => issues.push({ id, path, message });
  for (const [owner, device] of Object.entries(board.devices ?? {})) {
    if (!device.pins) continue;
    const part = parts?.[device.part] ?? (!device.part ? parts?.[device.kind] : null);
    if (!part?.pins) {
      issue("E_OWNER_NO_PART", `/devices/${owner}/pins`, "pin owner must reference a part with pins");
      continue;
    }
    for (const [pin, row] of Object.entries(device.pins)) {
      const definition = part.pins[pin];
      if (!definition) {
        issue("E_OWNER_PIN_UNKNOWN", `/devices/${owner}/pins/${pin}`, `${pin} is not declared by part ${device.part ?? device.kind}`);
        continue;
      }
      const caps = new Set(definition.cap ?? []);
      for (const role of row.roles ?? []) {
        const parsed = parseRole(role);
        const needsRail = parsed?.type === "pwr";
        const needsOutput = parsed?.type === "dev" || parsed?.type === "conn";
        if ((needsRail && !["rail", "ldo", "output"].some((cap) => caps.has(cap))) || (needsOutput && ![...caps].some((cap) => outputCaps.has(cap)))) {
          issue("E_OWNER_CAP", `/devices/${owner}/pins/${pin}/roles`, `${pin} lacks capability for ${role}`);
        }
      }
    }
  }
  return issues;
}

export function normalizeEndpoint(endpoint) {
  const gpio = /^pin:soc\.(\d+)$/.exec(endpoint)?.[1];
  return gpio === undefined ? endpoint : `gpio:${gpio}`;
}
