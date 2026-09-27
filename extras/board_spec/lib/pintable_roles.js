import { consumesPinTableRole, pinNameForRole, pinTableAssignments, stripRoleSource } from "./emit/m5unified_pin_table.js";

export function isPintableRole(board, role) {
  return consumesPinTableRole(board, role);
}

export function stripSource(role) {
  return stripRoleSource(role);
}

export function pintableRoleName(board, role) {
  return pinNameForRole(board, role);
}

export function pintableAssignments(board) {
  return pinTableAssignments(board);
}

export function pinRowIsConsumed(board, pin) {
  // main_spi is also consumed by the M5GFX descriptor target when no SD device
  // makes it part of the M5Unified table.
  return Array.isArray(pin?.roles) && pin.roles.some((role) => (
    isPintableRole(board, role) || /^bus:main_spi\./.test(stripSource(role))
  ));
}

export function isGeneratedField(board, objectPath, field) {
  const pinMatch = /^\/pins\/([^/]+)$/.exec(objectPath);
  if (pinMatch) {
    return field === "roles" && pinRowIsConsumed(board, board.pins?.[pinMatch[1]]);
  }

  const busMatch = /^\/buses\/([^/]+)$/.exec(objectPath);
  if (busMatch) {
    const generatedByKind = {
      spi: new Set(["signals", "preferred_host", "freq", "freq_read"]),
      i2c: new Set(["signals", "preferred_host", "freq"]),
    };
    return generatedByKind[board.buses?.[busMatch[1]]?.kind]?.has(field) ?? false;
  }

  const deviceMatch = /^\/devices\/([^/]+)$/.exec(objectPath);
  if (!deviceMatch) return false;
  const generatedByKind = {
    display: new Set(["signals"]),
    sd: new Set(["bus", "signals", "derived"]),
    touch: new Set(["signals"]),
    pmic: new Set([]),
    speaker: new Set(["enable"]),
    led_strip: new Set(["signals"]),
    power_hold: new Set(["signals"]),
    backlight: new Set(["signals", "spec"]),
  };
  return generatedByKind[board.devices?.[deviceMatch[1]]?.kind]?.has(field) ?? false;
}
