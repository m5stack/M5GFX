import { checkFormatIdempotent } from "./format.js";
import { resolveConnectorType } from "./ctypes.js";
import { effectiveChip, isPlainObject } from "./model.js";
import { isGeneratedField } from "./pintable_roles.js";
import { resolveAll, resolveBoard, resolveConnectorPositions } from "./resolve.js";
import { validateChoices } from "./choices.js";
import { ownerRoleEntries, parseRole, validateOwners } from "./owners.js";
import { validateParts } from "./parts.js";
import { validateTarget } from "./targets.js";
import { emitM5GFXWiring } from "./emit/m5gfx_board_wiring.js";
import { emitPinTable } from "./emit/m5unified_pin_table.js";
import { validateSchema } from "./schema.js";

const error = (id, path, message) => ({ id, path, message });
const warning = (id, path, message) => ({ id, path, message, severity: "warning" });
const own = (value, key) => Object.prototype.hasOwnProperty.call(value, key);

function validateDeviceSpecKeys(board, schema, errors) {
  const allowedByKind = schema.definitions.deviceKind["x-spec-properties"];
  for (const [id, device] of Object.entries(board.devices ?? {})) {
    if (!isPlainObject(device.spec)) continue;
    const allowed = new Set(allowedByKind[device.kind] ?? []);
    for (const key of Object.keys(device.spec)) {
      if (!allowed.has(key)) errors.push(error("E_UNKNOWN_KEY", `/devices/${id}/spec/${key}`, `spec key is not declared for ${device.kind}`));
    }
  }
}

function validateIds(board, schema, errors) {
  const pattern = new RegExp(schema.definitions.id.pattern);
  const check = (value, path) => {
    if (typeof value !== "string" || !pattern.test(value)) errors.push(error("E_ID_FORMAT", path, "ID must be snake_case ASCII"));
  };
  check(board.id, "/id");
  check(board.chip, "/chip");
  for (const group of ["buses", "devices", "connectors"]) for (const id of Object.keys(board[group] ?? {})) check(id, `/${group}/${id}`);
  for (const [index, revision] of (board.revisions ?? []).entries()) check(revision.id, `/revisions/${index}/id`);
}

function validatePinKeys(board, chip, errors) {
  for (const pin of Object.keys(board.pins ?? {})) {
    const path = `/pins/${pin}`;
    if (!/^[0-9]+$/.test(pin)) errors.push(error("E_PIN_KEY", path, "SoC pin key must contain decimal digits only"));
    else if (Number(pin) >= chip.gpio_count) errors.push(error("E_PIN_KEY", path, `GPIO ${pin} is outside chip range 0..${chip.gpio_count - 1}`));
    else if (chip.absent.includes(Number(pin))) errors.push(error("E_CHIP_ABSENT", path, `GPIO ${pin} does not exist on ${chip.id}`));
  }
}

function validateHex(value, schema, path, errors) {
  const pattern = new RegExp(schema.definitions.hexByte.pattern);
  if ((typeof value !== "string" || !pattern.test(value))) errors.push(error("E_HEX_FORMAT", path, "hex byte must use 0x notation"));
}

function validateHexFields(board, schema, errors) {
  for (const [id, device] of Object.entries(board.devices ?? {})) {
    if (own(device, "i2c_addr")) validateHex(device.i2c_addr, schema, `/devices/${id}/i2c_addr`, errors);
    for (const [choice, fragment] of Object.entries(device.choices ?? {})) if (own(fragment, "i2c_addr")) validateHex(fragment.i2c_addr, schema, `/devices/${id}/choices/${choice}/i2c_addr`, errors);
  }
  for (const [index, sensor] of (board.spec?.sensors ?? []).entries()) if (own(sensor, "i2c_addr")) validateHex(sensor.i2c_addr, schema, `/spec/sensors/${index}/i2c_addr`, errors);
}

function roleEntries(board) {
  return ownerRoleEntries(board).map((entry) => ({ ...entry, gpio: entry.owner === "soc" ? entry.pin : null }));
}

