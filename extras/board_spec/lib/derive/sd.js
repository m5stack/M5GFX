function endpointForRole(board, role) {
  for (const [gpio, pin] of Object.entries(board.pins ?? {})) if ((pin.roles ?? []).includes(role)) return `gpio:${gpio}`;
  return null;
}

function fixedSlotMatches(signals, slot, terminals) {
  return terminals.every((terminal) => signals[terminal] === `gpio:${slot.pins?.[terminal]}`);
}

export function deriveSd(board, chip, sdPart) {
  const alias = sdPart?.spi_alias ?? {};
  for (const [id, device] of Object.entries(board.devices ?? {})) {
    if (device.kind !== "sd") continue;
    const signals = device.signals ?? {};
    const modes = [];
    const bus = board.buses?.[device.bus];
    if (bus?.kind === "spi" && ["clk", "cmd", "d0"].every((terminal) => (
      alias[terminal] && signals[terminal] === endpointForRole(board, `bus:${device.bus}.${alias[terminal]}`)
    )) && signals.d3) {
      modes.push("spi");
    }

    const oneBit = ["clk", "cmd", "d0"];
    const fourBit = [...oneBit, "d1", "d2", "d3"];
    const matrix = chip?.sdmmc?.gpio_matrix === true;
    const slots = chip?.sdmmc?.slots ?? [];
    const supports = (terminals) => terminals.every((terminal) => signals[terminal]) && (
      matrix || slots.some((slot) => fixedSlotMatches(signals, slot, terminals))
    );
    if (supports(oneBit)) modes.push("sdio1");
    if (supports(fourBit)) modes.push("sdio4");

    device.derived = { modes };
    if (modes.includes("spi")) {
      device.derived.bus = device.bus;
      device.derived.cs = signals.d3;
    }
  }
  return board;
}
