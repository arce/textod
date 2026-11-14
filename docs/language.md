# The textod language

`textod` reads the plain language (see [plain-language.md](plain-language.md)) and draws it. This is the base of the family: everything the other levels add is expanded into this vocabulary before it gets here.

## A file

```
import silberschatz
diagram hello style=silberschatz caption="Two boxes"
  row gap=60
    box client "Client" light
    box server "Server" blue
  client -> server label="request"
```

- Indentation gives the structure. `import` goes first; then one or more `diagram` blocks, each producing one SVG.
- `diagram ID [options]`: the id may be omitted (the file name is used). Options include `style=`, `size=`, `caption=`, `theme=`, `frame`.
- `import NAME` accepts libraries that contain only `style` blocks (`import silberschatz`). A `style NAME` block sets defaults per element (`box radius=8 sw=1.6`) and is applied with `style=NAME`.

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
| `size`, `color`, `bold`, `italic`, `mono`, `rotate` | text |
| `caption`, `title` | text under or over a figure |
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

They are different things and can be combined (`style=silberschatz theme=print`).

- A **style** is shape and stroke: defaults per element (radius, stroke width, padding, italic arrows). It is a `.dg` library with `style` blocks, one per file, and the file name is the style name: `import oreilly` then `style=oreilly`. Included: `silberschatz` (square boxes, thin lines, italic arrows) and `oreilly` (rounded boxes, thick arrows).
- A **theme** is colors and fonts: a CSS file that redefines the `--c-*` palette. `theme=NAME` (or `-T NAME` on the command line); several can be combined (`theme=balsamiq,print`, the last wins). Included: `balsamiq` (hand-drawn look and font) and `print` (grays for printing).

Neither is embedded in the program: they are files, `lib/styles/NAME.dg` and `lib/themes/NAME.css`, and `make install` also copies them to `PREFIX/share/textod/styles` and `PREFIX/share/textod/themes` (`textod --libs` lists them). `import` and `theme=` look for them in the directory of the file, in the `-L` directories and in the ones that travel with the program (`lib/` or `../lib` next to the executable, `../share/textod` for an installation), in that order; a file of your own with the same name, next to the file or in a `-L` directory, wins.
