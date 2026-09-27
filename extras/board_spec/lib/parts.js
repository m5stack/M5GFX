import { validateSchema } from "./schema.js";

function busKind(board, device) {
  return device.bus ? board.buses?.[device.bus]?.kind : "none";
}

export function validatePartCatalog(parts) {
  const issues = [];
  const cppTypes = {
    integer: new Set(["int", "std::int8_t", "std::int16_t", "std::int32_t", "std::uint8_t", "std::uint16_t", "std::uint32_t"]),
    number: new Set(["float", "double"]),
    boolean: new Set(["bool", "int"]),
    string: new Set(["const char*"]),
  };
  for (const [id, part] of Object.entries(parts ?? {})) {
    if (!/^[a-z][a-z0-9_]*$/.test(id) || part.id !== id || typeof part.name !== "string" || typeof part.kind !== "string") {
      issues.push({ id: "E_PART_FORMAT", path: `/${id}`, message: "part needs a matching lowercase ID, name, and kind" });
    }
    if (part.spec_keys !== undefined && (!part.spec_keys || Array.isArray(part.spec_keys) || typeof part.spec_keys !== "object")) {
      issues.push({ id: "E_PART_FORMAT", path: `/${id}/spec_keys`, message: "spec_keys must map keys to schema definitions" });
      continue;
    }
    for (const [key, definition] of Object.entries(part.spec_keys ?? {})) {
      const path = `/${id}/spec_keys/${key}`;
      if (!definition || Array.isArray(definition) || typeof definition !== "object" || typeof definition.type !== "string") {
        issues.push({ id: "E_PART_SPEC_DEF", path, message: "spec definition needs a scalar type" });
        continue;
      }
      if (Object.prototype.hasOwnProperty.call(definition, "default")) {
        issues.push(...validateSchema(definition.default, definition, definition, `${path}/default`));
      }
      if (definition["x-cpp-type"] !== undefined && !cppTypes[definition.type]?.has(definition["x-cpp-type"])) {
        issues.push({ id: "E_PART_CPP_TYPE", path: `${path}/x-cpp-type`, message: `${definition["x-cpp-type"]} is incompatible with ${definition.type}` });
      }
    }
  }
  return issues;
}

function validateSpec(spec, part, path, issues) {
  const definitions = part.spec_keys ?? {};
  for (const [key, value] of Object.entries(spec ?? {})) {
    const definition = definitions[key];
    if (!definition) {
      issues.push({ id: "E_UNKNOWN_KEY", path: `${path}/${key}`, message: `${key} is not declared by ${part.id}` });
      continue;
    }
    issues.push(...validateSchema(value, definition, definition, `${path}/${key}`));
  }
}

export function validateParts(board, parts, { resolved = true } = {}) {
  const issues = [];
  const issue = (id, path, message, severity) => issues.push({ id, path, message, ...(severity ? { severity } : {}) });
  for (const [id, device] of Object.entries(board.devices ?? {})) {
    const path = `/devices/${id}`;
    const fragments = [[device, path], ...Object.entries(device.choices ?? {}).map(([choice, fragment]) => [
      { ...device, ...fragment, choices: undefined, selected_by: undefined, default: undefined }, `${path}/choices/${choice}`,
    ])];
    for (const [fragment, fragmentPath] of fragments) {
      if (!fragment.part) continue;
      const part = parts?.[fragment.part];
      if (!part) {
        issue("E_PART_MISSING", `${fragmentPath}/part`, `part ${fragment.part} is not in the catalog`);
        continue;
      }
      if (part.kind !== fragment.kind) issue("E_PART_KIND", `${fragmentPath}/part`, `${fragment.part} is ${part.kind}, not ${fragment.kind}`);
      const expectedBus = part.bus?.kind;
      const actualBus = busKind(board, fragment);
      if (expectedBus && expectedBus !== "any" && expectedBus !== actualBus) issue("E_PART_BUS_KIND", `${fragmentPath}/bus`, `${fragment.part} expects ${expectedBus}, got ${actualBus}`);
      if (fragment.i2c_addr && Array.isArray(part.i2c_addr) && !part.i2c_addr.includes(fragment.i2c_addr)) {
        issue("W_PART_ADDR_UNUSUAL", `${fragmentPath}/i2c_addr`, `${fragment.i2c_addr} is unusual for ${fragment.part}`, "warning");
      }
      validateSpec(fragment.spec, part, `${fragmentPath}/spec`, issues);
      if (!resolved || fragmentPath !== path) continue;
      const wired = new Set(Object.keys(fragment.signals ?? {}));
      if (fragment.enable) wired.add("enable");
      for (const signal of part.signals?.required ?? []) if (!wired.has(signal)) issue("E_PART_SIGNAL_MISSING", `${fragmentPath}/signals/${signal}`, `${fragment.part} requires ${signal}`);
      const known = new Set([...(part.signals?.required ?? []), ...(part.signals?.optional ?? [])]);
      if (known.size) for (const signal of wired) if (!known.has(signal)) issue("E_PART_SIGNAL_UNKNOWN", `${fragmentPath}/signals/${signal}`, `${signal} is not declared by ${fragment.part}`);
    }
  }
  return issues;
}
