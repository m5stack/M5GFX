# Board specification catalog

This dependency-free prototype keeps board wiring, revision choices, and part
facts in reviewable JSON. It is intentionally outside M5GFX's build inputs.

- `boards/`, `parts/`, `chips/`: source data (boards, replaceable parts, SoC pin facts)
- `accessories/`: bases, bottoms and modules that attach to a board (see "Accessories")
- `connector_types/`: physical and logical connector layouts (see "Connector types")
- `schema/`: JSON schemas for boards, accessories, connector types and `detector_order.json`
- `targets.json`: generator-specific option-bit to board-choice mappings
- `detector_order.json`: which catalog boards each package's detector families cover
- `lib/`, `cli/`: browser-safe model logic and the Node command wrapper
- `generated/docs/`: revision and composition tables rendered from the catalog
- `generated/m5unified_pin_table/`: the M5Unified external pin tables, per SoC
- `editor/`, `build/`: editor sources and the dependency-free bundler
- `dist/board_spec_editor.html`: standalone editor; open it directly with `file://`
- `test/`: Node tests and the C++ host tests that compile the generated detection data

```sh
node cli/spec.js validate boards/m5stack_core2.json      # schema and catalog rules (all boards when no file is given)
node cli/spec.js format boards/m5stack_core2.json --write # canonical formatting (without --write: report only)
node cli/spec.js resolve boards/m5stack_core2.json       # inspect all revision/runtime combinations as JSON on stdout
node cli/spec.js resolve boards/m5stack_core2.json --out /tmp/resolved     # optional local JSON files
node cli/spec.js emit-docs                              # generated/docs/ (all boards, or pass board files)
node cli/spec.js emit-wiring                             # src/board_detect/m5/generated/ (detection wiring, specs, detector order)
node cli/spec.js emit-pintable                           # generated/m5unified_pin_table/<soc>.{json,inl}
node cli/spec.js compare-pintable --m5unified <path>     # compile the generated tables against M5Unified's src/M5Unified.inl
node build/bundle.js                                     # dist/board_spec_editor.html
node cli/spec.js check                                   # everything below; CI runs this
npm test                                                 # test/*.test.js (the C++ host tests are skipped without a compiler)
```

`check` validates the connector types, parts, accessories and every board, requires
every board file to be in canonical format, and fails when any generated output
(docs, pin tables, detection wiring, the editor bundle) is missing, stale or
unexpected. Every revision/runtime combination is expanded and validated in
memory; resolved exports are not build inputs or freshness-check fixtures.
After editing a board, run `emit-docs`, `emit-wiring`, `emit-pintable` and
`build/bundle.js`, then `check` and `npm test`, and commit those generated files
with the source. Do not commit resolved exports.

Inspect resolved boards with `resolve` (one JSON array of `{combination, board}`
entries on stdout), or select a revision in the editor's read-only view.
The `--out` directory is arbitrary; its exports are not read by any tool.

Boards use `devices.<id>.choices` for replaceable parts and `revisions[]` for
known product revisions. Runtime-selected slots are expanded as additional
resolved combinations. Device `signals` and `enable` are derived from `dev:`
roles across the SoC and device-owned pin tables; they are not written in board
sources. `gpio:N` is the short form of `pin:soc.N`.

`official_name_verified: true` means that the product-name portion matches the
official documentation page title. The leading `M5Stack ` on M5Stack products
is added by this catalog's naming rule and is not part of the verified title.
An absent flag means that the spelling has not been verified.

SD wiring uses card terminal names (`clk`, `cmd`, `d0` through `d3`). The
`sd_slot` part defines their SPI aliases, and resolved boards report usable
`spi`, `sdio1`, and `sdio4` modes in `devices.sd.derived`.

M5GFX target metadata derives `chip` and `legacy_board_id` from the board
catalog and `board_enum` as `board_<name>`. The unused `desc_name` adapter
field is removed; descriptor names remain a handwritten C++ convention. The namespace removes the leading `m5stack_` or `m5`
and all underscores from the catalog ID; the overrides are
`m5cardputer_adv` with `cpp_namespace: "cardputer_adv"` and
`arduino_nesso_n1` with `cpp_namespace: "nesson1"`. These derived fields must
not be repeated in `targets.json`. `node cli/spec.js check` also verifies every
catalog ID against its explicit integer assignment in `src/lgfx/boards.hpp`,
as do `emit-wiring`, `emit-pintable` and `compare-pintable`. The checker accepts
a single `board_t` enum with unconditional declarations and decimal or hex
integer canonical values; octal values, duplicate declarations and conditional
enum declarations are rejected. The outer header include guard is allowed;
other macro definitions, undefinitions and included headers are rejected.
A namespace override identical to the derived value is rejected as redundant.

