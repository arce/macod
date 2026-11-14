# macod

Expands the **full language** (macros, data, modules) into **plain language**. It does not draw anything: its output is text that [textod](https://github.com/arce/textod) turns into SVG.

```
macod INPUT.dg | textod - -o OUTPUT.svg
```

Website: <https://arce.github.io/macod/> (the page in [docs/index.html](docs/index.html)).

## What it adds over textod

- `def` (components with parameters and `content`), `data`, `repeat`, `if`, `include`.
- `import` of libraries that contain `def` (wireframe components, UML, ER, state, kanban, timeline, ...).
- Embedded screens (`screen id=`) and links (`-> destination`), CSV tables.
- The modules `graph`, `tree`, `sequence`, `datatable` and `form`.

Everything is expanded away: the output has only plain-language vocabulary, no `def`, `data`, `repeat`, `if`, `content`, `include` or modules.

## Example

`flow.dg`:

```
diagram flow
  graph
    a "Parse" -> b "Expand" -> c "Draw"
```

`macod flow.dg` prints plain language: a `col` of `row`/`box` elements plus `a.bottom -> b.top elbow` connectors. Then:

```
macod flow.dg | textod - -o flow.svg
macod flow.dg -o flow-plain.dg        # keep the plain text in a file
```

## Usage

```
macod INPUT.dg [-o OUTPUT.dg] [-L DIR]...
```

| option | meaning |
|---|---|
| `INPUT` | a `.dg` file, or `-` for standard input |
| `-o OUTPUT` | output file; standard output by default (`-` also works) |
| `-L DIR` | extra directory with libraries for `import`; repeatable |
| `--libs` | list the libraries built into the program |
| `-h`, `--help` / `--version` | help / version |

Libraries are searched in: the input file's directory, the `-L` directories, then the built-in ones. Do not name your file like a library: a `kanban.dg` that runs `import kanban` would import itself.

`macod` checks the macros and modules; it does not check the drawing vocabulary. An unknown element goes through and `textod` reports it, with the line of your file.

## The plain-language output

`docs/plain-language.md` fixes what `macod` writes:

- **Canonical form.** A value is quoted if and only if it is text. Style blocks come first.
- **Deterministic ids.** Generated ids (`_1`, `_2`, ...) are the same for the same input.
- **`#line N`.** Because expansion loses the original line numbers, `macod` writes `#line N` where a node's line does not follow the previous one. `textod` honors it, so its errors still name the line of the file you wrote. A node produced by a `def` reports the line where the `def` was used.
- **Stable.** Expanding the plain text again leaves it unchanged.
- **Same drawing.** Rendering `macod`'s output gives the same SVG as the full language engine.

## Documentation

- [docs/DSL.md](docs/DSL.md): the full language.
- [docs/plain-language.md](docs/plain-language.md): the plain language that `macod` writes.
- [docs/examples](docs/examples): the `.dg` files of the website; their results are in [docs/img](docs/img).

## Build

```
make            # builds build/macod
make test       # unit tests and the examples
make install    # installs into /usr/local/bin (PREFIX=... to change it)
make clean      # removes build/
```

The program name is set with `-DPROGRAM`. It embeds the libraries for macros and components. Requires a C++17 compiler and Python 3 (only to embed the libraries).

`macod` uses only the C++ standard library: it does not contain or link any `textod` code. `textod` is a separate program that reads the text `macod` writes.

The images of the website are made with `macod X.dg | textod - -o X.svg` for each file in `docs/examples`.
