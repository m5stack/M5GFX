function plainObject(value) {
  return value !== null && typeof value === "object" && !Array.isArray(value);
}

function pointer(root, reference) {
  if (!reference.startsWith("#/")) return null;
  return reference.slice(2).split("/").reduce((value, key) => value?.[key.replace(/~1/g, "/").replace(/~0/g, "~")], root);
}

function matchesType(value, type) {
  if (type === "null") return value === null;
  if (type === "array") return Array.isArray(value);
  if (type === "object") return plainObject(value);
  if (type === "integer") return Number.isInteger(value);
  if (type === "number") return typeof value === "number" && Number.isFinite(value);
  return typeof value === type;
}

function sameJson(left, right) {
  return JSON.stringify(left) === JSON.stringify(right);
}

export function validateSchema(value, schema, root = schema, path = "") {
  const issues = [];
  const issue = (id, at, message) => issues.push({ id, path: at, message });
  const visit = (item, node, at) => {
    if (!node) return;
    if (node.$ref) {
      const target = pointer(root, node.$ref);
      if (!target) issue("E_SCHEMA_REF", at, `unresolved schema reference ${node.$ref}`);
      else visit(item, target, at);
      return;
    }
    const types = node.type === undefined ? [] : (Array.isArray(node.type) ? node.type : [node.type]);
    if (types.length && !types.some((type) => matchesType(item, type))) {
      issue("E_SCHEMA_TYPE", at, `expected ${types.join(" or ")}`);
      return;
    }
    if (Object.prototype.hasOwnProperty.call(node, "const") && !sameJson(item, node.const)) {
      issue("E_SCHEMA_CONST", at, `expected constant ${JSON.stringify(node.const)}`);
    }
    if (node.enum && !node.enum.some((candidate) => sameJson(item, candidate))) {
      issue("E_SCHEMA_ENUM", at, `value is not one of ${node.enum.map(JSON.stringify).join(", ")}`);
    }
    if (typeof item === "string" && node.pattern !== undefined && !new RegExp(node.pattern).test(item)) {
      issue("E_SCHEMA_PATTERN", at, `value does not match ${node.pattern}`);
    }
    if (typeof item === "number") {
      if (node.minimum !== undefined && item < node.minimum) issue("E_SCHEMA_MINIMUM", at, `value must be at least ${node.minimum}`);
      if (node.maximum !== undefined && item > node.maximum) issue("E_SCHEMA_MAXIMUM", at, `value must be at most ${node.maximum}`);
    }
    if (Array.isArray(item)) {
      if (node.minItems !== undefined && item.length < node.minItems) issue("E_SCHEMA_MIN_ITEMS", at, `array needs at least ${node.minItems} item(s)`);
      if (node.maxItems !== undefined && item.length > node.maxItems) issue("E_SCHEMA_MAX_ITEMS", at, `array permits at most ${node.maxItems} item(s)`);
      if (node.uniqueItems && new Set(item.map(JSON.stringify)).size !== item.length) issue("E_SCHEMA_UNIQUE", at, "array items must be unique");
      item.forEach((child, index) => visit(child, node.items, `${at}/${index}`));
    }
    if (plainObject(item)) {
      for (const key of node.required ?? []) if (!Object.prototype.hasOwnProperty.call(item, key)) {
        issue("E_SCHEMA_REQUIRED", `${at}/${key}`, "required property is missing");
      }
      const properties = node.properties ?? {};
      for (const [key, child] of Object.entries(item)) {
        if (node.propertyNames) visit(key, node.propertyNames, `${at}/${key}`);
        if (Object.prototype.hasOwnProperty.call(properties, key)) visit(child, properties[key], `${at}/${key}`);
        else if (plainObject(node.additionalProperties)) visit(child, node.additionalProperties, `${at}/${key}`);
        else if (node.additionalProperties === false) issue("E_UNKNOWN_KEY", `${at}/${key}`, "key is not declared by the schema");
      }
    }
  };
  visit(value, schema, path);
  return issues;
}
