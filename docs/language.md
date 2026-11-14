# The textod language

`textod` reads the plain language (see [plain-language.md](plain-language.md)) and draws it. This is the base of the family: everything the other levels add is expanded into this vocabulary before it gets here.

## A file

```
import boxed
diagram hello style=boxed caption="Two boxes"
  row gap=60
    box client "Client" light
    box server "Server" blue
  client -> server label="request"
```

- Indentation gives the structure. `import` goes first; then one or more `diagram` blocks, each producing one SVG.
- `diagram ID [options]`: the id may be omitted (the file name is used). Options include `style=`, `size=`, `caption=`, `theme=`, `frame`.
- `import NAME` accepts libraries that contain only `style` blocks (`import boxed`). A `style NAME` block sets defaults per element (`box radius=8 sw=1.6`) and is applied with `style=NAME`.

## Elements

- **Containers** (compute the position of their children): `row`, `col`, `grid cols=N`, `group "Title"`, `layer` (stacks its children), `canvas w= h=` (children placed with `at=x,y`, `alignx=` or `aligny=`), and a `box` with children (stacks like a `col`).
- **Leaves**: `box`, `text`, `spacer`, `shape`.
- **Shapes**: the body of a `shape` is drawn with `rect`, `ellipse`, `path`, `poly`, `line` and `label`; `set name=expr` defines a local variable. Numeric values are expressions over `w` and `h` (`+ - * /`, parentheses, `min`, `max`, `abs`, `sin`, `cos`, `pi`), evaluated when drawing.
- **Annotations**: `callout`.

## Attributes

| attribute | meaning |
|---|---|
| `id` | name used by connectors and anchors |
| `w`, `h`, `minw`, `minh` | size (minimum for `box`/`text`/`shape`, exact for `canvas` and `box` with children) |
| `gap`, `pad`, `padx`, `pady`, `gapx`, `gapy` | spacing between children / inner margin |
| `align`, `valign` | `start`, `center`, `end`, `stretch` |
| `at`, `alignx`, `aligny` | absolute position and alignment inside a `canvas` |
| `col`, `row`, `span`, `rspan` | cell of a `grid` |
| `fill`, `stroke`, `sw`, `dash`, `radius`, `alpha` | painting |
| `shadow=N` | a hard shadow, N pixels to the lower right, behind a `box` (its color is `shadow-color` in the palette); leave room for it with `margin` if N is larger than 6 |
| `size`, `color`, `bold`, `italic`, `mono`, `rotate` | text |
| `caption`, `title` | text under or over a figure |
| `class`, `tabindex`, `tooltip` | on the primitives of a `shape` (`rect`, `ellipse`, `line`, `path`, `poly`), they go to the SVG as they are: `class="a b"` adds classes, `tabindex=0` makes the mark focusable (a click keeps `:focus`), `tooltip="text"` is the text the browser shows over it (a `<title>`; `\n` is a new line). With a `css "rules"` line (copied inside the `<style>` of the figure: at the top level of the file it is for every figure, anywhere inside a `diagram` it is for that one, so a `def` can bring its own; a rule written twice goes once) they are all that CSS needs for an interaction: textod does not know what it means (`examples/interactive.dg`). It works when the SVG is opened in a browser or inline in a page, not as an `<img>` |
| `flex`, `flex=N` | a child of a `row` or `col` shares the free space of the main axis in proportion to N |

Colors are theme names (`light`, `blue`, `primary-soft`, ...) or `#rrggbb`. An unknown attribute is an error with a suggestion.

## Connectors

```
a -> b                            # between ids
a -- b                            # no arrowhead
a.bottom -> b.top elbow           # anchors: top bottom left right
a.bottom@25% -> b.top@=c          # anchor at 25% of the side, or at the height of c
a -> b -> c                       # chain
a -> b label="Writes" side=right
a -> b both                       # arrowhead at both ends
a -> b dash="7 5"                 # dashed
```

Options: `label`, `side`, `sw`, `color`, `dash`, `elbow`, `curve`, `both`, `full`, `tip`, `tail`, `margin`, `avoid`. `callout` draws a note attached to an element.

## Styles and themes

They are different things and can be combined (`style=boxed theme=solarized-light`).

- A **style** is shape and stroke: defaults per element (radius, stroke width, padding, italic arrows, shadow). It is a `.dg` library with `style` blocks, one per file, and the file name is the style name: `import rounded` then `style=rounded`. Several can be chained (`style=shadow,rounded`: for each setting the last one wins). Included:
  - `boxed`: square boxes, thin lines, italic arrows.
  - `rounded`: rounded boxes, thick arrows.
  - Inspired by FLTK's box types: `flat` (solid blocks with no outline, like `FL_FLAT_BOX`), `shadow` (square with a hard shadow, `FL_SHADOW_BOX`), `rshadow` (rounded with a hard shadow, `FL_RSHADOW_BOX`) and `oval` (ellipses, `FL_OVAL_BOX`).
- A **theme** is colors: a CSS file that redefines the `--c-*` palette and paints the background of the figure. `theme=NAME` (or `-T NAME` on the command line); several can be combined (`theme=github-light,nord-light`, the last wins). Included, light: `github-light`, `nord-light`, `rose-pine-dawn`, `papercolor-light` and `solarized-light`; dark: `monokai`.

Neither is embedded in the program: they are files, `lib/styles/NAME.dg` and `lib/themes/NAME.css`, and `make install` also copies them to `PREFIX/share/textod/styles` and `PREFIX/share/textod/themes` (`textod --libs` lists them). `import` and `theme=` look for them in the directory of the file, in the `-L` directories and in the ones that travel with the program (`lib/` or `../lib` next to the executable, `../share/textod` for an installation), in that order; a file of your own with the same name, next to the file or in a `-L` directory, wins.
