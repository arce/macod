# Unified DSL (diagram + wireframe)

One language with two uses: figures (`.dg`) and screens (`.wf`). They share syntax, the attribute vocabulary, `def`, `data`, `repeat`, `if`, `import` and arrows. This document is the reference; the cases in `cases/` are the contract between C++, JS and Python.

Status: implemented in C++ (`*_tec.hpp`) and checked against the reference SVGs in `dg/` (148 of 149 identical). The JS and Python ports must reproduce what this document says.

## 1. Syntax (module `dsl`)

- A line is a list of items separated by `;`. Indentation (spaces, never tabs) nests.
- An item is `name word "string" key=value ...`. Bare words are flags (`bold`, `light`, `elbow`).
- `#` opens a comment that runs to the end of the line. Strings take `\"`, `\\` and `\n`.
- Variables `$name` in any value; `$id` is the id of the use of a `def`.
- Everything else (`def`, `data`, `repeat n= var=`, `if`, `import`, `content`) works as in `dsl_tec.hpp`.

## 2. Structure of a file

```
import shapes
diagram fig13-1 frame style=oreilly size=11 caption="Figure 13-1. Sharding in Redis"
  row gap=130
    ...
screen cart "Cart" w=340
  ...
```

- `import` goes first. After it, one or more `diagram` or `screen` blocks; each block produces one SVG.
- The directives `@frame`, `@style`, `@size` and `@caption` are gone: they are options of the header (`frame`, `style=`, `size=`, `caption=`).
- The id may be omitted; then it is the name of the file.

## 3. Common vocabulary

| Attribute | Meaning | Replaces |
|---|---|---|
| `w`, `h` | width and height. In `box`, `text` and `shape` they are minimums: the element grows to fit its content. In `canvas` and in `box` with children they are exact | `width`, `height` of the wireframe |
| `gap`, `pad` | spacing between children / inner margin. A `pad` written on the element wins over the `padx`/`pady` of the style. `grid` accepts `gapx` and `gapy` | same |
| `fill`, `sw`, `dash` | fill, stroke width, stroke pattern | same |
| `caption` | text below or above the figure | same |
| `at=x,y` | absolute position inside a `canvas` | same |
| `minw`, `minh` | minimum size | new |
| `alignx=ID`, `aligny=ID` | center on another element of a `canvas` (§5) | `below`, `right`... |
| `col=`, `row=`, `span=` | cell of a `grid` | same (wf) |

Colors are names from the theme (`light`, `blue`, `primary-soft`, ...) or `#rrggbb`.

## 3b. Styles and fonts

Styles are data, not code: a `style` block in a library gives default values per element type.

```
style oreilly
  box radius=8 sw=1.6 padx=14 pady=8
  arrow sw=2.5

style silberschatz
  box radius=0 sw=1.3 padx=8 pady=4
  arrow sw=1.3
```

- `style=oreilly` in the header selects it; styles can be stacked (`style=silberschatz,local`) and each layer overrides the one before. Priority: an attribute written on the element, then the style, then the built-in value.
- A composite component (`def`) can set its own values and so override the style only inside its body.
- Fonts: sans (Helvetica) by default or the `mono` flag; there is no `font=` attribute. Text is measured with the Helvetica table multiplied by a fixed factor (1.1 for `mono`, 1.07 for bold); the real typeface is not measured.

## 3c. Vocabulary by levels

The core validates all of its attributes (an unknown one is an error with a suggestion). They are split into three levels; a normal diagram only needs the first.

| Level | Attributes |
|---|---|
| **Core** (27) | `id` `w` `h` `minw` `minh` · `gap` `pad` `padx` `pady` `align` · `cols` `span` `at` `alignx` `aligny` `x` `y` · `fill` `stroke` `sw` `dash` `radius` · `size` `color` `caption` `title` · `style` |
| **Shapes** (13, inside `shape`) | `r` `cx` `cy` `rx` `ry` `x1` `y1` `x2` `y2` `d` `pts` `tx` `ty` |
| **Extensions** (24) | Arrows and notes: `tip` `tail` `label` `side` `margin` · Stroke: `alpha` `linecap` `join` · Text: `base` `rotate` `capsize` `captionpos` `captionstyle` · Layout: `gapx` `gapy` `rspan` `valign` `weights` `layout` `around` `padtop` · Other: `border` `theme` `metric` |

