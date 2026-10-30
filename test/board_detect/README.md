# Board detection host tests

Run from any working directory with a C++11 compiler and a POSIX shell:

```sh
sh test/board_detect/run.sh
CXX=clang++ sh test/board_detect/run.sh
```

The runner compiles every test program, runs it from the repository root, reports
its result, and removes the temporary executables. Missing compilers and failed
assertions fail the run; remaining programs still run to list all family pin
mismatches. Node.js, ESP-IDF, SDL2, and connected devices are not
required. `BoardDetect.yml` runs the same command independently of catalog CI.

## Coverage

| Test | Behavior protected |
| --- | --- |
| `pin_coverage_*.cpp` | Every generated detector family list across all eight chip catalogs; ordered member IDs must match the catalog. Real GPIO/pull/SPI probes, hardware-I2C pad events, and every member hint; signature/confirm operations must belong to the transaction snapshot (including S3 conditional pads) and the family's catalog-derived pin union. prepare/refine are outside this check. Atom touch/LED stubs record pads from the production constants. |
| `airq_generated.cpp` | Physical pull combinations independently constrain the generated AirQ detect-class gate |
| `c6_display.cpp` | UnitC6L bus/ID/hint matrix, weak evidence, persistence, NessoN1 ID order and short circuit |
| `s3_families.cpp` | PM1 pre-power subdivision and PaperMono-only refinement; Cardputer/ADV/VAMeter evidence and preference table; Paper retry; CoreS3/SE/StackChan camera/release/firmware decisions; fixed CoreS3 startup and actual LCD read pins; scoped I2C closure |
| `esp32_families.cpp` | AXP pre-power table and Station exclusion; Core2/Tough refinement and read-only observation; Atom touch evidence and one measurement per boot; Stack physical gates and LCD confirmation; fixed Core2/Tough startup; actual shared StickC ID read and scoped I2C closure |
| `spi_variants.cpp` | First-attempt hint restriction, retry and final scan, per-member OPI exclusion, fixed selected-panel observation and slow-read fallback |
| `integration.cpp` | Real attempt runner, transactions, adoption/construction failure, provisional preference yield, power recovery, identified-family stop and rollback, fixed retries, pending refinement, optional observation and confirmed/strict reset-operation failures |
| `pull_release.cpp`, `pull_release_opi.cpp` | Every four-sample pull classification; actual per-pin sampling/restoration/delay sequence and independent 128 us DualKey release waits; DualKey weak/floating/key evidence and OPI exclusion |
| `package_select_*.cpp` | Real package/eFuse selection for S3 QFN56/LGA56, C6 QFN40/QFN32 and P4, unsupported packages, and active S3 OPI memory |
| `integration_s3.cpp` | S3 runner conditional GPIO capture and probe flag propagation, plus fixed-start OPI ownership refusal |
| `spi_preamble.cpp` | Real soft-SPI zero command, clock count and delay before asserting CS, followed by the actual ID read |
| `package_module.cpp`, `package_c5.cpp` | Actual C5/C6 eFuse-only candidates, generated protection edges, previous weak evidence, no GPIO/prepare/construct/persist |
| `package_p4.cpp` | Actual P4 detector list, absent evidence, Tab5/Tab5X revision candidates and retry policy; post-power touch polling limits/timing and caller hardware-I2C ownership; fixed StampP4 displayless acceptance and detector exclusion |
| `description_*.cpp`, chip tests, `fixtures/` | Complete production descriptors, PMIC device/operation lists, register masks, GPIO scopes, board registrations, constructor identity, GPIO power-hold agreement with generated catalog output |
| `construction_esp32.cpp` | Complete production setup helpers and constructors; fixed Tough PMIC backlight, panel variant/reset register, touch identity, bus frequencies and display-part ownership |
| `construction_c61.cpp` | CoreMatrix ACK success/failure and unavailable I2C; the temporary hardware port is closed before either successful construction or rejection |
| `construction_p4.cpp` | Tab5/Tab5X post-power software-I2C observations through real construction: ST firmware 1/3/unknown, GT911, silent touch with DSI ID fallback, retained power failures, DSI lane rate and display-part ownership |
| `ops_host.cpp` | Typed operation validation/execution, retry, dedicated-release summaries and I2C bus recovery policy |
| `detect_class_host.cpp`, `detect_session_host.cpp` | Detect-class matching and session verdict/preference/candidate/persistence rules |

The fixtures record the complete semantic fields
of 61 descriptions; pointers are represented by constructor names or callback
presence, rather than unstable addresses. Catalog tests also compare fixture
internal-I2C usage with `targets.json` metadata. Fixed callback identities are also
checked in the ESP32 and S3 tests.

## Hardware boundary

Tests include production `.hpp` and `.inl` files, the complete model types, board
IDs, and generated board-data and detector-order headers. They do not extract
production function text or redeclare reduced model types. Internal includes
separate transactions, probing, startup, SPI selection, preparation, package
selection, and integration without rewriting their bodies.

