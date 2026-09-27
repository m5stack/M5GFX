import { assertBoard, clone, parseJson } from "../lib/model.js";
import { choiceSlots, revisionSelection } from "../lib/choices.js";
import { validateBoard } from "../lib/validate.js";
import { resolveBoard } from "../lib/resolve.js";
import { formatBoard } from "../lib/format.js";
import { pinChangeLayer } from "./provenance.js";

const schema = readEmbedded("board-spec-schema");
const chips = readEmbedded("board-spec-chips");
const connectorTypes = readEmbedded("board-spec-connector-types");
const parts = readEmbedded("board-spec-parts");
const accessories = readEmbedded("board-spec-accessories");
const targets = readEmbedded("board-spec-targets");
const embeddedBoards = readEmbedded("board-spec-boards");
const messages = readEmbedded("board-spec-i18n");
const languageStorageKey = "m5-board-spec-language:v1";
let comboboxSerial = 0;

const elements = {
  boardSelect: document.querySelector("#board-select"),
  languageSelect: document.querySelector("#language-select"),
  fileInput: document.querySelector("#file-input"),
  download: document.querySelector("#download-button"),
  discard: document.querySelector("#discard-button"),
  dropZone: document.querySelector("#drop-zone"),
  optionPanel: document.querySelector("#option-panel"),
  viewTabs: document.querySelector("#view-tabs"),
  configurationView: document.querySelector("#configuration-view"),
  revisionsView: document.querySelector("#revisions-view"),
  ownersView: document.querySelector("#owners-view"),
  configurationList: document.querySelector("#configuration-list"),
  revisionEditor: document.querySelector("#revision-editor"),
  ownerEditor: document.querySelector("#owner-editor"),
  modeNote: document.querySelector("#mode-note"),
  editBase: document.querySelector("#edit-base-button"),
  saveState: document.querySelector("#save-state"),
  tableHead: document.querySelector("#gpio-columns"),
  tableBody: document.querySelector("#gpio-table tbody"),
  validationCount: document.querySelector("#validation-count"),
  validationList: document.querySelector("#validation-list"),
  connectorTabs: document.querySelector("#connector-tabs"),
  connectorViewToggle: document.querySelector("#connector-view-toggle"),
  connectorDiagram: document.querySelector("#connector-diagram"),
  connectorEditor: document.querySelector("#connector-editor"),
  connectorUndo: document.querySelector("#connector-undo"),
  relatedList: document.querySelector("#related-list"),
  toast: document.querySelector("#toast"),
};

const state = {
  documents: new Map(),
  currentId: null,
  selectedOptions: new Set(),
  runtimeSelections: new Map(),
  compositionEntries: new Map(),
  activeView: "configuration",
  ownerKey: null,
  compareRevisions: ["", ""],
  connectorId: null,
  connectorPosition: null,
  connectorView: "table",
  connectorEditorTab: "gpio",
  language: initialLanguage(),
  undo: null,
  toastTimer: null,
};

class SearchCombobox {
  constructor(root, options, onSelect, readonlyReason = "") {
    this.root = root;
    this.input = root.querySelector("input");
    this.list = root.querySelector("[role=listbox]");
    this.options = options.map((option) => typeof option === "string" ? { value: option, label: option, search: option } : option);
    this.onSelect = onSelect;
    this.filtered = [];
    this.activeIndex = 0;
    this.readonlyReason = readonlyReason;
    this.instanceId = ++comboboxSerial;
    if (readonlyReason) applyControlLock(this.input, readonlyReason);
    this.input.addEventListener("focus", () => this.open());
    this.input.addEventListener("input", () => this.open());
    this.input.addEventListener("keydown", (event) => this.keydown(event));
    this.input.addEventListener("blur", () => setTimeout(() => this.close(), 0));
    this.list.addEventListener("pointerdown", (event) => event.preventDefault());
    this.list.addEventListener("click", (event) => {
      const option = event.target.closest("[data-combobox-value]");
      if (option) this.select(decodeURIComponent(option.dataset.comboboxValue));
    });
  }

  matches(option, query) {
    const terms = query.toLowerCase().split(/[\s_.:]+/).filter(Boolean);
    const searchable = `${option.label} ${option.search ?? ""}`.toLowerCase();
    return terms.every((term) => searchable.includes(term));
  }

  open() {
    if (this.readonlyReason) {
      showToast(this.readonlyReason);
      return;
    }
    this.filtered = this.options.filter((option) => this.matches(option, this.input.value));
    this.activeIndex = 0;
    this.render();
    this.root.classList.add("open");
    this.input.setAttribute("aria-expanded", "true");
  }

  close() {
    this.root.classList.remove("open");
    this.input.setAttribute("aria-expanded", "false");
    this.input.removeAttribute("aria-activedescendant");
    this.input.value = "";
  }

  move(delta) {
    if (!this.filtered.length) return;
    this.activeIndex = (this.activeIndex + delta + this.filtered.length) % this.filtered.length;
    this.render();
    this.list.querySelector(".active")?.scrollIntoView({ block: "nearest" });
  }

  select(value) {
    const option = this.options.find((item) => item.value === value);
    if (!option) return;
    this.close();
    this.onSelect(option.value);
  }

  keydown(event) {
    if (event.key === "Escape") {
      event.preventDefault();
      this.close();
    } else if (event.key === "ArrowDown" || event.key === "ArrowUp") {
      event.preventDefault();
      if (!this.root.classList.contains("open")) this.open();
      else this.move(event.key === "ArrowDown" ? 1 : -1);
    } else if (event.key === "Enter") {
      event.preventDefault();
      if (!this.root.classList.contains("open")) this.open();
      else if (this.filtered.length) this.select(this.filtered[this.activeIndex].value);
    }
  }

  render() {
    if (!this.filtered.length) {
      this.list.innerHTML = `<li class="combobox-empty">${escapeHtml(t("combobox.empty"))}</li>`;
      return;
    }
    this.list.innerHTML = this.filtered.map((option, index) => (
      `<li><button type="button" role="option" id="combobox-${this.instanceId}-option-${index}" class="${index === this.activeIndex ? "active" : ""}" aria-selected="${index === this.activeIndex}" data-combobox-value="${encodeURIComponent(option.value)}">${escapeHtml(option.label)}</button></li>`
    )).join("");
    this.input.setAttribute("aria-activedescendant", `combobox-${this.instanceId}-option-${this.activeIndex}`);
  }
}

function readEmbedded(id) {
  const node = document.getElementById(id);
  if (!node) throw new Error(`missing embedded data: ${id}`);
  return JSON.parse(node.textContent);
}

function initialLanguage() {
  try {
    const stored = localStorage.getItem(languageStorageKey);
    if (["en", "zh", "ja"].includes(stored)) return stored;
  } catch (failure) {
    console.warn("language preference is unavailable", failure);
  }
  const browserLanguage = navigator.language?.toLowerCase() ?? "";
  if (browserLanguage.startsWith("zh")) return "zh";
  if (browserLanguage.startsWith("ja")) return "ja";
  return "en";
}

function t(key, variables = {}, fallback = key) {
  const template = messages[key]?.[state.language] ?? messages[key]?.en ?? fallback;
  return String(template).replace(/\{([a-zA-Z0-9_]+)\}/g, (match, name) => variables[name] ?? match);
}

function applyStaticTranslations() {
  document.documentElement.lang = state.language === "zh" ? "zh-CN" : state.language;
  document.title = t("app.title");
  document.querySelectorAll("[data-i18n]").forEach((node) => { node.textContent = t(node.dataset.i18n); });
  document.querySelectorAll("[data-i18n-aria-label]").forEach((node) => { node.setAttribute("aria-label", t(node.dataset.i18nAriaLabel)); });
  elements.languageSelect.value = state.language;
}

function saveLanguage(language) {
  try {
    localStorage.setItem(languageStorageKey, language);
  } catch (failure) {
    console.warn("language preference could not be saved", failure);
  }
}

function validationMessage(item) {
  return messages[`validation.${item.id}`]?.[state.language] ?? messages[`validation.${item.id}`]?.en ?? item.message;
}

function escapeHtml(value) {
  return String(value ?? "")
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#39;");
}

function storageKey(id) {
  return `m5-board-spec-draft:v1:${id}`;
}

function readDraft(board) {
  try {
    const text = localStorage.getItem(storageKey(board.id));
    if (!text) return null;
    const stored = parseJson(text, `draft:${board.id}`);
    const draft = stored.board ?? stored;
    assertBoard(draft);
    if (draft.id !== board.id || !chips[draft.chip]) return null;
    return {
      board: stored.board && !stored.dirty ? clone(board) : draft,
      dirty: stored.board ? Boolean(stored.dirty) : true,
      expandedGPIO: stored.board && stored.expandedGPIO != null ? String(stored.expandedGPIO) : null,
    };
  } catch {
    return null;
  }
}

function writeDraft(entry) {
  const stored = { board: entry.base, dirty: entry.dirty, expandedGPIO: entry.expandedGPIO };
  try { localStorage.setItem(storageKey(entry.base.id), JSON.stringify(stored)); } catch { /* storage is optional */ }
}

function removeDraft(id) {
  try { localStorage.removeItem(storageKey(id)); } catch { /* storage is optional */ }
}

function addEmbeddedBoards() {
  for (const board of Object.values(embeddedBoards)) {
    const original = clone(board);
    const draft = readDraft(board);
    state.documents.set(board.id, {
      base: draft?.board ?? clone(board),
      original,
      dirty: draft?.dirty ?? false,
      expandedGPIO: draft?.expandedGPIO ?? null,
      source: "embedded",
    });
  }
  for (const id of state.documents.keys()) {
    state.compositionEntries.set(id, clone(targets.m5unified_pin_table?.compositions?.[id]?.default?.accessories ?? []));
    const board = state.documents.get(id).base;
    state.runtimeSelections.set(id, Object.fromEntries(choiceSlots(board).filter(([, device]) => device.selected_by === "runtime").map(([slot, device]) => [slot, device.default])));
  }
  state.currentId = state.documents.has("m5stack_core2") ? "m5stack_core2" : [...state.documents.keys()][0];
}

function currentEntry() {
  return state.documents.get(state.currentId);
}

function currentBase() {
  return currentEntry().base;
}

