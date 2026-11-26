import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import test from "node:test";
import { m5gfxBoardMapping } from "../lib/emit/m5gfx_board_wiring.js";

// Existing-board regression data: [fields, probe pins, has specifications], or
// null for no generated wiring. New catalog IDs do not need an entry here.
const expected = {
  arduino_nesso_n1: [["display", "i2c", "hold", "touch"], [], true],
  m5airq: [["display", "hold", "power"], [], true],
  m5atom_psram: null,
  m5atomlite: [[], [2, 12, 13, 18, 23, 27, 34], false],
  m5atommatrix: [[], [2, 12, 13, 18, 21, 23, 25, 27, 34], false],
  m5atoms3: [["display", "hold", "backlight"], [], true],
  m5atoms3lite: [[], [4, 12, 16, 41], false],
  m5atoms3r: [["display", "i2c", "hold"], [], true],
  m5atoms3rcam: [["i2c"], [9, 12, 18, 21], false],
  m5atoms3rext: [["i2c"], [9, 12, 18, 21], false],
  m5atoms3u: [[], [4, 12, 16, 41], false],
  m5atomu: [[], [2, 12, 13, 18, 23, 27, 34], false],
  m5atomvoice: [[], [2, 18, 23, 34], false],
  m5atomvoices3r: [["i2c"], [], false],
  m5capsule: [["i2c", "power"], [], false],
  m5cardputer: [["display", "hold", "backlight"], [], true],
  m5cardputer_adv: [["display", "i2c", "hold", "backlight"], [], true],
  m5chaincaptain: [["display", "i2c", "hold"], [], true],
  m5dial: [["display", "i2c", "hold", "backlight", "touch", "power"], [], true],
  m5dinmeter: [["display", "i2c", "hold", "backlight", "power"], [], true],
  m5dualkey: [[], [17, 21, 38, 39], false],
  m5nanoc6: [[], [3], false],
  m5nanoh2: [[], [3], false],
  m5paper: [["display", "shared_sd", "power", "hold"], [], false],
  m5papercolor: [["display", "shared_sd", "i2c", "hold"], [], true],
  m5paperdiy: [["display", "i2c"], [], true],
  m5papermono: [["display", "i2c", "hold", "touch"], [], true],
  m5papers3: [["display", "i2c", "power", "touch"], [], true],
  m5powerhub: [["i2c"], [], false],
  m5stack: [["display", "shared_sd", "hold"], [], false],
  m5stack_core2: [["display", "shared_sd", "i2c", "hold"], [12, 32, 33], false],
  m5stack_coreink: [["display", "power", "hold"], [], true],
  m5stack_corematrix: [["i2c"], [], true],
  m5stack_corep4x: [["i2c"], [], true],
  m5stack_cores3: [["display", "shared_sd", "i2c", "hold", "touch", "camera"], [], true],
  m5stack_cores3se: [["display", "shared_sd", "i2c", "hold", "touch"], [], true],
  m5stack_stackchan: [["display", "shared_sd", "i2c", "hold", "touch"], [], true],
  m5stack_tab5: [["i2c", "touch"], [], true],
  m5stack_tab5x: [["i2c", "touch"], [], false],
  m5stampc3: [[], [20], false],
  m5stampc3u: [[], [], false],
  m5stampc5: null,
  m5stampc6: null,
  m5stamplc: [["display", "shared_sd", "i2c", "hold"], [], true],
  m5stampp4: null,
  m5stampp4x: null,
  m5stampp4xhub: [["i2c", "touch"], [], true],
  m5stamppico: null,
  m5stamps3: [[], [], false],
  m5stamps3bat: [["i2c"], [], false],
  m5stamps3mini: [["i2c"], [], false],
  m5station: [["display", "i2c", "hold"], [], false],
  m5stickc: [["display", "i2c", "hold"], [], true],
  m5stickcplus: [["display", "i2c", "hold"], [], true],
  m5stickcplus2: [["display", "power", "hold", "backlight"], [], true],
  m5sticks3: [["display", "i2c", "hold", "backlight", "power"], [], true],
  m5stopwatch: [["display", "i2c", "hold", "touch"], [], true],
  m5timercam: [["i2c", "power"], [], false],
  m5tough: [["display", "shared_sd", "i2c", "hold"], [12, 32, 33], false],
  m5toughc5: [["display", "i2c", "touch"], [], true],
  m5unit_c6l: [["display", "i2c", "hold", "power"], [], true],
  m5unitpoep4: [[], [], false],
  m5unitpoep4x: [[], [], false],
  m5vameter: [["display", "i2c", "hold", "backlight"], [], true],
};

test("existing boards preserve their generated wiring and specification selection", async () => {
  const target = JSON.parse(await fs.readFile(new URL("../targets.json", import.meta.url), "utf8")).m5gfx_board_desc;
  for (const [id, value] of Object.entries(expected)) {
    const board = JSON.parse(await fs.readFile(new URL(`../boards/${id}.json`, import.meta.url), "utf8"));
    const mapping = m5gfxBoardMapping(target, board);
    const actual = mapping ? [mapping.fields, mapping.probePins, Boolean(mapping.specsOutput)] : null;
    assert.deepEqual(actual, value, `${id}: review changes to its recorded fields, probe pins and specification selection`);
  }
});
