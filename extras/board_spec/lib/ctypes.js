import { clone, isPlainObject } from "./model.js";

const issue = (id, path, message) => ({ id, path, message });
const ctypeOwn = (value, key) => Object.prototype.hasOwnProperty.call(value, key);

function uniqueCtypeErrors(errors) {
  const seen = new Set();
  return errors.filter((item) => {
    const key = `${item.id}\0${item.path}\0${item.message}`;
    if (seen.has(key)) return false;
    seen.add(key);
    return true;
  });
}

export function validateConnectorTypes(types) {
  const errors = [];
  const idPattern = /^[a-z][a-z0-9_]*$/;
  for (const [catalogId, type] of Object.entries(types ?? {})) {
    const path = `/${catalogId}`;
    if (!isPlainObject(type) || !idPattern.test(type.id ?? "") || type.id !== catalogId) {
      errors.push(issue("E_CTYPE_FORMAT", `${path}/id`, "connector type ID must be snake_case and match its catalog key"));
      continue;
    }
    if (!isPlainObject(type.layout) || !Number.isInteger(type.layout.rows) || !Number.isInteger(type.layout.cols)) {
      errors.push(issue("E_CTYPE_FORMAT", `${path}/layout`, "connector type needs integer layout rows and cols"));
    }
    if (!Array.isArray(type.positions) || type.positions.length === 0) {
      errors.push(issue("E_CTYPE_FORMAT", `${path}/positions`, "connector type needs at least one position"));
      continue;
    }
    const ids = new Set();
    const cells = new Set();
    for (const [index, position] of type.positions.entries()) {
      const positionPath = `${path}/positions/${index}`;
      if (!/^[a-z0-9_]+$/.test(position.id ?? "")) errors.push(issue("E_CTYPE_FORMAT", `${positionPath}/id`, "position ID must be lowercase ASCII"));
      if (ids.has(position.id)) errors.push(issue("E_CTYPE_FORMAT", `${positionPath}/id`, `duplicate position ${position.id}`));
      ids.add(position.id);
      const cell = `${position.row}:${position.col}`;
      if (!Number.isInteger(position.row) || !Number.isInteger(position.col) || position.row < 0 || position.col < 0) {
        errors.push(issue("E_CTYPE_FORMAT", positionPath, "position row and col must be non-negative integers"));
      } else if (cells.has(cell)) errors.push(issue("E_CTYPE_FORMAT", positionPath, `layout cell ${cell} is duplicated`));
      else if (position.row >= type.layout.rows || position.col >= type.layout.cols) errors.push(issue("E_CTYPE_FORMAT", positionPath, "position is outside the declared layout"));
      cells.add(cell);
    }
    for (const field of ["fixed_positions", "default_positions"]) {
      for (const position of Object.keys(type[field] ?? {})) {
        if (!ids.has(position)) errors.push(issue("E_CTYPE_FORMAT", `${path}/${field}/${position}`, "position does not exist in this connector type"));
      }
    }
    for (const position of Object.keys(type.fixed_positions ?? {})) {
      if (ctypeOwn(type.default_positions ?? {}, position)) errors.push(issue("E_CTYPE_FORMAT", `${path}/default_positions/${position}`, "default and fixed positions must not overlap"));
    }
    for (const [standard, positions] of Object.entries(type.standards ?? {})) {
      for (const position of Object.keys(positions)) if (!ids.has(position)) {
        errors.push(issue("E_CTYPE_FORMAT", `${path}/standards/${standard}/${position}`, "standard refers to an unknown position"));
      }
    }
  }

  for (const [id, type] of Object.entries(types ?? {})) {
    if (!type?.extends) continue;
    const parent = types[type.extends];
    if (!parent) {
      errors.push(issue("E_CTYPE_EXTENDS", `/${id}/extends`, `missing parent connector type ${type.extends}`));
      continue;
    }
    if (parent.extends) errors.push(issue("E_CTYPE_EXTENDS", `/${id}/extends`, "multi-level connector type inheritance is not supported"));
    const childPositions = new Set((type.positions ?? []).map((position) => position.id));
    for (const position of parent.positions ?? []) {
      const childPosition = (type.positions ?? []).find((item) => item.id === position.id);
      if (!childPosition) errors.push(issue("E_CTYPE_EXTENDS", `/${id}/positions`, `inherited position ${position.id} is missing`));
      else if ((position.name ?? null) !== (childPosition.name ?? null)) {
        errors.push(issue("E_CTYPE_EXTENDS", `/${id}/positions/${position.id}`, "inherited position meaning must be preserved"));
      }
    }
    for (const [position, endpoint] of Object.entries(parent.fixed_positions ?? {})) {
      if (type.fixed_positions?.[position] !== endpoint) {
        errors.push(issue("E_CTYPE_EXTENDS", `/${id}/fixed_positions/${position}`, "inherited fixed position must be preserved"));
      }
    }
  }
  return uniqueCtypeErrors(errors);
}

export function resolveConnectorType(types, id) {
  const type = types?.[id];
  if (!type) return null;
  if (!type.extends || !types[type.extends]) return clone(type);
  const parent = types[type.extends];
  return {
    ...clone(parent),
    ...clone(type),
    layout: { ...clone(parent.layout ?? {}), ...clone(type.layout ?? {}) },
    fixed_positions: { ...clone(parent.fixed_positions ?? {}), ...clone(type.fixed_positions ?? {}) },
    default_positions: { ...clone(parent.default_positions ?? {}), ...clone(type.default_positions ?? {}) },
    standards: { ...clone(parent.standards ?? {}), ...clone(type.standards ?? {}) },
    rails: [...new Set([...(parent.rails ?? []), ...(type.rails ?? [])])],
  };
}

export function isCompatible(types, typeId, expectedId) {
  if (typeId === expectedId) return true;
  return types?.[typeId]?.extends === expectedId;
}