function validateRoles(board, chip, schema, errors, resolved) {
  const entries = roleEntries(board);
  const seen = new Map();
  const busSignals = schema.definitions.busKind["x-signals"];
  const deviceSignals = schema.definitions.deviceKind["x-signals"];
  const busOutputs = new Set(schema.definitions.busKind["x-output-signals"]);
  const deviceOutputs = new Set(schema.definitions.deviceKind["x-output-signals"]);

  for (const entry of entries) {
    if (!resolved && entry.owner === "soc" && entry.role.includes("/")) {
      errors.push(error("E_ROLE_SOURCE", entry.path, "source prefixes are reserved for composed accessory roles"));
    }
    if (!entry.parsed) {
      errors.push(error("E_REF_MISSING", entry.path, `invalid role ${entry.role}`));
      continue;
    }
    if (entry.parsed?.type !== "conn" && entry.parsed?.type !== "pwr") {
      const location = `${entry.owner}.${entry.pin}`;
      const normalizedRole = `${entry.parsed.type}:${entry.parsed.id}.${entry.parsed.signal}`;
      if (seen.has(normalizedRole) && seen.get(normalizedRole) !== location) errors.push(error("E_ROLE_DUP", entry.path, `${entry.role} is also on ${seen.get(normalizedRole)}`));
      else seen.set(normalizedRole, location);
    }

    const { type, id, signal } = entry.parsed;
    let output = false;
    if (type === "bus") {
      const bus = board.buses?.[id];
      if (!bus || !(busSignals[bus.kind] ?? []).includes(signal) || !(bus.signals ?? []).includes(signal)) {
        errors.push(error("E_REF_MISSING", entry.path, `${entry.role} does not resolve through the bus schema`));
      }
      output = busOutputs.has(signal);
    } else if (type === "dev") {
      const device = board.devices?.[id];
      if (!device || !(deviceSignals[device.kind] ?? []).includes(signal)) {
        errors.push(error("E_REF_MISSING", entry.path, `${entry.role} does not resolve through the device schema`));
      }
      output = deviceOutputs.has(signal);
    } else if (type === "conn" && !board.connectors?.[id]) {
      errors.push(error("E_REF_MISSING", entry.path, `${entry.role} refers to a missing connector`));
    }

    if (entry.owner === "soc") {
      const gpio = Number(entry.gpio);
      if (output && chip.input_only.includes(gpio)) errors.push(error("E_CHIP_INPUT_ONLY", entry.path, `output role ${entry.role} is on input-only GPIO ${gpio}`));
      const expectedPsram = chip.psram?.[board.spec?.storage?.psram_mode] ?? {};
      const psramSignal = Object.entries(expectedPsram).find(([, pin]) => pin === gpio)?.[0];
      const pinRoles = board.pins?.[String(gpio)]?.roles ?? [];
      const psramOwnsPin = psramSignal && pinRoles.includes(`dev:psram.${psramSignal}`);
      const allowedPsramShare = psramOwnsPin && (entry.role === `dev:psram.${psramSignal}` || entry.parsed?.type === "conn");
      if (chip.reserved.includes(gpio) && !allowedPsramShare) errors.push(error("E_CHIP_RESERVED", entry.path, `GPIO ${gpio} is reserved by the chip/package`));
      const conditional = chip.reserved_conditional?.[board.spec?.storage?.psram_mode] ?? [];
      if (conditional.includes(gpio) && !allowedPsramShare) errors.push(error("E_CHIP_RESERVED_COND", entry.path, `GPIO ${gpio} is reserved when PSRAM mode is ${board.spec.storage.psram_mode}`));
      if (gpio === chip.usb?.dn || gpio === chip.usb?.dp) errors.push(warning("W_CHIP_USB_PIN", entry.path, `GPIO ${gpio} is shared with native USB`));
    }
  }

  for (const [gpio, pin] of Object.entries(board.pins ?? {})) {
    const parsed = (pin.roles ?? []).map(parseRole).filter(Boolean);
    const busRoles = parsed.filter((item) => item.type === "bus");
    if (busRoles.length > 1 && new Set(busRoles.map((item) => item.signal)).size > 1) {
      errors.push(error("E_BUS_CONFLICT", `/pins/${gpio}/roles`, "different bus signals share one GPIO"));
    }
    const chipSelects = parsed.filter((item) => item.type === "dev" && (
      item.signal === "cs" || (board.devices?.[item.id]?.kind === "sd" && item.signal === "d3")
    ));
    if (chipSelects.length > 1) errors.push(error("E_CS_CONFLICT", `/pins/${gpio}/roles`, "multiple device chip-select roles share one GPIO"));
    const psramRoles = parsed.filter((item) => item.type === "dev" && item.id === "psram");
    const conflicting = parsed.filter((item) => item.type === "bus" || (item.type === "dev" && item.id !== "psram"));
    if (psramRoles.length && conflicting.length) errors.push(error("E_PSRAM_CONFLICT", `/pins/${gpio}/roles`, "PSRAM may share a GPIO only with connector roles"));
  }
}

