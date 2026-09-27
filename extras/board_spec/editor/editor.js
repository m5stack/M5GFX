import { assertBoard, clone, effectiveChip, parseJson } from "../lib/model.js";
import { choiceSlots, revisionSelection } from "../lib/choices.js";
import { validateBoard, validateResolvedVariants } from "../lib/validate.js";
import { resolveBoard } from "../lib/resolve.js";
import { formatBoard } from "../lib/format.js";
import { pinChangeLayer } from "./provenance.js";
import { BoardOperationError, addBus, addChoice, addConnector, addDevice, addRole, boardRoleOptions, createBoard, duplicateBoard, removeBus, removeChoice, removeConnector, removeDevice, setBoardField, setChoiceDefaults, setDeviceSpec, setSpec } from "./board_ops.js";
import { filterBoardOptions, gpioMatchesFilter, matchesSearchText } from "./filters.js";
import { assignConnectorLanes, connectorRolesByLane, objectRoleOwner, roleColor, roleOwner } from "./role_colors.js";
import { escapeHtml, gpioTargetButton, ownerSignalsHtml } from "./html.js";

const schema = readEmbedded("board-spec-schema");
const chips = readEmbedded("board-spec-chips");
const connectorTypes = readEmbedded("board-spec-connector-types");
const parts = readEmbedded("board-spec-parts");
const accessories = readEmbedded("board-spec-accessories");
const targets = readEmbedded("board-spec-targets");
const embeddedBoards = readEmbedded("board-spec-boards");
const messages = readEmbedded("board-spec-i18n");
const languageStorageKey = "m5-board-spec-language:v1";
const draftIndexKey = "m5-board-spec-drafts:v1";
const gpioFilterStorageKey = "m5-board-spec-gpio-filter:v1";
const roleColorStorageKey = "m5-board-spec-role-colors:v1";
const onboardingStorageKey = "m5-board-spec-onboarding:v1";
const layoutStorageKey = "m5-board-spec-layout:v1";
const motionDuration = 200;
const sidebarWidthLimits = { min: 360, max: 620, default: 460 };
let comboboxSerial = 0;

const chipForBoard = (board) => effectiveChip(chips[board.chip], board);
let boardCombobox = null;
const openingGPIOs = new Set();
const closingGPIOs = new Set();
const closingGPIOsStarted = new Set();
const gpioMotionTimers = new Map();
const disclosureMotionStates = new WeakMap();

const elements = {
  boardSelect: document.querySelector("#board-select"),
  newBoard: document.querySelector("#new-board-button"),
  duplicateBoard: document.querySelector("#duplicate-board-button"),
  closeBoard: document.querySelector("#close-board-button"),
  boardDialog: document.querySelector("#board-dialog"),
  boardDialogTitle: document.querySelector("#board-dialog-title"),
  boardDialogError: document.querySelector("#board-dialog-error"),
  boardDialogForm: document.querySelector("#board-dialog-form"),
  languageSelect: document.querySelector("#language-select"),
  onboarding: document.querySelector("#onboarding"),
  onboardingHelp: document.querySelector("#onboarding-help"),
  onboardingClose: document.querySelector("#onboarding-close"),
  fileInput: document.querySelector("#file-input"),
  download: document.querySelector("#download-button"),
  discard: document.querySelector("#discard-button"),
  dropZone: document.querySelector("#drop-zone"),
  repositoryNext: document.querySelector("#repository-next"),
  optionPanel: document.querySelector("#option-panel"),
  viewTabs: document.querySelector("#view-tabs"),
  configurationView: document.querySelector("#configuration-view"),
  boardInfo: document.querySelector("#board-info"),
  boardInfoSummary: document.querySelector("#board-info-summary"),
  boardInfoBody: document.querySelector("#board-info-body"),
  sideResizer: document.querySelector("#side-resizer"),
  sideColumn: document.querySelector(".side-column"),
  revisionsView: document.querySelector("#revisions-view"),
  ownersView: document.querySelector("#owners-view"),
  configurationList: document.querySelector("#configuration-list"),
  revisionEditor: document.querySelector("#revision-editor"),
  ownerEditor: document.querySelector("#owner-editor"),
  modeNote: document.querySelector("#mode-note"),
  editBase: document.querySelector("#edit-base-button"),
  saveState: document.querySelector("#save-state"),
  tableHead: document.querySelector("#gpio-columns"),
  connectorColumn: document.querySelector("#gpio-connectors-column"),
  tableBody: document.querySelector("#gpio-table tbody"),
  gpioFilter: document.querySelector("#gpio-filter"),
  gpioAssignedOnly: document.querySelector("#gpio-assigned-only"),
  roleColorLegend: document.querySelector("#role-color-legend"),
  validationCount: document.querySelector("#validation-count"),
  validationSection: document.querySelector("#validation-section"),
  validationList: document.querySelector("#validation-list"),
  connectorTabs: document.querySelector("#connector-tabs"),
  connectorViewToggle: document.querySelector("#connector-view-toggle"),
  connectorDiagram: document.querySelector("#connector-diagram"),
  connectorEditor: document.querySelector("#connector-editor"),
  connectorUndo: document.querySelector("#connector-undo"),
  connectorSection: document.querySelector("#connector-section"),
  connectorSectionTitle: document.querySelector("#connector-section-title"),
  relatedList: document.querySelector("#related-list"),
  relatedSection: document.querySelector("#related-section"),
  relatedSectionTitle: document.querySelector("#related-section-title"),
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
  boardDialogMode: "new",
  ...initialGPIOFilterState(),
  roleColors: initialRoleColors(),
  roleColorsOpen: false,
  onboardingOpen: initialOnboardingOpen(),
  layout: initialLayoutState(),
  lastExported: null,
};

class SearchCombobox {
  constructor(root, options, onSelect, readonlyReason = "", settings = {}) {
    this.root = root;
    this.input = root.querySelector("input");
    this.list = root.querySelector("[role=listbox]");
    this.options = [];
    this.onSelect = onSelect;
    this.filtered = [];
    this.activeIndex = 0;
    this.readonlyReason = readonlyReason;
    this.settings = settings;
    this.selectedValue = null;
    this.instanceId = ++comboboxSerial;
    this.reposition = () => this.positionList();
    this.setOptions(options);
    if (readonlyReason) applyControlLock(this.input, readonlyReason);
    this.input.addEventListener("focus", () => {
      if (this.settings.selectOnFocus) this.input.select();
      this.open(this.settings.showAllOnFocus ? "" : undefined);
    });
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
    return matchesSearchText(`${option.label} ${option.search ?? ""}`, query);
  }

  setOptions(options) {
    this.options = options.map((option) => typeof option === "string" ? { value: option, label: option, search: option } : option);
  }

  setSelected(value) {
    this.selectedValue = value;
    const option = this.options.find((item) => item.value === value);
    if (option) this.input.value = option.label;
  }

  open(query = this.input.value) {
    if (this.readonlyReason) {
      showToast(this.readonlyReason);
      return;
    }
    this.query = query;
    this.filtered = this.settings.filterOptions
      ? this.settings.filterOptions(this.options, query)
      : this.options.filter((option) => this.matches(option, query));
    this.activeIndex = 0;
    this.render();
    this.root.classList.add("open");
    this.input.setAttribute("aria-expanded", "true");
    window.addEventListener("resize", this.reposition);
    window.addEventListener("scroll", this.reposition, true);
    requestAnimationFrame(this.reposition);
  }

  close() {
    this.root.classList.remove("open");
    this.root.classList.remove("open-upwards");
    this.root.style.removeProperty("--combobox-left");
    this.root.style.removeProperty("--combobox-top");
    this.root.style.removeProperty("--combobox-bottom");
    this.root.style.removeProperty("--combobox-width");
    this.root.style.removeProperty("--combobox-max-height");
    window.removeEventListener("resize", this.reposition);
    window.removeEventListener("scroll", this.reposition, true);
    this.input.setAttribute("aria-expanded", "false");
    this.input.removeAttribute("aria-activedescendant");
    if (this.settings.preserveInput) this.setSelected(this.selectedValue);
    else this.input.value = "";
  }

