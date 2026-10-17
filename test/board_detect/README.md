# Board detection host tests

Run from any working directory with a C++11 compiler and a POSIX shell:

```sh
sh test/board_detect/run.sh
CXX=clang++ sh test/board_detect/run.sh
```

The runner compiles every test program, runs it from the repository root, reports
its result, and removes the temporary executables. Missing compilers and failed
assertions fail the run. Node.js, ESP-IDF, SDL2, and connected devices are not
required. `BoardDetect.yml` runs the same command independently of catalog CI.

## Coverage

| Test | Behavior protected |
| --- | --- |
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
| `package_p4.cpp` | Actual P4 detector list, absent evidence, Tab5/Tab5X revision candidates and retry policy; fixed StampP4 displayless acceptance and detector exclusion |
| `description_*.cpp`, chip tests, `fixtures/` | Complete production descriptors, PMIC device/operation lists, register masks, GPIO scopes, board registrations, constructor identity, GPIO power-hold agreement with generated catalog output |
| `construction_esp32.cpp` | Complete production setup helpers and constructors; fixed Tough PMIC backlight, panel variant/reset register, touch identity, bus frequencies and display-part ownership |
| `ops_host.cpp` | Typed operation validation/execution, retry, dedicated-release summaries and I2C bus recovery policy |
| `detect_class_host.cpp`, `detect_session_host.cpp` | Detect-class matching and session verdict/preference/candidate/persistence rules |

[MIGRATION.md](MIGRATION.md) maps original test names to the C++ checks and lists
removed source-text assertions. The fixtures record the complete semantic fields
of 61 descriptions; pointers are represented by constructor names or callback
presence, rather than unstable addresses. Catalog tests also compare fixture
internal-I2C usage with `targets.json` metadata. Fixed callback identities are also
checked in the ESP32 and S3 tests.

The previous synthetic P4 test returned false from every signature. The real
Tab5 signature returns true for a present bus: an unanswered ID retains weak
evidence and retries five times. The new P4 test exercises that production
behavior. No production decision was changed to match the synthetic fixture.

## Hardware boundary

Tests include production `.hpp` and `.inl` files, the complete model types, board
IDs, and generated wiring/specification/order headers. They do not extract
production function text or redeclare reduced model types. Internal includes
separate transactions, probing, startup, SPI selection, preparation, package
selection, and integration without rewriting their bodies.

The runner defines explicit host-only include macros for the ESP32 common header
and classic touch peripheral substitutes. Without those macros production keeps
its original relative includes and resolution within M5GFX. The low-level GPIO/I2C backend
records operations, clocks, snapshots, and responses. Family tests replace the
GPIO/I2C/SPI/dedicated-GPIO probes; timing tests compile the real probe instead.
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
- `detect_attempt.test.js`: public `setDetectConfig`, `getCandidateBoard`, legacy
  `autodetect`, and `init_impl` panel restoration. Its combined fixture also
  retains older integration assertions.
- The first test in `fixed_board.test.js`: public fixed acceptance/getter and
  startup-prefix behavior, including its combined legacy integration fixture.
- M5Unified-dependent checks: package defaults, `BOARD_ID`, GPIO46 pre-hold, and
  explicit fallback/candidate/build-default precedence. These belong in
  M5Unified's test suite when that migration is scheduled.

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
- The host configuration does not compile S3 dedicated-GPIO release probing,
  ToughC5 LP-I2C, the <=32-pin hold branch, or ESP-IDF 4.4 branches.
- Stub `pin_backup_t` and I2C response types can drift from the real HAL;
  firmware compilation remains necessary to detect API changes.