function validatePsram(board, chip, errors, resolved) {
  if (!resolved && board.devices?.psram?.choices) return;
  const size = board.spec?.storage?.psram_mb ?? 0;
  const mode = board.spec?.storage?.psram_mode;
  const device = board.devices?.psram;
  if (size > 0 && !device) errors.push(error("E_PSRAM_DEVICE", "/devices/psram", "storage declares PSRAM but the psram device is missing"));
  if (size <= 0 && device) errors.push(error("E_PSRAM_STORAGE", "/spec/storage/psram_mb", "psram device requires a positive storage.psram_mb"));
  if (size <= 0 && mode) errors.push(error("E_PSRAM_STORAGE", "/spec/storage/psram_mode", "PSRAM mode requires a positive storage.psram_mb"));
  if (size > 0 && !mode) errors.push(error("E_PSRAM_STORAGE", "/spec/storage/psram_mode", "PSRAM size requires storage.psram_mode"));
  // Storage claims must match chip capabilities even without a device declaration.
  if (size > 0 && Object.keys(chip.psram ?? {}).length === 0) {
    errors.push(error("E_PSRAM_UNSUPPORTED", "/spec/storage/psram_mb", `${chip.id}/${chip.package} does not support PSRAM`));
  }
  const expected = chip.psram?.[mode];
  if (mode && !Object.hasOwn(chip.psram ?? {}, mode)) {
    errors.push(error("E_PSRAM_MODE", "/spec/storage/psram_mode", `${chip.id}/${chip.package} does not declare ${mode} PSRAM wiring`));
    return;
  }
  if (!device || !mode) return;
  const actual = {};
  for (const entry of roleEntries(board)) if (entry.owner === "soc" && entry.parsed?.type === "dev" && entry.parsed.id === "psram") {
    actual[entry.parsed.signal] = Number(entry.pin);
  }
  if (JSON.stringify(Object.entries(actual).sort()) !== JSON.stringify(Object.entries(expected).sort())) {
    errors.push(error("E_PSRAM_PINS", "/devices/psram", `PSRAM wiring ${JSON.stringify(actual)} does not match chip ${JSON.stringify(expected)}`));
  }
}

function validateReferences(board, schema, errors) {
  const entries = roleEntries(board);
  for (const [busId, bus] of Object.entries(board.buses ?? {})) {
    const allowed = new Set(schema.definitions.busKind["x-signals"][bus.kind] ?? []);
    for (const signal of bus.signals ?? []) {
      if (!allowed.has(signal)) errors.push(error("E_REF_MISSING", `/buses/${busId}/signals`, `${signal} is not declared for bus kind ${bus.kind}`));
      if (!entries.some((entry) => entry.parsed?.type === "bus" && entry.parsed.id === busId && entry.parsed.signal === signal)) {
        errors.push(error("E_SIGNAL_UNBOUND", `/buses/${busId}/signals`, `${signal} has no GPIO role`));
      }
    }
    if (bus.fixed === true && !own(bus, "preferred_host")) errors.push(error("E_REF_MISSING", `/buses/${busId}/preferred_host`, "fixed bus requires preferred_host"));
  }

  const allowedDeviceSignals = schema.definitions.deviceKind["x-signals"];
  for (const [deviceId, device] of Object.entries(board.devices ?? {})) {
    if (device.bus && !board.buses?.[device.bus]) errors.push(error("E_REF_MISSING", `/devices/${deviceId}/bus`, `missing bus ${device.bus}`));
    const allowed = new Set(allowedDeviceSignals[device.kind] ?? []);
    for (const signal of Object.keys(device.signals ?? {})) if (!allowed.has(signal)) {
      errors.push(error("E_REF_MISSING", `/devices/${deviceId}/signals/${signal}`, `${signal} is not declared for device kind ${device.kind}`));
    }
  }
}

const DEFAULT_RAILS = new Set(["gnd", "3v3", "5v", "5vout", "5vin", "bat", "vin", "vbus"]);
const DEFAULT_CHIP_ENDPOINTS = new Set(["en", "usb_dp", "usb_dn"]);

