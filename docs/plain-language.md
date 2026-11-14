# The plain language

The plain language is the small text format that `textod` draws. Programs that expand richer languages (macros, data, charts) write it, and `textod` reads it. This document fixes what it is.

## Vocabulary

- Elements: `box text spacer shape label rect row col grid layer canvas group line arrow callout ellipse path poly set`.
- `style` blocks, `diagram`, and connectors such as `a.bottom -> b.top`.
- Nothing else: no `def`, `data`, `repeat`, `if`, `content`, `include`, and no modules (`graph`, `tree`, `sequence`, `datatable`, `form`) or `chart`.

## Rules

1. **Canonical form.** A value is quoted if and only if it is text. A value that comes from a parameter and contains spaces is text.
2. **Styles first.** `style` blocks come before the diagram. As a convenience for hand-written files, `import` of a library that contains only `style` blocks is also accepted; a library with anything else is an error.
3. **Generated ids.** Ids of the form `_N` (and derived ones such as `_N-a`) are deterministic: the same input always gives the same ids.
4. **Links.** The arrow of a link carries the `avoid` flag.
5. **Stable.** Reading the text and writing it again gives the same text.

## Error lines

Writing plain language loses the line numbers of the file the user wrote. To keep errors pointing at the original, the writer emits `#line N` wherever a node's line does not follow the previous one, and the reader honors it:

```
#line 5
box x
box x          # this is line 6 of the original file
#line 8
box x
```

`#line N` means that the next line of the text is line N of the original file. A node produced by a `def` carries the line where the `def` was used. The directive sits at the left margin and does not count as a line.

## Guarantee

Rendering the plain language written for a file gives the same SVG as rendering the file with the full engine. When the reader meets something that is not plain language, it says so: `'def' needs the expander module, which is not loaded`.
