# textod

Turns **plain-language** diagram text into SVG.

`textod` has no macro expander and no charts, so it is small and has no dependencies besides the C++ standard library. Use it alone for hand-written diagrams, or as the last step of a pipeline: any program that writes the plain language can feed it, for example `some-expander file.dg | textod - -o out.svg`.

## What it understands

- Elements: `box text spacer shape label rect row col grid canvas group line arrow callout ellipse path poly set`, plus `style` and `diagram`.
- Layout (rows, columns, grids, absolute `at=x,y`), sizes, fills, strokes, dashes, captions.
- Connectors: `a -> b`, `a.bottom -> b.top elbow`, chains, labels, `both`, `dash`, callouts.
- Themes (`theme=` or `-T`), and `import` of libraries that contain only `style` blocks (for example `import styles`).

It does **not** understand `def`, `data`, `repeat`, `if`, `content`, `include`, the modules (`graph`, `tree`, `sequence`, `datatable`, `form`) or `chart`. If it meets one, it says that the input must be plain language.

The full language reference is `docs/DSL.md` (in Spanish); the plain language itself is described in `docs/plain-language.md`.

## Example

`hello.dg`:

```
diagram hello
  row gap=60
    box client "Client" light
    box server "Server" blue
  client -> server label="request"
```

```
textod hello.dg              # writes hello.svg next to the input
textod hello.dg -o out.svg
textod - -o out.svg < hello.dg
```

## Usage

```
textod INPUT.dg [-o OUTPUT.svg] [-L DIR]... [-T THEME]
```

| option | meaning |
|---|---|
| `INPUT` | a `.dg` file, or `-` to read standard input |
| `-o OUTPUT` | output file; `-` for standard output (default: `INPUT.svg`, or standard output when reading `-`) |
| `-L DIR` | extra directory with libraries for `import`; repeatable |
| `-T THEME` | CSS theme (`THEME.css`) for diagrams that do not set `theme=` |
| `--libs` | list the libraries built into the program |
| `-h`, `--help` / `--version` | help / version |

- A file with several diagrams writes one `OUTPUT-ID.svg` per diagram.
- Libraries are searched in: the input file's directory, the `-L` directories, then the built-in ones. A library of your own with the same name wins.
- Errors go to standard error as `file: line N: message`, with exit code 1. When the text carries `#line N` directives (written by programs that expand other languages into plain language), the line is the one in the original file.

## Build

```
make
```

Only the `styles` and `temas` libraries are embedded. Requires a C++17 compiler and Python 3 (to embed the libraries). The build uses `-ffp-contract=off` so the output is identical on ARM and x86.
