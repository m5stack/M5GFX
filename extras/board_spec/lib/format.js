import { clone, isPlainObject } from "./model.js";
import { isGeneratedField } from "./pintable_roles.js";

const TOP_LEVEL_ORDER = ["schema_version", "id", "name", "legacy_board_id", "chip", "spec", "pins", "buses", "devices", "connectors", "revisions"];
const PRIORITY_KEYS = ["id", "kind", "type", "name"];

function orderedObject(value, priority = PRIORITY_KEYS) {
  const output = {};
  for (const key of priority) if (Object.prototype.hasOwnProperty.call(value, key)) output[key] = value[key];
  for (const [key, item] of Object.entries(value)) if (!priority.includes(key)) output[key] = item;
  return output;
}

function markGeneratedObject(board, value, path) {
  if (!isPlainObject(value)) return;
  const verified = isPlainObject(value.verified) ? clone(value.verified) : {};
  for (const field of Object.keys(value)) {
    if (!["verified", "measured_by"].includes(field) && isGeneratedField(board, path, field)) verified[field] = "generated";
  }
  if (Object.keys(verified).length) value.verified = verified;
}

function normalize(value, path = "") {
  if (Array.isArray(value)) return value.map((item, index) => normalize(item, `${path}/${index}`));
  if (!isPlainObject(value)) return value;

  let source = orderedObject(value);
  if (path === "") source = orderedObject(value, TOP_LEVEL_ORDER);
  if (path === "/pins") {
    source = Object.fromEntries(Object.entries(value).sort(([a], [b]) => Number(a) - Number(b)));
  }
  if (/^\/devices\/[^/]+\/pins$/.test(path)) source = orderedObject(value, []);

  const output = {};
  for (const [key, item] of Object.entries(source)) output[key] = normalize(item, `${path}/${key}`);
  return output;
}

export function formatBoardObject(board) {
  const output = clone(board);
  for (const [gpio, pin] of Object.entries(output.pins ?? {})) markGeneratedObject(output, pin, `/pins/${gpio}`);
  for (const [id, bus] of Object.entries(output.buses ?? {})) markGeneratedObject(output, bus, `/buses/${id}`);
  for (const [id, device] of Object.entries(output.devices ?? {})) markGeneratedObject(output, device, `/devices/${id}`);
  return normalize(output);
}

export function formatBoard(board) {
  return `${JSON.stringify(formatBoardObject(board), null, 2)}\n`;
}

export function checkFormatIdempotent(board, formatter = formatBoard) {
  const once = formatter(board);
  let parsed;
  try {
    parsed = JSON.parse(once);
  } catch {
    return false;
  }
  return formatter(parsed) === once;
}