Flags: `bold` `italic` `mono` (text) · `left` `right` `center` `top` `bottom` · `wrap` · `stretch` `grow` · `float` · `frame` · `open` · `elbow` `both` `curve` `full`. In C++ and JS the list is in `ATTRS_CORE`, `ATTRS_SHAPE` and `ATTRS_EXT` (`figure_tec`); `ATTRS` is their union.

## 4. Elements

Containers (they compute the position of their children): `row`, `col`, `grid cols=N`, `group "Title"`, `layer` (overlays its children), `canvas w= h=` (children with `at=`, `alignx=` or `aligny=`), and `box` with children (it stacks like a `col`).

Leaves: `box`, `text`, `spacer`, `shape` (a shape of your own, §6).

`align` is `start`, `center`, `end` or `stretch`. Rules:
- Without `align`, a `col` stretches its `box` elements to the width of the column and centers the rest; a `text` in a `col` is centered unless it has `left` or `right`.
- With an explicit `align`, the content is also distributed along the main axis when the container has a fixed size (`box h=62 align=center` centers its children vertically).
- In a `grid`, `align` applies to the cell horizontally and vertically.
- A `box` without text is at least 36×36; a `text` with a color flag (`text white`) draws its background.

Wireframe components (`button`, `field`, `select`, `divider`, `card`, `title`, `steps`, ...): they are `def`s of the `ui` library, written with `layer`, `shape`, `text`, `row`, `col`. A wireframe is a diagram that imports `ui`. `lines n` and `table` stay as primitives until `def`s have a counter and arithmetic.

## 5. Position

Two forms:

```
box a "A" at=0,90 w=110 h=44        # absolute, inside a canvas
box b "B" alignx=a at=0,10          # centered horizontally on a (aligny= for vertical)
```

- They are evaluated in file order: you can only refer to elements already declared (the error says so and suggests the closest name).
- Inside `row`, `col` and `grid` position is not used; the container and its `gap` rule.
- `below=`, `above=`, `right=`, `left=`, `dx=` and `dy=` were removed: in the corpus almost everything is `at=` or containers.
- The size of the SVG is the union of the declared canvas, the elements (except `float` ones), the arrows (±2 px) and the arrow labels, plus the margin (6 by default).

Rule of thumb: if the figure has structure (rows, columns, groups), use containers. `at=` is left for free-form drawings.

## 6. Shapes and expressions

The body of a `shape` is drawn with `rect`, `ellipse`, `path`, `poly`, `line` and `label`; `set` defines variables. Its numeric values are expressions in the `w` and `h` of the shape:

```
shape $id "$text" w=$w h=$h
  rect fill=self sw=1.6
  line x1=10 x2=w-10 y1=h/2 y2=h/2 dash="0 5"
```

- Grammar: numbers, `w`, `h`, `set` variables, `+ - * /`, parentheses, `min`, `max`, `abs`, `sin`, `cos` and the constant `pi`. No conditionals.
- `set name=expr` defines a local variable of the `shape`. `repeat n=N var=k` exposes `$k` (0..N-1).
- In `path d=` and `poly pts=`, capital letters are commands (`M L H V C S Q T A Z`, absolute only) and each value is an expression with no spaces outside parentheses; variables start with a lowercase letter.
- They are evaluated when drawing, not when expanding: `w` and `h` come from the layout.
- Percentages (`12%`) work in `layer` (`x=38% w=62%`) and in anchors (`t.bottom@12%`).

## 7. Arrows

