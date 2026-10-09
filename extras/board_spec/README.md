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

`official_name_verified: true` means that the product-name portion matches the
official documentation page title. The leading `M5Stack ` on M5Stack products
is added by this catalog's naming rule and is not part of the verified title.
An absent flag means that the spelling has not been verified.

SD wiring uses card terminal names (`clk`, `cmd`, `d0` through `d3`). The
`sd_slot` part defines their SPI aliases, and resolved boards report usable
`spi`, `sdio1`, and `sdio4` modes in `devices.sd.derived`.

M5GFX board-detection wiring is generated from the base board only. Accessory
and composition wiring does not participate in board descriptor generation.
Generated headers represent unspecified values with target-specific unknown or
sentinel values. Value precedence is board value, then part default, then the
panel-class default.

`detector_order.json` maps each package's detector families to catalog boards.
`emit-wiring` generates the detector arrays with GPIO power-hold families first,
preserving the declared order within each group. A board qualifies only when a
`power_hold` device also has a GPIO `dev:<id>.*` role. The generated board IDs
give the runtime hint pass the same priority rule. Add a new family to the
manifest; `npm test` checks that each generated array is included by its source.

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

The editor includes all catalog boards, parts, and chip data. It can also open or accept
dragged board JSON files, keeps best-effort drafts in `localStorage`, and
downloads canonical formatted JSON. Selecting a revision shows the resolved
board read-only; clear the revision selection to resume editing the base board.

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
2. Regenerate from `extras/board_spec`: `node cli/spec.js resolve boards/<board>.json`,
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
   (`src/board_detect/detect_class.hpp`). `spi_id_detector_t` takes such a signature
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
