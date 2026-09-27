import { clone } from "./model.js";
import { isCompatible } from "./ctypes.js";
import { parseRole } from "./owners.js";

const compIssue = (id, path, message, severity) => ({ id, path, message, ...(severity ? { severity } : {}) });

function selectedAccessory(accessory, settings, issues, path) {
  const value = clone(accessory);
  const hostPins = clone(value.host_pins ?? {});
  const staticPositions = new Set(Object.keys(hostPins));
  for (const id of Object.keys(settings ?? {})) if (!value.settings?.[id]) {
    issues.push(compIssue("E_COMP_SETTING_UNKNOWN", `${path}/settings/${id}`, `${id} is not a setting of ${value.id}`));
  }
  for (const [id, setting] of Object.entries(value.settings ?? {})) {
    const choice = settings?.[id] ?? setting.default;
    if (choice == null) {
      issues.push(compIssue("E_COMP_SETTING_REQUIRED", `${path}/settings/${id}`, `${id} requires an explicit choice`));
      continue;
    }
    if (!setting.choices?.[choice]) {
      issues.push(compIssue("E_COMP_SETTING_UNKNOWN", `${path}/settings/${id}`, `${choice} is not a choice for ${id}`));
      continue;
    }
    for (const [position, row] of Object.entries(setting.choices[choice].host_pins ?? {})) {
      if (staticPositions.has(position)) issues.push(compIssue("E_CHOICE_KEY_OVERLAP", `${path}/settings/${id}/choices/${choice}/host_pins/${position}`, `${position} is also static`));
      hostPins[position] = hostPins[position]
        ? { ...hostPins[position], ...clone(row), roles: [...(hostPins[position].roles ?? []), ...(row.roles ?? [])] }
        : clone(row);
    }
  }
  value.host_pins = hostPins;
  return value;
}

function renameRole(role, mapping) {
  const parsed = parseRole(role);
  if (!parsed || parsed.type === "pwr") return role;
  const id = mapping[parsed.id] ?? parsed.id;
  return `${parsed.type}:${id}.${parsed.signal}`;
}

function sourceRole(accessoryId, role, mapping) {
  return `${accessoryId}/${renameRole(role, mapping)}`;
}

function included(role, use) {
  if (!use) return true;
  const parsed = parseRole(role);
  return !parsed || parsed.type === "pwr" || use.has(parsed.id);
}

function sourceInfo(accessory) {
  return { accessory: accessory.id, origin: accessory.origin, bundled: accessory.bundled === true };
}

export function compose(resolvedBoard, entries = [], catalogs = {}) {
  const board = clone(resolvedBoard);
  const issues = [];
  const occupied = [];
  const allowedOrigins = new Set(catalogs.allow_origins ?? ["official", "third_party", "virtual"]);

  for (const [index, entry] of entries.entries()) {
    const path = `/composition/accessories/${index}`;
    const catalog = catalogs.accessories?.[entry.id];
    if (!catalog) {
      issues.push(compIssue("E_COMP_INCOMPATIBLE", `${path}/id`, `accessory ${entry.id} is missing`));
      continue;
    }
    if (!allowedOrigins.has(catalog.origin)) issues.push(compIssue("E_COMP_ORIGIN", `${path}/id`, `origin ${catalog.origin} is not allowed`));
    const host = Object.entries(board.connectors ?? {}).find(([, connector]) => isCompatible(catalogs.connectorTypes, connector.type, catalog.attaches?.connector_type));
    if (!host || (catalog.compatible_with && !catalog.compatible_with.includes(board.id))) {
      issues.push(compIssue("E_COMP_INCOMPATIBLE", `${path}/id`, `${catalog.id} cannot attach to ${board.id}`));
      continue;
    }
    const occupies = catalog.attaches?.occupies ?? "stack";
    if ((occupies === "bottom" && occupied.includes("bottom")) || (occupies === "exclusive" && occupied.length) || (occupied.includes("exclusive"))) {
      issues.push(compIssue("E_COMP_BOTTOM_DUP", path, `${catalog.id} conflicts with another occupied attachment`));
    }
    if (occupies !== "none") occupied.push(occupies);

    const accessory = selectedAccessory(catalog, entry.settings, issues, path);
    const use = entry.use ? new Set(entry.use) : null;
    const mapping = entry.as ?? {};
    for (const [hostRef, row] of Object.entries(accessory.host_pins ?? {})) {
      const [connectorType, position] = hostRef.split(".");
      if (!isCompatible(catalogs.connectorTypes, host[1].type, connectorType)) {
        issues.push(compIssue("E_COMP_INCOMPATIBLE", `${path}/host_pins/${hostRef}`, `${hostRef} does not match host connector`));
        continue;
      }
      const endpoint = host[1].positions?.[position];
      const endpoints = Array.isArray(endpoint) ? endpoint : [endpoint];
      for (const item of endpoints) {
        const gpio = /^gpio:(\d+)$/.exec(item)?.[1];
        if (gpio === undefined) {
          issues.push(compIssue("E_ACC_HOST_POS", `${path}/host_pins/${hostRef}`, `${hostRef} is not a GPIO host position`));
          continue;
        }
        if (!board.pins[gpio]) board.pins[gpio] = { roles: [] };
        for (const role of row.roles ?? []) if (included(role, use)) board.pins[gpio].roles.push(sourceRole(accessory.id, role, mapping));
      }
    }

    for (const group of ["buses", "devices", "connectors"]) for (const [id, object] of Object.entries(accessory[group] ?? {})) {
      if (use && !use.has(id)) continue;
      const mapped = mapping[id] ?? id;
      if (board[group]?.[mapped]) {
        issues.push(compIssue("E_COMP_CONN_DUP", `${path}/${group}/${id}`, `${mapped} already exists`));
        continue;
      }
      if (!board[group]) board[group] = {};
      const added = clone(object);
      added.source = sourceInfo(accessory);
      if (group === "devices") {
        if (added.bus) added.bus = mapping[added.bus] ?? added.bus;
        for (const pin of Object.values(added.pins ?? {})) pin.roles = (pin.roles ?? []).map((role) => sourceRole(accessory.id, role, mapping));
      }
      board[group][mapped] = added;
    }
  }
  return { board, issues };
}

