// DOM-free board editing operations shared by the browser editor and Node tests.

const BOARD_ID_PATTERN = /^[a-z][a-z0-9_]*$/;

export class BoardOperationError extends Error {
  constructor(code, message) {
    super(message);
    this.name = "BoardOperationError";
    this.code = code;
  }
}

function boardOpClone(value) {
  return JSON.parse(JSON.stringify(value));
}

function requireId(id, label = "id") {
  if (!BOARD_ID_PATTERN.test(id)) throw new BoardOperationError("E_ID_FORMAT", `${label} must be snake_case ASCII`);
}

function clearGeneratedStatus(object, field) {
  if (object?.verified) {
    delete object.verified[field];
    if (!Object.keys(object.verified).length) delete object.verified;
  }
  if (object?.measured_by) {
    delete object.measured_by[field];
    if (!Object.keys(object.measured_by).length) delete object.measured_by;
  }
}

function compactObject(root, keys) {
  for (let index = keys.length - 1; index >= 0; index -= 1) {
    const parent = keys.slice(0, index).reduce((value, key) => value?.[key], root);
    const key = keys[index];
    if (parent?.[key] && typeof parent[key] === "object" && !Array.isArray(parent[key]) && !Object.keys(parent[key]).length) delete parent[key];
  }
}

function roleRecords(board) {
  const records = [];
  for (const [gpio, pin] of Object.entries(board.pins ?? {})) for (const role of pin.roles ?? []) records.push({ owner: "soc", pin: gpio, role });
  for (const [deviceId, device] of Object.entries(board.devices ?? {})) {
    for (const [pinId, pin] of Object.entries(device.pins ?? {})) for (const role of pin.roles ?? []) records.push({ owner: deviceId, pin: pinId, role });
    for (const [choiceId, choice] of Object.entries(device.choices ?? {})) for (const [pinId, pin] of Object.entries(choice.pins ?? {})) {
      for (const role of pin.roles ?? []) records.push({ owner: `${deviceId}:${choiceId}`, pin: pinId, role });
    }
  }
  return records;
}

export function boardRoleOptions(board, { schema, chip, parts = {}, connectorTypes = {}, resolveConnectorType: resolveType, currentGPIO = null }) {
  const roles = [];
  const busSignals = schema.definitions.busKind["x-signals"] ?? {};
  for (const [id, bus] of Object.entries(board.buses ?? {})) for (const signal of bus.signals ?? []) {
    if ((busSignals[bus.kind] ?? []).includes(signal)) roles.push(`bus:${id}.${signal}`);
  }
  for (const [id, device] of Object.entries(board.devices ?? {})) {
    const selectedParts = [device.part, ...Object.values(device.choices ?? {}).map((choice) => choice.part)].filter(Boolean).map((partId) => parts[partId]).filter(Boolean);
    const signals = selectedParts.length
      ? selectedParts.flatMap((part) => [...(part.signals?.required ?? []), ...(part.signals?.optional ?? [])])
      : schema.definitions.deviceKind["x-signals"]?.[device.kind] ?? [];
    for (const signal of new Set(signals)) roles.push(`dev:${id}.${signal}`);
  }
  for (const [id, connector] of Object.entries(board.connectors ?? {})) {
    const type = resolveType ? resolveType(connectorTypes, connector.type) : connectorTypes[connector.type];
    const positions = new Set((type?.positions ?? []).filter((position) => !Object.hasOwn(type.fixed_positions ?? {}, position.id) && !Object.hasOwn(type.default_positions ?? {}, position.id) && !Object.hasOwn(connector.positions ?? {}, position.id)).map((position) => position.id));
    for (const pin of Object.values(board.pins ?? {})) for (const role of pin.roles ?? []) {
      const match = new RegExp(`^conn:${id.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")}\\.([a-z0-9_]+)$`).exec(role);
      if (match) positions.add(match[1]);
    }
    for (const position of positions) roles.push(`conn:${id}.${position}`);
  }
  const assigned = new Set();
  for (const [gpio, pin] of Object.entries(board.pins ?? {})) if (String(gpio) !== String(currentGPIO)) for (const role of pin.roles ?? []) {
    const match = /^conn:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
    if (!match || !(board.connectors?.[match[1]]?.multi_gpio ?? []).includes(match[2])) assigned.add(role);
  }
  for (const device of Object.values(board.devices ?? {})) {
    for (const pin of Object.values(device.pins ?? {})) for (const role of pin.roles ?? []) assigned.add(role);
    for (const choice of Object.values(device.choices ?? {})) for (const pin of Object.values(choice.pins ?? {})) for (const role of pin.roles ?? []) assigned.add(role);
  }
  let output = [...new Set(roles)].filter((role) => !assigned.has(role));
  if (chip?.input_only?.includes(Number(currentGPIO))) {
    const busOutputs = new Set(schema.definitions.busKind["x-output-signals"] ?? []);
    const deviceOutputs = new Set(schema.definitions.deviceKind["x-output-signals"] ?? []);
    output = output.filter((role) => {
      const signal = role.slice(role.lastIndexOf(".") + 1);
      return role.startsWith("bus:") ? !busOutputs.has(signal) : role.startsWith("dev:") ? !deviceOutputs.has(signal) : true;
    });
  }
  return output.sort((a, b) => a.localeCompare(b));
}