function shownBoard() {
  const revision = currentBase().revisions?.find((item) => item.id === [...state.selectedOptions][0]);
  const entries = state.compositionEntries.get(state.currentId) ?? [];
  const runtime = state.runtimeSelections.get(state.currentId) ?? {};
  if (!state.selectedOptions.size && !entries.length && !Object.keys(runtime).length) return currentBase();
  return resolveBoard(currentBase(), { ...revisionSelection(currentBase(), revision), ...runtime }, connectorTypes, {
    chip: chips[currentBase().chip], parts, accessories,
    composition: entries.length ? { accessories: entries } : null,
    allow_origins: targets.m5unified_pin_table?.allow_origins,
  });
}

function isResolvedMode() {
  return state.selectedOptions.size > 0
    || (state.compositionEntries.get(state.currentId)?.length ?? 0) > 0
    || Object.keys(state.runtimeSelections.get(state.currentId) ?? {}).length > 0;
}

function clearResolvedMode() {
  state.selectedOptions.clear();
  state.runtimeSelections.set(state.currentId, {});
  state.compositionEntries.set(state.currentId, []);
  state.ownerKey = null;
}

function readonlyAttributes(reason) {
  if (!reason) return "";
  const escaped = escapeHtml(reason);
  return ` aria-disabled="true" data-readonly-reason="${escaped}" title="${escaped}"`;
}

function readonlyNote(reason) {
  return reason ? `<small class="readonly-note" role="note">${escapeHtml(reason)}</small>` : "";
}

function applyControlLock(control, reason = "") {
  if (reason) {
    control.setAttribute("aria-disabled", "true");
    control.dataset.readonlyReason = reason;
    control.title = reason;
  } else {
    control.removeAttribute("aria-disabled");
    delete control.dataset.readonlyReason;
    control.removeAttribute("title");
  }
}

function resolveStage(board, selection, composition = null) {
  return resolveBoard(board, selection, connectorTypes, {
    chip: chips[board.chip], parts, accessories,
    ...(composition ? { composition } : {}),
    allow_origins: targets.m5unified_pin_table?.allow_origins,
  });
}

// Editing a resolved row is safe only while every selected layer leaves the
// complete base pin record untouched. This deliberately fails closed when a
// future resolver layer changes pins in a way the editor cannot classify.
function pinReadonlyReasons(shown) {
  const reasons = new Map();
  if (!isResolvedMode()) return reasons;
  const base = currentBase();
  const revision = base.revisions?.find((item) => item.id === [...state.selectedOptions][0]);
  const runtime = state.runtimeSelections.get(state.currentId) ?? {};
  const entries = state.compositionEntries.get(state.currentId) ?? [];
  const defaultSelection = revisionSelection(base);
  const revisionBoard = resolveStage(base, revisionSelection(base, revision));
  const defaultBoard = resolveStage(base, defaultSelection);
  const runtimeBoard = resolveStage(base, { ...revisionSelection(base, revision), ...runtime });
  const accessoryNames = new Map(entries.map((entry) => [entry.id, accessories[entry.id]?.name ?? entry.id]));
  const runtimeChoice = Object.entries(runtime).map(([slot, choice]) => `${slot}: ${choice}`).join(", ");

  const gpios = new Set([...Object.keys(base.pins ?? {}), ...Object.keys(shown.pins ?? {})]);
  for (const gpio of gpios) {
    const layer = pinChangeLayer({
      base: base.pins?.[gpio],
      defaults: defaultBoard.pins?.[gpio],
      revision: revisionBoard.pins?.[gpio],
      runtime: runtimeBoard.pins?.[gpio],
      shown: shown.pins?.[gpio],
    });
    if (layer === null) continue;
    if (layer === "accessory") {
      const prefix = (shown.pins?.[gpio]?.roles ?? []).map((role) => role.split("/", 1)[0]).find((id) => roleHasSource(shown.pins?.[gpio], id) && accessoryNames.has(id));
      reasons.set(gpio, t("readonly.accessory", { accessory: accessoryNames.get(prefix) ?? [...accessoryNames.values()][0] ?? t("configuration.accessories") }));
    } else if (layer === "runtime") {
      reasons.set(gpio, t("readonly.runtime", { choice: runtimeChoice || t("options.none") }));
    } else if (layer === "revision") {
      reasons.set(gpio, t("readonly.revision", { revision: revision?.name ?? t("options.none") }));
    } else {
      reasons.set(gpio, t("readonly.configuration"));
    }
  }
  return reasons;
}

function roleHasSource(pin, source) {
  return (pin?.roles ?? []).some((role) => role.startsWith(`${source}/`));
}

function markDirty() {
  const entry = currentEntry();
  entry.dirty = true;
  writeDraft(entry);
}

function clearVerification(object, field) {
  if (object.verified) {
    delete object.verified[field];
    if (Object.keys(object.verified).length === 0) delete object.verified;
  }
  if (object.measured_by) {
    delete object.measured_by[field];
    if (Object.keys(object.measured_by).length === 0) delete object.measured_by;
  }
}

function editablePin(gpio) {
  const board = currentBase();
  if (!board.pins[gpio]) board.pins[gpio] = { roles: [] };
  if (!Array.isArray(board.pins[gpio].roles)) board.pins[gpio].roles = [];
  return board.pins[gpio];
}

function cleanupPin(gpio) {
  const pin = currentBase().pins[gpio];
  if (!pin) return;
  if ((pin.roles?.length ?? 0) === 0) delete pin.roles;
  if (Object.keys(pin).length === 0) delete currentBase().pins[gpio];
  else if (!pin.roles) pin.roles = [];
}

function schemaSignals(definitionName) {
  return schema.definitions[definitionName]?.["x-signals"] ?? {};
}

function allRoleOptions(board, currentGPIO = null) {
  const roles = [];
  const busSignals = schemaSignals("busKind");
  for (const [id, bus] of Object.entries(board.buses ?? {})) {
    const allowed = new Set(busSignals[bus.kind] ?? []);
    for (const signal of bus.signals ?? []) if (allowed.has(signal)) roles.push(`bus:${id}.${signal}`);
  }

  for (const [id, device] of Object.entries(board.devices ?? {})) {
    for (const signal of deviceSignals(device)) roles.push(`dev:${id}.${signal}`);
  }

  for (const [id, connector] of Object.entries(board.connectors ?? {})) {
    const type = resolveConnectorType(connectorTypes, connector.type);
    const positions = new Set((type?.positions ?? []).filter((position) => !Object.hasOwn(type.fixed_positions ?? {}, position.id) && !Object.hasOwn(type.default_positions ?? {}, position.id) && !Object.hasOwn(connector.positions ?? {}, position.id)).map((position) => position.id));
    for (const pin of Object.values(board.pins ?? {})) {
      for (const role of pin.roles ?? []) {
        const match = new RegExp(`^conn:${id.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")}\\.([a-z0-9_]+)$`).exec(role);
        if (match) positions.add(match[1]);
      }
    }
    for (const position of positions) roles.push(`conn:${id}.${position}`);
  }
  const assignedElsewhere = new Set();
  for (const [gpio, pin] of Object.entries(board.pins ?? {})) if (String(gpio) !== String(currentGPIO)) for (const role of pin.roles ?? []) {
    const connector = /^conn:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
    if (!connector || !(board.connectors?.[connector[1]]?.multi_gpio ?? []).includes(connector[2])) assignedElsewhere.add(role);
  }
  for (const device of Object.values(board.devices ?? {})) {
    for (const pin of Object.values(device.pins ?? {})) for (const role of pin.roles ?? []) assignedElsewhere.add(role);
    for (const choice of Object.values(device.choices ?? {})) for (const pin of Object.values(choice.pins ?? {})) for (const role of pin.roles ?? []) assignedElsewhere.add(role);
  }
  return [...new Set(roles)].filter((role) => !assignedElsewhere.has(role)).sort((a, b) => a.localeCompare(b));
}

function roleDisplay(board, role) {
  const slash = role.indexOf("/");
  const source = slash === -1 ? "" : role.slice(0, slash);
  const plain = slash === -1 ? role : role.slice(slash + 1);
  const match = /^dev:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(plain);
  let alias = "";
  if (match && board.devices?.[match[1]]?.kind === "sd") {
    const spi = parts[board.devices[match[1]].part]?.spi_alias?.[match[2]];
    if (spi) alias = ` (${spi === "cs" ? "CS" : spi.toUpperCase()})`;
  }
  return { plain: `${plain}${alias}`, source };
}

function roleChip(board, role, compact = false, removable = "") {
  const display = roleDisplay(board, role);
  return `<span class="role-chip${compact ? " compact-chip" : ""}"><span>${escapeHtml(display.plain)}</span>${display.source ? `<small>${escapeHtml(display.source)}</small>` : ""}${removable}</span>`;
}

function verificationSummary(pin) {
  const fields = ["roles", "pull", "note"].filter((field) => (
    Object.prototype.hasOwnProperty.call(pin, field) && (field !== "roles" || pin.roles.length > 0)
  ));
  if (!fields.length) return "<span>—</span>";
  return fields.map((field) => {
    const status = pin.verified?.[field] ?? "datasheet";
    return `<span><code>${escapeHtml(field)}</code>: ${escapeHtml(status)}</span>`;
  }).join("");
}

function compactVerification(pin) {
  const fields = ["roles", "pull", "note"].filter((field) => (
    Object.prototype.hasOwnProperty.call(pin, field) && (field !== "roles" || pin.roles.length > 0)
  ));
  if (!fields.length) return "—";
  return fields.map((field) => `${field}:${pin.verified?.[field] ?? "datasheet"}`).join(" · ");
}

function gpioForError(item, board) {
  let match = /^\/pins\/(\d+)/.exec(item.path);
  if (match) return match[1];
  match = /^\/devices\/([^/]+)\/signals\/([^/]+)/.exec(item.path);
  if (match) {
    const endpoint = board.devices?.[match[1]]?.signals?.[match[2]];
    return /^gpio:(\d+)$/.exec(endpoint ?? "")?.[1] ?? null;
  }
  match = /^\/buses\/([^/]+)/.exec(item.path);
  if (match) return findRoleGPIO(board, `bus:${match[1]}.`)[0] ?? null;
  match = /^\/connectors\/([^/]+)\/positions\/([^/]+)/.exec(item.path);
  if (match) return findRoleGPIO(board, `conn:${match[1]}.${match[2]}`)[0] ?? null;
  return null;
}

function findRoleGPIO(board, prefix) {
  const result = [];
  for (const [gpio, pin] of Object.entries(board.pins ?? {})) {
    if ((pin.roles ?? []).some((role) => role === prefix || role.startsWith(prefix))) result.push(gpio);
  }
  return result.sort((a, b) => Number(a) - Number(b));
}

