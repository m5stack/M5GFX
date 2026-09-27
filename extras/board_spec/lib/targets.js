import { choiceSlots } from "./choices.js";

const WIRING_FIELDS = new Set(["display", "shared_sd", "i2c", "power", "backlight", "hold"]);
const CPP_IDENTIFIER = /^[A-Za-z_][A-Za-z0-9_]*$/;
const OUTPUT_FILENAME = /^[A-Za-z0-9_][A-Za-z0-9_.-]*\.hpp$/;

export function targetBoard(target, boardId) {
  return target.boards?.[boardId] ?? null;
}

export function targetOptions(target, boardId) {
  return targetBoard(target, boardId)?.options ?? [];
}

export function validateTarget(board, target = {}) {
  const issues = [];
  const issue = (id, path, message, severity) => issues.push({ id, path, message, ...(severity ? { severity } : {}) });
  const boardTarget = targetBoard(target, board.id);
  const boardPath = `/targets/boards/${board.id}`;
  if (!boardTarget) {
    issue("E_TGT_BOARD_MISSING", boardPath, "board has no target metadata");
    return issues;
  }
  if (boardTarget.chip !== board.chip) issue("E_TGT_CHIP", `${boardPath}/chip`, `${boardTarget.chip} does not match ${board.chip}`);
  if (boardTarget.legacy_board_id !== board.legacy_board_id) issue("E_TGT_LEGACY_ID", `${boardPath}/legacy_board_id`, `${boardTarget.legacy_board_id} does not match ${board.legacy_board_id}`);
  if (typeof boardTarget.board_enum !== "string" || !/^board_[A-Za-z][A-Za-z0-9_]*$/.test(boardTarget.board_enum)) {
    issue("E_TGT_BOARD_ENUM", `${boardPath}/board_enum`, "board_enum must be a board_ C++ identifier");
  }
  for (const field of ["cpp_namespace", "desc_name"]) if (boardTarget[field] !== undefined && !CPP_IDENTIFIER.test(boardTarget[field])) {
    issue("E_TGT_CPP_IDENTIFIER", `${boardPath}/${field}`, `${field} must be a C++ identifier`);
  }
  if (!Array.isArray(boardTarget.wiring_fields ?? [])) issue("E_TGT_WIRING_FIELD", `${boardPath}/wiring_fields`, "wiring_fields must be an array");
  else {
    const seenFields = new Set();
    for (const field of boardTarget.wiring_fields ?? []) {
      if (!WIRING_FIELDS.has(field) || seenFields.has(field)) issue("E_TGT_WIRING_FIELD", `${boardPath}/wiring_fields`, `${field} is unknown or duplicated`);
      seenFields.add(field);
    }
  }
  for (const field of ["wiring_output", "specs_output"]) if (boardTarget[field] !== undefined && !OUTPUT_FILENAME.test(boardTarget[field])) {
    issue("E_TGT_OUTPUT", `${boardPath}/${field}`, `${field} must be a safe .hpp filename`);
  }
  if (boardTarget.wiring_output && (!boardTarget.cpp_namespace || !boardTarget.desc_name)) {
    issue("E_TGT_GENERATION_FIELDS", boardPath, "generated wiring requires cpp_namespace and desc_name");
  }
  const options = targetOptions(target, board.id);
  const slots = new Set();
  const selections = new Map();
  const names = new Set();
  const bits = new Map();
  for (const [index, option] of options.entries()) {
    const path = `/targets/boards/${board.id}/options/${index}`;
    if (typeof option.name !== "string" || !/^[a-z][a-z0-9_]*$/.test(option.name) || names.has(option.name)) {
      issue("E_TGT_OPTION_NAME", `${path}/name`, "option name must be unique snake_case ASCII");
    }
    names.add(option.name);
    if (!Number.isInteger(option.bit) || option.bit < 0 || option.bit > 31) {
      issue("E_TGT_BIT_RANGE", `${path}/bit`, "option bit must be an integer from 0 to 31");
    } else if (bits.has(option.bit)) {
      issue("E_TGT_BIT_DUP", `${path}/bit`, `bit ${option.bit} is also used by ${bits.get(option.bit)}`);
    } else bits.set(option.bit, option.name);
    for (const [slot, choice] of Object.entries(option.select ?? {})) {
      const device = board.devices?.[slot];
      if (!device?.choices?.[choice]) issue("E_TGT_SELECT_UNKNOWN", `${path}/select/${slot}`, `${slot}:${choice} is not a board choice`);
      const selection = `${slot}\0${choice}`;
      if (selections.has(selection)) issue("E_TGT_SLOT_DUP", `${path}/select/${slot}`, `${slot}:${choice} is also selected by ${selections.get(selection)}`);
      else selections.set(selection, option.name);
      slots.add(slot);
    }
  }
  if (bits.size && [...bits.keys()].some((bit) => bit >= bits.size)) {
    issue("E_TGT_BIT_GAP", `/targets/boards/${board.id}/options`, "option bits must be contiguous from zero");
  }
  for (const [slot, device] of choiceSlots(board)) {
    if (device.selected_by === "runtime") {
      const represented = new Set(options.flatMap((option) => Object.entries(option.select ?? {}))
        .filter(([selectedSlot]) => selectedSlot === slot).map(([, choice]) => choice));
      for (const choice of Object.keys(device.choices).filter((choice) => choice !== device.default && !represented.has(choice))) {
        issue("E_TGT_CHOICE_UNREPRESENTED", `/devices/${slot}/choices/${choice}`, `${slot}:${choice} has no target option`);
      }
    }
    if (device.selected_by === "revision" && Object.keys(device.choices).length > 1 && !slots.has(slot)) {
      issue("W_TGT_REV_INDISTINGUISHABLE", `/devices/${slot}`, `${slot} revision choices are not represented by target options`, "warning");
    }
  }
  return issues;
}

