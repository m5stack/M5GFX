export function matchesSearchText(searchable, query) {
  const terms = String(query ?? "").toLowerCase().split(/[\s_.:]+/).filter(Boolean);
  const haystack = String(searchable ?? "").toLowerCase();
  return terms.every((term) => haystack.includes(term));
}

export function filterBoardOptions(options, query) {
  return options.filter((option) => matchesSearchText(
    `${option.label} ${option.officialName ?? ""} ${option.value} ${option.chip ?? ""} ${option.legacyId ?? ""} ${(option.aliases ?? []).join(" ")}`,
    query,
  ));
}

export function gpioMatchesFilter(gpio, pin, query, assignedOnly = false) {
  const roles = pin?.roles ?? [];
  if (assignedOnly && roles.length === 0) return false;
  return matchesSearchText(`${gpio} ${roles.join(" ")}`, query);
}
