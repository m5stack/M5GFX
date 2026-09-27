// Pure model helpers shared by the Node CLI and the future browser editor.

export function parseJson(text, source = "<json>") {
  try {
    return JSON.parse(text);
  } catch (error) {
    throw new Error(`${source}: ${error.message}`);
  }
}

export function isPlainObject(value) {
  return value !== null && typeof value === "object" && !Array.isArray(value);
}

export function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

export function effectiveChip(chip, board) {
  const packageId = board?.chip_package;
  const packageSpec = packageId ? chip?.packages?.[packageId] : null;
  return packageSpec ? { ...chip, ...clone(packageSpec), id: chip.id, package: packageId } : chip;
}

export function assertBoard(value, source = "<board>") {
  if (!isPlainObject(value) || !isPlainObject(value.pins) || !isPlainObject(value.buses) || !isPlainObject(value.devices)) {
    throw new TypeError(`${source}: expected a board object with pins, buses, and devices`);
  }
  return value;
}

export function assertSchema(value, source = "<schema>") {
  if (!isPlainObject(value) || !isPlainObject(value.properties) || !isPlainObject(value.definitions)) {
    throw new TypeError(`${source}: expected a board schema`);
  }
  return value;
}

export function assertChip(value, source = "<chip>") {
  if (!isPlainObject(value) || !Number.isInteger(value.gpio_count)) {
    throw new TypeError(`${source}: expected a chip object with gpio_count`);
  }
  if (!Array.isArray(value.absent)
    || value.absent.some((gpio) => !Number.isInteger(gpio) || gpio < 0 || gpio >= value.gpio_count)
    || new Set(value.absent).size !== value.absent.length) {
    throw new TypeError(`${source}: expected unique absent GPIO numbers inside the chip range`);
  }
  for (const [id, packageSpec] of Object.entries(value.packages ?? {})) {
    if (!/^[a-z][a-z0-9_]*$/.test(id) || !isPlainObject(packageSpec) || !Array.isArray(packageSpec.absent)
      || packageSpec.absent.some((gpio) => !Number.isInteger(gpio) || gpio < 0 || gpio >= value.gpio_count)
      || new Set(packageSpec.absent).size !== packageSpec.absent.length) {
      throw new TypeError(`${source}: package ${id} needs unique absent GPIO numbers inside the chip range`);
    }
  }
  return value;
}