function validateConnectorEndpoint(endpoint, path, type, chip, errors, resolved) {
  const endpoints = Array.isArray(endpoint) ? endpoint : [endpoint];
  for (const value of endpoints) {
    if (typeof value !== "string") {
      errors.push(error("E_CONN_POS_ENDPOINT", path, "connector endpoint must be a string"));
      continue;
    }
    if (/^gpio:\d{1,2}$/.test(value)) {
      if (!resolved) errors.push(error("E_CONN_POS_GPIO", path, "GPIO connector assignments belong in pin roles"));
      continue;
    }
    const rail = /^pwr:([a-z0-9_]+)$/.exec(value)?.[1];
    if (rail && !DEFAULT_RAILS.has(rail) && !(type.rails ?? []).includes(rail)) {
      errors.push(error("E_RAIL_UNKNOWN", path, `unknown power rail ${rail}`));
      continue;
    }
    const chipPin = /^chip:([a-z0-9_]+)$/.exec(value)?.[1];
    if (chipPin && !DEFAULT_CHIP_ENDPOINTS.has(chipPin) && !(chip.connector_pins ?? []).includes(chipPin)) {
      errors.push(error("E_CONN_POS_ENDPOINT", path, `unknown non-GPIO chip pin ${chipPin}`));
      continue;
    }
    if (value === "none" || !/^(pin:[a-z][a-z0-9_]*\.[a-z0-9_]+|fixed:[a-z0-9_]+\.[a-z0-9_]+|pwr:[a-z0-9_]+|chip:[a-z0-9_]+|nc)$/.test(value)) {
      errors.push(error("E_CONN_POS_ENDPOINT", path, "endpoint is not valid for a connector position"));
    }
  }
}

