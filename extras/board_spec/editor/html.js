export function escapeHtml(value) {
  return String(value ?? "")
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#39;");
}

export function gpioTargetButton(gpio) {
  const safe = escapeHtml(gpio);
  return `<button type="button" data-target-gpio="${safe}">GPIO ${safe}</button>`;
}

export function ownerSignalsHtml(device) {
  return Object.entries(device?.signals ?? {})
    .map(([signal, endpoint]) => `${escapeHtml(signal)} → ${escapeHtml(endpoint)}`)
    .join(" · ") || "—";
}

export function externalLinkHtml(url, label) {
  try {
    const parsed = new URL(url);
    if (!["https:", "http:"].includes(parsed.protocol)) return "";
    return `<a href="${escapeHtml(parsed.href)}" target="_blank" rel="noopener noreferrer">${escapeHtml(label)}</a>`;
  } catch { return ""; }
}

export function gpioConnectorTargets(board, gpio) {
  const targets = new Map();
  for (const role of board.pins?.[gpio]?.roles ?? []) {
    const match = /(?:^|\/)conn:([^.]+)\.(.+)$/.exec(role);
    if (match && board.connectors?.[match[1]]) targets.set(`${match[1]}.${match[2]}`, { connector: match[1], position: match[2] });
  }
  return [...targets.values()];
}

export function resolveEditorRoute(target, board, context = {}) {
  const kinds = ["board", "gpio", "connectors", "connector", "buses", "devices", "accessories", "revisions", "owners", "validation"];
  const route = { kind: kinds.includes(target?.kind) ? target.kind : "gpio" };
  let id = target?.id == null ? null : String(target.id);
  if (target?.kind === "connector-group") {
    id = Object.keys(board.connectors ?? {}).find((key) => board.connectors[key].group === id);
    route.kind = "connector";
  }
  if (route.kind === "connector") {
    if (!id || !board.connectors?.[id]) return { kind: "connectors" };
    route.id = id;
    if (board.connectors[id].group) route.group = board.connectors[id].group;
    const type = context.connectorTypes?.[board.connectors[id].type];
    if (target.position != null && type?.positions?.some((position) => position.id === String(target.position))) route.position = String(target.position);
  } else if (["buses", "devices"].includes(route.kind) && id && board[route.kind]?.[id]) route.id = id;
  else if (route.kind === "revisions" && id && board.revisions?.some((revision) => revision.id === id)) route.id = id;
  else if (route.kind === "owners" && id && (context.ownerKeys ?? Object.keys(board.devices ?? {})).includes(id)) route.id = id;
  if (route.kind === "gpio" && target?.gpio != null) {
    const gpio = Number(target.gpio);
    const count = context.chip?.gpio_count;
    if (Number.isInteger(gpio) && gpio >= 0 && (count === undefined || gpio < count) && !context.chip?.absent?.includes(gpio)) route.gpio = String(gpio);
  }
  return route;
}

export function editorErrorRoute(item, board) {
  const path = String(item.path ?? "").split("/").slice(1).map((part) => part.replaceAll("~1", "/").replaceAll("~0", "~"));
  if (path[0] === "pins") return { kind: "gpio", gpio: path[1] };
  if (path[0] === "connectors") return path[1] ? { kind: "connector", id: path[1], ...(path[2] === "positions" ? { position: path[3] } : {}) } : { kind: "connectors" };
  if (["buses", "devices"].includes(path[0])) return { kind: path[0], id: path[1] };
  if (path[0] === "revisions") return { kind: "revisions", id: board.revisions?.[Number(path[1])]?.id };
  return { kind: "board" };
}


export function connectorDisplaySets(board) {
  const sets = new Map();
  for (const [id, connector] of Object.entries(board.connectors ?? {})) {
    const key = connector.group ? `group:${connector.group}` : `connector:${id}`;
    if (!sets.has(key)) sets.set(key, { group: connector.group ?? null, ids: [] });
    sets.get(key).ids.push(id);
  }
  return [...sets.values()];
}

export function editorPageKey(route) {
  return `${route.kind}:${route.group ? `group:${route.group}` : route.kind === "owners" ? "" : route.id ?? ""}`;
}

export function firstConnectorPosition(type) {
  return type?.positions?.find((position) => position.id === "1")?.id ?? type?.positions?.[0]?.id ?? "1";
}

export function connectorTableColumns(type) {
  const wire = type.positions.some((position) => position.color) ? ["wire"] : [];
  return type.layout.cols === 2
    ? ["function", "endpoint", ...wire, "terminal", "terminal", ...wire, "endpoint", "function"]
    : ["terminal", ...wire, "endpoint", "function"];
}