function normalizedAliases(aliases) {
  if (aliases === undefined) return undefined;
  if (!Array.isArray(aliases) || aliases.some((alias) => typeof alias !== "string" || !alias.trim())) throw new BoardOperationError("E_FIELD", "aliases must be non-empty strings");
  const seen = new Set();
  return aliases.map((alias) => alias.trim()).filter((alias) => {
    const normalized = alias.toLowerCase();
    if (seen.has(normalized)) return false;
    seen.add(normalized);
    return true;
  });
}

export function createBoard({ id, name, official_name = name, official_name_verified, aliases, legacy_board_id, chip }) {
  requireId(id);
  requireId(chip, "chip");
  if (!name?.trim()) throw new BoardOperationError("E_NAME_REQUIRED", "name is required");
  if (!official_name?.trim()) throw new BoardOperationError("E_NAME_REQUIRED", "official_name is required");
  if (!Number.isInteger(legacy_board_id)) throw new BoardOperationError("E_LEGACY_ID", "legacy_board_id must be an integer");
  const output = { schema_version: 1, id, name: name.trim(), official_name: official_name.trim(), legacy_board_id, chip, pins: {}, buses: {}, devices: {}, connectors: {} };
  if (official_name_verified === true) output.official_name_verified = true;
  const normalized = normalizedAliases(aliases);
  if (normalized?.length) output.aliases = normalized;
  return output;
}

export function duplicateBoard(board, { id, name, official_name = name, official_name_verified, aliases, legacy_board_id }) {
  const output = boardOpClone(board);
  requireId(id);
  if (!name?.trim()) throw new BoardOperationError("E_NAME_REQUIRED", "name is required");
  if (!official_name?.trim()) throw new BoardOperationError("E_NAME_REQUIRED", "official_name is required");
  if (!Number.isInteger(legacy_board_id)) throw new BoardOperationError("E_LEGACY_ID", "legacy_board_id must be an integer");
  output.id = id;
  output.name = name.trim();
  output.official_name = official_name.trim();
  if (official_name_verified === true) output.official_name_verified = true;
  else delete output.official_name_verified;
  const normalized = normalizedAliases(aliases);
  if (normalized !== undefined) {
    if (normalized.length) output.aliases = normalized;
    else delete output.aliases;
  }
  output.legacy_board_id = legacy_board_id;
  return output;
}

export function setBoardField(board, key, value) {
  if (!["name", "official_name", "official_name_verified", "aliases", "legacy_board_id", "chip"].includes(key)) throw new BoardOperationError("E_FIELD", `unsupported board field ${key}`);
  if (key === "chip") requireId(value, "chip");
  if (key === "legacy_board_id" && !Number.isInteger(value)) throw new BoardOperationError("E_LEGACY_ID", "legacy_board_id must be an integer");
  if (["name", "official_name"].includes(key) && !String(value).trim()) throw new BoardOperationError("E_NAME_REQUIRED", `${key} is required`);
  if (key === "official_name_verified") {
    if (value) board.official_name_verified = true;
    else delete board.official_name_verified;
    return board;
  }
  if (key === "aliases") {
    const aliases = normalizedAliases(value);
    if (aliases.length) board.aliases = aliases;
    else delete board.aliases;
    return board;
  }
  board[key] = ["name", "official_name"].includes(key) ? String(value).trim() : value;
  return board;
}

export function setSpec(board, path, value) {
  const keys = Array.isArray(path) ? path : String(path).split(".").filter(Boolean);
  if (!keys.length) throw new BoardOperationError("E_FIELD", "spec path is required");
  if (value === undefined || value === "") {
    let parent = board.spec;
    for (const key of keys.slice(0, -1)) parent = parent?.[key];
    if (parent) delete parent[keys.at(-1)];
    compactObject(board, ["spec", ...keys.slice(0, -1)]);
    return board;
  }
  board.spec ??= {};
  let target = board.spec;
  for (const key of keys.slice(0, -1)) target = target[key] ??= {};
  target[keys.at(-1)] = boardOpClone(value);
  return board;
}

