import { choiceSlots } from "./choices.js";

export function validateTarget(board, target = {}) {
  const issues = [];
  const issue = (id, path, message, severity) => issues.push({ id, path, message, ...(severity ? { severity } : {}) });
  const options = target.options?.[board.id] ?? [];
  const slots = new Map();
  for (const [index, option] of options.entries()) {
    for (const [slot, choice] of Object.entries(option.select ?? {})) {
      const device = board.devices?.[slot];
      if (!device?.choices?.[choice]) issue("E_TGT_SELECT_UNKNOWN", `/targets/options/${board.id}/${index}/select/${slot}`, `${slot}:${choice} is not a board choice`);
      if (slots.has(slot)) issue("E_TGT_SLOT_DUP", `/targets/options/${board.id}/${index}/select/${slot}`, `${slot} is also selected by ${slots.get(slot)}`);
      else slots.set(slot, option.name);
    }
  }
  for (const [slot, device] of choiceSlots(board)) {
    if (device.selected_by === "runtime" && !slots.has(slot)) issue("W_TGT_SLOT_UNOBSERVED", `/devices/${slot}`, `${slot} is runtime-selected outside this target`, "warning");
    if (device.selected_by === "revision" && Object.keys(device.choices).length > 1 && !slots.has(slot)) {
      issue("W_TGT_REV_INDISTINGUISHABLE", `/devices/${slot}`, `${slot} revision choices are not represented by target options`, "warning");
    }
  }
  return issues;
}

export function revisionOptions(board, revision, target = {}) {
  const explicit = revision?.select ?? {};
  return (target.options?.[board.id] ?? []).filter((option) => {
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
