# Board specification catalog

This dependency-free prototype keeps board wiring, revision choices, and part
facts in reviewable JSON. It is intentionally outside M5GFX's build inputs.

- `boards/`, `parts/`, `chips/`, `schema/`: source data and its declaration
- `targets.json`: generator-specific option-bit to board-choice mappings
- `lib/`, `cli/`: browser-safe model logic and the Node command wrapper
- `generated/resolved/`: fully resolved revision/runtime combinations
- `editor/`, `build/`: editor sources and the dependency-free bundler
- `dist/board_spec_editor.html`: standalone editor; open it directly with `file://`

```sh
node cli/spec.js validate boards/m5stack_core2.json
node cli/spec.js resolve boards/m5stack_core2.json
node build/bundle.js
node cli/spec.js check
```

Boards use `devices.<id>.choices` for replaceable parts and `revisions[]` for
known product revisions. Runtime-selected slots are expanded as additional
resolved combinations. Device `signals` and `enable` are derived from `dev:`
roles across the SoC and device-owned pin tables; they are not written in board
sources. `gpio:N` is the short form of `pin:soc.N`.

SD wiring uses card terminal names (`clk`, `cmd`, `d0` through `d3`). The
`sd_slot` part defines their SPI aliases, and resolved boards report usable
`spi`, `sdio1`, and `sdio4` modes in `devices.sd.derived`.

M5GFX board-detection wiring is generated from the base board only. Accessory
and composition wiring does not participate in board descriptor generation.
Generated headers represent unspecified values with target-specific unknown or
sentinel values. Value precedence is board value, then part default, then the
panel-class default.

The editor includes all catalog boards, parts, and chip data. It can also open or accept
dragged board JSON files, keeps best-effort drafts in `localStorage`, and
downloads canonical formatted JSON. Selecting a revision shows the resolved
board read-only; clear the revision selection to resume editing the base board.
