function busKind(board, device) {
  return device.bus ? board.buses?.[device.bus]?.kind : "none";
}

export function validatePartCatalog(parts) {
  const issues = [];
  for (const [id, part] of Object.entries(parts ?? {})) {
    if (!/^[a-z][a-z0-9_]*$/.test(id) || part.id !== id || typeof part.name !== "string" || typeof part.kind !== "string") {
      issues.push({ id: "E_PART_FORMAT", path: `/${id}`, message: "part needs a matching lowercase ID, name, and kind" });
    }
  }
  return issues;
}

export function validateParts(board, parts) {
  const issues = [];
  const issue = (id, path, message, severity) => issues.push({ id, path, message, ...(severity ? { severity } : {}) });
  for (const [id, device] of Object.entries(board.devices ?? {})) {
    if (!device.part) continue;
    const part = parts?.[device.part];
    const path = `/devices/${id}`;
    if (!part) {
      issue("E_PART_MISSING", `${path}/part`, `part ${device.part} is not in the catalog`);
      continue;
    }
    if (part.kind !== device.kind) issue("E_PART_KIND", `${path}/part`, `${device.part} is ${part.kind}, not ${device.kind}`);
    const expectedBus = part.bus?.kind;
    const actualBus = busKind(board, device);
    if (expectedBus && expectedBus !== "any" && expectedBus !== actualBus) issue("E_PART_BUS_KIND", `${path}/bus`, `${device.part} expects ${expectedBus}, got ${actualBus}`);
    if (device.i2c_addr && Array.isArray(part.i2c_addr) && !part.i2c_addr.includes(device.i2c_addr)) {
      issue("W_PART_ADDR_UNUSUAL", `${path}/i2c_addr`, `${device.i2c_addr} is unusual for ${device.part}`, "warning");
    }
    const wired = new Set(Object.keys(device.signals ?? {}));
    if (device.enable) wired.add("enable");
    for (const signal of part.signals?.required ?? []) if (!wired.has(signal)) issue("E_PART_SIGNAL_MISSING", `${path}/signals/${signal}`, `${device.part} requires ${signal}`);
    const known = new Set([...(part.signals?.required ?? []), ...(part.signals?.optional ?? [])]);
    if (known.size) for (const signal of wired) if (!known.has(signal)) issue("E_PART_SIGNAL_UNKNOWN", `${path}/signals/${signal}`, `${signal} is not declared by ${device.part}`);
    for (const key of Object.keys(device.spec ?? {})) if (part.spec_keys && !part.spec_keys.includes(key)) issue("E_UNKNOWN_KEY", `${path}/spec/${key}`, `${key} is not declared by ${device.part}`);
  }
  return issues;
}