```
a -> b                           # between ids
a -- b                           # no head
a.bottom -> b.top elbow          # with anchors: top bottom left right
pub -> t1,t2 sw=3                # several targets
a -> b -> c                      # chain
app~2.right -> other             # the second child of app
a.bottom@25% -> b.top@=c         # anchor at 25 % of the side, or at the height of c
a -> b label="Writes" side=right
a -> b both                      # head at both ends
a -> b dash="7 5"                # dashed
```

- Options: `label`, `side`, `sw` (2.5 by default), `color`, `dash`, `elbow`, `curve`, `both`, `full`, `tip`, `tail`, `margin`.
- Head: length `max(11, 3.3·sw)`, half-width `0.46·length`. The line is shortened by `0.85·length`, unless the last segment is shorter than `1.2·length`.
- `elbow`: it leaves and enters with segments of `margin` (26 by default). If the target lies behind the exit, it goes around (in a U, in a seven-point Z, or in five points for perpendicular sides). Collinear vertices are kept.
- Label: at the midpoint of the longest segment of the unshortened route, in italics, with a 4 px white halo and 7 px from the line. It goes above if the segment is horizontal; to the right if it is vertical, or to the left if the segment is a detour outward. `side=` changes it.
- Removed (they are expressed with what is left): `<-` (write it the other way round), `<->` (`both`), `..>` (`dash`), `thick` (`sw=7`), `dashed`, `via`, `route`, `seg`, `plain`, `lstyle`, `lalign`, `ldx`, `ldy`.
- Between siblings, an arrow with no ends joins the previous element to the next one:

```
row
  box "Client"; -> "request"; box "Server"; -> "query"; box "DB"
```

- In a wireframe, `-> screen` after a `button`, `link` or `card` creates the flow link. It works on any use of a `def`: the arrow leaves the first element of the body (it gets an id if it has none) and its label is the first quoted text. The target is the id of an element of the same diagram; if it does not exist there, the link is ignored without an error, so a loose screen is still drawn and the link only appears when it is embedded in a flow (see below).
- The arrows of a flow do not cross the screens: they leave the edge of the screen that contains the button (at the height of the button), enter through the edge of the target screen and go around the other screens and the title through the free spaces, in horizontal and vertical segments with separate lanes. Labels look for a place with no screens or other labels. Arrows are laid one at a time and crossings between them are not minimized.
- `->#` instead of `->` documents the link but the flow does not draw it: `button "Cancel" outline ->#  login`. It is for return links (back, cancel, log out).
- `diagram flow links=forward` draws only the links to a screen that appears later in the flow (the order of the `screen id=` elements is the order of the screens); those that go backwards are omitted. `links=none` draws none and `links=all` (the default) draws all of them.
- A diagram can embed an earlier one from the same file: an element with no content or text of its own whose `id=` is the name of that diagram (`screen id=login`) is replaced by the content of the `login` diagram. The other attributes of the element (`w=`, `span=`...) are kept. With this a flow joins screens that are already defined and the `-> target` of their buttons are drawn between them:

```
diagram login style=ui
  screen "Sign in" w=200
    button "Sign in" -> dashboard
diagram dashboard style=ui
  screen "Dashboard" w=200
diagram flow style=ui,flow
  flow "Access" columns=2 gapx=100
    screen id=login
    screen id=dashboard
```

### Annotations (`callout`)

Notes outside the drawing, joined by a thin line that ends in a dot on the element. They are written like arrows (anywhere in the diagram; they take no room in the layout):

```
callout email "The email must be verified before signing in"
callout password.right "Masked field"             # anchor on one side of the element
callout signin "Main action.\nDisabled until valid"   # several lines with \n
callout login.top "Greeting" side=top             # the note goes on top
callout forgot@30% "Recovery" side=bottom         # anchor at 30 % of the side
```