export function addBus(board, id, { kind, signals = [], preferred_host, freq, freq_read, fixed = false }) {
  requireId(id);
  board.buses ??= {};
  if (board.buses[id]) throw new BoardOperationError("E_ID_DUP", `bus ${id} already exists`);
  const bus = { kind, signals: [...new Set(signals)], ...(preferred_host === undefined || preferred_host === "" ? {} : { preferred_host }), fixed: Boolean(fixed) };
  if (freq !== undefined && freq !== "") bus.freq = Number(freq);
  if (freq_read !== undefined && freq_read !== "") bus.freq_read = Number(freq_read);
  board.buses[id] = bus;
  return board;
}

export function removeBus(board, id) {
  if (!board.buses?.[id]) return board;
  if (Object.values(board.devices ?? {}).some((device) => device.bus === id || Object.values(device.choices ?? {}).some((choice) => choice.bus === id))) {
    throw new BoardOperationError("E_BUS_IN_USE", `bus ${id} is used by a device`);
  }
  if (roleRecords(board).some(({ role }) => role.startsWith(`bus:${id}.`))) throw new BoardOperationError("E_BUS_IN_USE", `bus ${id} has assigned roles`);
  delete board.buses[id];
  return board;
}

export function setBusField(board, id, key, value) {
  const bus = board.buses?.[id];
  if (!bus) throw new BoardOperationError("E_REF_MISSING", `bus ${id} is missing`);
  if (value === undefined || value === "") delete bus[key];
  else bus[key] = boardOpClone(value);
  clearGeneratedStatus(bus, key);
  return board;
}

export function addDevice(board, id, { kind, part, bus, spec }) {
  requireId(id);
  board.devices ??= {};
  if (board.devices[id]) throw new BoardOperationError("E_ID_DUP", `device ${id} already exists`);
  board.devices[id] = { kind, ...(part ? { part } : {}), ...(bus ? { bus } : {}), ...(spec && Object.keys(spec).length ? { spec: boardOpClone(spec) } : {}) };
  return board;
}

export function removeDevice(board, id) {
  if (!board.devices?.[id]) return board;
  if (roleRecords(board).some(({ role }) => role.startsWith(`dev:${id}.`))) throw new BoardOperationError("E_DEVICE_IN_USE", `device ${id} has assigned roles`);
  for (const device of Object.values(board.devices)) {
    const endpoints = [...Object.values(device.signals ?? {}), device.enable].filter(Boolean);
    if (endpoints.some((endpoint) => String(endpoint).startsWith(`pin:${id}.`))) throw new BoardOperationError("E_DEVICE_IN_USE", `device ${id} owns referenced pins`);
  }
  delete board.devices[id];
  return board;
}

export function setDeviceField(board, deviceId, key, value) {
  const device = board.devices?.[deviceId];
  if (!device) throw new BoardOperationError("E_REF_MISSING", `device ${deviceId} is missing`);
  if (value === undefined || value === "") delete device[key];
  else device[key] = boardOpClone(value);
  clearGeneratedStatus(device, key);
  return board;
}

export function addChoice(board, deviceId, choiceId, { part, spec, selected_by = "revision", default: defaultChoice } = {}) {
  requireId(choiceId, "choice id");
  const device = board.devices?.[deviceId];
  if (!device) throw new BoardOperationError("E_REF_MISSING", `device ${deviceId} is missing`);
  if (!device.choices) {
    const originalId = device.part;
    if (!originalId) throw new BoardOperationError("E_CHOICE_SOURCE", "the first choice needs an existing part");
    const fragment = { part: originalId };
    for (const key of ["i2c_addr", "pins", "rails", "spec", "note", "verified", "measured_by"]) {
      if (Object.hasOwn(device, key)) { fragment[key] = device[key]; delete device[key]; }
    }
    delete device.part;
    device.selected_by = selected_by;
    device.default = defaultChoice ?? originalId;
    device.choices = { [originalId]: fragment };
  }
  if (device.choices[choiceId]) throw new BoardOperationError("E_ID_DUP", `choice ${choiceId} already exists`);
  device.choices[choiceId] = { ...(part ? { part } : {}), ...(spec && Object.keys(spec).length ? { spec: boardOpClone(spec) } : {}) };
  if (defaultChoice) device.default = defaultChoice;
  if (selected_by) device.selected_by = selected_by;
  return board;
}

