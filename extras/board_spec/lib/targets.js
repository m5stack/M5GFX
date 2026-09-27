import { choiceSlots } from "./choices.js";

const WIRING_FIELDS = new Set(["display", "shared_sd", "i2c", "power", "backlight", "touch", "camera", "hold"]);
const CPP_IDENTIFIER = /^[A-Za-z_][A-Za-z0-9_]*$/;
const CPP_KEYWORDS = new Set((
  "alignas alignof and and_eq asm auto bitand bitor bool break "
  + "case catch char char8_t char16_t char32_t class compl concept const consteval constexpr constinit const_cast "
  + "continue co_await co_return co_yield decltype default delete do double dynamic_cast else enum explicit export "
  + "extern false float for friend goto if import inline int long module mutable namespace new noexcept not not_eq "
  + "nullptr operator or or_eq private protected public register reinterpret_cast requires return short signed "
  + "sizeof static static_assert static_cast struct switch template this thread_local throw true try typedef typeid "
  + "typename union unsigned using virtual void volatile wchar_t while xor xor_eq"
).split(" "));
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
  for (const field of ["cpp_namespace", "desc_name"]) if (boardTarget[field] !== undefined) {
    if (!CPP_IDENTIFIER.test(boardTarget[field])) {
      issue("E_TGT_CPP_IDENTIFIER", `${boardPath}/${field}`, `${field} must be a C++ identifier`);
    } else if (CPP_KEYWORDS.has(boardTarget[field])) {
      issue("E_TGT_CPP_KEYWORD", `${boardPath}/${field}`, `${field} must not be a C++ keyword`);
    }
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
  if (boardTarget.wiring_output && boardTarget.wiring_output === boardTarget.specs_output) {
    issue("E_TGT_OUTPUT_COLLISION", `${boardPath}/specs_output`, "wiring_output and specs_output must not use the same file");
  }
  if (boardTarget.reset !== undefined && boardTarget.reset !== "display_rst") {
    issue("E_TGT_RESET", `${boardPath}/reset`, `${boardTarget.reset} is not a supported reset source`);
  }
  if (boardTarget.desc_internal_i2c !== undefined && typeof boardTarget.desc_internal_i2c !== "boolean") {
    issue("E_TGT_DESC_INTERNAL_I2C", `${boardPath}/desc_internal_i2c`, "desc_internal_i2c must be a boolean");
  }
  if (boardTarget.desc_internal_i2c && !(boardTarget.wiring_fields ?? []).includes("i2c")) {
    issue("E_TGT_DESC_INTERNAL_I2C", `${boardPath}/desc_internal_i2c`, "desc_internal_i2c requires the i2c wiring field");
  }
  if (boardTarget.wiring_output && (!boardTarget.cpp_namespace || !boardTarget.desc_name)) {
    issue("E_TGT_GENERATION_FIELDS", boardPath, "generated wiring requires cpp_namespace and desc_name");
  }
  if (boardTarget.cardputer_subdivision !== undefined) {
    const subdivision = boardTarget.cardputer_subdivision;
    if (!subdivision || typeof subdivision !== "object" || Array.isArray(subdivision)
     || !Array.isArray(subdivision.sense_i2c_from_boards) || !subdivision.sense_i2c_from_boards.length
     || subdivision.sense_i2c_from_boards.some((id) => typeof id !== "string")
     || typeof subdivision.vameter_board !== "string"
     || !Array.isArray(subdivision.vameter_i2c_devices) || !subdivision.vameter_i2c_devices.length
     || subdivision.vameter_i2c_devices.some((id) => typeof id !== "string")) {
      issue("E_TGT_CARDPUTER_SUBDIVISION", `${boardPath}/cardputer_subdivision`,
            "Cardputer subdivision needs sense boards, a VAMeter board, and named I2C devices");
    }
  }
  if (boardTarget.identity_devices !== undefined) {
    if (!Array.isArray(boardTarget.identity_devices)
     || boardTarget.identity_devices.some((id) => typeof id !== "string" || !board.devices?.[id])) {
      issue("E_TGT_IDENTITY_DEVICE", `${boardPath}/identity_devices`,
            "identity_devices must name devices present on the board");
    }
  }
  if (boardTarget.release_probe !== undefined) {
    const probe = boardTarget.release_probe;
    const validRole = (role) => typeof role === "string" && /^dev:[a-z][a-z0-9_]*\.[a-z0-9_]+$/.test(role);
    const boardRoles = new Set(Object.values(board.pins ?? {}).flatMap((row) => row.roles ?? []));
    const rolePins = new Map(Object.entries(board.pins ?? {}).flatMap(([pin, row]) =>
      (row.roles ?? []).map((role) => [role, Number(pin)])));
    if (!probe || typeof probe !== "object" || Array.isArray(probe)
     || !Array.isArray(probe.pins) || probe.pins.length !== 8
     || new Set(probe.pins).size !== probe.pins.length
     || probe.pins.some((role) => !validRole(role) || !boardRoles.has(role))
     || probe.pins.some((role) => !Number.isInteger(rolePins.get(role))
                                  || rolePins.get(role) < 0 || rolePins.get(role) > 48
                                  || (rolePins.get(role) >= 22 && rolePins.get(role) <= 25))
     || !Number.isInteger(probe.reads) || probe.reads < 1 || probe.reads > 256
     || !Number.isInteger(probe.samples) || probe.samples < 1 || probe.samples > 15
     || !Number.isInteger(probe.settle_us) || probe.settle_us < 1
     || !Number.isInteger(probe.short_max_ns) || probe.short_max_ns < 0
     || !Number.isInteger(probe.long_min_ns)
     || probe.long_min_ns <= probe.short_max_ns) {
      issue("E_TGT_RELEASE_PROBE", `${boardPath}/release_probe`,
            "release_probe needs exactly 8 unique valid ESP32-S3 GPIO roles, bounded reads/samples, settle_us, and valid ns thresholds");
    }
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
    } else if (CPP_KEYWORDS.has(option.name)) {
      issue("E_TGT_CPP_KEYWORD", `${path}/name`, "option name must not be a C++ keyword");
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
  const outputs = new Map();
  for (const [id, entry] of Object.entries(target.boards ?? {})) {
    const path = `/targets/boards/${id}`;
    if (!catalog.has(id)) issues.push({ id: "E_TGT_BOARD_UNKNOWN", path, message: `${id} is not in the board catalog` });
    for (const [field, seen] of [["board_enum", enums], ["cpp_namespace", namespaces]]) {
      const value = entry[field];
      if (!value) continue;
      if (seen.has(value)) issues.push({ id: field === "board_enum" ? "E_TGT_BOARD_ENUM_DUP" : "E_TGT_NAMESPACE_DUP", path: `${path}/${field}`, message: `${value} is also used by ${seen.get(value)}` });
      else seen.set(value, id);
    }
    for (const field of ["wiring_output", "specs_output"]) {
      const value = entry[field];
      if (!value) continue;
      const prior = outputs.get(value);
      if (prior && (prior.field !== field || prior.chip !== entry.chip)) {
        issues.push({ id: "E_TGT_OUTPUT_COLLISION", path: `${path}/${field}`, message: `${value} is also ${prior.field} for ${prior.id} (${prior.chip})` });
      } else if (!prior) outputs.set(value, { field, id, chip: entry.chip });
    }
    const subdivision = entry.cardputer_subdivision;
    for (const sourceId of [...(subdivision?.sense_i2c_from_boards ?? []), subdivision?.vameter_board].filter(Boolean)) {
      if (!catalog.has(sourceId)) issues.push({ id: "E_TGT_CARDPUTER_SUBDIVISION", path: `${path}/cardputer_subdivision`, message: `${sourceId} is not in the board catalog` });
    }
    const vameter = catalog.get(subdivision?.vameter_board);
    for (const deviceId of subdivision?.vameter_i2c_devices ?? []) {
      if (!vameter?.devices?.[deviceId]) issues.push({ id: "E_TGT_CARDPUTER_SUBDIVISION", path: `${path}/cardputer_subdivision/vameter_i2c_devices`, message: `${deviceId} is not on ${subdivision.vameter_board}` });
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
