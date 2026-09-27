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

## Board-detection operation lists

`src/board_detect/ops.hpp` defines a typed source IR for ordered I2C, GPIO, and
delay operations. An I2C mask names the bits to change; masked writes compute
`(old & ~mask) | (value & mask)`. Execution validates the complete list before
the first side effect and then stops at the first failed operation. Fixed waits
are explicit operations. Every GPIO mentioned by a list must be present in the
detection pin set supplied to validation. Generic RMW must not be used for W1C,
read-to-clear, or other read-sensitive registers.
The legacy no-op RMW form (`keep=0xFF, value=0`) has a zero change mask and is
intentionally rejected; add a dedicated operation before relying on its bus side effects.

The IR layout is not a wire format. Its devices map mechanically to M5HAL bus
configuration plus transfer metadata; full writes, delays, and GPIO operations
map directly to their M5HAL bytecode counterparts. Masked writes and bounded
ready waits require dedicated critical operations or orchestration when a
future lowering layer is added.
The lgfx backend cannot shorten an I2C transaction already in flight. A bounded
ready wait therefore starts no transaction with zero time remaining, while one
attempt may exceed the list deadline by at most the fixed stall limit plus a few
byte times (about 26 ms).

The editor includes all catalog boards, parts, and chip data. It can also open or accept
dragged board JSON files, keeps best-effort drafts in `localStorage`, and
downloads canonical formatted JSON. Selecting a revision shows the resolved
board read-only; clear the revision selection to resume editing the base board.