export function validateTargets(boards, target = {}) {
  const issues = [];
  const catalog = new Map(boards.map((board) => [board.id, board]));
  const enums = new Map();
  const namespaces = new Map();
  for (const [id, entry] of Object.entries(target.boards ?? {})) {
    const path = `/targets/boards/${id}`;
    if (!catalog.has(id)) issues.push({ id: "E_TGT_BOARD_UNKNOWN", path, message: `${id} is not in the board catalog` });
    for (const [field, seen] of [["board_enum", enums], ["cpp_namespace", namespaces]]) {
      const value = entry[field];
      if (!value) continue;
      if (seen.has(value)) issues.push({ id: field === "board_enum" ? "E_TGT_BOARD_ENUM_DUP" : "E_TGT_NAMESPACE_DUP", path: `${path}/${field}`, message: `${value} is also used by ${seen.get(value)}` });
      else seen.set(value, id);
    }
  }
  for (const board of boards) issues.push(...validateTarget(board, target));
  return issues;
}

export function revisionOptions(board, revision, target = {}) {
  const explicit = revision?.select ?? {};
  return targetOptions(target, board.id).filter((option) => {
    const selections = Object.entries(option.select ?? {});
    return selections.length > 0 && selections.every(([slot, choice]) => (explicit[slot] ?? board.devices?.[slot]?.default) === choice);
  }).map((option) => option.name);
}

export function renderRevisionDoc(board, target = {}) {
  if (!board.revisions?.length) return null;
  const lines = [
    `# ${board.name} revisions`,
    "",
    "| Revision | Choices | M5GFX options | Note |",
    "|---|---|---|---|",
  ];
  for (const revision of board.revisions) {
    const choices = Object.entries(revision.select ?? {}).map(([slot, choice]) => `${slot}=${choice}`).join(", ") || "defaults";
    const options = revisionOptions(board, revision, target).join(", ") || "none";
    lines.push(`| ${revision.name} | ${choices} | ${options} | ${revision.note ?? ""} |`);
  }
  return `${lines.join("\n")}\n`;
}
