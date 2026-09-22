# Documentation guidelines

Use this guide for new and substantially revised M5GFX documentation. Existing
device guides keep their paths so that current links continue to work.

## Language and organization

- Write new pages, captions, and example comments in English.
- Use plain Markdown that renders on GitHub. A site generator is not required.
- Keep one topic per page, one level-one title, and sequential heading levels.
- Organize API pages by function group in `docs/api/`, using lower-case kebab-case
  filenames such as `display.md` and `text.md`. Document related functions in
  sections on the same page. Split a topic only when its size makes it hard to read.
- Put figures in `docs/assets/<topic>/`, for example
  `docs/assets/setRotation/rotation-f.svg`.
- Use relative links to repository files. Add pages to their topic or section index; avoid copying full API explanations into
  the project README.
- Keep existing device guides in place. Link to shared API behavior instead of
  duplicating it in every device guide.

## Documentation map

- `getting-started/`: setup and first-use instructions.
- `concepts/`: shared coordinate, color and rendering concepts.
- `api/`: function groups with API details and examples in the same page.
- `guides/`: task-oriented usage and links to runnable examples.
- `devices/`: navigation to device-specific instructions and orientation figures.
- `examples/`: task-based index of the repository examples.
- `assets/`: figures used by these pages.

Keep the home page focused on these entry points. Add functions to the matching
API topic and link to its section anchor. When reorganizing a page, update all
repository links, examples and figure captions. Avoid empty placeholder pages and broken links.
Keep material sources, generation details and work logs out of user-facing pages.

## API section template

Use this order, omitting sections that do not apply:

1. **Purpose**: what the function does and when to use it.
2. **Signature**: copy the current public declaration, including parameter types.
3. **Parameters and return value**: units, supported values, defaults if any,
   and the meaning of the result. Write `None (void)` for a void return.
4. **Diagram or behavior reference**: put the main visual before implementation
   detail. Provide a text equivalent for information conveyed by a figure.
5. **Example**: a minimal complete sketch, dependencies, and a relative link to
   the corresponding runnable example when one is included.
6. **Notes**: initialization requirements, state changes, redraw requirements,
   and relevant interactions with other APIs.
7. **Device differences**: separate common behavior from model-specific values.
8. **References**, when useful: link to related APIs and device guides. Keep
   implementation checks and test results in the change description, not the user guide.

Use [orientation and dimensions](api/display.md#orientation-and-dimensions) as an
example of a detailed API section inside a topic page.
Keep source references out of the way of the main explanation, but precise enough
for a maintainer to check the claims.

## Diagrams and direction conventions

- Prefer self-contained SVG for diagrams. Do not reference external fonts,
  images, scripts, or stylesheets. Draw orientation-test glyphs as paths so that
  font substitution cannot change their shape.
- Include an SVG title and description, plus useful Markdown alternative text.
- Declare the viewing side, reference orientation, coordinate origin, and axis
  directions. Distinguish rotating the image from physically turning a device.
- Keep the device frame fixed when comparing display rotations. Use an
  asymmetric pattern such as **F** to distinguish rotation from reflection.
- Pair colors with labels or shapes; never make color the only way to identify
  a rotation value. Show logical width and height when they can change.
- Label generic diagrams as generic. A chip's native direction, a board's panel
  mounting, and an application's startup direction are different references.
- For a physical-device diagram, state a visible chassis landmark and record
  whether that orientation was verified on hardware.

Keep device reference figures compact: model name, chassis, TOP, F, coordinates,
and dimensions. Put detailed explanations in the page text rather than a side
panel or camera callout. Use the [device figures](device-orientation.md) as the
layout reference, and label proposed conventions separately from existing API
behavior.

## Examples and evidence

- Use the smallest complete example that demonstrates the documented behavior.
  Include initialization, required headers, and any redraw or display update.
- Read current dimensions after changing rotation; do not hard-code a single
  model's resolution into a generic example.
- Keep diagram and example patterns consistent. Update both when behavior changes.
- Verify against the current implementation, including panel overrides and
  board configuration. Do not turn a property of one backend into a universal
  promise.
- Separate **source-verified**, **host-tested**, **compiled**, and
  **hardware-verified** claims. Compilation does not prove a physical direction.
- Document the library revision and relevant framework/board for reproducible
  checks. Do not add machine-specific paths to runnable repository examples.

## Before submitting

- Open the Markdown and SVG; check readability, clipping, links, and image paths.
- Check signatures, parameter ranges, defaults, and state changes against source.
- Compile new sketches for the relevant target families.
- For rotation documentation, compare all eight values using the same F and
  corner pattern; use a non-square surface to check width/height swapping.
- Record untested hardware mappings explicitly rather than guessing.
- Keep unrelated driver changes out of documentation changes.
