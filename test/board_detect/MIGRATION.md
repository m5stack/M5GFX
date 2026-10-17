# Test migration coverage

This map records original test names so descriptor/table refactors can find the
behavioral checks without depending on source-text anchors. Line references point
to the replacement C++ function or program. The retained public-class and
M5Unified checks are outside the completed host migration scope.

## Behavioral tests

| Original test | Replacement | Notes |
| --- | --- | --- |
| `detect_families.test.js:26` — production Core2 and Cardputer selectors cover preference, hint and observed compatibility | [esp32_families.cpp:122](esp32_families.cpp#L122), [s3_families.cpp:148](s3_families.cpp#L148) |  |
| `detect_families.test.js:79` — production SPI and Paper hint restrictions recover a different member on attempt one | [spi_variants.cpp:28](spi_variants.cpp#L28), [s3_families.cpp:267](s3_families.cpp#L267) |  |
| `detect_families.test.js:146` — production C6 display signature returns an unsaved weak UnitC6L independently of hint | [c6_display.cpp:89](c6_display.cpp#L89) |  |
| `detect_families.test.js:188` — production Atom pull mismatch stays provisional on attempt zero and Stack requires physical evidence | [esp32_families.cpp:201](esp32_families.cpp#L201) |  |
| `detect_families.test.js:249` — production PM1 unresolved tail keeps PaperMono without running hinted StopWatch power | [s3_families.cpp:98](s3_families.cpp#L98) |  |
| `detect_families.test.js:271` — production PM1 pre-power table gates writes and fixes observed members | [s3_families.cpp:21](s3_families.cpp#L21) |  |
| `detect_families.test.js:312` — production PM1 D/D refinement polls only PaperMono and never switches power | [s3_families.cpp:98](s3_families.cpp#L98) |  |
| `detect_families.test.js:339` — production prepare retries failed power before pending member refinement | [integration.cpp:279](integration.cpp#L279) |  |
| `detect_families.test.js:392` — production AXP subdivision excludes unsafe Station fallbacks before power | [esp32_families.cpp:21](esp32_families.cpp#L21) |  |
| `detect_families.test.js:469` — production AXP refinement reads both touches after SD preparation and panel-only observation preserves identity | [esp32_families.cpp:122](esp32_families.cpp#L122) |  |
| `detect_families.test.js:526` — production detector runner stops later families only after identified AXP or PM1 | [integration.cpp:239](integration.cpp#L239) |  |
| `detect_families.test.js:572` — production Stack confirmation requires LCD ID and persists confirmed identity | [esp32_families.cpp:273](esp32_families.cpp#L273) |  |
| `detect_family_members.test.js:24` — production CoreS3 refinement covers camera, release band and gated IOE outcomes | [s3_families.cpp:315](s3_families.cpp#L315) |  |
| `detect_family_members.test.js:87` — production Cardputer subdivision reads independent ICs and requires four released floating pins | [s3_families.cpp:148](s3_families.cpp#L148) |  |
| `detect_family_members.test.js:128` — production CoreS3 confirm transports measured release band to refinement | [s3_families.cpp:416](s3_families.cpp#L416) |  |
| `detect_package_candidates.test.js:10` — package-only Stamp candidates stop probing, never prepare or persist, and preserve prior weak evidence | [package_module.cpp:31](package_module.cpp#L31), [package_c5.cpp:1](package_c5.cpp#L1) |  |
| `detect_package_candidates.test.js:92` — P4 generated families leave absent evidence unknown and retain a Tab5 weak candidate | [package_p4.cpp:12](package_p4.cpp#L12) |  |
| `detect_package_candidates.test.js:118` — Unified defaults preserve build selections and supported package defaults only | Retained: M5Unified migration candidate |  |
| `detect_session.test.js:10` — detection session enforces provisional preference, stable hints, candidates and confirmed-only persistence | [detect_session_host.cpp:8](detect_session_host.cpp#L8) |  |
| `detect_attempt.test.js:12` — production attempt yields before construction and preserves adoption and rollback semantics | [integration.cpp:132](integration.cpp#L132) | Deferred public-class portion remains in the original JS test. |
| `detect_attempt.test.js:150` — public config stores the next preference and warns after either successful or failed init | Retained: deferred public-class test | Deferred public-class portion remains in the original JS test. |
| `detect_attempt.test.js:180` — production init restores panel according to adoption independently of verdict | Retained: deferred public-class test | Deferred public-class portion remains in the original JS test. |
| `fixed_board.test.js:9` — fixed acceptance bypasses NVS and detection and retries clean startup after failure | [integration.cpp:213](integration.cpp#L213) | Deferred public-class portion remains in the original JS test. |
| `fixed_board.test.js:159` — fixed variant callbacks never invoke family identification | [esp32_families.cpp:296](esp32_families.cpp#L296), [s3_families.cpp:473](s3_families.cpp#L473) |  |
| `fixed_board.test.js:177` — fixed Core2 and CoreS3 observe only construction variants in startup order | [esp32_families.cpp:296](esp32_families.cpp#L296), [s3_families.cpp:473](s3_families.cpp#L473) |  |
| `fixed_board.test.js:230` — M5Unified BOARD_ID guard accepts positive values without M5GFX_BOARD and fixed selection wins | Retained: M5Unified migration candidate |  |
| `fixed_board.test.js:277` — fixed Tough backlight follows observed PMIC while its panel and touch remain fixed | [construction_esp32.cpp:14](construction_esp32.cpp#L14) |  |
| `fixed_board.test.js:309` — Paper IT8951 initialization waits for BUSY before sending panel commands | Not ported: source statement-order/wording assertion |  |
| `fixed_board.test.js:315` — fixed SPI variants read only the selected display and keep default on unreadable ID | [spi_variants.cpp:28](spi_variants.cpp#L28) |  |
| `fixed_board.test.js:356` — fixed GPIO46 pre-hold follows the existing power pin table | Retained: M5Unified migration candidate |  |
| `fixed_board.test.js:390` — fixed Stack startup no longer collects unused IPS options | Not ported: source statement-order/wording assertion |  |
| `fixed_board.test.js:400` — StopWatch constructor GPIO is included in its rollback pins | [support/snapshot.hpp:91](support/snapshot.hpp#L91) | Source spelling assertions removed; full GPIO scope is frozen in the S3 descriptor fixture. |
| `pull_release.test.js:9` — release pull classifier covers every four-sample combination | [pull_release.cpp:12](pull_release.cpp#L12) |  |
| `pull_release.test.js:37` — production pull probe preserves zero-release sequence and samples release per pin | [pull_release.cpp:37](pull_release.cpp#L37) |  |
| `pull_release.test.js:84` — production DualKey rejects weak pulls with the shared release delay and preserves OPI exclusion | [pull_release.cpp:100](pull_release.cpp#L100), [pull_release_opi.cpp:1](pull_release_opi.cpp#L1) |  |
| `ops_host.test.js:11` — board-detection operation IR passes its C++11 host suite | [ops_host.cpp:894](ops_host.cpp#L894) |  |
| `ops_host.test.js:29` — detect-class matcher checks U, D, F, X and unconstrained pins | [detect_class_host.cpp:5](detect_class_host.cpp#L5) |  |

Additional production paths covered directly:

- `package_select_*.cpp` includes the complete `select_detection_package()` implementation
  and checks S3/C6/P4 package-to-array selection, including unsupported packages.
- `fixed_core_s3()` checks actual SCLK/MOSI/MISO/DC/CS and command values.
- `stickc_shared_read()` calls the real StickC detector and checks one ID read for
  both members and misses; `spi_preamble.cpp` checks the real zero-command GPIO clocks.
- `integration_s3.cpp` checks conditional GPIO capture, probe flags and fixed OPI refusal.
- Refinement and integration tables check scoped I2C ports are closed, and family
  probe substitutes validate expected SDA/SCL when configured.

## Catalog-side source assertions

The migration removed 138 assertions reading handwritten C++ from these mixed
tests; 41 construction assertions remain in their original catalog tests.
Schema, emitter, generated-output, and editor assertions remain in the catalog
suite. Complete descriptor fixtures, GPIO power-hold agreement, constructor
identity, startup validation, and the runtime tables cover the migrated
contracts. Device construction settings are not part of the descriptor snapshot;
the retained construction assertions are explicitly awaiting host migration.

| `board_spec.test.js` original test | Removed source assertions | Replacement or reason |
| --- | ---: | --- |
| all chip entries pass reset and hint policy through the common session | 3 | Runtime integration/descriptor tests; source wording is not asserted |
| displayless candidates do not retry and confirmed boards clear candidates | 5 | Runtime integration/descriptor tests; source wording is not asserted |
| DualKey requires NC pins to float and treats a pressed second key as a candidate | 4 | [support/snapshot.hpp:91](support/snapshot.hpp#L91) plus chip-specific runtime tables |
| M5GFX target metadata matches descriptor internal-I2C declarations | 2 | Catalog test `descriptor fixtures agree with target internal-I2C metadata` compares all mapped fixture IDs and bus usage with `targets.json` |
| AtomS3 generated panel construction matches every legacy field | 0 | 2 construction assertions retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. |
| AtomS3R generated specs preserve the legacy setup | 3 | 4 construction assertions retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. Other descriptor/decision checks use the host suite. |
| ChainCaptain and PaperColor generated setup preserves legacy fields | 4 | 2 construction assertions retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. Other descriptor/decision checks use the host suite. |
| Dial and DinMeter generated specs preserve the legacy setup | 0 | 7 construction assertions retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. |
| StamPLC generated specs preserve the legacy setup | 1 | 3 construction assertions retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. Other descriptor/decision checks use the host suite. |
| AirQ generated specs preserve the legacy setup | 2 | 4 construction assertions retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. Other descriptor/decision checks use the host suite. |
| ESP32 PICO catalogs preserve probe order, legacy SPI reads, and fallback | 10 | 1 construction assertion retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. Other descriptor/decision checks use the host suite. |
| ESP32-C6 catalogs and detector preserve both display boards | 9 | 4 construction assertions retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. Other descriptor/decision checks use the host suite. |
| Cardputer family generated specs and wiring preserve the legacy setup | 0 | 3 construction assertions retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. |
| ESP32-S3 detector filters hinted and OPI-conflicting candidates per model | 7 | [support/snapshot.hpp:91](support/snapshot.hpp#L91) plus chip-specific runtime tables |
| detection transaction exposes non-consuming start-state restoration | 14 | Runtime integration/descriptor tests; source wording is not asserted |
| confirmed boards survive post-detection power setup failures | 5 | Runtime integration/descriptor tests; source wording is not asserted |
| embedded autodetect routes detected boards through descriptor setup | 2 | Runtime integration/descriptor tests; source wording is not asserted |
| confirmed boards tolerate reset-list faults while confirm remains strict | 7 | `integration.cpp:reset_faults` tests assert/release failures and skipped resets for retained, strict and unidentified profiles; `esp32_families.cpp:strict_reset` tests Core2/Tough defaults |
| post-power member refinement and confirmed option variants are opt-in | 6 | Runtime integration/descriptor tests; source wording is not asserted |
| PM1 extension failure injection rejects only its selected board | 4 | Runtime integration/descriptor tests; source wording is not asserted |
| Core2 and Tough share the PMIC option bit at the C++ assumption | 1 | [support/snapshot.hpp:91](support/snapshot.hpp#L91) plus chip-specific runtime tables |
| gpio_power keeps the active-high hold level for existing members | 4 | Runtime integration/descriptor tests; source wording is not asserted |
| no_display_pins is used only when a display has no GPIO bus or is absent | 3 | Runtime integration/descriptor tests; source wording is not asserted |
| CoreP4X generated DSI setup preserves the legacy fields | 2 | [support/snapshot.hpp:91](support/snapshot.hpp#L91) plus chip-specific runtime tables |
| Tab5 touch identities select the three DSI panels | 2 | 5 construction assertions retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. Other descriptor/decision checks use the host suite. |
| every descriptor operation GPIO is inside its execution and rollback scope | 12 | [support/snapshot.hpp:91](support/snapshot.hpp#L91) plus chip-specific runtime tables |
| CoreMatrix generated I2C setup preserves the legacy fields | 2 | [support/snapshot.hpp:91](support/snapshot.hpp#L91) plus chip-specific runtime tables |
| PaperS3 and PaperDIY generated setup preserves the legacy fields | 6 | 5 construction assertions retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. Other descriptor/decision checks use the host suite. |
| CoreS3 family catalog keeps shared wiring and option power variants | 18 | 1 construction assertion retained in `extras/board_spec/test/board_spec.test.js`; construction settings await host migration. Other descriptor/decision checks use the host suite. |

The catalog detector-order test retains comparisons between emitter output and
generated headers. Its source-include spelling checks were removed. The previous
GPIO power-hold source-regex test is now a comparison of actual `power.hold_pin`
against generated `gpio_power_hold_board_ids` for every descriptor.

Additional source-only assertions not copied:

- `fixed_board.test.js`: callback body spelling, unused IPS spelling, and Paper
  IT8951 `_wait_busy()`/`startWrite()` statement order. No claim of panel-driver
  command-stream coverage is made by this suite.
- `fixed_board.test.js`: StopWatch GPIO constant/list spelling. The actual complete
  operation GPIO list, including GPIO38, is covered by the S3 descriptor fixture.
- Diagnostic warning text and synthetic log counters in PM1/CoreS3 fixtures are
  not stable behavioral contracts. Decisions, polling limits, ID reads, candidate
  ownership, and persistence remain checked.
- Synthetic calls into mocked `prepare_power` with a description lacking any
  fallible power operation had no equivalent complete production description.
  The replacement preparation matrix injects real I2C operation failure instead.
- Mock transaction/prepare/SD call counters are replaced by actual GPIO/bus state,
  restoration events, SPI transfers, prepared flags, and constructor/adoption
  counts. Rollback can legitimately skip rewriting an unchanged snapshot.

The synthetic P4 fixture's unconditional false signatures implied one attempt
for weak Tab5 evidence. The actual detector returns true for a present I2C bus,
so unanswered confirmation retries five times. `package_p4.cpp` tests the actual
production behavior; production was not changed to match the synthetic fixture.

## Retained M5Unified candidates

- `detect_package_candidates.test.js`: supported package/build defaults.
- `fixed_board.test.js`: `BOARD_ID`/fixed precedence and GPIO46 pre-hold.
- `board_spec.test.js`: explicit fallback before weak candidate before build
  default, including supported chip/package defaults.

These remain runnable by `npm test` until M5Unified owns their replacement.
