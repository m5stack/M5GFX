import { validateSchema } from "./schema.js";

const CPP_TYPES = {
  integer: new Set(["int", "std::int8_t", "std::int16_t", "std::int32_t", "std::uint8_t", "std::uint16_t", "std::uint32_t"]),
  number: new Set(["float", "double"]),
  boolean: new Set(["bool", "int", "std::int8_t"]),
  string: new Set(["const char*"]),
};

const CPP_RANGES = {
  int: [-2147483648, 2147483647],
  "std::int8_t": [-128, 127],
  "std::int16_t": [-32768, 32767],
  "std::int32_t": [-2147483648, 2147483647],
  "std::uint8_t": [0, 255],
  "std::uint16_t": [0, 65535],
  "std::uint32_t": [0, 4294967295],
};

const M5GFX_SENTINELS = {
  memory_width: 0,
  memory_height: 0,
  offset_x: -32768,
  offset_y: -32768,
  rotation_offset: 255,
  invert: -1,
  readable: -1,
};

export const DEVICE_KIND_SPEC_KEYS = {
  backlight: {
    freq: { type: "integer", minimum: 1, maximum: 4294967295, required: true, "x-cpp-type": "std::uint32_t" },
    channel: { type: "integer", minimum: 0, maximum: 7, required: true, "x-cpp-type": "std::uint8_t" },
    invert: { type: "boolean", default: false, "x-cpp-type": "bool" },
    offset: { type: "integer", minimum: 0, maximum: 255, default: 0, "x-cpp-type": "std::uint8_t" },
  },
};

function cppTypeSupports(type, value) {
  const range = CPP_RANGES[type];
  return range ? value >= range[0] && value <= range[1] : false;
}

function cppRangeIssue(cppType, value, path) {
  if (typeof value !== "number" || !CPP_RANGES[cppType] || cppTypeSupports(cppType, value)) return [];
  return [{ id: "E_PART_CPP_RANGE", path, message: `${cppType} cannot represent ${value}` }];
}

function definitionIssues(id, definitions) {
  const issues = [];
  for (const [key, definition] of Object.entries(definitions ?? {})) {
    const path = `/${id}/spec_keys/${key}`;
    if (!definition || Array.isArray(definition) || typeof definition !== "object" || typeof definition.type !== "string") {
      issues.push({ id: "E_PART_SPEC_DEF", path, message: "spec definition needs a scalar type" });
      continue;
    }
    if (definition.required !== undefined && typeof definition.required !== "boolean") {
      issues.push({ id: "E_PART_SPEC_DEF", path: `${path}/required`, message: "required must be a boolean" });
    }
    if (Object.prototype.hasOwnProperty.call(definition, "default")) {
      issues.push(...validateSchema(definition.default, definition, definition, `${path}/default`));
    }
    const cppType = definition["x-cpp-type"];
    if (cppType !== undefined && !CPP_TYPES[definition.type]?.has(cppType)) {
      issues.push({ id: "E_PART_CPP_TYPE", path: `${path}/x-cpp-type`, message: `${cppType} is incompatible with ${definition.type}` });
    }
    for (const field of ["minimum", "maximum", "default"]) if (Object.prototype.hasOwnProperty.call(definition, field)) {
      issues.push(...cppRangeIssue(cppType, definition[field], `${path}/${field}`));
    }
    const sentinel = M5GFX_SENTINELS[key];
    if (sentinel !== undefined && !definition.required && definition.default === undefined
     && !cppTypeSupports(cppType ?? "int", sentinel)) {
      issues.push({ id: "E_PART_CPP_SENTINEL", path: `${path}/x-cpp-type`, message: `${cppType ?? "int"} cannot represent the ${sentinel} sentinel for ${key}` });
    }
  }
  return issues;
}

export function assertM5GFXSentinelTypes(definitions, label) {
  const issue = definitionIssues(label, definitions).find((item) => item.id === "E_PART_CPP_SENTINEL");
  if (issue) throw new Error(`${label}: ${issue.message}`);
}

function busKind(board, device) {
  return device.bus ? board.buses?.[device.bus]?.kind : "none";
}

export function validatePartCatalog(parts) {
  const issues = [];
  for (const [id, part] of Object.entries(parts ?? {})) {
    if (!/^[a-z][a-z0-9_]*$/.test(id) || part.id !== id || typeof part.name !== "string" || typeof part.kind !== "string") {
      issues.push({ id: "E_PART_FORMAT", path: `/${id}`, message: "part needs a matching lowercase ID, name, and kind" });
    }
    if (part.spec_keys !== undefined && (!part.spec_keys || Array.isArray(part.spec_keys) || typeof part.spec_keys !== "object")) {
      issues.push({ id: "E_PART_FORMAT", path: `/${id}/spec_keys`, message: "spec_keys must map keys to schema definitions" });
      continue;
    }
    issues.push(...definitionIssues(id, part.spec_keys));
  }
  for (const [kind, definitions] of Object.entries(DEVICE_KIND_SPEC_KEYS)) issues.push(...definitionIssues(`kind:${kind}`, definitions));
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
    issues.push(...cppRangeIssue(definition["x-cpp-type"], value, `${path}/${key}`));
  }
  for (const [key, definition] of Object.entries(definitions)) if (definition.required && !Object.prototype.hasOwnProperty.call(spec ?? {}, key)) {
    issues.push({ id: "E_PART_SPEC_REQUIRED", path: `${path}/${key}`, message: `${key} is required by ${part.id}` });
  }
}

export function validateParts(board, parts, { resolved = true } = {}) {
  const issues = [];
  const issue = (id, path, message, severity) => issues.push({ id, path, message, ...(severity ? { severity } : {}) });
  for (const [id, device] of Object.entries(board.devices ?? {})) {
    const path = `/devices/${id}`;
    const fragments = [[device, path], ...Object.entries(device.choices ?? {}).map(([choice, fragment]) => [
      { ...device, ...fragment, spec: { ...(device.spec ?? {}), ...(fragment.spec ?? {}) }, choices: undefined, selected_by: undefined, default: undefined }, `${path}/choices/${choice}`,
    ])];
    for (const [fragment, fragmentPath] of fragments) {
      if (!fragment.part) {
        const definitions = DEVICE_KIND_SPEC_KEYS[fragment.kind];
        if (definitions) validateSpec(fragment.spec, { id: `${fragment.kind} kind`, spec_keys: definitions }, `${fragmentPath}/spec`, issues);
        continue;
      }
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