- The target is an id (with `.side`, `@pct` or `~n`, as in arrows). The note goes on `side=` (`right` by default; if missing, the side of the target).
- The box is placed at `gap` (48) from the edge of the drawing, centered on the anchor; on each side they are ordered by anchor and stacked 8 px apart so they do not overlap. The size of the SVG includes them.
- Attributes: `side`, `gap`, `size`, `padx` (12), `pady` (8), `radius` (4), `fill`, `stroke` (box, line and dot), `color` (text) and the flags `bold`, `italic`, `mono`. Default colors: `callout`, `callout-border`, `callout-ink` (in `THEME`). The `ui` style sets them to `size=10`.
- Example: `wf/login-annotations.dg`. So that the `field`, `button` and `link` of `ui` can be annotated, they accept `id=`.

### CSS themes (`theme=`)

`diagram ... theme=balsamiq` (or `textod INPUT.dg -T balsamiq` for diagrams that do not carry it) embeds in the SVG the base color CSS (`c-f-*`, `c-s-*` with `--c-*` variables) and that of `balsamiq.css`, and puts the class `theme-balsamiq` on the `<svg>`. It works the same for wf and dg.

- Several themes are separated by commas (`theme=balsamiq,print`): their sheets go in that order, the last one wins, and the root carries one class per theme. `lib/themes/print.css` is the gray palette for printing; it only redefines the `--c-*` variables, so it combines with any other theme. The default color palette lives in `THEME::PALETTE` (fallback values of `var(--c-name, …)`), so without a theme the SVG does not change.
- If several SVGs with different themes are inlined in one HTML page, the order between their sheets is decided by the document: to combine them safely, use `<img>` or one SVG per theme.
- The theme is looked up like an `import`: `NAME.css` next to the file, in the `-L` directories or in the built-in libraries (`lib/themes/`). If it does not exist it is an error with the line.
- A theme sheet is plain CSS: variables (`--c-primary`, `--c-font`…), rules per tag (`rect`, `text`, `line`, `path`, `polygon`, `ellipse`) and filters. All the rules hang from `.theme-NAME` so they do not affect the rest of the page when the SVG is inlined.
- There are no per-component hooks: `def`s end up in those primitives and in the color classes, so a new component is already covered.
- Example: `lib/themes/balsamiq.css` (Patrick Hand font, OFL license, embedded in base64; irregular stroke with an `feTurbulence` + `feDisplacementMap` filter) and `wf/theme-balsamiq.dg`. The CSS does not change the geometry: the width of the texts is still measured with Helvetica.

## 8. Before and after

A figure with containers (from `fig13-1.dg`), with no directives or coordinates:

```
import shapes
diagram fig13-1 frame style=oreilly caption="Figure 13-1. Sharding in Redis using hash slots"
  row gap=130
    col gap=110
      box ord "Order[OrderNum]" w=200 h=54 aligny=n0
      box inv "Invoice[InvNum]" w=200 h=54 aligny=n2
    group slots "Hash slots" pad=16 gap=10
      row gap=26
        box n0 "Node 0" w=120 h=50
        db d0 "0-4095" w=90 h=62
      row gap=26
        box n1 "Node 1" w=120 h=50
        db d1 "4096-\n8191" w=90 h=62
      ...
  ord.right -> slots.left@=n0 label="Hash (OrderNum)\nmod 16384" sw=8
  n0 -> d0 sw=3
  n1 -> d1 sw=3
```

A free-form figure (from `os/ch10-01.dg`), with containers instead of three `at=`:

```
import os
diagram ch10-01 style=silberschatz size=11 caption="A file system: a collection of files and a directory structure."
  col gap=46 align=center
    box t "File\nSystem" light w=110 h=44
    row gap=60
      box a "Collection of\nfiles" light w=110 h=44 sw=2
      box b "Directory\nstructure" light w=110 h=44 sw=2
  t.bottom -> a.top elbow
  t.bottom -> b.top elbow
```

A screen (from `wf/ui-shop.wf`): it does not change except for `diagram`/`screen` as the header and `w` instead of `width`.

## 9. Migration of the current files

