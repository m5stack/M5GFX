import { clone, isPlainObject } from "./model.js";

const choiceOwn = (value, key) => Object.prototype.hasOwnProperty.call(value, key);

export function choiceSlots(board, selectedBy = null) {
  return Object.entries(board.devices ?? {}).filter(([, device]) => (
    isPlainObject(device.choices) && (selectedBy === null || device.selected_by === selectedBy)
  ));
}

export function revisionSelection(board, revision = null) {
  const selected = {};
  for (const [id, device] of choiceSlots(board)) selected[id] = device.default;
  for (const [id, choice] of Object.entries(revision?.select ?? {})) selected[id] = choice;
  return selected;
}

export function mergeChoice(device, choiceId) {
  const fragment = device.choices?.[choiceId] ?? {};
  const base = clone(device);
  delete base.choices;
  delete base.selected_by;
  delete base.default;
  return { ...base, ...clone(fragment) };
}

export function materializeChoices(board, selection = {}) {
  const resolved = clone(board);
  for (const [id, device] of Object.entries(resolved.devices ?? {})) {
    if (!isPlainObject(device.choices)) continue;
    resolved.devices[id] = mergeChoice(device, selection[id] ?? device.default);
  }
  delete resolved.revisions;
  return resolved;
}

export function runtimeCombinations(board) {
  let combinations = [{}];
  for (const [id, device] of choiceSlots(board, "runtime")) {
    combinations = combinations.flatMap((selection) => Object.keys(device.choices).map((choice) => ({ ...selection, [id]: choice })));
  }
  return combinations;
}

export function validateChoices(board) {
  const issues = [];
  const issue = (id, path, message, severity) => issues.push({ id, path, message, ...(severity ? { severity } : {}) });
  for (const [id, device] of choiceSlots(board)) {
    const path = `/devices/${id}`;
    const choices = Object.keys(device.choices);
    if (!choices.includes(device.default)) issue("E_CHOICE_DEFAULT", `${path}/default`, `default ${device.default ?? "<missing>"} is not a choice`);
    if (choices.length === 1) issue("W_CHOICE_SINGLE", `${path}/choices`, "a single choice does not need a choice slot", "warning");
    for (const [choiceId, fragment] of Object.entries(device.choices)) {
      for (const key of Object.keys(fragment)) if (choiceOwn(device, key) && !["choices", "selected_by", "default"].includes(key)) {
        issue("E_CHOICE_KEY_OVERLAP", `${path}/choices/${choiceId}/${key}`, `${key} is also present on the device`);
      }
      if (choiceOwn(fragment, "soc_pins")) issue("E_UNKNOWN_KEY", `${path}/choices/${choiceId}/soc_pins`, "choice fragments cannot change SoC pins");
    }
  }
  const revisionIds = new Set();
  for (const [index, revision] of (board.revisions ?? []).entries()) {
    const path = `/revisions/${index}`;
    if (revisionIds.has(revision.id)) issue("E_ID_FORMAT", `${path}/id`, `duplicate revision ${revision.id}`);
    revisionIds.add(revision.id);
    for (const [slot, choice] of Object.entries(revision.select ?? {})) {
      const device = board.devices?.[slot];
      if (!device?.choices || !choiceOwn(device.choices, choice)) issue("E_REV_SELECT_UNKNOWN", `${path}/select/${slot}`, `${slot}:${choice} is not a known choice`);
      else if (device.selected_by === "runtime") issue("E_REV_SELECT_RUNTIME", `${path}/select/${slot}`, `${slot} is selected at runtime`);
    }
  }
  return issues;
}