function validationFor(board, resolved = isResolvedMode()) {
  const chip = chips[board.chip];
  if (!chip) return [{ id: "E_REF_MISSING", path: "/chip", message: `embedded chip not found: ${board.chip}` }];
  return validateBoard(board, { schema, chip, connectorTypes, parts, target: targets.m5gfx_board_desc, resolved });
}

function renderBoardSelect() {
  const entries = [...state.documents.entries()].sort(([, a], [, b]) => a.base.name.localeCompare(b.base.name));
  elements.boardSelect.innerHTML = entries.map(([id, entry]) => {
    const suffix = entry.dirty ? " •" : "";
    return `<option value="${escapeHtml(id)}"${id === state.currentId ? " selected" : ""}>${escapeHtml(entry.base.name)}${suffix}</option>`;
  }).join("");
}

function renderOptions() {
  const revisions = currentBase().revisions ?? [];
  const selectedRevision = [...state.selectedOptions][0] ?? "";
  const revisionHtml = `<label>${escapeHtml(t("options.revision"))}<select data-display-revision><option value="">${escapeHtml(t("options.none"))}</option>${revisions.map((revision) => `<option value="${escapeHtml(revision.id)}"${revision.id === selectedRevision ? " selected" : ""}>${escapeHtml(revision.name)}</option>`).join("")}</select></label>`;
  const runtime = state.runtimeSelections.get(state.currentId) ?? {};
  const runtimeHtml = choiceSlots(currentBase()).filter(([, device]) => device.selected_by === "runtime").map(([slot, device]) => (
    `<label>${escapeHtml(t("options.runtime"))}: ${escapeHtml(slot.replaceAll("_", " ").toUpperCase())}<select data-display-runtime="${escapeHtml(slot)}"><option value=""${runtime[slot] === undefined ? " selected" : ""}>${escapeHtml(t("options.none"))}</option>${Object.keys(device.choices).map((choice) => `<option value="${escapeHtml(choice)}"${choice === runtime[slot] ? " selected" : ""}>${escapeHtml(choice)}</option>`).join("")}</select></label>`
  )).join("");
  elements.optionPanel.innerHTML = `<fieldset><legend>${escapeHtml(t("options.legend"))}</legend>${revisionHtml}${runtimeHtml}</fieldset>`;
  const revision = revisions.find((item) => item.id === selectedRevision);
  const choices = Object.entries({ ...revisionSelection(currentBase(), revision), ...runtime }).map(([slot, choice]) => `${slot}: ${choice}`).join(", ") || t("options.none");
  elements.modeNote.textContent = isResolvedMode()
    ? t("mode.resolved", { board: currentBase().name, revision: revision?.name ?? t("options.none"), choices })
    : t("mode.base");
}

function renderTable(board, errors) {
  const chip = chips[board.chip];
  const schemaColumns = schema["x-editor"]?.pinColumns ?? [];
  const compactIds = ["gpio", "roles", "pull", "verified", "validation"];
  const columns = compactIds.map((id) => schemaColumns.find((column) => column.id === id) ?? { id, label: id });
  const columnLabels = {
    gpio: t("column.gpio"), roles: t("column.roles"), pull: t("column.pull"),
    verified: t("column.verified"), validation: t("column.validation"),
  };
  elements.tableHead.innerHTML = columns.map((column) => `<th>${escapeHtml(columnLabels[column.id] ?? column.label)}</th>`).join("");
  const errorsByGPIO = new Map();
  for (const item of errors) {
    const gpio = gpioForError(item, board);
    if (gpio === null) continue;
    if (!errorsByGPIO.has(gpio)) errorsByGPIO.set(gpio, []);
    errorsByGPIO.get(gpio).push(item);
  }
  const readonlyReasons = pinReadonlyReasons(board);
  const rows = [];
  for (let gpio = 0; gpio < chip.gpio_count; gpio += 1) {
    const key = String(gpio);
    const pin = board.pins?.[key] ?? { roles: [] };
    const readonlyReason = readonlyReasons.get(key) ?? "";
    const readonly = readonlyAttributes(readonlyReason);
    const expanded = currentEntry().expandedGPIO === key;
    const rowErrors = errorsByGPIO.get(key) ?? [];
    const classes = ["gpio-row", gpio % 2 ? "zebra-odd" : "zebra-even"];
    if (chip.reserved.includes(gpio)) classes.push("gpio-reserved");
    if (chip.input_only.includes(gpio)) classes.push("gpio-input");
    if (chip.strapping.includes(gpio)) classes.push("gpio-strapping");
    if (rowErrors.length) classes.push("row-error");
    if (expanded) classes.push("is-expanded");
    const allRoles = pin.roles ?? [];
    const compactRoles = allRoles.slice(0, 3).map((role) => roleChip(board, role, true)).join("");
    const overflow = allRoles.length > 3 ? `<span class="role-overflow">+${allRoles.length - 3}</span>` : "";
    const editableRoles = allRoles.map((role) => roleChip(board, role, false, `<button type="button" aria-label="${escapeHtml(t("detail.removeRole"))}" data-remove-role="${encodeURIComponent(role)}" data-gpio="${key}"${readonly || ` title="${escapeHtml(t("detail.removeRole"))}"`}>×</button>`)).join("");
    const pull = pin.pull ?? "";
    const errorSummary = rowErrors.length
      ? `<span class="error-indicator" title="${escapeHtml(rowErrors.map((item) => item.id).join(", "))}">! ${rowErrors.length}</span>`
      : '<span class="ok-indicator">✓</span>';
    const cells = {
      gpio: `<td class="gpio-number">${key}</td>`,
      roles: `<td><div class="compact-roles">${compactRoles || "<span>—</span>"}${overflow}</div></td>`,
      pull: `<td>${escapeHtml(pull || "—")}</td>`,
      verified: `<td class="compact-verified">${escapeHtml(compactVerification(pin))}</td>`,
      validation: `<td>${errorSummary}</td>`,
    };
    const renderedCells = columns.map((column) => cells[column.id] ?? "<td>—</td>").join("");
    rows.push(`<tr id="gpio-${key}" data-gpio-row="${key}" aria-expanded="${expanded}" class="${classes.join(" ")}">${renderedCells}</tr>`);
    if (expanded) {
      const related = relatedObjectsForGPIO(board, key);
      rows.push(`<tr class="gpio-detail-row"><td colspan="${columns.length}"><div class="gpio-detail">
        <section class="detail-roles"><h3>${escapeHtml(t("column.roles"))}</h3><div class="role-list">${editableRoles || "<span>—</span>"}</div>
          <div class="search-combobox" data-role-combobox="${key}"><input type="text" role="combobox" aria-autocomplete="list" aria-expanded="false" placeholder="${escapeHtml(t("detail.searchRole"))}" autocomplete="off"${readonly}><ul role="listbox"></ul></div>${readonlyNote(readonlyReason)}
        </section>
        <label>${escapeHtml(t("column.pull"))}<select class="cell-select" data-pull data-gpio="${key}"${readonly}><option value=""${pull === "" ? " selected" : ""}>${escapeHtml(t("detail.empty"))}</option><option value="up"${pull === "up" ? " selected" : ""}>up</option><option value="down"${pull === "down" ? " selected" : ""}>down</option><option value="none"${pull === "none" ? " selected" : ""}>none</option></select>${readonlyNote(readonlyReason)}</label>
        <label>${escapeHtml(t("column.note"))}<input class="cell-input note-input" data-note data-gpio="${key}" type="text" value="${escapeHtml(pin.note ?? "")}"${readonly}>${readonlyNote(readonlyReason)}</label>
        <section><h3>${escapeHtml(t("column.verified"))}</h3><div class="verified">${verificationSummary(pin)}</div></section>
        <section><h3>${escapeHtml(t("detail.related"))}</h3><div class="detail-related">${related || `<span>${escapeHtml(t("common.none"))}</span>`}</div></section>
      </div></td></tr>`);
    }
  }
  elements.tableBody.innerHTML = rows.join("");
  const combobox = elements.tableBody.querySelector("[data-role-combobox]");
  if (combobox) {
    const gpio = combobox.dataset.roleCombobox;
    const assigned = new Set(board.pins?.[gpio]?.roles ?? []);
    new SearchCombobox(combobox, allRoleOptions(board, gpio).filter((role) => !assigned.has(role)).map((role) => ({ value: role, label: roleDisplay(board, role).plain, search: role })), (role) => addRole(gpio, role), readonlyReasons.get(gpio));
  }
}

function renderValidation(board, errors) {
  elements.validationCount.textContent = t("validation.count", { count: errors.length });
  if (!errors.length) {
    elements.validationList.innerHTML = `<div class="validation-ok">${escapeHtml(t("validation.ok"))}</div>`;
    return;
  }
  elements.validationList.innerHTML = `<ul class="error-list">${errors.map((item, index) => {
    const gpio = gpioForError(item, board);
    const reason = gpio === null ? t("unavailable.noGPIO") : "";
    return `<li><button type="button" data-error-index="${index}"${readonlyAttributes(reason)}><strong>${escapeHtml(item.id)}</strong>: ${escapeHtml(validationMessage(item))}<code>${escapeHtml(item.path || "/")}</code></button></li>`;
  }).join("")}</ul>`;
}

function objectAssignments(board, prefix, id, object) {
  const pins = new Set(findRoleGPIO(board, `${prefix}:${id}.`));
  if (prefix === "dev") {
    for (const endpoint of Object.values(object.signals ?? {})) {
      const gpio = /^gpio:(\d+)$/.exec(endpoint)?.[1];
      if (gpio !== undefined) pins.add(gpio);
    }
  }
  if (prefix === "conn") {
    for (const endpoint of Object.values(object.positions ?? {})) {
      const gpio = /^gpio:(\d+)$/.exec(endpoint)?.[1];
      if (gpio !== undefined) pins.add(gpio);
    }
  }
  return [...pins].sort((a, b) => Number(a) - Number(b));
}

function relatedObjectsForGPIO(board, gpio) {
  const groups = [["buses", "bus"], ["devices", "dev"], ["connectors", "conn"]];
  const links = [];
  for (const [field, prefix] of groups) {
    for (const [id, object] of Object.entries(board[field] ?? {})) {
      if (objectAssignments(board, prefix, id, object).includes(gpio)) {
        links.push(`<button type="button" data-target-object="${field}:${escapeHtml(id)}">${escapeHtml(prefix)}:${escapeHtml(id)}</button>`);
      }
    }
  }
  return links.join("");
}