M5GFX board-detection wiring is generated from the base board only. Accessory
and composition wiring does not participate in board descriptor generation.
Generated headers represent unspecified values with target-specific unknown or
sentinel values. Value precedence is board value, then part default, then the
panel-class default.

`detector_order.json` maps each package's detector families to catalog boards.
`emit-wiring` generates a topological order from the declared constraints.
Among families whose predecessors are already placed, GPIO power-hold families
go first, then declaration order breaks ties. A board qualifies only when a
`power_hold` device also has a GPIO `dev:<id>.*` role. Without a hint, runtime follows this generated order. With a hint, runtime
selects constraint-ready families by GPIO power hold, then hint, then generated
order. A hinted family can therefore precede a power-hold family whose
predecessors have not run yet. Add a new family to the
manifest; `npm test` checks that each generated array is included by its source.

## Connector types

`connector_types/<id>.json` describes a connector once, and boards and accessories
refer to it by `type`. `layout` gives the grid (`rows`, `cols`, `pitch_mm`,
`numbering`, `key`, `view`), `positions` the pins with their `id`, grid cell and
optional `name` and wire `color`, and `rails` the power rails the connector
carries. `standards` names the pin assignments a board can pick with
`standard`: for example the 4-pin port (`hy2_4p`) defines `port_i2c`
(`scl`, `sda`), `port_uart` (`rx`, `tx`) and `port_io` (`in`, `out`) on
positions 1 and 2. `extends` inherits another type. `verified.layout` records how
the layout was confirmed (`datasheet`, `measured`, `generated`); `null` means not
confirmed. `logical_terminals_<n>` types carry `pitch_mm: null` and no verified
layout: they only number the terminals of a group whose placement is not
documented.

## Accessories

`accessories/<id>.json` describes a base, bottom or module that attaches to a
board: `attaches` names the host connector type and the slot it occupies
(`stack`, `bottom`, ...), `compatible_with` lists the boards, `host_pins` maps
the host connector positions to the roles the accessory adds (for example
`conn:port_b.1` for a port on a base), and `connectors` / `devices` describe what
the accessory itself exposes. `settings` lists user-selectable options such as DIP
switches, each with its choices and the pins they reroute. Accessory wiring is
resolved through the compositions declared in `targets.json` into
`generated/docs/<board>_composition.md` and the M5Unified pin table; it does not
take part in board-detection generation.

## Exposed connectors

Boards list the connectors that users can reach (headers, sockets, HAT buses,
Grove ports, Proto pads, expansion FPC/BTB connectors) in `connectors`, and link
the official product page and schematic in `spec.links`. GPIOs on those connectors
carry `conn:<id>.<position>` roles, which keeps them out of `detect_class`
(`E_DETECT_CLASS_CONNECTOR`).

- Write only values confirmed by the official pin table, PinMap or schematic. Leave
  out what cannot be confirmed. When official sources disagree, state in `note`
  which source was followed.
- `port_a` to `port_e` (and `port_b2` / `port_c2`) are the M5Unified pin-table
  contract: renaming or adding them changes the generated pin table. A physical
  port that has no pin-table entry uses another name (for example `grove_c`), and
  ports on a base or sub-board belong to the accessory definition, not the core
  board. If the contract order differs from the physical wire colors, say so in
  `note`.
- When the physical placement or pitch of a terminal group is not documented, use a
  logically numbered connector type with `pitch_mm: null` and no `verified.layout`.
- One position that reaches several GPIOs is listed in `multi_gpio`.
- Separate sockets that form one physical set (for example the two bottom headers of
  an Atom) share an optional `group`. Each socket keeps its own position numbering
  and first pin; `group` only lets tools show them together.
- USB connectors, battery connectors, antennas, test pads and terminals that are not
  directly connected to a GPIO (isolated inputs, relay contacts, CAN/RS485 terminal
  blocks) are not cataloged as connectors.

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
configuration plus transfer metadata; 8-bit writes, one-transfer little-endian
16-bit writes (`i2c_write16le`), delays, and GPIO operations
map directly to their M5HAL bytecode counterparts. Masked writes and bounded
ready waits require dedicated critical operations or orchestration when a
future lowering layer is added.
The lgfx backend cannot shorten an I2C transaction already in flight. A bounded
ready wait therefore starts no transaction with zero time remaining. How far an
in-flight attempt can exceed the list deadline depends on the lower I2C
implementation: hardware I2C is about 26 ms, while software I2C can take tens
of milliseconds when SCL is stuck.