The runner selects `support/detect_io.hpp` through
`M5GFX_BOARD_DETECT_IO_HEADER`. It records GPIO/I2C operations, clocks,
snapshots, and responses without SDK headers or firmware driver result types.
`support/native_devices_io.hpp` adapts native device calls only for construction
fixtures and test setup; production detection calls the selected I/O directly.
Family tests replace GPIO/I2C/SPI/dedicated-GPIO probes; timing tests compile the
real probe instead. Classic touch/LED transport returns the simulated raw counts,
while detection decisions remain production code.
Transactions, candidate selection, startup validation, power/refine policy, and
session policy remain production implementations. Family probes check expected
I2C SDA/SCL pins where configured; multi-bus Cardputer probes select their expected
pins by device address. The integration and refinement tables check scoped ports
are closed before starting the next case.

The ESP32 construction test replaces physical bus, panel, light, and touch devices
while including the real setup descriptors, ownership helpers, constructor
bodies, and dispatch table. It checks the configuration sent to those devices;
it does not exercise panel-driver command streams or emulate physical devices.
The standalone C6 family adapter still assumes successful display adoption, as
its original test did; adoption failures are covered by `integration.cpp`.

Production boundary changes are checked with firmware builds and comparisons of
all allocated ELF sections. For Arduino comparisons, use `CORE_DEBUG_LEVEL=0`
so file/line information in logging arguments does not change the code being
compared. Debug information is excluded. Also compare an ESP-IDF configuration.

## Deferred legacy checks

The following checks remain under `extras/board_spec/test/` and continue to run
with `npm test`:

- `board_spec.test.js`: construction settings awaiting host migration. The 41
  retained assertions cover S3 family panel types, DMA, bus sharing, backlight and
  touch selection, plus CoreS3, classic Atom, C6 displays, and Tab5 setup. Complete
  descriptor snapshots do not contain these device construction settings.
- `detect_attempt.test.js`: public `setDetectConfig`, `getCandidateBoard`,
  and `init_impl` panel restoration. Its combined fixture also
  retains older integration assertions.
- The first test in `fixed_board.test.js`: public fixed acceptance/getter and
  startup-prefix behavior, including its combined legacy integration fixture.

M5Unified-dependent checks (package defaults, `BOARD_ID`, GPIO46 pre-hold, and
explicit fallback/candidate/build-default precedence) are not covered by these
tests.

The public class needs a separately designed test boundary. Options include a
native SDL2 build of the real class, an includeable implementation shared with
its real class definition, or firmware `build_test` coverage. This suite adds no
SDL2 dependency and does not claim those public API tests have migrated.

## Updating descriptor fixtures

Fixtures characterize the existing descriptions and their operation sequences.
After reviewing an intentional semantic change, regenerate and inspect them:

```sh
M5GFX_UPDATE_SNAPSHOTS=1 sh test/board_detect/run.sh
git diff -- test/board_detect/fixtures
sh test/board_detect/run.sh
```

The runner rejects snapshot updates whenever `CI` is nonempty, including when
the update variable is set to an empty value.

Do not update fixtures to accept an unexplained failure. Runtime decision-table
assertions are independent of the fixture update option.

## Not covered yet

- Catalog invariants need rule checks for new entries: no-display eligibility,
  active-high GPIO holds, reverse rollback order, LP-pad release at capture, and
  absence of hardcoded board selection in legacy autodetect.
- The host configuration compiles S3 dedicated-GPIO release probing against
  stubs but does not execute it. It does not compile ToughC5 LP-I2C,
  the <=32-pin hold branch, or ESP-IDF 4.4 branches.
- The in-memory I/O implementation can drift from the firmware implementation;
  firmware compilation remains necessary to detect API changes.

The pin-coverage tests call each family's signature and confirmation separately
even when the signature rejects. Early returns inside confirmation still need
success scenarios: the S3 test supplies real SPI ID bits and checks the later
Cardputer sense reads, both I2C buses and both SDA recovery clock sequences.
They use the production probe implementations, not the response-level probe doubles. Snapshot
membership comes from capture events, while GPIO mode/read/output, restore and
hardware-I2C pad selection are observed through the I/O implementation. Low/high, floating and
mixed GPIO responses, I2C response bytes and every member hint exercise different
paths; this is a snapshot containment check, not exhaustive hardware simulation.
Peripheral-specific touch/LED transport remains a host boundary, as in the
family truth tables. S3 conditional-pad exclusion is checked separately by
`integration_s3.cpp`, `spi_variants.cpp` and `pull_release_opi.cpp`.

The S3 dedicated GPIO release probe is an empty implementation on the host.
Direct register writes do not emit GPIO events; current uses are covered by
preceding `pinMode` operations, not by observing the register writes themselves.

Each family reports successful `confirm()` returns over calls as a conservative
lower bound on reaching its end. A zero does not prove the absence of later
GPIO paths; generic response patterns cannot reproduce every peripheral ID.
The explicit Cardputer scenarios also assert the events after its SPI-ID gate,
including the conflicting-variant path that intentionally returns false.

Each family must emit at least one GPIO mode/read/output or I2C-pad-selection
event over its cases. Snapshot capture/restore and delay events do not count.
The only GPIO-free families allowed are StampC5 and StampC6: they inspect eFuse
package facts, nominate a weak candidate, and never enter GPIO confirmation.
Per-family event totals are printed so empty coverage cannot silently pass.