function objectSummary(field, object) {
  const attributes = [];
  const add = (label, value) => {
    if (value !== undefined && value !== null && value !== "") attributes.push(`<span><strong>${escapeHtml(label)}</strong> ${escapeHtml(value)}</span>`);
  };
  add(t("object.kind"), object.kind);
  if (field === "buses") {
    add(t("object.signals"), (object.signals ?? []).join(" / "));
    add(t("object.host"), object.preferred_host);
    add(t("object.freq"), object.freq);
  } else if (field === "devices") {
    add(t("object.part"), object.part);
    add(t("object.bus"), object.bus);
    add(t("object.addr"), object.i2c_addr);
    const signals = Object.entries(object.signals ?? {}).map(([name, endpoint]) => `${name}→${endpoint}`).join(" / ");
    add(t("object.signals"), signals);
  } else {
    add(t("object.type"), object.type);
    add(t("object.standard"), object.standard);
  }
  return attributes.join("") || `<span>${escapeHtml(t("object.noAttributes"))}</span>`;
}

function endpointLabel(endpoint) {
  const values = Array.isArray(endpoint) ? endpoint : [endpoint];
  return values.filter(Boolean).map((value) => {
    const gpio = /^gpio:(\d+)$/.exec(value)?.[1];
    if (gpio !== undefined) return `G${gpio}`;
    const simple = /^(?:pwr|chip):(.+)$/.exec(value)?.[1];
    return (simple ?? value).toUpperCase();
  }).join("/") || "—";
}

function endpointGPIOs(endpoint) {
  const values = Array.isArray(endpoint) ? endpoint : [endpoint];
  return values.map((value) => /^gpio:(\d+)$/.exec(value ?? "")?.[1]).filter((value) => value !== undefined);
}

function connectorFunctionLabel(board, endpoint) {
  const signalNames = { sclk: "SCK", data_in: "DATA IN", data_out: "DATA OUT" };
  const labels = [];
  for (const gpio of endpointGPIOs(endpoint)) {
    for (const role of board.pins?.[gpio]?.roles ?? []) {
      const bus = /^bus:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
      if (bus) {
        const signal = signalNames[bus[2]] ?? bus[2].replaceAll("_", " ").toUpperCase();
        labels.push(/^main_/.test(bus[1]) ? signal : `${signal} (${bus[1]})`);
        continue;
      }
      const device = /^dev:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
      if (device) labels.push(`${device[1].replaceAll("_", " ").toUpperCase()} ${device[2].replaceAll("_", " ").toUpperCase()}`);
    }
  }
  return [...new Set(labels)].join(" / ");
}

function connectorTableEndpointLabel(endpoint, position) {
  const label = endpointLabel(endpoint);
  const standardName = position?.name?.toUpperCase();
  if (!endpointGPIOs(endpoint).length && standardName && label !== standardName && label !== "—") return `${label} (${standardName})`;
  return label;
}

function connectorPositionClasses(source, endpoint, positionId) {
  const sourceClass = source.fixed ? "fixed" : source.explicit || source.gpios?.length ? "explicit" : source.default ? "default" : "missing";
  const highlighted = currentEntry().expandedGPIO && endpointGPIOs(endpoint).includes(currentEntry().expandedGPIO) ? " gpio-highlight" : "";
  const selected = state.connectorPosition === positionId ? " selected" : "";
  return `${sourceClass}${highlighted}${selected}`;
}

function ensureConnectorSelection(board) {
  const ids = Object.keys(board.connectors ?? {});
  if (!ids.includes(state.connectorId)) {
    state.connectorId = ids[0] ?? null;
    state.connectorPosition = null;
  }
}

function connectorKeyMarkup(layout, width, height) {
  const length = 28;
  const depth = 10;
  if (layout.key === "top") return `<path class="connector-key" d="M ${width / 2 - length / 2} 2 h ${length} v ${depth} h -${length} z"></path>`;
  if (layout.key === "bottom") return `<path class="connector-key" d="M ${width / 2 - length / 2} ${height - 2} h ${length} v -${depth} h -${length} z"></path>`;
  if (layout.key === "left") return `<path class="connector-key" d="M 2 ${height / 2 - length / 2} v ${length} h ${depth} v -${length} z"></path>`;
  if (layout.key === "right") return `<path class="connector-key" d="M ${width - 2} ${height / 2 - length / 2} v ${length} h -${depth} v -${length} z"></path>`;
  return "";
}

function renderConnectorDiagram(board, connectorId) {
  const connector = board.connectors?.[connectorId];
  const type = resolveConnectorType(connectorTypes, connector?.type);
  if (!connector || !type) return `<div class="connector-empty">${escapeHtml(t("connector.noType"))}</div>`;
  const display = resolveConnectorPositions(board, connectorId, connectorTypes);
  const source = resolveConnectorPositions(currentBase(), connectorId, connectorTypes);
  const cellWidth = 92;
  const cellHeight = 62;
  const width = type.layout.cols * cellWidth + 24;
  const height = type.layout.rows * cellHeight + 24;
  const positions = type.positions.map((position) => {
    const column = type.layout.mirror ? type.layout.cols - 1 - position.col : position.col;
    const x = 12 + column * cellWidth + cellWidth / 2;
    const y = 12 + position.row * cellHeight + cellHeight / 2;
    const endpoint = display.positions[position.id];
    const sources = source.sources[position.id] ?? {};
    const classes = connectorPositionClasses(sources, endpoint, position.id);
    const label = position.label ?? (position.name ? position.name.toUpperCase() : "");
    const fill = position.color ? ` style="--position-color:${escapeHtml(position.color)}"` : "";
    return `<g class="connector-position ${classes}" role="button" tabindex="0" data-connector-position="${escapeHtml(position.id)}" transform="translate(${x} ${y})"${fill}>
      <title>${escapeHtml(`${position.id} ${label} ${endpointLabel(endpoint)}`)}</title>
      <circle r="25"></circle>
      <text class="position-id" y="-10">${escapeHtml(position.id)}</text>
      <text class="position-label" y="4">${escapeHtml(label)}</text>
      <text class="position-endpoint" y="18">${escapeHtml(endpointLabel(endpoint))}</text>
    </g>`;
  }).join("");
  return `<div class="connector-meta"><strong>${escapeHtml(type.name)}</strong><span>${escapeHtml(type.layout.view)}</span></div><svg viewBox="0 0 ${width} ${height}" role="img" aria-label="${escapeHtml(t("connector.pinoutAria", { id: connectorId }))}">${connectorKeyMarkup(type.layout, width, height)}${positions}</svg>`;
}

function renderConnectorTableCells(board, display, source, position, side = "single") {
  if (!position) return '<td colspan="3"></td>';
  const endpoint = display.positions[position.id];
  const classes = connectorPositionClasses(source.sources[position.id] ?? {}, endpoint, position.id);
  const data = `data-connector-position="${escapeHtml(position.id)}"`;
  const endpointText = escapeHtml(connectorTableEndpointLabel(endpoint, position));
  const functionText = escapeHtml(connectorFunctionLabel(board, endpoint));
  const positionText = escapeHtml(position.id);
  const color = position.color ? `<span class="connector-color" style="--position-color:${escapeHtml(position.color)}"></span>` : "";
  if (side === "left") return `<td ${data} class="connector-table-position pin-endpoint pin-left ${classes}" title="${endpointText}">${endpointText}</td><td ${data} class="connector-table-position pin-function pin-left ${classes}" title="${functionText}">${functionText}</td><td ${data} class="connector-table-position pin-number pin-left ${classes}" role="button" tabindex="0">${positionText}</td>`;
  if (side === "right") return `<td ${data} class="connector-table-position pin-number pin-right pin-divider ${classes}" role="button" tabindex="0">${positionText}</td><td ${data} class="connector-table-position pin-function pin-right ${classes}" title="${functionText}">${functionText}</td><td ${data} class="connector-table-position pin-endpoint pin-right ${classes}" title="${endpointText}">${endpointText}</td>`;
  return `<td ${data} class="connector-table-position pin-number ${classes}" role="button" tabindex="0">${color}${positionText}</td><td ${data} class="connector-table-position pin-function ${classes}" title="${functionText}">${functionText}</td><td ${data} class="connector-table-position pin-endpoint ${classes}" title="${endpointText}">${endpointText}</td>`;
}

function renderConnectorTable(board, connectorId) {
  const connector = board.connectors?.[connectorId];
  const type = resolveConnectorType(connectorTypes, connector?.type);
  if (!connector || !type) return `<div class="connector-empty">${escapeHtml(t("connector.noType"))}</div>`;
  const display = resolveConnectorPositions(board, connectorId, connectorTypes);
  const source = resolveConnectorPositions(currentBase(), connectorId, connectorTypes);
  const editorRow = (positions, colspan) => positions.some((position) => position?.id === state.connectorPosition)
    ? `<tr class="connector-inline-row"><td class="connector-inline-cell" colspan="${colspan}"><div class="connector-editor open" data-connector-editor>${connectorEditorMarkup(board)}</div></td></tr>`
    : "";
  let rows;
  if (type.layout.cols === 2) {
    const positions = new Map(type.positions.map((position) => {
      const column = type.layout.mirror ? 1 - position.col : position.col;
      return [`${position.row}:${column}`, position];
    }));
    rows = Array.from({ length: type.layout.rows }, (_, row) => {
      const rowPositions = [positions.get(`${row}:0`), positions.get(`${row}:1`)];
      return `<tr>${renderConnectorTableCells(board, display, source, rowPositions[0], "left")}${renderConnectorTableCells(board, display, source, rowPositions[1], "right")}</tr>${editorRow(rowPositions, 6)}`;
    }).join("");
  } else {
    const direction = type.layout.mirror ? -1 : 1;
    const positions = [...type.positions].sort((a, b) => a.row - b.row || direction * (a.col - b.col));
    rows = positions.map((position) => `<tr>${renderConnectorTableCells(board, display, source, position)}</tr>${editorRow([position], 3)}`).join("");
  }
  return `<div class="connector-meta"><strong>${escapeHtml(type.name)}</strong><span>${escapeHtml(type.layout.view)}</span></div><table class="connector-pin-table" aria-label="${escapeHtml(t("connector.pinoutAria", { id: connectorId }))}"><tbody>${rows}</tbody></table>`;
}

function gpioChoices(board) {
  const chip = chips[board.chip];
  return Array.from({ length: chip.gpio_count }, (_, gpio) => {
    const pin = board.pins?.[gpio] ?? {};
    return {
      value: String(gpio),
      label: `GPIO ${gpio}`,
      search: `G${gpio} ${(pin.roles ?? []).join(" ")} ${pin.note ?? ""}`,
    };
  });
}