function validateConnectors(board, chip, connectorTypes, errors, resolved) {
  const entries = roleEntries(board).filter((entry) => entry.parsed?.type === "conn");
  for (const entry of entries) {
    const connector = board.connectors?.[entry.parsed.id];
    if (!connector) continue;
    const type = resolveConnectorType(connectorTypes, connector.type);
    if (type && !(type.positions ?? []).some((position) => position.id === entry.parsed.signal)) {
      errors.push(error("E_CONN_POS_UNKNOWN", entry.path, `${entry.role} uses a position absent from ${connector.type}`));
    }
  }

  for (const [id, connector] of Object.entries(board.connectors ?? {})) {
    const path = `/connectors/${id}`;
    const type = resolveConnectorType(connectorTypes, connector.type);
    if (!type) {
      errors.push(error("E_CONN_TYPE_MISSING", `${path}/type`, `connector type ${connector.type ?? "<missing>"} is not in the catalog`));
      continue;
    }
    if (connector.standard && !own(type.standards ?? {}, connector.standard)) {
      errors.push(error("E_CONN_TYPE_MISSING", `${path}/standard`, `standard ${connector.standard} is not declared by ${connector.type}`));
    }
    const known = new Set(type.positions.map((position) => position.id));
    for (const position of [...Object.keys(connector.positions ?? {}), ...(connector.multi_gpio ?? []), ...(connector.shared_gpio ?? [])]) {
      if (!known.has(position)) errors.push(error("E_CONN_POS_UNKNOWN", `${path}/positions/${position}`, `position ${position} is not declared by ${connector.type}`));
    }

    if (resolved) {
      const gpioPositions = new Map();
      const gpioPositionCounts = new Map();
      for (const endpoint of Object.values(connector.positions ?? {})) {
        for (const item of Array.isArray(endpoint) ? endpoint : [endpoint]) {
          const gpio = /^gpio:(\d+)$/.exec(item)?.[1];
          if (gpio !== undefined) gpioPositionCounts.set(gpio, (gpioPositionCounts.get(gpio) ?? 0) + 1);
        }
      }
      for (const position of known) {
        if (!own(connector.positions ?? {}, position)) errors.push(error("E_CONN_POS_MISSING", `${path}/positions/${position}`, "resolved connector position is missing"));
        else {
          const endpoint = connector.positions[position];
          if (Array.isArray(endpoint) && !(connector.multi_gpio ?? []).includes(position)) {
            errors.push(error("E_CONN_POS_DUP", `${path}/positions/${position}`, "multiple resolved GPIOs require multi_gpio"));
          }
          validateConnectorEndpoint(endpoint, `${path}/positions/${position}`, type, chip, errors, true);
          for (const item of Array.isArray(endpoint) ? endpoint : [endpoint]) {
            const gpio = /^gpio:(\d+)$/.exec(item)?.[1];
            if (gpio !== undefined && gpioPositions.has(gpio) && gpioPositions.get(gpio) !== position
             && !(connector.shared_gpio ?? []).includes(position)) {
              errors.push(error("E_CONN_SAME_GPIO", `${path}/positions/${position}`, `GPIO ${gpio} is also assigned to position ${gpioPositions.get(gpio)}`));
            } else if (gpio !== undefined) gpioPositions.set(gpio, position);
          }
        }
      }
      for (const position of connector.shared_gpio ?? []) {
        const endpoint = connector.positions?.[position];
        const gpios = (Array.isArray(endpoint) ? endpoint : [endpoint])
          .map((item) => /^gpio:(\d+)$/.exec(item)?.[1]).filter((item) => item !== undefined);
        if (!gpios.some((gpio) => gpioPositionCounts.get(gpio) > 1)) {
          errors.push(warning("W_CONN_SHARED_UNUSED", `${path}/shared_gpio`, `position ${position} declares shared_gpio but its GPIO is not used by another position`));
        }
      }
      continue;
    }

    const { sources } = resolveConnectorPositions(board, id, connectorTypes);
    for (const [position, source] of Object.entries(sources)) {
      const positionPath = `${path}/positions/${position}`;
      if (source.explicit) {
        validateConnectorEndpoint(connector.positions[position], positionPath, type, chip, errors, false);
        if (source.fixed) errors.push(error("E_CONN_POS_FIXED", positionPath, "fixed connector position cannot be overridden by a board"));
      }
      if ((source.explicit && source.gpios.length) || (source.fixed && source.gpios.length) || (source.gpios.length > 1 && !(connector.multi_gpio ?? []).includes(position))) {
        errors.push(error("E_CONN_POS_DUP", positionPath, "connector position has multiple assignment sources"));
      }
      if (!source.fixed && !source.default && !source.explicit && source.gpios.length === 0) {
        errors.push(error("E_CONN_POS_MISSING", positionPath, "connector position is not assigned"));
      }
      if ((connector.multi_gpio ?? []).includes(position) && source.gpios.length <= 1) {
        errors.push(warning("W_CONN_MULTI_UNUSED", `${path}/multi_gpio`, `position ${position} declares multi_gpio but has ${source.gpios.length} GPIO role(s)`));
      }
    }
  }
}

function walkVerified(board, value, path, errors) {
  if (Array.isArray(value)) {
    value.forEach((item, index) => walkVerified(board, item, `${path}/${index}`, errors));
    return;
  }
  if (!isPlainObject(value)) return;
  if (isPlainObject(value.verified)) {
    for (const [field, status] of Object.entries(value.verified)) {
      if (!own(value, field)) errors.push(error("E_VERIFIED_KEY", `${path}/verified/${field}`, "verified key has no sibling field"));
      if (status === "generated" && !isGeneratedField(board, path, field)) errors.push(error("E_VERIFIED_FAKE", `${path}/verified/${field}`, "field is not consumed by a stage-0 target"));
      if (status === "measured" && (!isPlainObject(value.measured_by) || !value.measured_by[field])) {
        errors.push(error("E_MEASURED_BY", `${path}/verified/${field}`, "measured field needs measured_by"));
      }
    }
  }
  for (const [key, item] of Object.entries(value)) if (!["verified", "measured_by"].includes(key)) walkVerified(board, item, `${path}/${key}`, errors);
}

function validateSourceDerivedFields(board, errors) {
  for (const [id, device] of Object.entries(board.devices ?? {})) for (const field of ["signals", "enable", "derived"]) {
    if (own(device, field)) errors.push(error("E_UNKNOWN_KEY", `/devices/${id}/${field}`, `${field} is derived from pin roles`));
  }
}