export function validateComposition(board, entries, catalogs) {
  return compose(board, entries, catalogs).issues;
}

export function validateAccessory(accessory, catalogs) {
  const issues = [];
  if (!accessory.origin || !["official", "third_party", "virtual"].includes(accessory.origin)) issues.push(compIssue("E_ACC_FORMAT", "/origin", "origin is required"));
  if (accessory.origin === "third_party" && (!accessory.vendor || !accessory.source_url)) issues.push(compIssue("E_ACC_FORMAT", "/origin", "third-party accessories require vendor and source_url"));
  const type = catalogs.connectorTypes?.[accessory.attaches?.connector_type];
  if (!type) issues.push(compIssue("E_COMP_INCOMPATIBLE", "/attaches/connector_type", "attachment connector type is missing"));
  const positions = new Set((type?.positions ?? []).map((position) => position.id));
  for (const ref of Object.keys(accessory.host_pins ?? {})) {
    const [connector, position] = ref.split(".");
    if (connector !== accessory.attaches?.connector_type || !positions.has(position)) issues.push(compIssue("E_ACC_HOST_POS", `/host_pins/${ref}`, `${ref} is not an attachment position`));
  }
  for (const [id, setting] of Object.entries(accessory.settings ?? {})) {
    if (setting.default !== null && !setting.choices?.[setting.default]) issues.push(compIssue("E_CHOICE_DEFAULT", `/settings/${id}/default`, "default is not a choice"));
    for (const [choice, fragment] of Object.entries(setting.choices ?? {})) for (const position of Object.keys(fragment.host_pins ?? {})) {
      const [connector, connectorPosition] = position.split(".");
      if (connector !== accessory.attaches?.connector_type || !positions.has(connectorPosition)) issues.push(compIssue("E_ACC_HOST_POS", `/settings/${id}/choices/${choice}/host_pins/${position}`, `${position} is not an attachment position`));
      if (accessory.host_pins?.[position]) issues.push(compIssue("E_CHOICE_KEY_OVERLAP", `/settings/${id}/choices/${choice}/host_pins/${position}`, `${position} is also static`));
    }
  }
  return issues;
}

export function renderCompositionDoc(board, composition, accessories) {
  if (!composition?.accessories?.length) return null;
  const lines = [
    `# ${board.official_name} default composition`, "",
    "| Accessory | Origin | Bundled | Settings | Use |", "|---|---|---|---|---|",
  ];
  for (const entry of composition.accessories) {
    const accessory = accessories[entry.id];
    const settings = Object.entries(accessory?.settings ?? {}).map(([id, setting]) => `${id}=${entry.settings?.[id] ?? setting.default ?? "required"}`).join(", ") || "none";
    lines.push(`| ${entry.id} | ${accessory?.origin ?? "missing"} | ${accessory?.bundled === true ? "yes" : "no"} | ${settings} | ${(entry.use ?? ["all"]).join(", ")} |`);
  }
  return `${lines.join("\n")}\n`;
}