function nonGPIOChoices(type) {
  const rails = ["gnd", "3v3", "5v", "5vout", "5vin", "bat", "vin", "vbus", ...(type.rails ?? [])];
  return [
    ...[...new Set(rails)].map((rail) => ({ value: `pwr:${rail}`, label: `PWR ${rail.toUpperCase()}`, search: rail })),
    ...["en", "usb_dp", "usb_dn"].map((pin) => ({ value: `chip:${pin}`, label: `CHIP ${pin.toUpperCase()}`, search: pin })),
    { value: "nc", label: "NC", search: "not connected" },
  ];
}

function connectorEditorMarkup(board) {
  const connectorId = state.connectorId;
  const positionId = state.connectorPosition;
  if (!connectorId || !positionId) {
    return `<p class="connector-hint">${escapeHtml(t("connector.prompt"))}</p>`;
  }
  const connector = currentBase().connectors?.[connectorId];
  const type = resolveConnectorType(connectorTypes, connector?.type);
  const resolved = resolveConnectorPositions(currentBase(), connectorId, connectorTypes);
  const source = resolved.sources[positionId];
  if (!type || !source) {
    return `<p class="connector-hint">${escapeHtml(t("connector.noPosition"))}</p>`;
  }
  const position = type.positions.find((item) => item.id === positionId);
  const label = position.label ?? position.name?.toUpperCase() ?? position.id;
  if (source.fixed) {
    return `<div class="position-editor-heading"><strong>${escapeHtml(positionId)} · ${escapeHtml(label)}</strong><span>${escapeHtml(t("connector.fixedStatus"))} · ${escapeHtml(endpointLabel(resolved.positions[positionId]))}</span><button type="button" data-connector-close aria-label="${escapeHtml(t("connector.close"))}">×</button></div><p class="connector-hint">${escapeHtml(t("connector.fixedHint"))}</p>`;
  }
  const readonlyReason = isResolvedMode() ? t("readonly.configuration") : "";
  const readonly = readonlyAttributes(readonlyReason);
  const status = source.explicit || source.gpios.length ? t("connector.boardStatus") : source.default ? t("connector.defaultStatus") : t("connector.unsetStatus");
  return `<div class="position-editor-heading"><strong>${escapeHtml(positionId)} · ${escapeHtml(label)}</strong><span>${escapeHtml(status)} · ${escapeHtml(endpointLabel(resolved.positions[positionId]))}</span><button type="button" data-connector-close aria-label="${escapeHtml(t("connector.close"))}">×</button></div>
    <div class="position-editor-tabs" role="tablist"><button type="button" data-connector-editor-tab="gpio" class="${state.connectorEditorTab === "gpio" ? "active" : ""}">${escapeHtml(t("connector.gpioTab"))}</button><button type="button" data-connector-editor-tab="endpoint" class="${state.connectorEditorTab === "endpoint" ? "active" : ""}">${escapeHtml(t("connector.endpointTab"))}</button></div>
    <div class="search-combobox" data-connector-combobox><input type="text" role="combobox" aria-autocomplete="list" aria-expanded="false" placeholder="${escapeHtml(state.connectorEditorTab === "gpio" ? t("connector.searchGPIO") : t("connector.searchEndpoint"))}" autocomplete="off"${readonly}><ul role="listbox"></ul></div>
    <button type="button" class="secondary connector-default" data-connector-default${readonly}>${escapeHtml(t("connector.restoreDefault"))}</button>${readonlyNote(readonlyReason)}`;
}

function hydrateConnectorEditor(root) {
  const combobox = root?.querySelector("[data-connector-combobox]");
  if (!combobox) return;
  const connectorId = state.connectorId;
  const positionId = state.connectorPosition;
  const connector = currentBase().connectors?.[connectorId];
  const type = resolveConnectorType(connectorTypes, connector?.type);
  if (!type) return;
  const options = state.connectorEditorTab === "gpio" ? gpioChoices(currentBase()) : nonGPIOChoices(type);
  new SearchCombobox(combobox, options, (value) => {
    const endpoint = state.connectorEditorTab === "gpio" ? `gpio:${value}` : value;
    assignConnectorPosition(currentBase(), connectorId, positionId, endpoint);
  }, isResolvedMode() ? t("readonly.configuration") : "");
}

function renderConnectorEditor(board) {
  if (state.connectorView === "table") {
    elements.connectorEditor.hidden = true;
    elements.connectorEditor.innerHTML = "";
    hydrateConnectorEditor(elements.connectorDiagram.querySelector("[data-connector-editor]"));
    return;
  }
  elements.connectorEditor.hidden = false;
  elements.connectorEditor.classList.toggle("open", Boolean(state.connectorPosition));
  elements.connectorEditor.innerHTML = connectorEditorMarkup(board);
  hydrateConnectorEditor(elements.connectorEditor);
}

function renderConnectors(board) {
  ensureConnectorSelection(board);
  const ids = Object.keys(board.connectors ?? {});
  elements.connectorTabs.innerHTML = ids.map((id) => `<button type="button" role="tab" aria-selected="${id === state.connectorId}" class="${id === state.connectorId ? "active" : ""}" data-connector-tab="${escapeHtml(id)}">${escapeHtml(id)}</button>`).join("");
  elements.connectorViewToggle.querySelectorAll("[data-connector-view]").forEach((button) => {
    const active = button.dataset.connectorView === state.connectorView;
    button.classList.toggle("active", active);
    button.setAttribute("aria-pressed", String(active));
  });
  const renderer = state.connectorView === "svg" ? renderConnectorDiagram : renderConnectorTable;
  elements.connectorDiagram.innerHTML = state.connectorId ? renderer(board, state.connectorId) : `<div class="connector-empty">${escapeHtml(t("connector.none"))}</div>`;
  const undoReason = isResolvedMode()
    ? t("readonly.configuration")
    : (!state.undo || state.undo.boardId !== state.currentId) ? t("unavailable.noUndo") : "";
  applyControlLock(elements.connectorUndo, undoReason);
  renderConnectorEditor(board);
}

function renderRelated(board) {
  const groups = [["buses", "bus", t("related.buses")], ["devices", "dev", t("related.devices")]];
  elements.relatedList.innerHTML = groups.map(([field, prefix, label]) => {
    const items = Object.entries(board[field] ?? {}).map(([id, object]) => {
      const pins = objectAssignments(board, prefix, id, object);
      const gpioLinks = pins.map((gpio) => `<button type="button" data-target-gpio="${gpio}">GPIO ${gpio}</button>`).join("");
      const ownerLinks = field === "devices" ? [...Object.values(object.signals ?? {}), object.enable].filter(Boolean).flatMap((endpoint) => {
        const match = /^pin:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(endpoint);
        return match ? [`<button type="button" data-owner-target="${escapeHtml(`${match[1]}:${match[2]}`)}">${escapeHtml(endpoint)}</button>`] : [];
      }).join("") : "";
      const links = gpioLinks + ownerLinks;
      return `<div class="object-item" data-object-key="${field}:${escapeHtml(id)}"><div class="object-title"><span>${escapeHtml(id)}</span><span>${escapeHtml(object.kind ?? object.type ?? "")}</span></div><div class="object-summary">${objectSummary(field, object)}</div><div class="gpio-links">${links || `<span>${escapeHtml(t("related.noGPIO"))}</span>`}</div></div>`;
    }).join("");
    return `<section class="object-group"><h3>${escapeHtml(label)}</h3>${items || `<div class="object-item">${escapeHtml(t("related.empty"))}</div>`}</section>`;
  }).join("");
}

function deviceSignals(device) {
  const selectedParts = [device.part, ...Object.values(device.choices ?? {}).map((choice) => choice.part)].filter(Boolean).map((id) => parts[id]).filter(Boolean);
  return [...new Set(selectedParts.length ? selectedParts.flatMap((part) => [...(part.signals?.required ?? []), ...(part.signals?.optional ?? [])]) : schemaSignals("deviceKind")[device.kind] ?? [])];
}

function requiredDeviceSignals(device) {
  const selectedParts = [device.part, ...Object.values(device.choices ?? {}).map((choice) => choice.part)].filter(Boolean).map((id) => parts[id]).filter(Boolean);
  return [...new Set(selectedParts.flatMap((part) => part.signals?.required ?? []))];
}

function roleAssigned(board, role) {
  if (Object.values(board.pins ?? {}).some((pin) => (pin.roles ?? []).includes(role))) return true;
  return Object.values(board.devices ?? {}).some((device) => (
    Object.values(device.pins ?? {}).some((pin) => (pin.roles ?? []).includes(role))
    || Object.values(device.choices ?? {}).some((choice) => Object.values(choice.pins ?? {}).some((pin) => (pin.roles ?? []).includes(role)))
  ));
}