On the original ESP32 package, the PSRAM catalog lists only CS and CLK. Its
data lines share the flash-only pads represented by the chip's reserved GPIOs.

### Detection compatibility notes

- ChainCaptain detection holds the generated display chip-select high during
  prepare. Earlier deselection is intentional and matches the transaction
  discipline used by other detected boards.
- Paper family (PaperS3 / PaperDIY) signature no longer drives the internal
  I2C lines high before the pull-up check; it only measures them against the
  internal pull-down (and pull-up) with a 10 us settle, which is read-only.
  PaperS3 drives its power-off request line low before setting it to output,
  and the detection transaction records the parallel-EPD pins as well.

## Editor

`dist/board_spec_editor.html` is a single file with all catalog boards, parts,
chip data, connector types and accessories bundled in; open it with `file://`.
The left navigation lists the catalog pages of the selected board (board, GPIO,
connectors, one page per connector, owners, revisions) and a validation page with
the current error count. A connector page shows the connector as a table in its
physical layout (terminal, wire color, endpoint, function); connector edits can be
undone. The editor can also open or accept dragged
board JSON files, keeps best-effort drafts in `localStorage`, and exports
canonical formatted JSON. Selecting a revision shows the resolved board read-only;
clear the revision selection to resume editing the base board. Exporting does not
touch the repository: replace `boards/<id>.json`, run `node cli/spec.js check`,
then review and commit.

## Pin detection classes (`detect_class`)

`detect_class` on a GPIO entry states how that pin reads when detection samples it
with the internal pulls. A detector checks it in its signature, so the candidate's own
driven probes (reset pulse, SPI, I2C) run only when the reading matches. This lets a
candidate be rejected without driving pins that, on other boards, are wired to loads
(a buzzer, an addressable LED, a reset input). Detectors earlier in the list still run
their own probes first. For example, on Capsule GPIO2 drives the buzzer; AirQ declares
its GPIO2 (EPD reset) as `floating`, so on Capsule the AirQ candidate is dropped
before its reset pulse would click the buzzer.

### Measurement

`probe_pin_pulls()` (`src/board_detect/board_detect.hpp`) measures one pin at a time in
ascending GPIO order: input with the internal pull-down, wait 10 us and read, then
input with the internal pull-up, wait 10 us and read. Each pin is restored to the
detection-start snapshot before the next one, so it must be called within an active
detection transaction. The same reading can be reproduced on hardware with a plain
sketch (`INPUT_PULLDOWN`, 10 us, read; `INPUT_PULLUP`, 10 us, read).

| `detect_class` | measured (pull-down, pull-up) | typical cause |
|---|---|---|
| `up` | U: (High, High) | external pull-up, or driven High |
| `down` | D: (Low, Low) | external pull-down, or driven Low |
| `floating` | F: (Low, High) | nothing holds the pin |
| `fixed` | U or D | something always holds the pin, either way (a push-pull output) |

An inverted reading X (High, Low) never matches any class. An absent field means no
constraint; there is no value for "unknown".

`detect_class` is not `pull`. `pull` records wiring (whether a resistor is fitted),
while `detect_class` records what detection measures. A pin with `pull: none` can
still read U or D when another chip drives it, so `pull` is never used as a
detection expectation.

### Which pins may carry a class

Write `detect_class` only when the class holds in every state detection can meet:
cold boot, software reset, wake from sleep, and after an interrupted detection.
Measure each of these states on real hardware before writing a value.

- Do not classify pins whose state users can change through buttons, connectors,
  or headers. Validation rejects connector and button roles.
- Use `up` / `down` / `floating` for pins defined by resistors alone.
- Use `fixed` for a driven pin only when the driver is always powered. For example,
  an EPD BUSY output reads D normally and U while the EPD is in deep sleep, so it can
  be `fixed`, but not `down`.
- Do not use pins without internal pulls (`chip.no_internal_pull`; rejected by
  validation), pull-ups switched by a PMIC, or pins whose class depends on a power
  rail that may be off.

### Adding a class to a board

1. Set `pins.<gpio>.detect_class` in the board JSON. Validation (`node cli/spec.js
   validate`, also shown in the editor) rejects it on pins without internal pulls.
2. Regenerate from `extras/board_spec`: `node cli/spec.js emit-docs boards/<board>.json`,
   `node cli/spec.js emit-wiring`, and `node build/bundle.js`; then run
   `node cli/spec.js check` and `npm test`. Commit the generated files; do not edit them
   by hand.