Done by `tools/conv.py` without touching the originals: `@frame`/`@style`/`@size`/`@caption` move to the `diagram` header; `width`/`height` to `w`/`h`; `box ... noborder` to `sw=0`; `none` to `fill=none`; `dashed` to `dash="6 4"`; uses of a `def` with a positional id move to `id=`; the dominant font sizes per file move to a `style local`. `tools/build.sh` converts, renders and compares with `tools/compare.py` (`python3 compare.py ch3-10` shows the differences of one file).

## 10. Pending

- `silberschatz` is checked against 149 references; `oreilly` (`lib/styles.dg`) has no references and its values come from `diagram.js`.
- Known difference: `ldx`/`ldy` (moving a label) were removed; `ch3-10` is the only file affected.
- `alignx=` on a parent container is not supported (`fig2-8b`).
- The wireframe `ui` library (§4) is missing: `wf/` has not been tried yet.
- Wireframe `diagram`s with `node`, `action` (a hexagon with `ok`/`error` outputs) and `frame ids=` are treated as `def`s of the `flow` library.

## `data` rows
- Inside `data` each line is a row, with no keyword: `Peter Smith, 10, #4C78A8`.
- Values are separated by commas and the spaces around them are trimmed. Quotes are only needed when the value has a comma, quotes or edge spaces: `"Monitor, 24", $210` (inside quotes, `\"` and `\\` escape).
- A line that starts with `# ` is a comment. A value may start with `#` (colors) if it is not followed by a space.
- The header accepts spaces or commas: `data sales month, total, product`.
- A row with fewer values than columns leaves the missing ones with the name of the column and the row number.

## Implicit layout

A `diagram` with two or more elements and no `at`, `alignx`, `aligny` or `around` stacks the elements in a column, like `col`. The spacing is adjusted with `gap=` on the `diagram` (12 by default). The size of each element comes from its text, so `w` and `h` are only written to force it. If any element has an explicit position, the diagram is still a free canvas as before.

## Macro `graph`

One line per relation; nodes are created when they are mentioned and are ordered by levels according to the arrows.

    graph [direction=down|right] [gap=N] [sep=N]
      id "Label" [box attributes]           declares a node
      a -> b -> c                           chain of arrows
      a -> "text" b                         arrow with a label
      a "A" -> b "B"                        declares and connects on the same line

- The id goes first and the label in quotes after it; without a label the id is used.
- The extra attributes of the node (`light`, `w=`, ...) go to the box.
- Backward arrows (cycles) go around with `margin`.
- It expands to `col`/`row` + `box` + `elbow` connectors, so it can be mixed with the normal DSL.
- Ids must not coincide with the name of a `def`.

## Macros `tree`, `sequence` and `datatable`

**`tree [direction=down|right] [gap=N] [sep=N]`**: the hierarchy comes from the indentation; one line per node, `id "Label" [box attributes]`. A single root is required. It expands to nested `col`/`row` and `elbow` connectors.

**`sequence [step=N]`**: sequence diagram.

    sequence
      c "Client"             participant (optional; if not declared the id is used)
      c -> s "request"       message
      s --> c "response"     dotted response

Participants come out in order of appearance. A message needs two different participants. It uses `box`, `spacer` and connectors with computed positions.

**`datatable [zebra] [w=N]`**: CSV rows (like `data`, quotes not required); the first row is the header. All rows must have the same number of cells; an empty cell is written `""`. It expands to a `grid` of `box` elements. The name `table` is already used by `blocks.dg` and `ui.dg`.

The parser reads as CSV rows the children of `data` and of any header registered in `DSL::raws()` (today `data` and `datatable`).

## Macro `form` (compact screens for wf)

