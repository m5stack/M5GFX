export function escapeHtml(value) {
  return String(value ?? "")
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#39;");
}

export function gpioTargetButton(gpio) {
  const safe = escapeHtml(gpio);
  return `<button type="button" data-target-gpio="${safe}">GPIO ${safe}</button>`;
}

export function ownerSignalsHtml(device) {
  return Object.entries(device?.signals ?? {})
    .map(([signal, endpoint]) => `${escapeHtml(signal)} → ${escapeHtml(endpoint)}`)
    .join(" · ") || "—";
}
