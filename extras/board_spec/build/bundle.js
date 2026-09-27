import { promises as fs } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");

function safeJson(value) {
  return JSON.stringify(value).replace(/</g, "\\u003c");
}

function safeScript(source) {
  return source.replace(/<\/script/gi, "<\\/script");
}

function flattenModule(source) {
  return source
    .replace(/^\s*import\s+.*;\s*$/gm, "")
    .replace(/^export\s+(?=(?:async\s+)?(?:function|class|const|let|var))/gm, "")
    .replace(/^export\s*\{[^}]*\};?\s*$/gm, "");
}

function topLevelNames(source) {
  const names = [];
  const declaration = /^(?:export\s+)?(?:(?:async\s+)?(?:function|class)\s+([A-Za-z_$][\w$]*)|(?:const|let|var)\s+([A-Za-z_$][\w$]*))/gm;
  for (const match of source.matchAll(declaration)) names.push(match[1] ?? match[2]);
  return names;
}

export function assertUniqueFlattenedNames(sources) {
  const owners = new Map();
  for (const [label, source] of sources) {
    for (const name of topLevelNames(source)) {
      const previous = owners.get(name);
      if (previous) throw new Error(`bundle top-level name collision: ${name} (${previous}, ${label})`);
      owners.set(name, label);
    }
  }
}

function replaceMarker(html, marker, replacement) {
  if (!html.includes(marker)) throw new Error(`bundle marker missing: ${marker}`);
  return html.replace(marker, () => replacement);
}

async function readJsonDirectory(directory) {
  const names = (await fs.readdir(directory)).filter((name) => name.endsWith(".json")).sort();
  const entries = [];
  for (const name of names) {
    const value = JSON.parse(await fs.readFile(path.join(directory, name), "utf8"));
    entries.push([value.id ?? path.basename(name, ".json"), value]);
  }
  return Object.fromEntries(entries);
}

export async function renderBundle() {
  let html = await fs.readFile(path.join(root, "editor/index.html"), "utf8");
  const css = await fs.readFile(path.join(root, "editor/editor.css"), "utf8");
  const editorNames = ["provenance.js", "editor.js"];
  const editorSources = await Promise.all(editorNames.map((name) => fs.readFile(path.join(root, "editor", name), "utf8")));
  assertUniqueFlattenedNames(editorNames.map((name, index) => [`editor/${name}`, editorSources[index]]));
  const moduleNames = ["model.js", "ctypes.js", "emit/m5unified_pin_table.js", "emit/m5gfx_board_wiring.js", "pintable_roles.js", "choices.js", "owners.js", "parts.js", "targets.js", "compose.js", "derive/sd.js", "resolve.js", "format.js", "validate.js"];
  const moduleSources = await Promise.all(moduleNames.map((name) => fs.readFile(path.join(root, "lib", name), "utf8")));
  assertUniqueFlattenedNames(moduleNames.map((name, index) => [`lib/${name}`, moduleSources[index]]));
  const modules = moduleNames.map((name, index) => `// lib/${name}\n${flattenModule(moduleSources[index])}`);

  const schema = JSON.parse(await fs.readFile(path.join(root, "schema/board.schema.json"), "utf8"));
  const chips = await readJsonDirectory(path.join(root, "chips"));
  const connectorTypes = await readJsonDirectory(path.join(root, "connector_types"));
  const parts = await readJsonDirectory(path.join(root, "parts"));
  const accessories = await readJsonDirectory(path.join(root, "accessories"));
  const targets = JSON.parse(await fs.readFile(path.join(root, "targets.json"), "utf8"));
  const boards = await readJsonDirectory(path.join(root, "boards"));
  const data = [
    `<script type="application/json" id="board-spec-schema">${safeJson(schema)}</script>`,
    `<script type="application/json" id="board-spec-chips">${safeJson(chips)}</script>`,
    `<script type="application/json" id="board-spec-connector-types">${safeJson(connectorTypes)}</script>`,
    `<script type="application/json" id="board-spec-parts">${safeJson(parts)}</script>`,
    `<script type="application/json" id="board-spec-accessories">${safeJson(accessories)}</script>`,
    `<script type="application/json" id="board-spec-targets">${safeJson(targets)}</script>`,
    `<script type="application/json" id="board-spec-boards">${safeJson(boards)}</script>`,
  ].join("\n");

  html = replaceMarker(html, "<!-- @inline style -->", `<style>\n${css}\n</style>`);
  html = replaceMarker(html, "<!-- @inline data -->", data);
  html = replaceMarker(html, "<!-- @inline lib -->", `<script>\n${safeScript(modules.join("\n\n"))}\n</script>`);
  html = replaceMarker(html, "<!-- @inline app -->", `<script>\n(() => {\n${safeScript(editorSources.map(flattenModule).join("\n\n"))}\n})();\n</script>`);
  return html.endsWith("\n") ? html : `${html}\n`;
}

export async function writeBundle() {
  const output = path.join(root, "dist/board_spec_editor.html");
  await fs.mkdir(path.dirname(output), { recursive: true });
  await fs.writeFile(output, await renderBundle());
  return output;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  writeBundle()
    .then((output) => console.log(`wrote ${path.relative(root, output)}`))
    .catch((failure) => {
      console.error(failure.message);
      process.exitCode = 1;
    });
}