function renderConfiguration() {
  const board = currentBase();
  const connectorRows = Object.entries(board.connectors ?? {}).map(([id, connector]) => {
    const type = connectorTypes[connector.type];
    const standards = Object.keys(type?.standards ?? {});
    return `<div class="config-row"><code>${escapeHtml(id)}</code><span>${escapeHtml(connector.type)}</span><select data-config-standard="${escapeHtml(id)}"><option value="">—</option>${standards.map((standard) => `<option value="${escapeHtml(standard)}"${standard === connector.standard ? " selected" : ""}>${escapeHtml(standard)}</option>`).join("")}</select><button type="button" class="secondary" data-remove-connector="${escapeHtml(id)}">${escapeHtml(t("configuration.remove"))}</button></div>`;
  }).join("");
  const deviceRows = Object.entries(board.devices ?? {}).map(([id, device]) => {
    const missing = requiredDeviceSignals(device).filter((signal) => !roleAssigned(board, `dev:${id}.${signal}`));
    return `<div class="config-row config-device"><code>${escapeHtml(id)}</code><span>${escapeHtml(device.part ?? device.kind)}</span><button type="button" class="secondary" data-remove-device="${escapeHtml(id)}">${escapeHtml(t("configuration.remove"))}</button>${missing.length ? `<div class="unassigned"><strong>${escapeHtml(t("configuration.unassigned"))}:</strong>${missing.map((signal) => `<button type="button" class="secondary" data-unassigned-role="${escapeHtml(`dev:${id}.${signal}`)}">${escapeHtml(signal)}</button>`).join("")}</div>` : ""}</div>`;
  }).join("");
  const entries = state.compositionEntries.get(state.currentId) ?? [];
  const candidates = Object.values(accessories).filter((accessory) => !accessory.compatible_with || accessory.compatible_with.includes(state.currentId));
  const accessoryRows = candidates.map((accessory) => {
    const entry = entries.find((item) => item.id === accessory.id);
    const settings = !entry ? "" : Object.entries(accessory.settings ?? {}).map(([id, setting]) => {
      const selected = entry.settings?.[id] ?? setting.default;
      return `<label>${escapeHtml(setting.name ?? id)}<select data-accessory-setting="${escapeHtml(id)}" data-accessory="${escapeHtml(accessory.id)}">${Object.keys(setting.choices ?? {}).map((choice) => `<option value="${escapeHtml(choice)}"${choice === selected ? " selected" : ""}>${escapeHtml(choice)}</option>`).join("")}</select></label>`;
    }).join("");
    return `<div class="config-row accessory-row"><label><input type="checkbox" data-accessory="${escapeHtml(accessory.id)}"${entry ? " checked" : ""}>${escapeHtml(accessory.name)}</label><span class="origin-badge">${escapeHtml(accessory.origin)}${accessory.bundled ? " · bundled" : ""}</span>${settings}</div>`;
  }).join("");
  const typeOptions = Object.keys(connectorTypes).map((id) => `<option value="${escapeHtml(id)}">${escapeHtml(id)}</option>`).join("");
  const partOptions = Object.values(parts).map((part) => `<option value="${escapeHtml(part.id)}">${escapeHtml(part.kind)} · ${escapeHtml(part.name)}</option>`).join("");
  elements.configurationList.innerHTML = `
    <section class="config-group"><h3>${escapeHtml(t("configuration.connectors"))}</h3>${connectorRows}<div class="config-add"><input data-new-connector-id placeholder="${escapeHtml(t("configuration.idPrompt"))}"><select data-new-connector-type>${typeOptions}</select><button type="button" data-add-connector>${escapeHtml(t("configuration.add"))}</button></div></section>
    <section class="config-group"><h3>${escapeHtml(t("configuration.devices"))}</h3>${deviceRows}<div class="config-add"><input data-new-device-id placeholder="${escapeHtml(t("configuration.idPrompt"))}"><select data-new-device-part>${partOptions}</select><button type="button" data-add-device>${escapeHtml(t("configuration.add"))}</button></div></section>
    <section class="config-group"><h3>${escapeHtml(t("configuration.accessories"))}</h3>${accessoryRows || `<p>${escapeHtml(t("related.empty"))}</p>`}</section>`;
}

function revisionChoice(board, revision, slot, device) {
  return revision?.select?.[slot] ?? device.default;
}

function renderRevisionEditor() {
  const board = currentBase();
  const revisions = board.revisions ?? [];
  const slots = choiceSlots(board).filter(([, device]) => device.selected_by === "revision");
  const [left, right] = state.compareRevisions;
  const leftRevision = revisions.find((revision) => revision.id === left);
  const rightRevision = revisions.find((revision) => revision.id === right);
  const visibleSlots = left && right ? slots.filter(([slot, device]) => revisionChoice(board, leftRevision, slot, device) !== revisionChoice(board, rightRevision, slot, device)) : slots;
  const header = revisions.map((revision, index) => `<th><span>${escapeHtml(revision.name)}</span><button type="button" class="secondary revision-delete" data-delete-revision="${index}" title="${escapeHtml(t("revisions.delete"))}">×</button></th>`).join("");
  const rows = visibleSlots.map(([slot, device]) => `<tr><th>${escapeHtml(slot)}</th>${revisions.map((revision, index) => {
    const selected = revisionChoice(board, revision, slot, device);
    const explicit = revision.select?.[slot] !== undefined;
    return `<td class="${explicit ? "explicit-choice" : "default-choice"}"><select data-revision-index="${index}" data-revision-slot="${escapeHtml(slot)}">${Object.keys(device.choices).map((choice) => `<option value="${escapeHtml(choice)}"${choice === selected ? " selected" : ""}>${escapeHtml(choice)}${choice === device.default ? ` · ${escapeHtml(t("revisions.default"))}` : ""}</option>`).join("")}</select></td>`;
  }).join("")}</tr>`).join("");
  const runtimeRows = choiceSlots(board).filter(([, device]) => device.selected_by === "runtime").map(([slot, device]) => `<div class="runtime-row"><strong>${escapeHtml(t("options.runtime"))}: ${escapeHtml(slot)}</strong><span>${Object.keys(device.choices).map(escapeHtml).join(" / ")}</span></div>`).join("");
  const revisionOptions = `<option value="">${escapeHtml(t("revisions.all"))}</option>${revisions.map((revision) => `<option value="${escapeHtml(revision.id)}">${escapeHtml(revision.name)}</option>`).join("")}`;
  elements.revisionEditor.innerHTML = `<div class="revision-tools"><label>${escapeHtml(t("revisions.compare"))}<select data-compare="0">${revisionOptions}</select><select data-compare="1">${revisionOptions}</select></label><input data-new-revision-id placeholder="id"><input data-new-revision-name placeholder="name"><button type="button" data-add-revision>${escapeHtml(t("revisions.add"))}</button></div><div class="table-wrap revision-table-wrap"><table class="revision-table"><thead><tr><th>${escapeHtml(t("configuration.devices"))}</th>${header}</tr></thead><tbody>${rows}</tbody></table></div>${runtimeRows}`;
  elements.revisionEditor.querySelectorAll("[data-compare]").forEach((select) => { select.value = state.compareRevisions[Number(select.dataset.compare)] ?? ""; });
}

function ownerDefinitions(board) {
  const result = [];
  for (const [id, device] of Object.entries(board.devices ?? {})) {
    if (device.pins) result.push({ key: id, id, part: device.part, pins: device.pins });
    for (const [choice, fragment] of Object.entries(device.choices ?? {})) if (fragment.pins) result.push({ key: `${id}:${choice}`, id, part: fragment.part ?? choice, pins: fragment.pins, choice });
  }
  return result;
}

function renderOwnerEditor(board) {
  const owners = ownerDefinitions(currentBase());
  if (!owners.length) { elements.ownerEditor.innerHTML = `<p class="owner-empty">${escapeHtml(t("owners.none"))}</p>`; return; }
  if (!owners.some((owner) => owner.key === state.ownerKey)) state.ownerKey = owners.find((owner) => owner.part === board.devices?.[owner.id]?.part)?.key ?? owners[0].key;
  const current = owners.find((owner) => owner.key === state.ownerKey);
  const tabs = owners.map((owner) => {
    const active = owner.part === board.devices?.[owner.id]?.part;
    return `<button type="button" data-owner-key="${escapeHtml(owner.key)}" class="${owner.key === current.key ? "active" : ""}${active ? "" : " inactive"}" title="${active ? "" : escapeHtml(t("owners.inactive"))}">${escapeHtml(owner.id)} (${escapeHtml(owner.part ?? "?")})</button>`;
  }).join("");
  const definitions = parts[current.part]?.pins ?? {};
  const pinNames = [...new Set([...Object.keys(definitions), ...Object.keys(current.pins ?? {})])];
  const rows = pinNames.map((pin) => {
    const row = current.pins?.[pin] ?? { roles: [] };
    return `<tr id="owner-${escapeHtml(current.key)}-${escapeHtml(pin)}"><td><code>${escapeHtml(pin)}</code></td><td><div class="role-list">${(row.roles ?? []).map((role) => roleChip(board, role, false)).join("") || "—"}</div></td><td>${escapeHtml(row.note ?? "—")}</td><td>${verificationSummary(row)}</td></tr>`;
  }).join("");
  const revision = currentBase().revisions?.find((item) => item.id === [...state.selectedOptions][0]);
  const runtime = state.runtimeSelections.get(state.currentId) ?? {};
  const readonlyReason = current.choice && revision?.select?.[current.id] === current.choice
    ? t("readonly.revision", { revision: revision.name })
    : current.choice && runtime[current.id] === current.choice
      ? t("readonly.runtime", { choice: `${current.id}: ${current.choice}` })
      : "";
  elements.ownerEditor.innerHTML = `<div class="owner-tabs">${tabs}</div>${readonlyNote(readonlyReason)}<div class="table-wrap owner-table-wrap"${readonlyAttributes(readonlyReason)}><table><thead><tr><th>${escapeHtml(t("owners.pin"))}</th><th>${escapeHtml(t("column.roles"))}</th><th>${escapeHtml(t("column.note"))}</th><th>${escapeHtml(t("column.verified"))}</th></tr></thead><tbody>${rows}</tbody></table></div><p class="owner-signals">${escapeHtml(t("object.signals"))}: ${Object.entries(board.devices?.[current.id]?.signals ?? {}).map(([signal, endpoint]) => `${signal} → ${endpoint}`).join(" · ") || "—"}</p>`;
}

function renderViews(board) {
  for (const button of elements.viewTabs.querySelectorAll("[data-view]")) button.classList.toggle("active", button.dataset.view === state.activeView);
  elements.configurationView.hidden = state.activeView !== "configuration";
  elements.revisionsView.hidden = state.activeView !== "revisions";
  elements.ownersView.hidden = state.activeView !== "owners";
  renderConfiguration();
  renderRevisionEditor();
  renderOwnerEditor(board);
}

function renderSaveState() {
  const entry = currentEntry();
  elements.saveState.textContent = entry.dirty ? t("save.dirty") : t("save.saved");
  elements.saveState.classList.toggle("dirty", entry.dirty);
  applyControlLock(elements.download);
  applyControlLock(elements.discard, entry.dirty ? "" : t("unavailable.noChanges"));
}

function renderAll() {
  try {
    applyStaticTranslations();
    const board = shownBoard();
    const errors = validationFor(board);
    renderBoardSelect();
    renderOptions();
    renderViews(board);
    renderTable(board, errors);
    renderValidation(board, errors);
    renderConnectors(board);
    renderRelated(board);
    renderSaveState();
  } catch (failure) {
    showToast(failure.message);
  }
}

function updatePin(gpio, field, value) {
  const pin = editablePin(gpio);
  if (value === "") delete pin[field];
  else pin[field] = value;
  clearVerification(pin, field);
  cleanupPin(gpio);
  markDirty();
  renderAll();
}

function removeRole(gpio, role) {
  const connectorRole = /^conn:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
  if (connectorRole) {
    assignConnectorPosition(currentBase(), connectorRole[1], connectorRole[2], null, { removeGPIO: gpio });
    return;
  }
  const pin = editablePin(gpio);
  pin.roles = pin.roles.filter((item) => item !== role);
  clearVerification(pin, "roles");
  cleanupPin(gpio);
  markDirty();
  renderAll();
}

