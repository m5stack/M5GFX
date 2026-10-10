// Detector membership is declared separately from board wiring. Priority is
// derived only from the GPIO power-hold device and role in the board catalog.
export function hasGpioPowerHold(board) {
  const holdIds = Object.entries(board.devices ?? {})
    .filter(([, device]) => device.kind === "power_hold")
    .map(([id]) => id);
  return holdIds.some((id) => Object.values(board.pins ?? {}).some((pin) =>
    (pin.roles ?? []).some((role) => role.startsWith(`dev:${id}.`))));
}

export function detectorPriority(boards, groups) {
  const byId = new Map(boards.map((board) => [board.id, board]));
  const result = new Map();
  for (const [name, group] of Object.entries(groups)) {
    if (!/^[a-z][a-z0-9_]*$/.test(name) || !/^[a-z][a-z0-9_]*\.hpp$/.test(group.output)) {
      throw new Error(`invalid detector-order target ${name}`);
    }
    const seen = new Set();
    const families = group.families.map(({ detector, boards: members }, index) => {
      if (!/^[a-z][a-z0-9_]*$/.test(detector) || seen.has(detector) || !members?.length) {
        throw new Error(`${name}: invalid or repeated detector ${detector}`);
      }
      seen.add(detector);
      const sources = members.map((id) => {
        const board = byId.get(id);
        if (!board || board.chip !== group.chip) throw new Error(`${name}: invalid member ${id}`);
        return board;
      });
      return { detector, hold: sources.some(hasGpioPowerHold), index, id: sources[0].legacy_board_id };
    });
    const byDetector = new Map(families.map((family) => [family.detector, family]));
    const edges = [];
    for (const constraint of group.constraints ?? []) {
      const { before, after, reason } = constraint;
      if (!byDetector.has(before) || !byDetector.has(after) || before === after ||
          typeof reason !== "string" || !reason.trim() ||
          edges.some((edge) => edge.before === before && edge.after === after)) {
        throw new Error(`${name}: invalid detector-order constraint ${before} -> ${after}`);
      }
      edges.push({ before, after });
    }
    const pending = new Set(families.map((family) => family.detector));
    const ordered = [];
    while (pending.size) {
      const ready = families.filter((family) => pending.has(family.detector) &&
        !edges.some((edge) => edge.after === family.detector && pending.has(edge.before)));
      if (!ready.length) throw new Error(`${name}: cyclic detector-order constraints`);
      ready.sort((a, b) => Number(b.hold) - Number(a.hold) || a.index - b.index);
      ordered.push(ready[0].detector);
      pending.delete(ready[0].detector);
    }
    result.set(name, {
      output: group.output,
      ordered,
      holdCount: families.filter((family) => family.hold).length,
      edges: edges.map(({ before, after }) => ({ before: byDetector.get(before).id, after: byDetector.get(after).id })),
    });
  }
  return result;
}

export function renderDetectorOrderHeaders(boards, groups) {
  const outputs = new Map();
  const allEdges = [];
  let maxFamilies = 0;
  for (const [name, group] of detectorPriority(boards, groups)) {
    allEdges.push(...group.edges);
    maxFamilies = Math.max(maxFamilies, group.ordered.length);
    const array = `static const board_detector_t* const ${name}[] = {\n`
      + group.ordered.map((detector) => `  &${detector},`).join("\n")
      + "\n  nullptr,\n};\n"
      + `static_assert(sizeof(${name}) / sizeof(${name}[0]) - 1 <= max_detector_families,\n`
      + `              "${name} exceeds the detection session family limit");\n`;
    outputs.set(group.output, (outputs.get(group.output) ?? "// Generated from detector_order.json and board catalog.\n#pragma once\n\n") + array + "\n");
  }
  const ids = boards.filter(hasGpioPowerHold).map((board) => board.legacy_board_id).sort((a, b) => a - b);
  outputs.set("gpio_power_hold_board_ids.hpp",
    "// Generated from GPIO power-hold devices and pin roles in the board catalog.\n#pragma once\n\n"
    + "namespace m5gfx { namespace board_detect {\n"
    + "static constexpr board_id_t gpio_power_hold_board_ids[] = {\n"
    + ids.map((id) => `  ${id},`).join("\n") + "\n};\n"
    + "} } // namespace m5gfx::board_detect\n");
  outputs.set("detector_order_constraints.hpp",
    "// Generated from detector_order.json and board catalog.\n#pragma once\n\n"
    + "namespace m5gfx { namespace board_detect {\n"
    + `static constexpr unsigned max_detector_families = ${maxFamilies};\n`
    + "struct detector_order_edge_t { board_id_t before; board_id_t after; };\n"
    + "static constexpr detector_order_edge_t detector_order_edges[] = {\n"
    + allEdges.map(({ before, after }) => `  { ${before}, ${after} },`).join("\n") + "\n};\n"
    + "} } // namespace m5gfx::board_detect\n");
  return outputs;
}
