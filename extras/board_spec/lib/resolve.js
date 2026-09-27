import { clone } from "./model.js";
import { materializeChoices, revisionSelection, runtimeCombinations } from "./choices.js";
import { deriveDeviceEndpoints, ownerRoleEntries } from "./owners.js";
import { deriveSd } from "./derive/sd.js";
import { resolveConnectorType } from "./ctypes.js";
import { compose } from "./compose.js";

export function connectorRoleGPIOs(board, connectorId) {
  const result = new Map();
  for (const entry of ownerRoleEntries(board)) {
    if (entry.owner !== "soc" || entry.parsed?.type !== "conn" || entry.parsed.id !== connectorId) continue;
    if (!result.has(entry.parsed.signal)) result.set(entry.parsed.signal, []);
    result.get(entry.parsed.signal).push(Number(entry.pin));
  }
  for (const gpios of result.values()) gpios.sort((a, b) => a - b);
  return result;
}

function connectorRoleEndpoints(board, connectorId) {
  const result = new Map();
  for (const entry of ownerRoleEntries(board)) {
    if (entry.parsed?.type !== "conn" || entry.parsed.id !== connectorId) continue;
    if (!result.has(entry.parsed.signal)) result.set(entry.parsed.signal, []);
    result.get(entry.parsed.signal).push(entry.endpoint);
  }
  return result;
}

export function resolveConnectorPositions(board, connectorId, connectorTypes) {
  const connector = board.connectors?.[connectorId];
  const type = resolveConnectorType(connectorTypes, connector?.type);
  if (!connector || !type) return { positions: clone(connector?.positions ?? {}), sources: {}, type };
  const endpoints = connectorRoleEndpoints(board, connectorId);
  const positions = {};
  const sources = {};
  for (const position of type.positions) {
    const id = position.id;
    sources[id] = {
      fixed: Object.hasOwn(type.fixed_positions ?? {}, id),
      default: Object.hasOwn(type.default_positions ?? {}, id),
      explicit: Object.hasOwn(connector.positions ?? {}, id),
      endpoints: endpoints.get(id) ?? [],
      gpios: (endpoints.get(id) ?? []).flatMap((endpoint) => /^gpio:(\d+)$/.exec(endpoint)?.[1] ?? []).map(Number),
    };
    if (sources[id].fixed) positions[id] = clone(type.fixed_positions[id]);
    else if (sources[id].default) positions[id] = clone(type.default_positions[id]);
    if (sources[id].explicit) positions[id] = clone(connector.positions[id]);
    if (sources[id].endpoints.length === 1) positions[id] = sources[id].endpoints[0];
    else if (sources[id].endpoints.length > 1) positions[id] = sources[id].endpoints;
  }
  return { positions, sources, type };
}

function finishResolved(board, connectorTypes, catalogs) {
  deriveDeviceEndpoints(board);
  deriveSd(board, catalogs.chip, catalogs.parts?.sd_slot);
  for (const [id, connector] of Object.entries(board.connectors ?? {})) {
    const { positions, type } = resolveConnectorPositions(board, id, connectorTypes);
    if (type) connector.positions = positions;
  }
  return board;
}

export function resolveBoard(board, selection = {}, connectorTypes = {}, catalogs = {}) {
  let resolved = finishResolved(materializeChoices(board, selection), connectorTypes, catalogs);
  if (catalogs.composition) {
    const result = compose(resolved, catalogs.composition.accessories ?? [], { ...catalogs, connectorTypes });
    resolved = finishResolved(result.board, connectorTypes, catalogs);
    Object.defineProperty(resolved, "__compositionIssues", { value: result.issues, enumerable: false });
  }
  return resolved;
}

export function resolvedFilename(board, revision, runtime = {}) {
  const requireId = (value, label) => {
    if (typeof value !== "string" || !/^[a-z][a-z0-9_]*$/.test(value)) throw new Error(`${label} is not a safe ID`);
  };
  requireId(board.id, "board ID");
  if (revision) requireId(revision.id, "revision ID");
  for (const [slot, choice] of Object.entries(runtime)) {
    requireId(slot, "runtime slot");
    requireId(choice, "runtime choice");
  }
  const revisionPart = revision ? `@${revision.id}` : "";
  const runtimePart = Object.entries(runtime).map(([slot, choice]) => `+${slot}=${choice}`).join("");
  return `${board.id}${revisionPart}${runtimePart}.json`;
}

export function resolveAll(board, connectorTypes = {}, catalogs = {}) {
  const revisions = board.revisions?.length ? board.revisions : [null];
  const runtimes = runtimeCombinations(board);
  return revisions.flatMap((revision) => runtimes.map((runtime) => {
    const selection = { ...revisionSelection(board, revision), ...runtime };
    return {
      filename: resolvedFilename(board, revision, runtime),
      revision: revision?.id ?? null,
      runtime,
      selection,
      board: resolveBoard(board, selection, connectorTypes, catalogs),
    };
  }));
}