It is `screen` with a body of one line per element, with no quotes and no attributes. The arguments of the first line (`id=`, `w=`, `phone`, the title) go as they are to `screen`. It requires `import ui`.

    form id=p1 "Sign up" w=270
      Name                 field
      Password *           password field
      v Country            select
      ? Search             search field
      [x] I agree          checked checkbox   ([ ] unchecked)
      @ Already have an account? link
      = Hello, Maria       title
      ~ note               note
      --                   separator
      > Create account     button on the right
      >> Cancel            outlined button, on the right
      >< Center            centered button

It generates the same SVG as writing `screen` with `field`, `button`, `note`... by hand. What it does not cover (cards, rows, images) can be written in the long syntax inside a normal `screen`. Since the rows are read as CSV, a comma inside a text is kept (`= Hello, Maria`).

## `form`: cards, rows, images and more

Besides the simple elements (field, `v` select, `?` search, checkbox, `@` link, `=` title, `~` note, `--`, buttons `>` `>>` `><`), `form` understands:

    [img]  [img 80]        image (optional height)      [thumb]  thumbnail
    (new)                  badge
    - text                 plain text (one line)        ~~~ 3  filler bars (lines n=3)
    | ? Search | > Go      row: elements separated by |, each in short syntax
    + Title                card; the indented lines below it are its content
    +gray Title            card with a tone (+gray with no title also works)

Only a card (`+`) can have indented lines below it. Each shortcut generates the same SVG as its long form (`card`, `row`, `image`, `badge`, `text`, `lines`). Anything that needs fine attributes is written in the long syntax inside a normal `screen`.

Change in the parser: the children of the "raw" headers (`data`, `datatable`, `form`) keep their indentation in `Node.indent`.

## Distribution with `flex`, open components, `include` and `alias prefix`

Optional changes that do not alter any existing diagram (the 210 dg and 113 wf come out identical). They are in C++ (`src/`), in `js_port/` and in the
bundle, with the same result.

- **`flex` / `flex=N`**: a child of a `row` or a `col` with the word `flex` (or `flex=N`) shares the free space of the main axis in proportion to N, like CSS flex.
  It is fixed by a `w=` (in a row) or an `h=` (in a column), and `flex=0` does not share. Unlike `grow` (rows only, and also "take the whole width"), it works in columns.
  `weights=` is also accepted in a `col` or a `box`. For there to be free space, the `col` or the `box` needs an `h=` larger than its content.
- **`*` at the end of a `def`**: `def name a b *`. The component ignores the named attributes it does not declare, the bare unquoted words
  that are not parameters and the texts left over, instead of forwarding them or giving an error. It is useful for reading text written for another program.
- **`include file key=value`** (in the text of a diagram): it is replaced by the content of `file` (supplied by the resolver, as in `import`).
  In the file, `key=$name` takes the value of the parameter and disappears if none was given. An `include` can include others.
- **`alias prefix swl_`** (in a library): in the text of the diagram, not in the body of the `def`s, the name `x` means `swl_x` if that `def` exists.
  This way a library can give meaning to names that textod already uses (`row`, `line`, `label`, `list`, `table`…) without its own bodies confusing them.
- The bundle exports `Diagrams.renderSwl(text, resolver, theme, name)`: it wraps the text of a Swinglet `.swl` in `import swl` + `diagram name style=ui`.
  The `swl` library (`lib/wireframes/swl.dg`) draws the Swinglet widgets.
  A drawing is one state of the window: before drawing, `swlStates()` (in `tools/bundle.js`) leaves a single page in each `stack` (the one with `show`, the one chosen by the
  `tabbedpane`/`buttongroup` of `choice=`, `value=` or the first), turns `options="a,b"` into segments with the chosen one pressed (`option "x" show`) and, with
  `menu "File" show` (or `include menus/file.swl show`; `swlStates` first expands the `include`s with the same parameter rule), draws the open menu as an overlay (`dropdown`: `at=` under its title, after the `frame`, so nothing moves; only one open menu at a time). If the bar is not the first thing in the window, or the text has other elements at the root, it falls back to `popup`, which does take space under the bar (`menuitem`, `checkboxmenuitem`, `menuline`).