  positionList() {
    if (!this.root.classList.contains("open")) return;
    const rect = this.input.getBoundingClientRect();
    const margin = 8;
    const gap = 4;
    const desiredHeight = Math.min(240, this.list.scrollHeight);
    const spaceBelow = window.innerHeight - rect.bottom - margin;
    const spaceAbove = rect.top - margin;
    const opensUpwards = spaceBelow < desiredHeight && spaceAbove > spaceBelow;
    const availableHeight = Math.max(80, (opensUpwards ? spaceAbove : spaceBelow) - gap);
    const left = Math.max(margin, Math.min(rect.left, window.innerWidth - rect.width - margin));
    this.root.classList.toggle("open-upwards", opensUpwards);
    this.root.style.setProperty("--combobox-left", `${left}px`);
    this.root.style.setProperty("--combobox-top", opensUpwards ? "auto" : `${rect.bottom + gap}px`);
    this.root.style.setProperty("--combobox-bottom", opensUpwards ? `${window.innerHeight - rect.top + gap}px` : "auto");
    this.root.style.setProperty("--combobox-width", `${rect.width}px`);
    this.root.style.setProperty("--combobox-max-height", `${Math.min(240, availableHeight)}px`);
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
    this.selectedValue = value;
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
      const message = typeof this.settings.emptyMessage === "function"
        ? this.settings.emptyMessage(this.query ?? "")
        : this.settings.emptyMessage || t("combobox.empty");
      this.list.innerHTML = `<li class="combobox-empty">${escapeHtml(message)}</li>`;
      return;
    }
    let previousGroup = null;
    this.list.innerHTML = this.filtered.map((option, index) => {
      const group = option.group && option.group !== previousGroup
        ? `<li class="combobox-group" role="presentation">${escapeHtml(option.group)}</li>`
        : "";
      previousGroup = option.group ?? null;
      return `${group}<li><button type="button" role="option" id="combobox-${this.instanceId}-option-${index}" class="${index === this.activeIndex ? "active" : ""}" aria-selected="${index === this.activeIndex}" data-combobox-value="${encodeURIComponent(option.value)}">${escapeHtml(option.label)}</button></li>`;
    }).join("");
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

function initialOnboardingOpen() {
  try {
    return localStorage.getItem(onboardingStorageKey) !== "dismissed";
  } catch {
    return true;
  }
}

function initialLayoutState() {
  try {
    const stored = JSON.parse(localStorage.getItem(layoutStorageKey) ?? "{}");
    const width = Number(stored.sidebarWidth);
    return {
      sidebarWidth: Number.isFinite(width) ? Math.max(sidebarWidthLimits.min, Math.min(sidebarWidthLimits.max, width)) : sidebarWidthLimits.default,
      sections: stored.sections && typeof stored.sections === "object" ? stored.sections : {},
    };
  } catch {
    return { sidebarWidth: sidebarWidthLimits.default, sections: {} };
  }
}

function saveLayoutState() {
  try { localStorage.setItem(layoutStorageKey, JSON.stringify(state.layout)); } catch { /* storage is optional */ }
}

function disclosureOpen(id, fallback = false) {
  return Object.hasOwn(state.layout.sections, id) ? Boolean(state.layout.sections[id]) : fallback;
}

function setSidebarWidth(width) {
  state.layout.sidebarWidth = Math.max(sidebarWidthLimits.min, Math.min(sidebarWidthLimits.max, Math.round(width)));
  elements.configurationView.style.setProperty("--side-width", `${state.layout.sidebarWidth}px`);
  elements.sideResizer.setAttribute("aria-valuenow", String(state.layout.sidebarWidth));
}

function dismissOnboarding() {
  state.onboardingOpen = false;
  try { localStorage.setItem(onboardingStorageKey, "dismissed"); } catch { /* storage is optional */ }
  renderOnboarding();
}

function initialGPIOFilterState() {
  try {
    const stored = JSON.parse(localStorage.getItem(gpioFilterStorageKey) ?? "{}");
    return { gpioFilter: typeof stored.query === "string" ? stored.query : "", gpioAssignedOnly: Boolean(stored.assignedOnly) };
  } catch {
    return { gpioFilter: "", gpioAssignedOnly: false };
  }
}

function saveGPIOFilterState() {
  try {
    localStorage.setItem(gpioFilterStorageKey, JSON.stringify({ query: state.gpioFilter, assignedOnly: state.gpioAssignedOnly }));
  } catch { /* storage is optional */ }
}

function initialRoleColors() {
  try {
    const stored = JSON.parse(localStorage.getItem(roleColorStorageKey) ?? "{}");
    return Object.fromEntries(Object.entries(stored).filter(([key, value]) => /^(?:bus|dev|conn|pwr):[a-z][a-z0-9_]*$/.test(key) && /^#[0-9a-f]{6}$/i.test(value)));
  } catch {
    return {};
  }
}

function saveRoleColors() {
  try {
    localStorage.setItem(roleColorStorageKey, JSON.stringify(state.roleColors));
  } catch { /* storage is optional */ }
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
  document.querySelectorAll("[data-i18n-placeholder]").forEach((node) => { node.setAttribute("placeholder", t(node.dataset.i18nPlaceholder)); });
  document.querySelectorAll("[data-i18n-title]").forEach((node) => { node.setAttribute("title", t(node.dataset.i18nTitle)); });
  elements.languageSelect.value = state.language;
}

function renderOnboarding() {
  prepareMotionRegion(elements.onboarding);
  elements.onboarding.classList.toggle("is-open", state.onboardingOpen);
  elements.onboarding.setAttribute("aria-hidden", String(!state.onboardingOpen));
  elements.onboarding.inert = !state.onboardingOpen;
  elements.onboardingHelp.setAttribute("aria-expanded", String(state.onboardingOpen));
}

function prepareMotionRegion(element) {
  if (!element.classList.contains("motion-region")) {
    const inner = document.createElement("div");
    inner.className = "motion-region-inner";
    inner.append(...element.childNodes);
    element.append(inner);
    element.classList.add("motion-region");
    element.hidden = false;
  }
}

function prepareDisclosureTransitions(root = document) {
  for (const details of root.querySelectorAll("details:not(.motion-details)")) {
    const summary = details.querySelector(":scope > summary");
    if (!summary) continue;
    const collapse = document.createElement("div");
    const inner = document.createElement("div");
    collapse.className = "motion-collapse";
    inner.className = "motion-collapse-inner";
    while (summary.nextSibling) inner.append(summary.nextSibling);
    collapse.append(inner);
    details.append(collapse);
    details.classList.add("motion-details");
    details.dataset.motionTarget = details.open ? "open" : "closed";
    summary.addEventListener("click", (event) => {
      if (event.target !== summary && event.target.closest("button, input, select, textarea, a")) return;
      event.preventDefault();
      setDisclosureOpen(details, details.dataset.motionTarget !== "open");
    });
    details.addEventListener("toggle", () => {
      if (!details.classList.contains("motion-opening") && !details.classList.contains("motion-closing")) {
        details.dataset.motionTarget = details.open ? "open" : "closed";
      }
    });
  }
}

function clearDisclosureMotion(details) {
  const motion = disclosureMotionStates.get(details);
  if (!motion) return;
  if (motion.frame) cancelAnimationFrame(motion.frame);
  if (motion.nextFrame) cancelAnimationFrame(motion.nextFrame);
  if (motion.timer) clearTimeout(motion.timer);
  if (motion.collapse && motion.finish) motion.collapse.removeEventListener("transitionend", motion.finish);
  disclosureMotionStates.delete(details);
}

function setDisclosureOpen(details, open) {
  clearDisclosureMotion(details);
  details.dataset.motionTarget = open ? "open" : "closed";
  if (prefersReducedMotion()) {
    details.classList.remove("motion-opening", "motion-closing");
    details.open = open;
    return;
  }
  if (open) {
    details.classList.remove("motion-closing");
    if (details.open) return;
    details.classList.add("motion-opening");
    details.open = true;
    const motion = {};
    motion.frame = requestAnimationFrame(() => {
      motion.nextFrame = requestAnimationFrame(() => {
        if (details.dataset.motionTarget === "open") details.classList.remove("motion-opening");
        disclosureMotionStates.delete(details);
      });
    });
    disclosureMotionStates.set(details, motion);
    return;
  }
  if (!details.open) return;
  details.classList.remove("motion-opening");
  details.classList.add("motion-closing");
  const collapse = details.querySelector(":scope > .motion-collapse");
  const motion = { collapse };
  motion.finish = (event) => {
    if (event && event.propertyName !== "grid-template-rows") return;
    if (details.dataset.motionTarget === "closed") details.open = false;
    details.classList.remove("motion-closing");
    clearDisclosureMotion(details);
  };
  collapse.addEventListener("transitionend", motion.finish);
  motion.timer = setTimeout(() => motion.finish(), motionDuration + 50);
  disclosureMotionStates.set(details, motion);
}

function prefersReducedMotion() {
  return window.matchMedia?.("(prefers-reduced-motion: reduce)").matches;
}

function renderWithFade(callback) {
  if (!document.startViewTransition || prefersReducedMotion()) {
    callback();
    return;
  }
  if (state.viewTransition) state.viewTransition.skipTransition();
  const transition = document.startViewTransition(callback);
  state.viewTransition = transition;
  transition.finished.finally(() => {
    if (state.viewTransition === transition) state.viewTransition = null;
  });
}

function animateStateChange(element) {
  if (prefersReducedMotion()) return;
  element.classList.remove("state-change");
  requestAnimationFrame(() => element.classList.add("state-change"));
  setTimeout(() => element.classList.remove("state-change"), motionDuration);
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

function validationAction(item) {
  const key = `validationAction.${item.id}`;
  return messages[key] ? t(key, { board: currentBase().id }) : "";
}

function helpMark(key) {
  const message = escapeHtml(t(key));
  return `<span class="help-mark" tabindex="0" title="${message}" aria-label="${message}">?</span>`;
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
  try {
    localStorage.setItem(storageKey(entry.base.id), JSON.stringify(stored));
    const ids = new Set(JSON.parse(localStorage.getItem(draftIndexKey) ?? "[]"));
    ids.add(entry.base.id);
    localStorage.setItem(draftIndexKey, JSON.stringify([...ids]));
  } catch { /* storage is optional */ }
}

function removeDraft(id) {
  try {
    localStorage.removeItem(storageKey(id));
    const ids = new Set(JSON.parse(localStorage.getItem(draftIndexKey) ?? "[]"));
    ids.delete(id);
    localStorage.setItem(draftIndexKey, JSON.stringify([...ids]));
  } catch { /* storage is optional */ }
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
  try {
    for (const id of JSON.parse(localStorage.getItem(draftIndexKey) ?? "[]")) {
      if (state.documents.has(id)) continue;
      const stored = parseJson(localStorage.getItem(storageKey(id)), `draft:${id}`);
      const board = assertBoard(stored.board ?? stored);
      if (!chips[board.chip]) continue;
      state.documents.set(id, { base: clone(board), original: clone(board), dirty: true, expandedGPIO: stored.expandedGPIO ?? null, source: "draft" });
    }
  } catch { /* ignore stale draft indexes */ }
  for (const id of state.documents.keys()) {
    state.compositionEntries.set(id, clone(targets.m5unified_pin_table?.compositions?.[id]?.default?.accessories ?? []));
    const board = state.documents.get(id).base;
    state.runtimeSelections.set(id, Object.fromEntries(choiceSlots(board).filter(([, device]) => device.selected_by === "runtime").map(([slot, device]) => [slot, device.default])));
  }
  state.currentId = state.documents.has("m5stack_core2") ? "m5stack_core2" : [...state.documents.keys()][0];
}

function initializeDocument(board, { original = board, dirty = true, source = "editor" } = {}) {
  state.documents.set(board.id, { base: clone(board), original: clone(original), dirty, expandedGPIO: null, source });
  state.runtimeSelections.set(board.id, Object.fromEntries(choiceSlots(board).filter(([, device]) => device.selected_by === "runtime").map(([slot, device]) => [slot, device.default])));
  state.compositionEntries.set(board.id, []);
  state.currentId = board.id;
  state.lastExported = null;
  state.selectedOptions.clear();
  state.connectorId = null;
  state.connectorPosition = null;
  state.undo = null;
  if (dirty) writeDraft(state.documents.get(board.id));
}

function legacyIdWarning(value, currentId = null) {
  const duplicate = [...state.documents.values()].find((entry) => entry.base.id !== currentId && entry.base.legacy_board_id === value);
  return duplicate ? t("dialog.legacyWarning", { board: duplicate.base.name }) : "";
}

function validateBoardDialog() {
  const form = elements.boardDialogForm;
  const id = form.elements.id.value.trim();
  const legacy = Number(form.elements.legacy_board_id.value);
  let message = "";
  if (!(new RegExp(schema.definitions.id.pattern)).test(id)) message = t("validation.E_ID_FORMAT");
  else if (state.documents.has(id) && (state.boardDialogMode === "new" || id !== state.currentId)) message = t("dialog.idExists");
  else if (!form.elements.name.value.trim()) message = t("dialog.nameRequired");
  else if (!form.elements.official_name.value.trim()) message = t("dialog.officialNameRequired");
  else if (!Number.isInteger(legacy)) message = t("dialog.legacyInteger");
  else message = legacyIdWarning(legacy, state.boardDialogMode === "duplicate" ? state.currentId : null);
  elements.boardDialogError.textContent = message;
  elements.boardDialogError.classList.toggle("warning", message.startsWith("⚠"));
  return !message || message.startsWith("⚠");
}

function openBoardDialog(mode) {
  state.boardDialogMode = mode;
  const source = currentBase();
  const form = elements.boardDialogForm;
  elements.boardDialogTitle.textContent = t(mode === "new" ? "dialog.newTitle" : "dialog.duplicateTitle");
  form.elements.id.value = mode === "duplicate" ? `${source.id}_copy` : "";
  form.elements.name.value = mode === "duplicate" ? `${source.name} copy` : "";
  form.elements.official_name.value = mode === "duplicate" ? `${source.official_name} copy` : "";
  form.elements.legacy_board_id.value = mode === "duplicate" ? source.legacy_board_id : "";
  form.elements.chip.innerHTML = Object.keys(chips).sort().map((id) => `<option value="${escapeHtml(id)}">${escapeHtml(id)}</option>`).join("");
  form.elements.chip.value = mode === "duplicate" ? source.chip : Object.keys(chips).sort()[0];
  form.elements.chip.closest("label").hidden = mode === "duplicate";
  elements.boardDialogError.textContent = "";
  elements.boardDialog.showModal();
  form.elements.id.focus();
}

function closeCurrentDocument() {
  if (state.documents.size <= 1) return showToast(t("reason.lastBoard"));
  const entry = currentEntry();
  if (entry.dirty && !window.confirm(t("dialog.closeDirtyConfirm"))) return;
  const id = state.currentId;
  removeDraft(id);
  state.documents.delete(id);
  state.runtimeSelections.delete(id);
  state.compositionEntries.delete(id);
  state.currentId = state.documents.keys().next().value;
  state.lastExported = null;
  state.selectedOptions.clear();
  state.connectorId = null;
  renderAll();
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
    chip: chipForBoard(currentBase()), parts, accessories,
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
    chip: chipForBoard(board), parts, accessories,
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
  state.lastExported = null;
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
  return boardRoleOptions(board, { schema, chip: chipForBoard(board), parts, connectorTypes, resolveConnectorType, currentGPIO });
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
  const owner = roleOwner(role);
  const attributes = owner ? ` data-role-kind="${owner.kind}" style="${roleColorInlineStyle(owner)}"` : "";
  return `<span class="role-chip${compact ? " compact-chip" : ""}"${attributes}><span>${escapeHtml(display.plain)}</span>${display.source ? `<small>${escapeHtml(display.source)}</small>` : ""}${removable}</span>`;
}

function roleColorInlineStyle(owner, connectorPosition = false) {
  const color = roleColor(owner, state.roleColors);
  return `--role-bg:${color.background};--role-fg:${color.foreground}${connectorPosition ? `;--position-color:${color.background}` : ""}`;
}

function roleOwnerAttributes(owner, connectorPosition = false) {
  return `data-role-kind="${owner.kind}" style="${roleColorInlineStyle(owner, connectorPosition)}"`;
}

function roleOwnerLabel(owner, label = owner.id, className = "") {
  return `<span class="role-owner-label${className ? ` ${className}` : ""}" ${roleOwnerAttributes(owner)}>${escapeHtml(label)}</span>`;
}

function roleOwnersForBoard(board) {
  const owners = new Map();
  const addRoles = (pins) => {
    for (const pin of Object.values(pins ?? {})) for (const role of pin.roles ?? []) {
      const owner = roleOwner(role);
      if (owner) owners.set(owner.key, owner);
    }
  };
  addRoles(board.pins);
  for (const device of Object.values(board.devices ?? {})) {
    addRoles(device.pins);
    for (const choice of Object.values(device.choices ?? {})) addRoles(choice.pins);
  }
  return [...owners.values()].sort((left, right) => left.kind.localeCompare(right.kind) || left.id.localeCompare(right.id));
}

function renderRoleColorLegend(board) {
  const owners = roleOwnersForBoard(board);
  const items = owners.map((owner) => {
    const color = roleColor(owner, state.roleColors);
    const label = `${owner.kind}:${owner.id}`;
    return `<label class="role-color-item"><span class="role-owner-label" data-role-kind="${owner.kind}" style="${roleColorInlineStyle(owner)}">${escapeHtml(label)}</span><input type="color" value="${color.background}" data-role-color="${escapeHtml(owner.key)}" aria-label="${escapeHtml(t("roleColors.pick", { owner: label }))}"></label>`;
  }).join("");
  const prefixes = `<div class="role-prefix-legend"><strong>${escapeHtml(t("rolePrefixes.heading"))}</strong><span><code>bus:</code> ${escapeHtml(t("rolePrefixes.bus").replace(/^bus:\s*/, ""))}</span><span><code>dev:</code> ${escapeHtml(t("rolePrefixes.dev").replace(/^dev:\s*/, ""))}</span><span><code>conn:</code> ${escapeHtml(t("rolePrefixes.conn").replace(/^conn:\s*/, ""))}</span><span><code>|</code> ${escapeHtml(t("rolePrefixes.source").replace(/^\|\s*/, ""))}</span></div>`;
  elements.roleColorLegend.innerHTML = `${prefixes}<details${state.roleColorsOpen ? " open" : ""}><summary>${escapeHtml(t("roleColors.heading"))}</summary><div class="role-color-grid">${items || `<span>${escapeHtml(t("roleColors.empty"))}</span>`}<button type="button" class="secondary role-color-reset" data-reset-role-colors>${escapeHtml(t("roleColors.reset"))}</button></div></details>`;
}

function verificationSummary(pin) {
  const fields = ["roles", "pull", "note"].filter((field) => (
    Object.prototype.hasOwnProperty.call(pin, field) && (field !== "roles" || pin.roles.length > 0)
  ));
  if (!fields.length) return "<span>—</span>";
  return fields.map((field) => {
    const status = pin.verified?.[field] ?? "datasheet";
    return `<span><code>${escapeHtml(field)}</code>: ${escapeHtml(t(`evidence.${status}`, {}, status))}</span>`;
  }).join("");
}

function compactVerification(pin) {
  const fields = ["roles", "pull", "note"].filter((field) => (
    Object.prototype.hasOwnProperty.call(pin, field) && (field !== "roles" || pin.roles.length > 0)
  ));
  if (!fields.length) return "—";
  return fields.map((field) => {
    const status = pin.verified?.[field] ?? "datasheet";
    return `${field}:${t(`evidence.${status}`, {}, status)}`;
  }).join(" · ");
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

function validationFor(board) {
  const chip = chipForBoard(board);
  if (!chip) return [{ id: "E_REF_MISSING", path: "/chip", message: `embedded chip not found: ${board.chip}` }];
  const compositionTarget = targets.m5unified_pin_table ?? {};
  const context = {
    schema, chip, connectorTypes, parts, accessories,
    target: targets.m5gfx_board_desc,
    pinTableTarget: compositionTarget,
    composition: compositionTarget.compositions?.[board.id]?.default,
    allow_origins: compositionTarget.allow_origins,
  };
  return [...validateBoard(board, context), ...validateResolvedVariants(board, context)];
}

function renderBoardSelect() {
  const entries = [...state.documents.entries()].sort(([, a], [, b]) => a.base.chip.localeCompare(b.base.chip) || a.base.name.localeCompare(b.base.name));
  const options = entries.map(([id, entry]) => {
    const suffix = entry.dirty ? ` · ● ${t("toolbar.draftMarker")}` : "";
    const marker = entry.base.official_name_verified === true ? "" : " ⚠";
    return { value: id, label: `${entry.base.name} — ${entry.base.official_name}${marker}${suffix}`, officialName: entry.base.official_name, chip: entry.base.chip, legacyId: entry.base.legacy_board_id, aliases: entry.base.aliases ?? [], group: entry.base.chip };
  });
  if (!boardCombobox) {
    boardCombobox = new SearchCombobox(elements.boardSelect, options, (id) => {
      state.currentId = id;
      state.lastExported = null;
      state.selectedOptions.clear();
      state.connectorId = null;
      state.connectorPosition = null;
      renderAll();
    }, "", { preserveInput: true, selectOnFocus: true, showAllOnFocus: true, filterOptions: filterBoardOptions });
  } else boardCombobox.setOptions(options);
  boardCombobox.input.placeholder = t("toolbar.boardSearch");
  boardCombobox.setSelected(state.currentId);
}

function renderOptions() {
  const revisions = currentBase().revisions ?? [];
  const selectedRevision = [...state.selectedOptions][0] ?? "";
  const revisionHtml = `<label>${escapeHtml(t("options.revision"))}<select data-display-revision><option value="">${escapeHtml(t("options.none"))}</option>${revisions.map((revision) => `<option value="${escapeHtml(revision.id)}"${revision.id === selectedRevision ? " selected" : ""}>${escapeHtml(revision.name)}</option>`).join("")}</select></label>`;
  const runtime = state.runtimeSelections.get(state.currentId) ?? {};
  const runtimeHtml = choiceSlots(currentBase()).filter(([, device]) => device.selected_by === "runtime").map(([slot, device]) => (
    `<label>${escapeHtml(t("options.runtime"))}: ${escapeHtml(slot.replaceAll("_", " ").toUpperCase())}<select data-display-runtime="${escapeHtml(slot)}"><option value=""${runtime[slot] === undefined ? " selected" : ""}>${escapeHtml(t("options.none"))}</option>${Object.keys(device.choices).map((choice) => `<option value="${escapeHtml(choice)}"${choice === runtime[slot] ? " selected" : ""}>${escapeHtml(choice)}</option>`).join("")}</select></label>`
  )).join("");
  elements.optionPanel.innerHTML = `<fieldset><legend>${escapeHtml(t("options.legend"))} ${helpMark("help.displayConfiguration")}</legend>${revisionHtml}${runtimeHtml}</fieldset>`;
  const revision = revisions.find((item) => item.id === selectedRevision);
  const choices = Object.entries({ ...revisionSelection(currentBase(), revision), ...runtime }).map(([slot, choice]) => `${slot}: ${choice}`).join(", ") || t("options.none");
  elements.modeNote.textContent = isResolvedMode()
    ? t("mode.resolved", { board: currentBase().name, revision: revision?.name ?? t("options.none"), choices })
    : t("mode.base");
}

function renderTable(board, errors) {
  const chip = chipForBoard(board);
  const schemaColumns = schema["x-editor"]?.pinColumns ?? [];
  const connectorLanes = assignConnectorLanes(board.pins);
  const hasConnectors = connectorLanes.lanes.length > 0;
  const compactIds = ["gpio", "internalRoles", ...(hasConnectors ? ["connectorRoles"] : []), "pull", "verified", "validation"];
  const columns = compactIds.map((id) => schemaColumns.find((column) => column.id === id) ?? { id, label: id });
  const columnLabels = {
    gpio: t("column.gpio"), internalRoles: t("column.internalRoles"), connectorRoles: t("column.connectors"), pull: t("column.pull"),
    verified: t("column.verified"), validation: t("column.validation"),
  };
  const columnHelp = { internalRoles: "help.roles", connectorRoles: "help.roles", verified: "help.verification", validation: "help.validation" };
  elements.connectorColumn.hidden = !hasConnectors;
  elements.tableHead.closest("table").classList.toggle("has-connector-lanes", hasConnectors);
  elements.tableHead.innerHTML = columns.map((column) => `<th class="column-${escapeHtml(column.id)}">${escapeHtml(columnLabels[column.id] ?? column.label)}${columnHelp[column.id] ? ` ${helpMark(columnHelp[column.id])}` : ""}</th>`).join("");
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
    if (chip.absent.includes(gpio)) continue;
    const key = String(gpio);
    const pin = board.pins?.[key] ?? { roles: [] };
    if (!gpioMatchesFilter(key, pin, state.gpioFilter, state.gpioAssignedOnly)) continue;
    const conditionalReserved = (chip.reserved_conditional?.[board.spec?.storage?.psram_mode] ?? []).includes(gpio);
    const readonlyReason = readonlyReasons.get(key) ?? (chip.reserved.includes(gpio) ? t("reason.chipReserved") : conditionalReserved ? t("reason.chipReservedConditional") : "");
    const readonly = readonlyAttributes(readonlyReason);
    const expanded = currentEntry().expandedGPIO === key;
    const closing = closingGPIOs.has(key);
    const rowErrors = errorsByGPIO.get(key) ?? [];
    const classes = ["gpio-row", gpio % 2 ? "zebra-odd" : "zebra-even"];
    if (chip.reserved.includes(gpio) || conditionalReserved) classes.push("gpio-reserved");
    if (chip.input_only.includes(gpio)) classes.push("gpio-input");
    if (chip.strapping.includes(gpio)) classes.push("gpio-strapping");
    if (rowErrors.length) classes.push("row-error");
    if (readonlyReason) classes.push("is-readonly");
    if (expanded) classes.push("is-expanded");
    const allRoles = pin.roles ?? [];
    const internalRoles = allRoles.filter((role) => roleOwner(role)?.kind !== "conn");
    const connectorRoles = connectorRolesByLane(allRoles, connectorLanes);
    const psramShared = allRoles.some((role) => role.startsWith("dev:psram.")) && allRoles.some((role) => role.startsWith("conn:"));
    const compactInternalRoles = internalRoles.map((role) => roleChip(board, role, true)).join("");
    const compactConnectorRoles = connectorRoles.map((lane, laneIndex) => `<div class="connector-role-lane" data-connector-lane="${laneIndex}">${lane.map((role) => roleChip(board, role, true)).join("")}</div>`).join("");
    const safeKey = escapeHtml(key);
    const editableRoles = allRoles.map((role) => roleChip(board, role, false, `<button type="button" aria-label="${escapeHtml(t("detail.removeRole"))}" data-remove-role="${encodeURIComponent(role)}" data-gpio="${safeKey}"${readonly || ` title="${escapeHtml(t("detail.removeRole"))}"`}>×</button>`)).join("");
    const pull = pin.pull ?? "";
    const errorSummary = rowErrors.length
      ? `<span class="error-indicator" title="${escapeHtml(rowErrors.map((item) => item.id).join(", "))}">! ${rowErrors.length}</span>`
      : allRoles.length ? '<span class="ok-indicator">✓</span>' : "";
    const readonlyOrigin = `${readonlyReason ? `<small class="row-readonly-origin" title="${escapeHtml(readonlyReason)}">🔒 ${escapeHtml(readonlyReason)}</small>` : ""}${psramShared ? `<small class="psram-shared-note" title="${escapeHtml(t("gpio.psramSharedDetail"))}">⚠ ${escapeHtml(t("gpio.psramShared"))}</small>` : ""}`;
    const rowHint = `<span class="row-edit-hint" aria-hidden="true"><span>${escapeHtml(t("gpio.rowHint"))} ${expanded ? "▾" : "▸"}</span></span>`;
    const cells = {
      gpio: `<td class="gpio-number">${safeKey}${gpio === chip.usb?.dn ? '<small class="gpio-badge">USB D−</small>' : ""}${gpio === chip.usb?.dp ? '<small class="gpio-badge">USB D+</small>' : ""}${(chip.reserved_conditional?.opi ?? []).includes(gpio) && !conditionalReserved ? `<small class="gpio-badge">${escapeHtml(t("gpio.opiReserved"))}</small>` : ""}</td>`,
      internalRoles: `<td class="gpio-roles-cell gpio-internal-roles-cell"><div class="role-row-main"><div class="compact-roles">${compactInternalRoles || "<span>—</span>"}</div>${rowHint}</div>${readonlyOrigin}</td>`,
      connectorRoles: `<td class="gpio-connector-roles-cell"><div class="connector-role-lanes">${compactConnectorRoles}</div></td>`,
      pull: `<td>${escapeHtml(pull || "—")}</td>`,
      verified: `<td class="compact-verified">${escapeHtml(compactVerification(pin))}</td>`,
      validation: `<td>${errorSummary}</td>`,
    };
    const renderedCells = columns.map((column) => cells[column.id] ?? "<td>—</td>").join("");
    rows.push(`<tr id="gpio-${safeKey}" data-gpio-row="${safeKey}" tabindex="0" role="button" aria-label="GPIO ${safeKey}: ${escapeHtml(t("gpio.rowHint"))}" aria-expanded="${expanded}" class="${classes.join(" ")}" title="${escapeHtml(t("gpio.rowHint"))}">${renderedCells}</tr>`);
    if (expanded || closing) {
      const related = relatedObjectsForGPIO(board, key);
      const transitionClass = openingGPIOs.has(key) ? " is-opening" : closingGPIOsStarted.has(key) ? " is-closing" : closing ? " is-closing-pending" : "";
      rows.push(`<tr class="gpio-detail-row${transitionClass}" data-gpio-detail="${safeKey}"><td colspan="${columns.length}"><div class="motion-collapse"><div class="motion-collapse-inner"><div class="gpio-detail">
        <section class="detail-roles"><h3>${escapeHtml(t("column.roles"))}</h3><div class="role-list">${editableRoles || "<span>—</span>"}</div>
          <div class="search-combobox" data-role-combobox="${safeKey}"><input type="text" role="combobox" aria-autocomplete="list" aria-expanded="false" placeholder="${escapeHtml(t("detail.searchRole"))}" autocomplete="off"${readonly}><ul role="listbox"></ul></div>${readonlyNote(readonlyReason)}
        </section>
        <label>${escapeHtml(t("column.pull"))}<select class="cell-select" data-pull data-gpio="${safeKey}"${readonly}><option value=""${pull === "" ? " selected" : ""}>${escapeHtml(t("detail.empty"))}</option><option value="up"${pull === "up" ? " selected" : ""}>up</option><option value="down"${pull === "down" ? " selected" : ""}>down</option><option value="none"${pull === "none" ? " selected" : ""}>none</option></select>${chip.no_internal_pull?.includes(gpio) ? `<small class="readonly-note">${escapeHtml(t("gpio.noInternalPull"))}</small>` : ""}${readonlyNote(readonlyReason)}</label>
        <label>${escapeHtml(t("column.note"))}<input class="cell-input note-input" data-note data-gpio="${safeKey}" type="text" value="${escapeHtml(pin.note ?? "")}"${readonly}>${readonlyNote(readonlyReason)}</label>
        <section><h3>${escapeHtml(t("column.verified"))}</h3><div class="verified">${verificationSummary(pin)}</div></section>
        <section><h3>${escapeHtml(t("detail.related"))}</h3><div class="detail-related">${related || `<span>${escapeHtml(t("common.none"))}</span>`}</div></section>
      </div></div></div></td></tr>`);
    }
  }
  elements.tableBody.innerHTML = rows.join("");
  requestAnimationFrame(() => requestAnimationFrame(() => {
    sizeConnectorLanes(connectorLanes.lanes.length);
    for (const gpio of openingGPIOs) {
      elements.tableBody.querySelector(`[data-gpio-detail="${gpio}"]`)?.classList.remove("is-opening");
    }
    openingGPIOs.clear();
    for (const gpio of closingGPIOs) {
      if (closingGPIOsStarted.has(gpio)) continue;
      const detail = elements.tableBody.querySelector(`[data-gpio-detail="${gpio}"]`);
      if (!detail) continue;
      closingGPIOsStarted.add(gpio);
      detail.classList.remove("is-closing-pending");
      detail.classList.add("is-closing");
    }
  }));
  for (const combobox of elements.tableBody.querySelectorAll("[data-role-combobox]")) {
    const gpio = combobox.dataset.roleCombobox;
    const assigned = new Set(board.pins?.[gpio]?.roles ?? []);
    new SearchCombobox(combobox, allRoleOptions(board, gpio).filter((role) => !assigned.has(role)).map((role) => ({ value: role, label: roleDisplay(board, role).plain, search: role })), (role) => addRoleUI(gpio, role), readonlyReasons.get(gpio), {
      emptyMessage: (query) => t("detail.noRoleMatches", { query: query || "…" }),
    });
  }
}

function sizeConnectorLanes(laneCount) {
  const table = elements.tableHead.closest("table");
  if (!laneCount) {
    table.style.removeProperty("--connector-lane-columns");
    table.style.removeProperty("--connector-column-width");
    return;
  }
  const gap = 4;
  const widths = Array.from({ length: laneCount }, (_, laneIndex) => Math.max(1, ...[...elements.tableBody.querySelectorAll(`[data-connector-lane="${laneIndex}"] .role-chip`)].map((chip) => Math.ceil(chip.getBoundingClientRect().width))));
  table.style.setProperty("--connector-lane-columns", widths.map((width) => `${width}px`).join(" "));
  table.style.setProperty("--connector-column-width", `${widths.reduce((sum, width) => sum + width, 0) + gap * Math.max(0, laneCount - 1) + 16}px`);
}

function renderValidation(board, errors) {
  const errorCount = errors.filter((item) => item.severity !== "warning").length;
  elements.validationCount.textContent = t("validation.count", { count: errors.length });
  elements.validationCount.classList.toggle("has-errors", errorCount > 0);
  elements.validationSection.open = disclosureOpen("validation", errorCount > 0);
  if (!errors.length) {
    elements.validationList.innerHTML = `<div class="validation-ok">${escapeHtml(t("validation.ok"))}</div>`;
    return;
  }
  elements.validationList.innerHTML = `<ul class="error-list">${errors.map((item, index) => {
    const gpio = gpioForError(item, board);
    const reason = gpio === null ? t("unavailable.noGPIO") : "";
    const action = validationAction(item);
    return `<li class="${item.severity === "warning" ? "warning" : "error"}"><button type="button" data-error-index="${index}"${readonlyAttributes(reason)}><strong>${escapeHtml(item.id)}</strong>: ${escapeHtml(validationMessage(item))}<code>${escapeHtml(item.path || "/")}</code>${action ? `<small class="validation-action">${escapeHtml(action)}</small>` : ""}</button></li>`;
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
        const owner = objectRoleOwner(prefix, id);
        links.push(`<button type="button" class="related-object-link" ${roleOwnerAttributes(owner)} data-target-object="${field}:${escapeHtml(id)}">${escapeHtml(t("detail.relatedButton", { object: `${prefix}:${id}` }))}</button>`);
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
    const fill = endpointGPIOs(endpoint).length
      ? ` style="${roleColorInlineStyle(objectRoleOwner("conn", connectorId), true)}"`
      : position.color ? ` style="--position-color:${escapeHtml(position.color)}"` : "";
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
  const connectorOwner = objectRoleOwner("conn", state.connectorId);
  const color = endpointGPIOs(endpoint).length
    ? `<span class="connector-color" style="${roleColorInlineStyle(connectorOwner, true)}"></span>`
    : position.color ? `<span class="connector-color" style="--position-color:${escapeHtml(position.color)}"></span>` : "";
  if (side === "left") return `<td ${data} class="connector-table-position pin-endpoint pin-left ${classes}" title="${endpointText}">${endpointText}</td><td ${data} class="connector-table-position pin-function pin-left ${classes}" title="${functionText}">${functionText}</td><td ${data} class="connector-table-position pin-number pin-left ${classes}" role="button" tabindex="0">${color}${positionText}</td>`;
  if (side === "right") return `<td ${data} class="connector-table-position pin-number pin-right pin-divider ${classes}" role="button" tabindex="0">${color}${positionText}</td><td ${data} class="connector-table-position pin-function pin-right ${classes}" title="${functionText}">${functionText}</td><td ${data} class="connector-table-position pin-endpoint pin-right ${classes}" title="${endpointText}">${endpointText}</td>`;
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
  const chip = chipForBoard(board);
  return Array.from({ length: chip.gpio_count }, (_, gpio) => gpio).filter((gpio) => !chip.absent.includes(gpio)).map((gpio) => {
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
  elements.connectorSectionTitle.textContent = `${t("connector.pinoutHeading")} (${ids.length})`;
  elements.connectorSection.open = disclosureOpen("connectorMap");
  elements.connectorTabs.innerHTML = ids.map((id) => `<button type="button" role="tab" aria-selected="${id === state.connectorId}" class="connector-object-tab ${id === state.connectorId ? "active" : ""}" ${roleOwnerAttributes(objectRoleOwner("conn", id))} data-connector-tab="${escapeHtml(id)}">${escapeHtml(id)}</button>`).join("");
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
  const count = groups.reduce((sum, [field]) => sum + Object.keys(board[field] ?? {}).length, 0);
  elements.relatedSectionTitle.textContent = `${t("related.heading")} (${count})`;
  elements.relatedSection.open = disclosureOpen("related");
  elements.relatedList.innerHTML = groups.map(([field, prefix, label]) => {
    const items = Object.entries(board[field] ?? {}).map(([id, object]) => {
      const pins = objectAssignments(board, prefix, id, object);
      const gpioLinks = pins.map(gpioTargetButton).join("");
      const ownerLinks = field === "devices" ? [...Object.values(object.signals ?? {}), object.enable].filter(Boolean).flatMap((endpoint) => {
        const match = /^pin:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(endpoint);
        return match ? [`<button type="button" data-owner-target="${escapeHtml(`${match[1]}:${match[2]}`)}">${escapeHtml(endpoint)}</button>`] : [];
      }).join("") : "";
      const links = gpioLinks + ownerLinks;
      return `<div class="object-item" data-object-key="${field}:${escapeHtml(id)}"><div class="object-title">${roleOwnerLabel(objectRoleOwner(prefix, id), id, "object-owner-label")}<span>${escapeHtml(object.kind ?? object.type ?? "")}</span></div><div class="object-summary">${objectSummary(field, object)}</div><div class="gpio-links">${links || `<span>${escapeHtml(t("related.noGPIO"))}</span>`}</div></div>`;
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

function valueAtPath(object, path) {
  return path.split(".").reduce((value, key) => value?.[key], object);
}

function scalarInput(value, attributes, type = "string", placeholder = "", choices = null) {
  if (type === "boolean") return `<select ${attributes}><option value="">—${placeholder !== "" ? ` (${escapeHtml(placeholder)})` : ""}</option><option value="true"${value === true ? " selected" : ""}>true</option><option value="false"${value === false ? " selected" : ""}>false</option></select>`;
  if (choices) return `<select ${attributes}><option value="">—${placeholder !== "" ? ` (${escapeHtml(placeholder)})` : ""}</option>${choices.map((choice) => `<option value="${escapeHtml(choice)}"${value === choice ? " selected" : ""}>${escapeHtml(choice)}</option>`).join("")}</select>`;
  return `<input ${attributes} type="${type === "number" ? "number" : "text"}" value="${escapeHtml(value ?? "")}"${placeholder !== "" ? ` placeholder="${escapeHtml(placeholder)}"` : ""}>`;
}

function boardSpecFields(board) {
  const fieldLabels = {
    "display.touch": t("field.displayTouch"),
    "storage.psram_mb": t("field.psramSize"),
    "storage.psram_mode": t("field.psramMode"),
    "storage.flash_mb": t("field.flashSize"),
    "storage.sd": t("field.sdStorage"),
    "power.pmic_part": t("field.pmicPart"),
    "power.battery_mah": t("field.batteryCapacity"),
    "power.usb": t("field.usbPower"),
    "links.schematic": t("field.schematicLink"),
    "links.product": t("field.productLink"),
  };
  const fields = [
    ["display.touch", "boolean"], ["storage.psram_mb", "number"], ["storage.psram_mode", "string"],
    ["storage.flash_mb", "number"], ["storage.sd", "boolean"], ["power.pmic_part", "string"],
    ["power.battery_mah", "number"], ["power.usb", "string"], ["links.schematic", "string"], ["links.product", "string"],
  ];
  const resolution = valueAtPath(board.spec, "display.resolution") ?? [];
  return `<div class="field-grid"><label>${escapeHtml(t("field.displayWidth"))}<input data-board-resolution="0" type="number" value="${escapeHtml(resolution[0] ?? "")}"></label><label>${escapeHtml(t("field.displayHeight"))}<input data-board-resolution="1" type="number" value="${escapeHtml(resolution[1] ?? "")}"></label>${fields.map(([path, type]) => `<label>${escapeHtml(fieldLabels[path])}${scalarInput(valueAtPath(board.spec, path), `data-board-spec="${path}" data-value-type="${type}"`, type)}</label>`).join("")}</div>`;
}

function renderBoardInfo(shown) {
  const board = currentBase();
  const resolution = shown.spec?.display?.resolution;
  const display = Array.isArray(resolution) && resolution.length >= 2 ? `${resolution[0]}×${resolution[1]}` : "—";
  const psram = shown.spec?.storage?.psram_mb ? `PSRAM ${shown.spec.storage.psram_mb}MB` : "PSRAM —";
  elements.boardInfoSummary.textContent = `${shown.name} — ${shown.chip} — ${display} — ${psram}`;
  elements.boardInfo.open = disclosureOpen("boardInfo");
  elements.boardInfoBody.innerHTML = `<div class="field-grid board-info-grid"><label>${escapeHtml(t("field.id"))}<code class="readonly-value">${escapeHtml(board.id)}</code></label><label>${escapeHtml(t("field.name"))}<input data-board-field="name" value="${escapeHtml(board.name)}"></label><label>${escapeHtml(t("field.officialName"))}<input data-board-field="official_name" value="${escapeHtml(board.official_name)}"></label><label>${escapeHtml(t("field.legacyId"))}<input data-board-field="legacy_board_id" data-value-type="number" type="number" value="${escapeHtml(board.legacy_board_id)}"></label><label>${escapeHtml(t("field.chip"))}<select data-board-field="chip">${Object.keys(chips).map((id) => `<option value="${escapeHtml(id)}"${id === board.chip ? " selected" : ""}>${escapeHtml(id)}</option>`).join("")}</select></label><label class="official-verification-field"><span>${escapeHtml(t("field.officialNameStatus"))} ${helpMark("help.officialNameVerified")}</span><span class="checkbox-row"><input data-board-field="official_name_verified" data-value-type="boolean" type="checkbox"${board.official_name_verified === true ? " checked" : ""}>${escapeHtml(t("field.officialNameVerified"))}${board.official_name_verified === true ? "" : `<small class="unverified-mark">${escapeHtml(t("field.officialNameUnverified"))}</small>`}</span></label><label class="aliases-field">${escapeHtml(t("field.aliases"))}<input data-board-field="aliases" value="${escapeHtml((board.aliases ?? []).join(", "))}" placeholder="${escapeHtml(t("field.aliasesPlaceholder"))}"></label></div>${boardSpecFields(board)}`;
}

function configurationDisclosure(id, label, count, description, body) {
  return `<details class="config-group sidebar-disclosure" data-layout-section="${escapeHtml(id)}"${disclosureOpen(id) ? " open" : ""}><summary><strong>${escapeHtml(label)} (${count})</strong><span class="disclosure-chevron" aria-hidden="true">⌄</span></summary><div class="sidebar-section-body"><p class="config-help">${escapeHtml(description)}</p>${body}</div></details>`;
}

function busRemovalReason(board, id) {
  if (Object.values(board.devices ?? {}).some((device) => device.bus === id || Object.values(device.choices ?? {}).some((choice) => choice.bus === id))) return t("reason.busInUse");
  if (Object.values(board.pins ?? {}).some((pin) => (pin.roles ?? []).some((role) => role.startsWith(`bus:${id}.`)))) return t("reason.busInUse");
  return "";
}

function objectRemovalReason(board, prefix, id) {
  if (Object.values(board.pins ?? {}).some((pin) => (pin.roles ?? []).some((role) => role.startsWith(`${prefix}:${id}.`)))) return t(prefix === "dev" ? "reason.deviceInUse" : "reason.connectorInUse");
  return "";
}

function hostSuggestions(kind, chip) {
  if (kind === "spi" && Number.isInteger(chip.spi_hosts)) return Array.from({ length: chip.spi_hosts }, (_, index) => `SPI${index + 2}_HOST`);
  const count = kind === "i2c" ? chip.i2c_hosts : kind === "i2s" ? chip.i2s_ports : undefined;
  return Number.isInteger(count) ? Array.from({ length: count }, (_, index) => index) : [];
}

function specKeys(device, fragment = device) {
  const part = parts[fragment.part];
  if (part?.spec_keys) return part.spec_keys;
  return Object.fromEntries((schema.definitions.deviceKind["x-spec-properties"][fragment.kind ?? device.kind] ?? [])
    .map((key) => [key, { type: ["invert", "readable"].includes(key) ? "boolean" : key === "panel_type" ? "string" : "number" }]));
}

function deviceSpecType(definition) {
  return ["integer", "number"].includes(definition.type) ? "number" : definition.type;
}

function deviceSpecEditor(deviceId, device, choiceId = null) {
  const fragment = choiceId === null ? device : device.choices[choiceId];
  const part = parts[fragment.part];
  const rows = Object.entries(specKeys(device, fragment)).map(([key, definition]) => {
    const value = fragment.spec?.[key];
    const fallback = definition.default;
    const type = deviceSpecType(definition);
    const attrs = `data-device-spec="${escapeHtml(deviceId)}" data-choice-id="${escapeHtml(choiceId ?? "")}" data-spec-key="${escapeHtml(key)}" data-value-type="${type}"`;
    return `<label>${escapeHtml(key)}${scalarInput(value, attrs, type, fallback ?? "", definition.enum)}</label>`;
  }).join("");
  return `<div class="field-grid device-spec">${rows || `<span>${escapeHtml(t("configuration.noSpec"))}</span>`}</div>`;
}

function deviceRow(board, id, device) {
  const required = requiredDeviceSignals(device);
  const optional = deviceSignals(device).filter((signal) => !required.includes(signal));
  const missing = required.filter((signal) => !roleAssigned(board, `dev:${id}.${signal}`));
  const availableOptional = optional.filter((signal) => !roleAssigned(board, `dev:${id}.${signal}`));
  const reason = objectRemovalReason(board, "dev", id);
  const choices = device.choices ? Object.entries(device.choices).map(([choiceId, choice]) => `<section class="choice-editor"><div class="object-title"><code>${escapeHtml(choiceId)}</code><span>${escapeHtml(choice.part ?? "")}</span><button type="button" class="secondary" data-remove-choice="${escapeHtml(id)}" data-choice-id="${escapeHtml(choiceId)}">${escapeHtml(t("configuration.remove"))}</button></div>${deviceSpecEditor(id, device, choiceId)}</section>`).join("") : deviceSpecEditor(id, device);
  const choiceControls = device.choices
    ? `<div class="choice-settings"><label>${escapeHtml(t("configuration.selectedBy"))}<select data-choice-selected-by="${escapeHtml(id)}"><option value="revision"${device.selected_by === "revision" ? " selected" : ""}>revision</option><option value="runtime"${device.selected_by === "runtime" ? " selected" : ""}>runtime</option><option value="user"${device.selected_by === "user" ? " selected" : ""}>user</option></select></label><label>${escapeHtml(t("configuration.defaultChoice"))}<select data-choice-default="${escapeHtml(id)}">${Object.keys(device.choices).map((choice) => `<option value="${escapeHtml(choice)}"${choice === device.default ? " selected" : ""}>${escapeHtml(choice)}</option>`).join("")}</select></label></div>`
    : "";
  const partChoices = Object.values(parts).filter((part) => part.kind === device.kind && part.id !== device.part).map((part) => `<option value="${escapeHtml(part.id)}">${escapeHtml(part.name)}</option>`).join("");
  return `<details class="config-object"><summary>${roleOwnerLabel(objectRoleOwner("dev", id), id, "configuration-owner-label")}<span>${escapeHtml(device.part ?? device.kind)}</span><button type="button" class="secondary" data-remove-device="${escapeHtml(id)}"${readonlyAttributes(reason)}>${escapeHtml(t("configuration.remove"))}</button></summary>${readonlyNote(reason)}${choiceControls}${choices}<div class="choice-add"><select data-new-choice-part="${escapeHtml(id)}">${partChoices}</select>${device.choices ? "" : `<select data-new-choice-selected-by="${escapeHtml(id)}"><option value="runtime">runtime</option><option value="revision">revision</option><option value="user">user</option></select>`}<button type="button" class="secondary" data-add-choice="${escapeHtml(id)}">${escapeHtml(t("configuration.addChoice"))}</button></div>${missing.length ? `<div class="unassigned"><strong>${escapeHtml(t("configuration.unassigned"))}:</strong>${missing.map((signal) => `<button type="button" class="secondary" data-unassigned-role="${escapeHtml(`dev:${id}.${signal}`)}">${escapeHtml(signal)}</button>`).join("")}</div>` : ""}${availableOptional.length ? `<div class="unassigned optional"><strong>${escapeHtml(t("configuration.optional"))}:</strong>${availableOptional.map((signal) => `<button type="button" class="secondary" data-unassigned-role="${escapeHtml(`dev:${id}.${signal}`)}">${escapeHtml(signal)}</button>`).join("")}</div>` : ""}</details>`;
}

function addField(label, control, className = "") {
  return `<label class="config-add-field${className ? ` ${className}` : ""}"><span>${escapeHtml(label)}</span>${control}</label>`;
}

function renderConfiguration() {
  const board = currentBase();
  const connectorRows = Object.entries(board.connectors ?? {}).map(([id, connector]) => {
    const type = connectorTypes[connector.type];
    const standards = Object.keys(type?.standards ?? {});
    const reason = objectRemovalReason(board, "conn", id);
    const mark = reason ? `<span class="reference-mark" title="${escapeHtml(reason)}" aria-label="${escapeHtml(reason)}">🔗</span>` : "";
    return `<div class="config-row"><span class="config-id">${mark}${roleOwnerLabel(objectRoleOwner("conn", id), id, "configuration-owner-label")}</span><span>${escapeHtml(connector.type)}</span><select data-config-standard="${escapeHtml(id)}"><option value="">—</option>${standards.map((standard) => `<option value="${escapeHtml(standard)}"${standard === connector.standard ? " selected" : ""}>${escapeHtml(standard)}</option>`).join("")}</select><button type="button" class="secondary" data-remove-connector="${escapeHtml(id)}"${readonlyAttributes(reason)}>${escapeHtml(t("configuration.remove"))}</button></div>`;
  }).join("");
  const busRows = Object.entries(board.buses ?? {}).map(([id, bus]) => {
    const reason = busRemovalReason(board, id);
    const mark = reason ? `<span class="reference-mark" title="${escapeHtml(reason)}" aria-label="${escapeHtml(reason)}">🔗</span>` : "";
    return `<div class="config-row"><span class="config-id">${mark}${roleOwnerLabel(objectRoleOwner("bus", id), id, "configuration-owner-label")}</span><span>${escapeHtml(bus.kind)}</span><span>${escapeHtml((bus.signals ?? []).join(", "))}</span><button type="button" class="secondary" data-remove-bus="${escapeHtml(id)}"${readonlyAttributes(reason)}>${escapeHtml(t("configuration.remove"))}</button></div>`;
  }).join("");
  const deviceRows = Object.entries(board.devices ?? {}).map(([id, device]) => deviceRow(board, id, device)).join("");
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
  const busKinds = schema.definitions.busKind.enum.map((kind) => `<option value="${escapeHtml(kind)}">${escapeHtml(kind)}</option>`).join("");
  const deviceKinds = schema.definitions.deviceKind.enum.map((kind) => `<option value="${escapeHtml(kind)}">${escapeHtml(kind)}</option>`).join("");
  const spiSignals = schemaSignals("busKind").spi.map((signal) => `<label><input type="checkbox" data-new-bus-signal value="${escapeHtml(signal)}"${["sclk", "mosi"].includes(signal) ? " checked" : ""}>${escapeHtml(signal)}</label>`).join("");
  elements.configurationList.innerHTML = `
    ${configurationDisclosure("buses", t("configuration.buses"), Object.keys(board.buses ?? {}).length, t("configuration.busesDescription"), `${busRows}<details class="config-add-panel"><summary>${escapeHtml(t("configuration.addBus"))}</summary><div class="config-add bus-add">${addField(t("configuration.idLabel"), `<input data-new-bus-id placeholder="${escapeHtml(t("configuration.busIdPlaceholder"))}">`)}${addField(t("configuration.kindLabel"), `<select data-new-bus-kind>${busKinds}</select>`)}<fieldset class="signal-checks"><legend>${escapeHtml(t("configuration.signalsLabel"))}</legend><div data-new-bus-signals>${spiSignals}</div></fieldset>${addField(t("configuration.hostLabel"), `<input data-new-bus-host list="new-bus-hosts" placeholder="${escapeHtml(t("configuration.hostPlaceholder"))}"><datalist id="new-bus-hosts"></datalist>`)}${addField(t("configuration.freqLabel"), `<input data-new-bus-freq type="number" placeholder="${escapeHtml(t("configuration.freqPlaceholder"))}">`)}${addField(t("configuration.freqReadLabel"), `<input data-new-bus-freq-read type="number" placeholder="${escapeHtml(t("configuration.freqReadPlaceholder"))}">`)}<label class="config-check"><input data-new-bus-fixed type="checkbox">${escapeHtml(t("configuration.fixedLabel"))}</label><button type="button" data-add-bus>${escapeHtml(t("configuration.add"))}</button></div></details>`)}
    ${configurationDisclosure("connectors", t("configuration.connectors"), Object.keys(board.connectors ?? {}).length, t("configuration.connectorsDescription"), `${connectorRows}<details class="config-add-panel"><summary>${escapeHtml(t("configuration.addConnector"))}</summary><div class="config-add">${addField(t("configuration.idLabel"), `<input data-new-connector-id placeholder="${escapeHtml(t("configuration.connectorIdPlaceholder"))}">`)}${addField(t("configuration.typeLabel"), `<select data-new-connector-type>${typeOptions}</select>`)}<button type="button" data-add-connector>${escapeHtml(t("configuration.add"))}</button></div></details>`)}
    ${configurationDisclosure("devices", t("configuration.devices"), Object.keys(board.devices ?? {}).length, t("configuration.devicesDescription"), `${deviceRows}<details class="config-add-panel"><summary>${escapeHtml(t("configuration.addDevice"))}</summary><div class="config-add device-add">${addField(t("configuration.idLabel"), `<input data-new-device-id placeholder="${escapeHtml(t("configuration.deviceIdPlaceholder"))}">`)}${addField(t("configuration.kindLabel"), `<select data-new-device-kind>${deviceKinds}</select>`)}${addField(t("configuration.partLabel"), `<div class="search-combobox" data-new-device-part><input type="text" role="combobox" aria-autocomplete="list" aria-expanded="false" placeholder="${escapeHtml(t("configuration.partOptional"))}" autocomplete="off"><ul role="listbox"></ul></div>`)}${addField(t("configuration.busLabel"), `<select data-new-device-bus><option value="">—</option></select>`)}<button type="button" data-add-device>${escapeHtml(t("configuration.add"))}</button><small data-sd-note hidden></small></div></details>`)}
    ${configurationDisclosure("accessories", t("configuration.accessories"), candidates.length, t("configuration.accessoriesDescription"), accessoryRows || `<p>${escapeHtml(t("related.empty"))}</p>`)}`;
  const partRoot = elements.configurationList.querySelector("[data-new-device-part]");
  partRoot.dataset.value = "";
  new SearchCombobox(partRoot, Object.values(parts).map((part) => ({ value: part.id, label: `${part.kind} · ${part.name}`, search: `${part.id} ${part.vendor ?? ""}` })), (partId) => {
    partRoot.dataset.value = partId;
    const part = parts[partId];
    elements.configurationList.querySelector("[data-new-device-kind]").value = part.kind;
    updateNewDeviceBuses();
    partRoot.querySelector("input").placeholder = `${part.kind} · ${part.name}`;
  });
  updateNewBusSignals();
  updateNewDeviceBuses();
}

function controlValue(control) {
  if (control.type === "checkbox") return control.checked;
  if (control.value === "") return undefined;
  if (control.dataset.valueType === "number") return Number(control.value);
  if (control.dataset.valueType === "boolean") return control.value === "true";
  return control.value;
}

function updateNewBusSignals() {
  const kind = elements.configurationList.querySelector("[data-new-bus-kind]")?.value;
  const root = elements.configurationList.querySelector("[data-new-bus-signals]");
  if (!kind || !root) return;
  const defaults = kind === "spi" ? new Set(["sclk", "mosi"]) : kind === "i2c" ? new Set(["sda", "scl"]) : new Set();
  root.innerHTML = (schemaSignals("busKind")[kind] ?? []).map((signal) => `<label><input type="checkbox" data-new-bus-signal value="${escapeHtml(signal)}"${defaults.has(signal) ? " checked" : ""}>${escapeHtml(signal)}</label>`).join("");
  const host = elements.configurationList.querySelector("[data-new-bus-host]");
  const suggestions = hostSuggestions(kind, chipForBoard(currentBase()));
  host.placeholder = suggestions.join(" / ") || "preferred_host";
  elements.configurationList.querySelector("#new-bus-hosts").innerHTML = suggestions.map((value) => `<option value="${escapeHtml(value)}"></option>`).join("");
  elements.configurationList.querySelector("[data-new-bus-freq-read]").hidden = kind !== "spi";
}

function updateNewDeviceBuses() {
  const partId = elements.configurationList.querySelector("[data-new-device-part]")?.dataset.value;
  const kind = elements.configurationList.querySelector("[data-new-device-kind]")?.value;
  const expected = parts[partId]?.bus?.kind;
  const busKind = expected && !["none", "any"].includes(expected) ? expected : null;
  const select = elements.configurationList.querySelector("[data-new-device-bus]");
  if (!select) return;
  const candidates = Object.entries(currentBase().buses ?? {}).filter(([, bus]) => !busKind || bus.kind === busKind);
  select.innerHTML = `<option value="">—</option>${candidates.map(([id]) => `<option value="${escapeHtml(id)}">${escapeHtml(id)}</option>`).join("")}`;
  select.hidden = expected === "none";
  if (!partId) select.hidden = !["display", "sd", "touch", "speaker", "mic", "pmic", "rtc", "imu"].includes(kind);
  const sdNote = elements.configurationList.querySelector("[data-sd-note]");
  sdNote.hidden = kind !== "sd";
  sdNote.textContent = chipForBoard(currentBase()).sdmmc?.gpio_matrix ? t("configuration.sdMatrix") : t("configuration.sdFixed");
}

function operationFailure(failure) {
  if (!(failure instanceof BoardOperationError)) throw failure;
  showToast(messages[`validation.${failure.code}`] ? validationMessage(failure) : failure.message);
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
  const rows = visibleSlots.map(([slot, device]) => `<tr><th><span class="role-owner-label" data-role-kind="dev" style="${roleColorInlineStyle(objectRoleOwner("dev", slot))}">${escapeHtml(slot)}</span></th>${revisions.map((revision, index) => {
    const selected = revisionChoice(board, revision, slot, device);
    const explicit = revision.select?.[slot] !== undefined;
    return `<td class="${explicit ? "explicit-choice" : "default-choice"}"><select data-revision-index="${index}" data-revision-slot="${escapeHtml(slot)}">${Object.keys(device.choices).map((choice) => `<option value="${escapeHtml(choice)}"${choice === selected ? " selected" : ""}>${escapeHtml(choice)}${choice === device.default ? ` · ${escapeHtml(t("revisions.default"))}` : ""}</option>`).join("")}</select></td>`;
  }).join("")}</tr>`).join("");
  const runtimeRows = choiceSlots(board).filter(([, device]) => device.selected_by === "runtime").map(([slot, device]) => `<div class="runtime-row"><strong>${escapeHtml(t("options.runtime"))}: ${escapeHtml(slot)}</strong><span>${Object.keys(device.choices).map(escapeHtml).join(" / ")}</span><small>${escapeHtml(t("revisions.runtimeHint"))}</small></div>`).join("");
  const revisionOptions = `<option value="">${escapeHtml(t("revisions.all"))}</option>${revisions.map((revision) => `<option value="${escapeHtml(revision.id)}">${escapeHtml(revision.name)}</option>`).join("")}`;
  elements.revisionEditor.innerHTML = `<div class="revision-tools"><span>${escapeHtml(t("revisions.compare"))}</span><label>${escapeHtml(t("revisions.compareLeft"))}<select data-compare="0">${revisionOptions}</select></label><label>${escapeHtml(t("revisions.compareRight"))}<select data-compare="1">${revisionOptions}</select></label><small>${escapeHtml(t("revisions.compareHint"))}</small><label>${escapeHtml(t("revisions.idLabel"))}<input data-new-revision-id placeholder="${escapeHtml(t("revisions.idPlaceholder"))}"></label><label>${escapeHtml(t("revisions.nameLabel"))}<input data-new-revision-name placeholder="${escapeHtml(t("revisions.namePlaceholder"))}"></label><button type="button" data-add-revision>${escapeHtml(t("revisions.add"))}</button></div><div class="table-wrap revision-table-wrap"><table class="revision-table"><thead><tr><th>${escapeHtml(t("configuration.devices"))}</th>${header}</tr></thead><tbody>${rows}</tbody></table></div>${runtimeRows}`;
  elements.revisionEditor.querySelector("[data-add-revision]").disabled = true;
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
    return `<button type="button" data-owner-key="${escapeHtml(owner.key)}" class="owner-object-tab ${owner.key === current.key ? "active" : ""}${active ? "" : " inactive"}" ${roleOwnerAttributes(objectRoleOwner("dev", owner.id))} title="${active ? "" : escapeHtml(t("owners.inactive"))}">${escapeHtml(owner.id)} (${escapeHtml(owner.part ?? "?")})</button>`;
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
  elements.ownerEditor.innerHTML = `<div class="owner-tabs">${tabs}</div>${readonlyNote(readonlyReason)}<div class="table-wrap owner-table-wrap"${readonlyAttributes(readonlyReason)}><table><thead><tr><th>${escapeHtml(t("owners.pin"))}</th><th>${escapeHtml(t("column.roles"))}</th><th>${escapeHtml(t("column.note"))}</th><th>${escapeHtml(t("column.verified"))}</th></tr></thead><tbody>${rows}</tbody></table></div><p class="owner-signals">${escapeHtml(t("object.signals"))}: ${ownerSignalsHtml(board.devices?.[current.id])}</p>`;
}

function renderViews(board) {
  for (const button of elements.viewTabs.querySelectorAll("[data-view]")) button.classList.toggle("active", button.dataset.view === state.activeView);
  elements.configurationView.hidden = state.activeView !== "configuration";
  elements.revisionsView.hidden = state.activeView !== "revisions";
  elements.ownersView.hidden = state.activeView !== "owners";
  renderBoardInfo(board);
  renderConfiguration();
  renderRevisionEditor();
  renderOwnerEditor(board);
}

function renderSaveState() {
  const entry = currentEntry();
  const text = entry.dirty ? t("save.dirty") : t("save.saved");
  if (elements.saveState.textContent && elements.saveState.textContent !== text) animateStateChange(elements.saveState);
  elements.saveState.textContent = text;
  elements.saveState.classList.toggle("dirty", entry.dirty);
  applyControlLock(elements.download);
  applyControlLock(elements.discard, entry.dirty ? "" : t("unavailable.noChanges"));
}

function renderRepositoryNext() {
  const file = state.lastExported;
  prepareMotionRegion(elements.repositoryNext);
  if (file) elements.repositoryNext.querySelector(".motion-region-inner").innerHTML = `<div><strong>${escapeHtml(t("repository.downloaded", { file }))}</strong><p>${escapeHtml(t("repository.notAutomatic"))}</p></div><ol><li>${escapeHtml(t("repository.replace", { file }))}</li><li><code>node cli/spec.js check</code></li><li>${escapeHtml(t("repository.commit"))}</li></ol>`;
  elements.repositoryNext.classList.toggle("is-open", Boolean(file));
  elements.repositoryNext.setAttribute("aria-hidden", String(!file));
  elements.repositoryNext.inert = !file;
}

function renderAll() {
  try {
    applyStaticTranslations();
    setSidebarWidth(state.layout.sidebarWidth);
    renderOnboarding();
    const board = shownBoard();
    const errors = validationFor(currentBase());
    renderBoardSelect();
    renderOptions();
    renderViews(board);
    renderTable(board, errors);
    renderRoleColorLegend(board);
    renderValidation(board, errors);
    renderConnectors(board);
    renderRelated(board);
    renderSaveState();
    renderRepositoryNext();
    prepareDisclosureTransitions();
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

function addRoleUI(gpio, role) {
  if (!role) return;
  const connectorRole = /^conn:([a-z][a-z0-9_]*)\.([a-z0-9_]+)$/.exec(role);
  if (connectorRole) {
    assignConnectorPosition(currentBase(), connectorRole[1], connectorRole[2], `gpio:${gpio}`);
    return;
  }
  try {
    addRole(currentBase(), gpio, role, { chip: chipForBoard(currentBase()) });
    markDirty();
    renderAll();
  } catch (failure) { operationFailure(failure); }
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
  const chip = chipForBoard(currentBase());
  const conditional = chip.reserved_conditional?.[currentBase().spec?.storage?.psram_mode] ?? [];
  const gpio = Array.from({ length: chip.gpio_count }, (_, index) => String(index)).find((id) => !chip.absent.includes(Number(id)) && !chip.reserved.includes(Number(id)) && !conditional.includes(Number(id)) && allRoleOptions(currentBase(), id).includes(role) && !(currentBase().pins[id]?.roles?.length)) ?? "0";
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
    const existing = state.documents.get(value.id);
    if (existing?.dirty && !window.confirm(t("file.replaceDirtyConfirm", { board: existing.base.name }))) return;
    state.documents.set(value.id, { base: clone(value), original: clone(value), dirty: false, expandedGPIO: null, source: file.name });
    removeDraft(value.id);
    state.currentId = value.id;
    state.lastExported = null;
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
  const errors = validationFor(entry.base).filter((item) => item.severity !== "warning");
  if (errors.length && !window.confirm(t("file.downloadErrorsConfirm", { count: errors.length }))) return;
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
  state.lastExported = anchor.download;
  writeDraft(entry);
  renderAll();
  showToast(t("repository.downloaded", { file: anchor.download }));
}

function discardDraft() {
  const entry = currentEntry();
  if (!entry.dirty || !window.confirm(t("file.discardConfirm"))) return;
  entry.base = clone(entry.original);
  entry.dirty = false;
  entry.expandedGPIO = null;
  state.undo = null;
  state.lastExported = null;
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

elements.newBoard.addEventListener("click", () => openBoardDialog("new"));
elements.duplicateBoard.addEventListener("click", () => openBoardDialog("duplicate"));
elements.closeBoard.addEventListener("click", closeCurrentDocument);
elements.boardDialogForm.addEventListener("input", validateBoardDialog);
elements.boardDialogForm.addEventListener("submit", (event) => {
  event.preventDefault();
  if (!validateBoardDialog()) return;
  const form = elements.boardDialogForm;
  const values = { id: form.elements.id.value.trim(), name: form.elements.name.value.trim(), official_name: form.elements.official_name.value.trim(), legacy_board_id: Number(form.elements.legacy_board_id.value) };
  try {
    const board = state.boardDialogMode === "duplicate"
      ? duplicateBoard(currentBase(), values)
      : createBoard({ ...values, chip: form.elements.chip.value });
    initializeDocument(board);
    elements.boardDialog.close();
    renderAll();
  } catch (failure) { operationFailure(failure); }
});
elements.boardDialog.querySelector("[data-dialog-cancel]").addEventListener("click", () => elements.boardDialog.close());

elements.languageSelect.addEventListener("change", () => {
  state.language = elements.languageSelect.value;
  saveLanguage(state.language);
  renderAll();
});
elements.onboardingHelp.addEventListener("click", () => {
  state.onboardingOpen = !state.onboardingOpen;
  renderOnboarding();
});
elements.onboardingClose.addEventListener("click", dismissOnboarding);

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

elements.gpioFilter.value = state.gpioFilter;
elements.gpioAssignedOnly.checked = state.gpioAssignedOnly;
elements.gpioFilter.addEventListener("input", () => {
  state.gpioFilter = elements.gpioFilter.value;
  saveGPIOFilterState();
  renderWithFade(() => renderTable(shownBoard(), validationFor(currentBase())));
});
elements.gpioAssignedOnly.addEventListener("change", () => {
  state.gpioAssignedOnly = elements.gpioAssignedOnly.checked;
  saveGPIOFilterState();
  renderWithFade(() => renderTable(shownBoard(), validationFor(currentBase())));
});

elements.roleColorLegend.addEventListener("change", (event) => {
  const owner = event.target.dataset.roleColor;
  if (!owner || !/^#[0-9a-f]{6}$/i.test(event.target.value)) return;
  state.roleColors = { ...state.roleColors, [owner]: event.target.value.toLowerCase() };
  saveRoleColors();
  renderWithFade(renderAll);
});
elements.roleColorLegend.addEventListener("toggle", (event) => {
  if (event.target.matches("details")) state.roleColorsOpen = event.target.open;
}, true);
elements.roleColorLegend.addEventListener("click", (event) => {
  if (!event.target.closest("[data-reset-role-colors]")) return;
  state.roleColors = {};
  saveRoleColors();
  renderWithFade(renderAll);
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

elements.configurationView.addEventListener("focusout", (event) => {
  if (event.target.matches("input[data-board-field], input[data-board-spec], [data-board-resolution], input[data-device-spec]")) {
    event.target.dispatchEvent(new Event("change", { bubbles: true }));
  }
});

elements.configurationView.addEventListener("change", (event) => {
  if (event.target.matches("[data-new-bus-kind]")) { updateNewBusSignals(); return; }
  if (event.target.matches("[data-new-device-kind]")) { updateNewDeviceBuses(); return; }
  if (event.target.matches("[data-board-field]")) {
    const key = event.target.dataset.boardField;
    const value = key === "aliases"
      ? [...new Set(event.target.value.split(",").map((alias) => alias.trim()).filter(Boolean))]
      : controlValue(event.target);
    if (key === "chip" && value !== currentBase().chip && !window.confirm(t("dialog.chipChangeConfirm"))) { renderAll(); return; }
    try { setBoardField(currentBase(), key, value); markDirty(); renderAll(); } catch (failure) { operationFailure(failure); renderAll(); }
    return;
  }
  if (event.target.matches("[data-board-spec]")) {
    setSpec(currentBase(), event.target.dataset.boardSpec, controlValue(event.target));
    markDirty(); renderAll(); return;
  }
  if (event.target.matches("[data-board-resolution]")) {
    const inputs = [...elements.boardInfoBody.querySelectorAll("[data-board-resolution]")];
    const values = inputs.map((input) => input.value === "" ? undefined : Number(input.value));
    setSpec(currentBase(), "display.resolution", values[0] === undefined ? undefined : values[1] === undefined ? [values[0]] : values);
    markDirty(); renderAll(); return;
  }
  if (event.target.matches("[data-device-spec]")) {
    setDeviceSpec(currentBase(), event.target.dataset.deviceSpec, event.target.dataset.choiceId || null, event.target.dataset.specKey, controlValue(event.target));
    markDirty(); renderAll(); return;
  }
  if (event.target.matches("[data-choice-selected-by]")) {
    setChoiceDefaults(currentBase(), event.target.dataset.choiceSelectedBy, { selected_by: event.target.value });
    markDirty(); renderAll(); return;
  }
  if (event.target.matches("[data-choice-default]")) {
    setChoiceDefaults(currentBase(), event.target.dataset.choiceDefault, { default: event.target.value });
    markDirty(); renderAll(); return;
  }
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

elements.configurationView.addEventListener("toggle", (event) => {
  const section = event.target.dataset?.layoutSection;
  if (!section) return;
  state.layout.sections[section] = event.target.open;
  saveLayoutState();
}, true);

elements.sideResizer.addEventListener("pointerdown", (event) => {
  if (event.button !== 0 || window.matchMedia("(max-width: 980px)").matches) return;
  event.preventDefault();
  const startX = event.clientX;
  const startWidth = state.layout.sidebarWidth;
  elements.sideResizer.classList.add("is-dragging");
  elements.sideResizer.setPointerCapture?.(event.pointerId);
  const move = (moveEvent) => setSidebarWidth(startWidth - (moveEvent.clientX - startX));
  const finish = () => {
    elements.sideResizer.classList.remove("is-dragging");
    window.removeEventListener("pointermove", move);
    window.removeEventListener("pointerup", finish);
    window.removeEventListener("pointercancel", finish);
    saveLayoutState();
  };
  window.addEventListener("pointermove", move);
  window.addEventListener("pointerup", finish);
  window.addEventListener("pointercancel", finish);
});

elements.sideResizer.addEventListener("keydown", (event) => {
  if (!["ArrowLeft", "ArrowRight"].includes(event.key)) return;
  event.preventDefault();
  const step = event.shiftKey ? 32 : 16;
  setSidebarWidth(state.layout.sidebarWidth + (event.key === "ArrowLeft" ? step : -step));
  saveLayoutState();
});

elements.configurationList.addEventListener("click", (event) => {
  const roleButton = event.target.closest("[data-unassigned-role]");
  if (roleButton) return openRoleChooser(roleButton.dataset.unassignedRole);
  const removeConnectorButton = event.target.closest("[data-remove-connector]");
  if (removeConnectorButton) { try { removeConnector(currentBase(), removeConnectorButton.dataset.removeConnector); markDirty(); renderAll(); } catch (failure) { operationFailure(failure); } return; }
  const removeDeviceButton = event.target.closest("[data-remove-device]");
  if (removeDeviceButton) { try { removeDevice(currentBase(), removeDeviceButton.dataset.removeDevice); markDirty(); renderAll(); } catch (failure) { operationFailure(failure); } return; }
  const removeBusButton = event.target.closest("[data-remove-bus]");
  if (removeBusButton) { try { removeBus(currentBase(), removeBusButton.dataset.removeBus); markDirty(); renderAll(); } catch (failure) { operationFailure(failure); } return; }
  const removeChoiceButton = event.target.closest("[data-remove-choice]");
  if (removeChoiceButton) { try { removeChoice(currentBase(), removeChoiceButton.dataset.removeChoice, removeChoiceButton.dataset.choiceId); markDirty(); renderAll(); } catch (failure) { operationFailure(failure); } return; }
  if (event.target.closest("[data-add-connector]")) {
    const id = elements.configurationList.querySelector("[data-new-connector-id]").value.trim();
    const type = elements.configurationList.querySelector("[data-new-connector-type]").value;
    if (!/^[a-z][a-z0-9_]*$/.test(id) || currentBase().connectors?.[id]) return showToast(t("validation.E_ID_FORMAT"));
    try { addConnector(currentBase(), id, { type }); markDirty(); renderAll(); } catch (failure) { operationFailure(failure); } return;
  }
  if (event.target.closest("[data-add-bus]")) {
    const id = elements.configurationList.querySelector("[data-new-bus-id]").value.trim();
    const kind = elements.configurationList.querySelector("[data-new-bus-kind]").value;
    const signals = [...elements.configurationList.querySelectorAll("[data-new-bus-signal]:checked")].map((input) => input.value);
    const rawHost = elements.configurationList.querySelector("[data-new-bus-host]").value.trim();
    const preferred_host = ["i2c", "i2s"].includes(kind) && /^\d+$/.test(rawHost) ? Number(rawHost) : rawHost || undefined;
    const freq = elements.configurationList.querySelector("[data-new-bus-freq]").value;
    const freq_read = elements.configurationList.querySelector("[data-new-bus-freq-read]").value;
    const fixed = elements.configurationList.querySelector("[data-new-bus-fixed]").checked;
    try { addBus(currentBase(), id, { kind, signals, preferred_host, freq: freq || undefined, freq_read: freq_read || undefined, fixed }); markDirty(); renderAll(); } catch (failure) { operationFailure(failure); }
    return;
  }
  if (event.target.closest("[data-add-device]")) {
    const id = elements.configurationList.querySelector("[data-new-device-id]").value.trim();
    const partId = elements.configurationList.querySelector("[data-new-device-part]").dataset.value;
    const kind = partId ? parts[partId].kind : elements.configurationList.querySelector("[data-new-device-kind]").value;
    const bus = elements.configurationList.querySelector("[data-new-device-bus]").value;
    if (!/^[a-z][a-z0-9_]*$/.test(id) || currentBase().devices?.[id]) return showToast(t("validation.E_ID_FORMAT"));
    try { addDevice(currentBase(), id, { kind, part: partId || undefined, bus: bus || undefined }); markDirty(); renderAll(); } catch (failure) { operationFailure(failure); }
    return;
  }
  const addChoiceButton = event.target.closest("[data-add-choice]");
  if (addChoiceButton) {
    const id = addChoiceButton.dataset.addChoice;
    const select = elements.configurationList.querySelector(`[data-new-choice-part="${id}"]`);
    const partId = select.value;
    const selectedBy = elements.configurationList.querySelector(`[data-new-choice-selected-by="${id}"]`)?.value;
    try { addChoice(currentBase(), id, partId, { part: partId, selected_by: selectedBy }); markDirty(); renderAll(); } catch (failure) { operationFailure(failure); }
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

elements.revisionEditor.addEventListener("input", (event) => {
  if (!event.target.matches("[data-new-revision-id], [data-new-revision-name]")) return;
  const id = elements.revisionEditor.querySelector("[data-new-revision-id]").value.trim();
  const name = elements.revisionEditor.querySelector("[data-new-revision-name]").value.trim();
  elements.revisionEditor.querySelector("[data-add-revision]").disabled = !id || !name;
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

function toggleGPIORow(row) {
  const entry = currentEntry();
  const previous = entry.expandedGPIO;
  const next = previous === row.dataset.gpioRow ? null : row.dataset.gpioRow;
  if (prefersReducedMotion()) {
    entry.expandedGPIO = next;
    if (next !== null) revealConnectorForGPIO(next);
    writeDraft(entry);
    renderAll();
    return;
  }
  if (previous !== null && previous !== next) {
    openingGPIOs.delete(previous);
    closingGPIOs.add(previous);
    closingGPIOsStarted.delete(previous);
    clearTimeout(gpioMotionTimers.get(previous));
    gpioMotionTimers.set(previous, setTimeout(() => {
      closingGPIOs.delete(previous);
      closingGPIOsStarted.delete(previous);
      gpioMotionTimers.delete(previous);
      renderTable(shownBoard(), validationFor(currentBase()));
    }, motionDuration));
  }
  if (next !== null) {
    clearTimeout(gpioMotionTimers.get(next));
    gpioMotionTimers.delete(next);
    closingGPIOs.delete(next);
    closingGPIOsStarted.delete(next);
    openingGPIOs.add(next);
  }
  entry.expandedGPIO = next;
  if (entry.expandedGPIO !== null) revealConnectorForGPIO(entry.expandedGPIO);
  writeDraft(entry);
  renderAll();
}

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
  toggleGPIORow(row);
});

elements.tableBody.addEventListener("keydown", (event) => {
  const row = event.target.closest("[data-gpio-row]");
  if (!row || !["Enter", " "].includes(event.key)) return;
  event.preventDefault();
  toggleGPIORow(row);
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
  const errors = validationFor(currentBase());
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
  const target = event.target;
  const isTyping = target instanceof HTMLInputElement || target instanceof HTMLTextAreaElement || target instanceof HTMLSelectElement || target?.isContentEditable;
  if (event.key === "/" && !isTyping && !event.metaKey && !event.ctrlKey && !event.altKey) {
    event.preventDefault();
    boardCombobox?.input.focus();
    return;
  }
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