export function removeChoice(board, deviceId, choiceId) {
  const device = board.devices?.[deviceId];
  if (!device?.choices?.[choiceId]) return board;
  delete device.choices[choiceId];
  if (Object.keys(device.choices).length === 1) {
    const fragment = Object.values(device.choices)[0];
    for (const key of ["part", "i2c_addr", "pins", "rails", "spec", "note", "verified", "measured_by"]) if (Object.hasOwn(fragment, key)) device[key] = fragment[key];
    delete device.selected_by;
    delete device.default;
    delete device.choices;
    return board;
  }
  if (device.default === choiceId) device.default = Object.keys(device.choices)[0];
  return board;
}

export function setChoiceDefaults(board, deviceId, { selected_by, default: defaultChoice }) {
  const device = board.devices?.[deviceId];
  if (!device?.choices) throw new BoardOperationError("E_REF_MISSING", `choice device ${deviceId} is missing`);
  if (selected_by) device.selected_by = selected_by;
  if (defaultChoice) device.default = defaultChoice;
  return board;
}

export function setDeviceSpec(board, deviceId, choiceId, key, value) {
  const device = board.devices?.[deviceId];
  const target = choiceId === null ? device : device?.choices?.[choiceId];
  if (!target) throw new BoardOperationError("E_REF_MISSING", `device spec target ${deviceId}${choiceId === null ? "" : `:${choiceId}`} is missing`);
  if (value === undefined || value === "") {
    if (target.spec) {
      delete target.spec[key];
      if (!Object.keys(target.spec).length) delete target.spec;
    }
  } else {
    target.spec ??= {};
    target.spec[key] = value;
  }
  clearGeneratedStatus(target, "spec");
  return board;
}

export function addConnector(board, id, { type, standard } = {}) {
  requireId(id);
  board.connectors ??= {};
  if (board.connectors[id]) throw new BoardOperationError("E_ID_DUP", `connector ${id} already exists`);
  board.connectors[id] = { type, ...(standard ? { standard } : {}) };
  return board;
}

export function removeConnector(board, id) {
  if (roleRecords(board).some(({ role }) => role.startsWith(`conn:${id}.`))) throw new BoardOperationError("E_CONNECTOR_IN_USE", `connector ${id} has assigned roles`);
  delete board.connectors?.[id];
  return board;
}

export function addRole(board, gpio, role, { chip } = {}) {
  const number = Number(gpio);
  const psramSignal = Object.entries(chip?.psram?.[board.spec?.storage?.psram_mode] ?? {}).find(([, pin]) => pin === number)?.[0];
  const expectedPsramRole = psramSignal ? `dev:psram.${psramSignal}` : null;
  if (!Number.isInteger(number) || number < 0 || (chip && number >= chip.gpio_count)) throw new BoardOperationError("E_GPIO_RANGE", `GPIO ${gpio} is outside the chip`);
  if (chip?.absent?.includes(number)) throw new BoardOperationError("E_CHIP_ABSENT", `GPIO ${gpio} does not exist on this chip`);
  if (chip?.reserved?.includes(number) && role !== expectedPsramRole) throw new BoardOperationError("E_CHIP_RESERVED", `GPIO ${gpio} is reserved`);
  const conditional = chip?.reserved_conditional?.[board.spec?.storage?.psram_mode] ?? [];
  if (conditional.includes(number) && role !== expectedPsramRole) throw new BoardOperationError("E_CHIP_RESERVED_COND", `GPIO ${gpio} is conditionally reserved`);
  board.pins ??= {};
  const pin = board.pins[String(number)] ??= { roles: [] };
  pin.roles ??= [];
  if (!pin.roles.includes(role)) pin.roles.push(role);
  clearGeneratedStatus(pin, "roles");
  return board;
}

export function setPin(board, gpio, fields) {
  const key = String(gpio);
  board.pins ??= {};
  const pin = board.pins[key] ??= { roles: [] };
  for (const [field, value] of Object.entries(fields)) {
    if (value === undefined || value === "") delete pin[field];
    else pin[field] = boardOpClone(value);
    clearGeneratedStatus(pin, field);
  }
  return board;
}

export function addOwnerRole(board, deviceId, pinId, role) {
  const device = board.devices?.[deviceId];
  if (!device) throw new BoardOperationError("E_REF_MISSING", `device ${deviceId} is missing`);
  device.pins ??= {};
  const pin = device.pins[pinId] ??= { roles: [] };
  if (!pin.roles.includes(role)) pin.roles.push(role);
  clearGeneratedStatus(pin, "roles");
  return board;
}

export function removeRoleFromBoard(board, gpio, role) {
  const key = String(gpio);
  const pin = board.pins?.[key];
  if (!pin) return board;
  pin.roles = (pin.roles ?? []).filter((item) => item !== role);
  clearGeneratedStatus(pin, "roles");
  if (!pin.roles.length && Object.keys(pin).every((field) => field === "roles")) delete board.pins[key];
  return board;
}