3. The generated wiring header (`src/board_detect/m5/generated/<chip>_wiring.hpp`)
   gets `detect_class_mask` (the classified pins) and one mask per class
   (`detect_class_up` / `_down` / `_floating` / `_fixed`). These constants exist only
   for boards with at least one class. The classified pins also join the chip's
   detection pin set: they are captured and restored like descriptor pins, and the
   same reservation checks apply (absent or chip-reserved GPIOs and native USB pins
   are rejected; pins reserved by the PSRAM mode are rejected or become conditional).
4. A catalog entry does not install a gate by itself. In the candidate's signature,
   build a `detect_class_expected_t` from the five constants and return
   `match_detect_class(expected, probe_pin_pulls(ctx, expected.mask))`
   (`src/board_detect/probe_types.hpp`). `spi_id_detector_t` takes such a signature
   function as its fifth constructor argument; see `airq_signature()` in
   `src/board_detect/m5/esp32s3/families.inl`. Other detector types call the match
   from their own signature.

A match only means "not excluded". The candidate is still confirmed by its normal
probe (for example the display ID).

`detect_class` on `choices.<id>.soc_pins` is reserved for selecting a board revision or
part by measurement. That selection is not implemented yet, and choice values are not
used as board-wide gates.

### Limits

- A pin under GPIO hold keeps its latched pad configuration, so the pull changes do
  not reach it and it cannot be classified.
- The pull reads are not completely silent: the 10 us pull-down briefly pulls an
  externally held-high pin low. On a board that M5GFX does not detect (Capsule has no
  display), autodetect repeats the whole list on each retry, so a faint buzzer click
  remains once per attempt.

Board-wide `detect_class` facts generate a `constexpr detect_class_expected_t
detect_class_expected` in the board's wiring namespace. Detectors copy this
value into a function-local `constexpr` before matching physical samples.
Revision-choice classes remain local evidence rather than board-wide gates.
Descriptors and probe procedures remain handwritten C++.

## Autodetection test injections

These compile-time macros exercise detection failures and observations. They do
not change the public detection configuration. Fixed startup bypasses detector
hooks, but still uses constructor, adopter, and VBUS observation hooks.

| Macro (`M5GFX_AUTODETECT_TEST_` prefix) | Injection point and effect |
| --- | --- |
| `EXCLUDE_ATOMS3` | Attempt target filter excludes AtomS3 and other unlisted boards to exercise excluded-result rollback. |
| `FAIL_SETUP_BOARDS` | Comma-separated board IDs reject adoption in `reject_detected_setup`; autodetection uses the pre-refine representative ID. |
| `FAIL_CORES3_SETUP` | CoreS3 constructor returns failure before allocating display parts. |
| `STOPWATCH_NO_DISPLAY` | StopWatch constructor returns no-display without allocating its panel. |
| `FAIL_STOPWATCH_TOUCH` | PM1 family confirm uses an unanswered touch address for StopWatch. |
| `FAIL_PAPERMONO_TOUCH` | PM1 family confirm and refine use an unanswered touch address for PaperMono. |
| `FAIL_CARDPUTER_VARIANT` | Cardputer confirm discards both VAMeter and ADV responses. |
| `FAIL_CORE2_TOUCH` | AXP family confirm and post-power refine discard Core2 and Tough touch responses. |
| `FAIL_CORE2_TOUCH_PRE` | AXP family confirm discards Core2 and Tough touch responses. |
| `FAIL_CORE2_TOUCH_POST` | AXP family post-power refine discards Core2 and Tough touch responses. |
| `CORE_FAMILY_FORCE_STATION` | AXP family confirm discards touch responses and forces the G12 class to floating for Station selection. |
| `FAIL_PAPER_CONFIRM` | Paper confirm fails after signature retained reset state, restoring its probe pins. |
| `CORES3_FORCE_VBUS` | CoreS3 confirm and fixed preparation observe external 5 V after performing the VBUS pin measurement. |
| `CORES3_STACKCHAN_GATE` | CoreS3 post-power base gate returns true after measuring its pins. |
| `CORES3_FORCE_STACKCHAN` | CoreS3 post-power base ACK and firmware meet the StackChan threshold. |
| `CORES3_STACKCHAN_NACK` | CoreS3 post-power base ACK loop returns NACK throughout its poll window. |
| `CORES3_NO_CAMERA` | CoreS3 confirm and post-power refine skip camera reads and treat the camera as absent. |
| `CORES3_CAMERA_ABSENT_AT_CONFIRM` | CoreS3 confirm skips the initial camera read; post-power refine can still find a waking camera. |
| `CORES3_FORCE_SE` | CoreS3 confirm skips the initial camera read and forces the short-release band. |
| `CORES3_RELEASE_AMBIGUOUS` | CoreS3 confirm skips the initial camera read and forces the ambiguous-release band. |