function validateSdAliases(board, parts, errors) {
  for (const [gpio, pin] of Object.entries(board.pins ?? {})) {
    const parsed = (pin.roles ?? []).map((role) => ({ role, parsed: parseRole(role) })).filter((entry) => entry.parsed);
    const spiRoles = parsed.filter((entry) => entry.parsed.type === "bus" && board.buses?.[entry.parsed.id]?.kind === "spi");
    const sdRoles = parsed.filter((entry) => entry.parsed.type === "dev" && board.devices?.[entry.parsed.id]?.kind === "sd");
    for (const sdRole of sdRoles) for (const spiRole of spiRoles) {
      const device = board.devices[sdRole.parsed.id];
      const alias = parts?.[device.part]?.spi_alias?.[sdRole.parsed.signal];
      if (device.bus !== spiRole.parsed.id || alias !== spiRole.parsed.signal) {
        errors.push(error("E_SD_ALIAS_MISMATCH", `/pins/${gpio}/roles`, `${sdRole.role} does not alias ${spiRole.role}`));
      }
    }
  }
}

function validateSdModes(board, errors) {
  for (const [id, device] of Object.entries(board.devices ?? {})) if (device.kind === "sd" && !(device.derived?.modes?.length)) {
    errors.push(warning("W_SD_NO_MODE", `/devices/${id}/derived/modes`, "SD wiring supports no known mode"));
  }
}

function validateEndpointOwners(board, errors) {
  const check = (endpoint, path) => {
    const owner = /^pin:([a-z][a-z0-9_]*)\./.exec(endpoint)?.[1];
    if (owner && owner !== "soc" && !board.devices?.[owner]) errors.push(error("E_ENDPOINT_OWNER", path, `pin owner ${owner} is not a device`));
  };
  for (const [id, device] of Object.entries(board.devices ?? {})) {
    for (const [signal, endpoint] of Object.entries(device.signals ?? {})) check(endpoint, `/devices/${id}/signals/${signal}`);
    if (device.enable) check(device.enable, `/devices/${id}/enable`);
  }
  for (const [id, connector] of Object.entries(board.connectors ?? {})) for (const [position, endpoint] of Object.entries(connector.positions ?? {})) {
    for (const item of Array.isArray(endpoint) ? endpoint : [endpoint]) check(item, `/connectors/${id}/positions/${position}`);
  }
}

function unique(errors) {
  const seen = new Set();
  return errors.filter((item) => {
    const key = `${item.id}\0${item.path}\0${item.message}`;
    if (seen.has(key)) return false;
    seen.add(key);
    return true;
  });
}

export function validateBoard(board, { schema, chip, connectorTypes = {}, parts = {}, target = {}, resolved = false }) {
  const packageKnown = !board.chip_package || Object.hasOwn(chip.packages ?? {}, board.chip_package);
  chip = effectiveChip(chip, board);
  const errors = [...(board.__compositionIssues ?? []), ...validateSchema(board, schema)];
  if (!packageKnown) errors.push(error("E_CHIP_PACKAGE", "/chip_package", `${board.chip_package} is not declared by ${chip.id}`));
  validateAliases(board, errors);
  validateDeviceSpecKeys(board, schema, errors);
  validateIds(board, schema, errors);
  validatePinKeys(board, chip, errors);
  const checkDetectClasses = (pins, prefix) => {
    for (const [gpio, pin] of Object.entries(pins ?? {})) {
      if (pin.detect_class !== undefined) {
        if ((pin.roles ?? []).some((role) => /(?:^|\/)conn:/.test(role))) {
          errors.push(error("E_DETECT_CLASS_CONNECTOR", `${prefix}/${gpio}/detect_class`, "detect_class cannot use a user connector pin"));
        }
        if ((pin.roles ?? []).some((role) => {
          const match = /^dev:([^.]*)\./.exec(role);
          return match && board.devices?.[match[1]]?.kind === "button";
        })) {
          errors.push(error("E_DETECT_CLASS_BUTTON", `${prefix}/${gpio}/detect_class`, "detect_class cannot use a user button pin"));
        }
      }
      if (pin.detect_class !== undefined && (chip.no_internal_pull ?? []).includes(Number(gpio))) {
        errors.push(error("E_DETECT_CLASS_NO_PULL", `${prefix}/${gpio}/detect_class`, "detect_class requires internal GPIO pulls"));
      }
    }
  };
  checkDetectClasses(board.pins, "/pins");
  for (const [id, device] of Object.entries(board.devices ?? {})) {
    for (const [index, choice] of Object.entries(device.choices ?? {})) {
      checkDetectClasses(choice.soc_pins, `/devices/${id}/choices/${index}/soc_pins`);
    }
  }

  validateHexFields(board, schema, errors);
  validateRoles(board, chip, schema, errors, resolved);
  validatePsram(board, chip, errors, resolved);
  validateReferences(board, schema, errors);
  validateSdAliases(board, parts, errors);
  errors.push(...validateParts(board, parts, { resolved }));
  validateConnectors(board, chip, connectorTypes, errors, resolved);
  errors.push(...validateOwners(board, parts));
  walkVerified(board, board, "", errors);
  if (resolved) {
    validateEndpointOwners(board, errors);
    validateSdModes(board, errors);
  } else {
    validateSourceDerivedFields(board, errors);
    errors.push(...validateChoices(board));
    errors.push(...validateTarget(board, target));
  }
  errors.push(...validateFormat(board));
  return unique(errors);
}