function addRole(gpio, role) {
  if (!role) return;
  const connectorRole = /^conn:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
  if (connectorRole) {
    assignConnectorPosition(currentBase(), connectorRole[1], connectorRole[2], `gpio:${gpio}`);
    return;
  }
  const pin = editablePin(gpio);
  if (!pin.roles.includes(role)) pin.roles.push(role);
  clearVerification(pin, "roles");
  markDirty();
  renderAll();
}

function rememberConnectorUndo(board) {
  const entry = currentEntry();
  state.undo = {
    boardId: state.currentId,
    base: clone(board),
    dirty: entry.dirty,
    expandedGPIO: entry.expandedGPIO,
  };
}

function removeConnectorRole(board, connectorId, positionId, gpio = null) {
  const role = `conn:${connectorId}.${positionId}`;
  for (const [pinGPIO, pin] of Object.entries(board.pins ?? {})) {
    if (gpio !== null && String(gpio) !== pinGPIO) continue;
    const next = (pin.roles ?? []).filter((item) => item !== role);
    if (next.length === (pin.roles ?? []).length) continue;
    pin.roles = next;
    clearVerification(pin, "roles");
    cleanupPin(pinGPIO);
  }
}

function assignConnectorPosition(board, connectorId, positionId, endpoint, options = {}) {
  const connector = board.connectors?.[connectorId];
  const type = resolveConnectorType(connectorTypes, connector?.type);
  if (!connector || !type) return;
  const resolved = resolveConnectorPositions(board, connectorId, connectorTypes);
  const source = resolved.sources[positionId];
  if (!source || source.fixed) {
    showToast(t("connector.fixedToast"));
    return;
  }

  const gpio = /^gpio:(\d+)$/.exec(endpoint ?? "")?.[1];
  if (gpio !== undefined && source.gpios.some((item) => String(item) === gpio)) return;
  if (gpio !== undefined && source.gpios.length && !(connector.multi_gpio ?? []).includes(positionId)) {
    if (!window.confirm(t("connector.multiConfirm"))) return;
  }

  rememberConnectorUndo(board);
  state.connectorId = connectorId;
  state.connectorPosition = positionId;

  if (options.removeGPIO !== undefined) {
    removeConnectorRole(board, connectorId, positionId, options.removeGPIO);
  } else if (gpio !== undefined) {
    if (source.gpios.length && !(connector.multi_gpio ?? []).includes(positionId)) {
      connector.multi_gpio = [...(connector.multi_gpio ?? []), positionId];
    }
    if (connector.positions) {
      delete connector.positions[positionId];
      if (!Object.keys(connector.positions).length) delete connector.positions;
    }
    const pin = editablePin(gpio);
    const role = `conn:${connectorId}.${positionId}`;
    if (!pin.roles.includes(role)) pin.roles.push(role);
    clearVerification(pin, "roles");
  } else {
    removeConnectorRole(board, connectorId, positionId);
    if (endpoint === null) {
      if (connector.positions) {
        delete connector.positions[positionId];
        if (!Object.keys(connector.positions).length) delete connector.positions;
      }
      if (connector.multi_gpio) {
        connector.multi_gpio = connector.multi_gpio.filter((position) => position !== positionId);
        if (!connector.multi_gpio.length) delete connector.multi_gpio;
      }
    } else {
      connector.positions ??= {};
      connector.positions[positionId] = endpoint;
      if (connector.multi_gpio) {
        connector.multi_gpio = connector.multi_gpio.filter((position) => position !== positionId);
        if (!connector.multi_gpio.length) delete connector.multi_gpio;
      }
    }
  }
  clearVerification(connector, "positions");
  markDirty();
  renderAll();
}

function connectorForGPIO(board, gpio) {
  for (const [id, connector] of Object.entries(board.connectors ?? {})) {
    const positions = connectorRoleGPIOs(board, id);
    if ([...positions.values()].some((gpios) => gpios.includes(Number(gpio)))) return id;
  }
  return null;
}

function revealConnectorForGPIO(gpio) {
  const connectorId = connectorForGPIO(currentBase(), gpio);
  if (connectorId && connectorId !== state.connectorId) {
    state.connectorId = connectorId;
    state.connectorPosition = null;
  }
}

function focusGPIO(gpio) {
  if (gpio === null || gpio === undefined) return;
  const entry = currentEntry();
  entry.expandedGPIO = String(gpio);
  revealConnectorForGPIO(gpio);
  writeDraft(entry);
  renderAll();
  const row = document.getElementById(`gpio-${gpio}`);
  if (!row) return;
  row.scrollIntoView({ behavior: "smooth", block: "center" });
  row.classList.remove("flash");
  requestAnimationFrame(() => row.classList.add("flash"));
}

function openRoleChooser(role) {
  state.activeView = "configuration";
  clearResolvedMode();
  const chip = chips[currentBase().chip];
  const gpio = Array.from({ length: chip.gpio_count }, (_, index) => String(index)).find((id) => !chip.reserved.includes(Number(id)) && !(currentBase().pins[id]?.roles?.length)) ?? "0";
  currentEntry().expandedGPIO = gpio;
  renderAll();
  const input = elements.tableBody.querySelector(`[data-role-combobox="${gpio}"] input`);
  if (input) {
    input.value = role;
    input.focus();
    input.dispatchEvent(new Event("input", { bubbles: true }));
  }
}

async function openFile(file) {
  try {
    const value = assertBoard(parseJson(await file.text(), file.name), file.name);
    if (!chips[value.chip]) throw new Error(t("file.chipMissing", { file: file.name, chip: value.chip }));
    state.documents.set(value.id, { base: clone(value), original: clone(value), dirty: false, expandedGPIO: null, source: file.name });
    removeDraft(value.id);
    state.currentId = value.id;
    state.selectedOptions.clear();
    state.runtimeSelections.set(value.id, Object.fromEntries(choiceSlots(value).filter(([, device]) => device.selected_by === "runtime").map(([slot, device]) => [slot, device.default])));
    state.compositionEntries.set(value.id, clone(targets.m5unified_pin_table?.compositions?.[value.id]?.default?.accessories ?? []));
    state.connectorId = null;
    state.connectorPosition = null;
    state.undo = null;
    renderAll();
    showToast(t("file.opened", { file: file.name }));
  } catch (failure) {
    showToast(failure.message);
  } finally {
    elements.fileInput.value = "";
  }
}

