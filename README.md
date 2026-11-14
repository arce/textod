# textod

Turns **plain-language** diagram text into SVG.

`textod` is small and has no dependencies besides the C++ standard library.

Website: <https://arce.github.io/textod/> (the page in [docs/index.html](docs/index.html)).

The language it reads is described in [docs/language.md](docs/language.md); the plain-language subset that other programs can generate is fixed in [docs/plain-language.md](docs/plain-language.md).

## What it understands

- Elements: `box text spacer shape label rect row col grid layer canvas group line arrow callout ellipse path poly set`, plus `style` and `diagram`.
- Layout (rows, columns, grids, absolute `at=x,y`), sizes, fills, strokes, dashes, captions.
- Connectors: `a -> b`, `a.bottom -> b.top elbow`, chains, labels, `both`, `dash`, callouts.
- Styles (`style=`: shape and stroke defaults, one `.dg` file per style: `import silberschatz`) and themes (`theme=` or `-T`: colors and fonts, CSS).

## Example

`hello.dg`:

```
diagram hello
  row gap=60
    box client "Client" light
    box server "Server" blue
  client -> server label="request"
```

`textod` works like a Unix filter: it writes the SVG to the standard output.

```
textod hello.dg > hello.svg
textod hello.dg -o hello.svg
cat hello.dg | textod > hello.svg
```

## Usage

```
textod [-o OUTPUT] [-L DIR]... [-T THEME] [INPUT]
```

| option | meaning |
| --- | --- |
| `INPUT` | a `.dg` file; without it (or with `-`) the standard input is read, so `textod` works in a pipe |
| `-o OUTPUT` | write to this file instead of the standard output (`-o -` is the standard output) |
| `-L DIR` | extra directory with styles and themes (they are looked for in its subdirectories too, so it may be a `lib` directory); repeatable |
| `-T THEME` | CSS theme (`THEME.css`) for diagrams that do not set `theme=` |
| `--libs` | list the styles and themes that are found in those places |
| `-h`, `--help` / `--version` | help / version |

- Options take their value separated (`-o out.svg`) or attached (`-oout.svg`); `--` ends the options.
- Nothing is printed when it works. Errors go to the standard error as `file: line N: message` (`stdin: line N: message` when the text comes from the standard input), with exit code 1; a wrong command line exits with 2. When the text carries `#line N` directives (written by programs that expand other languages into plain language), the line is the one in the original file.
- A file with several diagrams writes them one after the other to the standard output; with `-o out.svg` it writes one `out-ID.svg` per diagram.
- Styles (`import`) are searched in: the input file's directory (the current directory when reading the standard input), the `-L` directories, then the ones that travel with the program: `lib/` and `../lib` next to the executable, and `../share/textod` (where `make install` puts them). Themes are searched in the same places. A file of your own with the same name wins.

## Build

```
make            # builds build/textod
make check      # renders examples/*.dg and compares them with the .svg next to them
make install    # /usr/local/bin/textod and the styles and themes in /usr/local/share/textod (PREFIX=... to change it)
make clean
```

It needs `make` and a C++17 compiler (g++ or clang++). `make` compiles `src/textod.cpp` (the rest of `src/` are headers) into a single executable.

The styles (`lib/styles/*.dg`) and the themes (`lib/themes/*.css`) are files that `textod` reads when it runs, so one can be added or changed without compiling. `textod` looks for them next to the executable (`lib/` or `../lib`), so `build/textod` finds the ones of this repository from any directory. `make install` copies them to `$(DATADIR)/styles` and `$(DATADIR)/themes` (`DATADIR` is `PREFIX/share/textod`), where the installed `textod` finds them (`../share/textod` from its `bin/`), so the installation can be moved as a whole.

The build uses `-ffp-contract=off` so the output is identical on ARM and x86.

## Website

The project page is a static page in `docs/` (`docs/index.html`, with the figures in `docs/img`). To publish it with GitHub Pages: Settings, Pages, "Deploy from a branch", branch `main`, folder `/docs`. The figures are examples rendered by textod: `make site` regenerates them and `make check` keeps them in sync with their sources.