function validateAliases(board, errors) {
  if (!Array.isArray(board.aliases)) return;
  const seen = new Set();
  board.aliases.forEach((alias, index) => {
    if (typeof alias !== "string") return;
    const normalized = alias.trim().toLowerCase();
    if (!normalized || alias !== alias.trim() || seen.has(normalized)) {
      errors.push(error("E_ALIAS_FORMAT", `/aliases/${index}`, "aliases must be trimmed, non-empty, and unique ignoring case"));
    }
    seen.add(normalized);
  });
}

export function validateFormat(board, formatter) {
  return checkFormatIdempotent(board, formatter) ? [] : [error("E_FORMAT", "", "formatter is not idempotent")];
}

export function validateCatalog(boards, contextForBoard) {
  const errors = [];
  const legacyIds = new Map();
  for (const board of boards) {
    errors.push(...validateBoard(board, contextForBoard(board)));
    const ids = [[board.legacy_board_id, board.id]];
    for (const [legacy, id] of ids) {
      if (!Number.isInteger(legacy)) continue;
      if (legacyIds.has(legacy)) errors.push(error("E_ID_FORMAT", `/${id}/legacy_board_id`, `legacy_board_id ${legacy} duplicates ${legacyIds.get(legacy)}`));
      else legacyIds.set(legacy, id);
    }
  }
  return unique(errors);
}

export function validateResolvedVariants(board, context) {
  const errors = [];
  const fullCatalogs = {
    chip: context.chip,
    parts: context.parts,
    accessories: context.accessories,
    composition: context.composition,
    allow_origins: context.allow_origins,
  };
  const fullOutputs = resolveAll(board, context.connectorTypes, fullCatalogs);
  for (const output of fullOutputs) {
    errors.push(...validateBoard(output.board, { ...context, resolved: true }));
  }
  const wiringCatalogs = { chip: context.chip, parts: context.parts };
  const emitWiring = (resolved) => {
    try {
      return emitM5GFXWiring(resolved, context.parts, context.target);
    } catch (failure) {
      errors.push(error("E_WIRING_GENERATION", "/pins", failure.message));
      return null;
    }
  };
  const baseWiring = emitWiring(resolveBoard(board, {}, context.connectorTypes, wiringCatalogs));
  if (baseWiring) {
    const expected = JSON.stringify(baseWiring);
    for (const output of resolveAll(board, context.connectorTypes, wiringCatalogs)) {
      const actual = emitWiring(output.board);
      if (actual && JSON.stringify(actual) !== expected) {
        errors.push(error("E_CHOICE_PINTABLE", `/revisions/${output.revision ?? "base"}`, "choice changes GPIO wiring consumed by a board-level generator"));
      }
    }
  }
  const emitPins = (resolved) => {
    try {
      return JSON.stringify(emitPinTable(resolved, context.pinTableTarget).values);
    } catch (failure) {
      errors.push(error("E_CHOICE_PINTABLE", "/pins", failure.message));
      return null;
    }
  };
  const expectedPinTable = emitPins(resolveBoard(board, {}, context.connectorTypes, fullCatalogs));
  for (const output of fullOutputs) {
    const actualPinTable = emitPins(output.board);
    if (expectedPinTable && actualPinTable && actualPinTable !== expectedPinTable) {
      errors.push(error("E_CHOICE_PINTABLE", `/revisions/${output.revision ?? "base"}`, "choice changes GPIO wiring consumed by a board-level generator"));
    }
  }
  return unique(errors);
}