function downloadCurrent() {
  const entry = currentEntry();
  const formatted = formatBoard(entry.base);
  const normalized = JSON.parse(formatted);
  const blob = new Blob([formatted], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = `${normalized.id}.json`;
  document.body.append(anchor);
  anchor.click();
  anchor.remove();
  setTimeout(() => URL.revokeObjectURL(url), 0);
  entry.base = normalized;
  entry.original = clone(normalized);
  entry.dirty = false;
  state.undo = null;
  writeDraft(entry);
  renderAll();
  showToast(t("file.downloaded", { file: anchor.download }));
}

function discardDraft() {
  const entry = currentEntry();
  if (!entry.dirty || !window.confirm(t("file.discardConfirm"))) return;
  entry.base = clone(entry.original);
  entry.dirty = false;
  entry.expandedGPIO = null;
  state.undo = null;
  removeDraft(entry.base.id);
  state.selectedOptions.clear();
  renderAll();
}

function showToast(message) {
  elements.toast.textContent = message;
  elements.toast.classList.add("show");
  clearTimeout(state.toastTimer);
  state.toastTimer = setTimeout(() => elements.toast.classList.remove("show"), 3200);
}

function readonlyTarget(event) {
  return event.target.closest?.("[data-readonly-reason]");
}

function explainReadonly(event) {
  const control = readonlyTarget(event);
  if (!control) return;
  if (event.type === "keydown" && event.key === "Tab") return;
  event.preventDefault();
  event.stopPropagation();
  showToast(control.dataset.readonlyReason);
  if (event.type === "change") renderAll();
}

document.addEventListener("click", explainReadonly, true);
document.addEventListener("keydown", explainReadonly, true);
document.addEventListener("beforeinput", explainReadonly, true);
document.addEventListener("change", explainReadonly, true);
document.addEventListener("focusin", (event) => {
  const control = readonlyTarget(event);
  if (control) showToast(control.dataset.readonlyReason);
});

elements.boardSelect.addEventListener("change", () => {
  state.currentId = elements.boardSelect.value;
  state.selectedOptions.clear();
  state.connectorId = null;
  state.connectorPosition = null;
  renderAll();
});

elements.languageSelect.addEventListener("change", () => {
  state.language = elements.languageSelect.value;
  saveLanguage(state.language);
  renderAll();
});

elements.fileInput.addEventListener("change", () => {
  const [file] = elements.fileInput.files;
  if (file) openFile(file);
});

elements.download.addEventListener("click", downloadCurrent);
elements.discard.addEventListener("click", discardDraft);
elements.editBase.addEventListener("click", () => {
  clearResolvedMode();
  renderAll();
});

elements.optionPanel.addEventListener("change", (event) => {
  if (event.target.matches("[data-display-revision]")) {
    state.selectedOptions.clear();
    if (event.target.value) state.selectedOptions.add(event.target.value);
    state.ownerKey = null;
    renderAll();
  } else if (event.target.matches("[data-display-runtime]")) {
    const runtime = { ...(state.runtimeSelections.get(state.currentId) ?? {}) };
    if (event.target.value) runtime[event.target.dataset.displayRuntime] = event.target.value;
    else delete runtime[event.target.dataset.displayRuntime];
    state.runtimeSelections.set(state.currentId, runtime);
    renderAll();
  }
});

elements.viewTabs.addEventListener("click", (event) => {
  const button = event.target.closest("[data-view]");
  if (!button) return;
  state.activeView = button.dataset.view;
  renderAll();
});

elements.configurationList.addEventListener("change", (event) => {
  const accessoryId = event.target.dataset.accessory;
  if (accessoryId) {
    const entries = clone(state.compositionEntries.get(state.currentId) ?? []);
    const index = entries.findIndex((item) => item.id === accessoryId);
    const setting = event.target.dataset.accessorySetting;
    if (setting && index >= 0) entries[index].settings = { ...(entries[index].settings ?? {}), [setting]: event.target.value };
    else if (event.target.checked && index < 0) entries.push({ id: accessoryId });
    else if (!event.target.checked && index >= 0) entries.splice(index, 1);
    state.compositionEntries.set(state.currentId, entries);
    renderAll();
    return;
  }
  const connector = event.target.dataset.configStandard;
  if (connector) {
    if (event.target.value) currentBase().connectors[connector].standard = event.target.value;
    else delete currentBase().connectors[connector].standard;
    markDirty(); renderAll();
  }
});

elements.configurationList.addEventListener("click", (event) => {
  const roleButton = event.target.closest("[data-unassigned-role]");
  if (roleButton) return openRoleChooser(roleButton.dataset.unassignedRole);
  const removeConnector = event.target.closest("[data-remove-connector]");
  if (removeConnector) { delete currentBase().connectors[removeConnector.dataset.removeConnector]; markDirty(); renderAll(); return; }
  const removeDevice = event.target.closest("[data-remove-device]");
  if (removeDevice) { delete currentBase().devices[removeDevice.dataset.removeDevice]; markDirty(); renderAll(); return; }
  if (event.target.closest("[data-add-connector]")) {
    const id = elements.configurationList.querySelector("[data-new-connector-id]").value.trim();
    const type = elements.configurationList.querySelector("[data-new-connector-type]").value;
    if (!/^[a-z][a-z0-9_]*$/.test(id) || currentBase().connectors?.[id]) return showToast(t("validation.E_ID_FORMAT"));
    currentBase().connectors ??= {};
    currentBase().connectors[id] = { type };
    markDirty(); renderAll(); return;
  }
  if (event.target.closest("[data-add-device]")) {
    const id = elements.configurationList.querySelector("[data-new-device-id]").value.trim();
    const partId = elements.configurationList.querySelector("[data-new-device-part]").value;
    if (!/^[a-z][a-z0-9_]*$/.test(id) || currentBase().devices?.[id]) return showToast(t("validation.E_ID_FORMAT"));
    currentBase().devices[id] = { kind: parts[partId].kind, part: partId };
    markDirty(); renderAll();
  }
});

elements.revisionEditor.addEventListener("change", (event) => {
  if (event.target.matches("[data-compare]")) {
    state.compareRevisions[Number(event.target.dataset.compare)] = event.target.value;
    renderAll(); return;
  }
  const index = event.target.dataset.revisionIndex;
  const slot = event.target.dataset.revisionSlot;
  if (index !== undefined && slot) {
    const revision = currentBase().revisions[Number(index)];
    const device = currentBase().devices[slot];
    revision.select ??= {};
    if (event.target.value === device.default) delete revision.select[slot];
    else revision.select[slot] = event.target.value;
    markDirty(); renderAll();
  }
});

elements.revisionEditor.addEventListener("click", (event) => {
  const remove = event.target.closest("[data-delete-revision]");
  if (remove) { currentBase().revisions.splice(Number(remove.dataset.deleteRevision), 1); markDirty(); renderAll(); return; }
  if (event.target.closest("[data-add-revision]")) {
    const id = elements.revisionEditor.querySelector("[data-new-revision-id]").value.trim();
    const name = elements.revisionEditor.querySelector("[data-new-revision-name]").value.trim();
    if (!/^[a-z][a-z0-9_]*$/.test(id) || !name || currentBase().revisions?.some((revision) => revision.id === id)) return showToast(t("validation.E_ID_FORMAT"));
    currentBase().revisions ??= [];
    currentBase().revisions.push({ id, name, select: {} });
    markDirty(); renderAll();
  }
});

elements.ownerEditor.addEventListener("click", (event) => {
  const tab = event.target.closest("[data-owner-key]");
  if (!tab) return;
  state.ownerKey = tab.dataset.ownerKey;
  renderAll();
});

elements.tableBody.addEventListener("click", (event) => {
  const button = event.target.closest("[data-remove-role]");
  if (button) {
    removeRole(button.dataset.gpio, decodeURIComponent(button.dataset.removeRole));
    return;
  }
  const related = event.target.closest("[data-target-object]");
  if (related) {
    if (related.dataset.targetObject.startsWith("connectors:")) {
      state.connectorId = related.dataset.targetObject.slice("connectors:".length);
      state.connectorPosition = null;
      renderAll();
      return;
    }
    const object = elements.relatedList.querySelector(`[data-object-key="${related.dataset.targetObject}"]`);
    object?.scrollIntoView({ behavior: "smooth", block: "center" });
    object?.classList.add("flash");
    setTimeout(() => object?.classList.remove("flash"), 1200);
    return;
  }
  const row = event.target.closest("[data-gpio-row]");
  if (!row) return;
  const entry = currentEntry();
  entry.expandedGPIO = entry.expandedGPIO === row.dataset.gpioRow ? null : row.dataset.gpioRow;
  if (entry.expandedGPIO !== null) revealConnectorForGPIO(entry.expandedGPIO);
  writeDraft(entry);
  renderAll();
});

elements.tableBody.addEventListener("change", (event) => {
  const gpio = event.target.dataset.gpio;
  if (gpio === undefined) return;
  if (event.target.matches("[data-pull]")) updatePin(gpio, "pull", event.target.value);
  else if (event.target.matches("[data-note]")) updatePin(gpio, "note", event.target.value.trim());
});

elements.validationList.addEventListener("click", (event) => {
  const button = event.target.closest("[data-error-index]");
  if (!button) return;
  const board = shownBoard();
  const errors = validationFor(board);
  focusGPIO(gpioForError(errors[Number(button.dataset.errorIndex)], board));
});

elements.relatedList.addEventListener("click", (event) => {
  const button = event.target.closest("[data-target-gpio]");
  if (button) focusGPIO(button.dataset.targetGpio);
  const owner = event.target.closest("[data-owner-target]");
  if (owner) {
    const [ownerId, pin] = owner.dataset.ownerTarget.split(":");
    const shown = shownBoard();
    state.ownerKey = ownerDefinitions(currentBase()).find((item) => item.id === ownerId && item.part === shown.devices?.[ownerId]?.part)?.key ?? ownerId;
    state.activeView = "owners";
    renderAll();
    requestAnimationFrame(() => document.getElementById(`owner-${state.ownerKey}-${pin}`)?.scrollIntoView({ behavior: "smooth", block: "center" }));
  }
});

elements.connectorTabs.addEventListener("click", (event) => {
  const tab = event.target.closest("[data-connector-tab]");
  if (!tab) return;
  state.connectorId = tab.dataset.connectorTab;
  state.connectorPosition = null;
  renderAll();
});

elements.connectorViewToggle.addEventListener("click", (event) => {
  const button = event.target.closest("[data-connector-view]");
  if (!button) return;
  state.connectorView = button.dataset.connectorView;
  renderAll();
});

function selectConnectorPosition(target) {
  state.connectorPosition = state.connectorPosition === target.dataset.connectorPosition ? null : target.dataset.connectorPosition;
  renderAll();
}

elements.connectorDiagram.addEventListener("click", (event) => {
  const position = event.target.closest("[data-connector-position]");
  if (position) selectConnectorPosition(position);
});

elements.connectorDiagram.addEventListener("keydown", (event) => {
  if (event.key !== "Enter" && event.key !== " ") return;
  const position = event.target.closest("[data-connector-position]");
  if (!position) return;
  event.preventDefault();
  selectConnectorPosition(position);
});

elements.connectorDiagram.addEventListener("mouseover", (event) => {
  const position = event.target.closest("[data-connector-position]");
  const related = event.relatedTarget?.closest?.("[data-connector-position]");
  if (!position || related?.dataset.connectorPosition === position.dataset.connectorPosition) return;
  const endpoint = resolveConnectorPositions(shownBoard(), state.connectorId, connectorTypes).positions[position.dataset.connectorPosition];
  const rows = endpointGPIOs(endpoint).map((gpio) => document.getElementById(`gpio-${gpio}`)).filter(Boolean);
  rows.forEach((row) => row.classList.add("diagram-highlight"));
  rows[0]?.scrollIntoView({ behavior: "smooth", block: "nearest" });
});

elements.connectorDiagram.addEventListener("mouseout", (event) => {
  const position = event.target.closest("[data-connector-position]");
  const related = event.relatedTarget?.closest?.("[data-connector-position]");
  if (!position || related?.dataset.connectorPosition === position.dataset.connectorPosition) return;
  elements.tableBody.querySelectorAll(".diagram-highlight").forEach((row) => row.classList.remove("diagram-highlight"));
});

function handleConnectorEditorClick(event) {
  if (event.target.closest("[data-connector-close]")) {
    state.connectorPosition = null;
    renderAll();
    return;
  }
  const tab = event.target.closest("[data-connector-editor-tab]");
  if (tab) {
    state.connectorEditorTab = tab.dataset.connectorEditorTab;
    renderAll();
    return;
  }
  if (event.target.closest("[data-connector-default]")) {
    assignConnectorPosition(currentBase(), state.connectorId, state.connectorPosition, null);
  }
}

elements.connectorEditor.addEventListener("click", handleConnectorEditorClick);
elements.connectorDiagram.addEventListener("click", (event) => {
  if (event.target.closest("[data-connector-editor]")) handleConnectorEditorClick(event);
});

document.addEventListener("keydown", (event) => {
  if (event.key !== "Escape" || !state.connectorPosition) return;
  state.connectorPosition = null;
  renderAll();
});

elements.connectorUndo.addEventListener("click", () => {
  if (!state.undo || state.undo.boardId !== state.currentId) return;
  const entry = currentEntry();
  entry.base = clone(state.undo.base);
  entry.dirty = state.undo.dirty;
  entry.expandedGPIO = state.undo.expandedGPIO;
  state.undo = null;
  if (entry.dirty || entry.expandedGPIO !== null) writeDraft(entry);
  else removeDraft(entry.base.id);
  renderAll();
});

for (const eventName of ["dragenter", "dragover"]) {
  elements.dropZone.addEventListener(eventName, (event) => {
    event.preventDefault();
    elements.dropZone.classList.add("dragging");
  });
}
for (const eventName of ["dragleave", "drop"]) {
  elements.dropZone.addEventListener(eventName, (event) => {
    event.preventDefault();
    elements.dropZone.classList.remove("dragging");
  });
}
elements.dropZone.addEventListener("drop", (event) => {
  const [file] = event.dataTransfer.files;
  if (file) openFile(file);
});
elements.dropZone.addEventListener("keydown", (event) => {
  if (event.key === "Enter" || event.key === " ") elements.fileInput.click();
});

window.addEventListener("beforeunload", (event) => {
  if (![...state.documents.values()].some((entry) => entry.dirty)) return;
  event.preventDefault();
  event.returnValue = "";
});

addEmbeddedBoards();
renderAll();
