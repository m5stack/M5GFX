function samePin(left, right) {
  return JSON.stringify(left ?? null) === JSON.stringify(right ?? null);
}

export function pinChangeLayer({ base, defaults, revision, runtime, shown }) {
  if (samePin(base, shown)) return null;
  if (!samePin(runtime, shown)) return "accessory";
  if (!samePin(revision, runtime)) return "runtime";
  if (!samePin(defaults, revision)) return "revision";
  return "configuration";
}
