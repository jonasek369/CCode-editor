# CCode Editor

A terminal-based code editor written in C, inspired by Vi.  
It features Tree-sitter parsing and custom LSP integration.

---

## Features

- Runs on **Linux** (primary target) and **Windows** (native or cross-compiled with mingw)
- Syntax highlighting with Tree-sitter for C, Python, JSON, C# and Rust
  (highlight queries are built into the binary unless you build with `-no-embed-queries`;
  files in `<config>/scm_queries` override them)
  and a fallback highlighter (comments, strings, numbers, keywords) for every other language
- LSP support: diagnostics (underlines, gutter signs, error counts), completion popup with
  filtering and snippet placeholders, go to definition
- Unlimited undo / redo, consecutive typing is undone as one step
- Line number gutter (absolute or relative), current line, matching bracket highlighting
- Smart editing: auto indentation, auto closing brackets and quotes, wrapping a selection in
  brackets, closing braces dedent, smart Home, smart Backspace in indentation
- Line operations: duplicate, delete, move up/down, toggle comment, indent / dedent selection
- Find (smart case, all matches highlighted, match counter) and replace all
- Tabs, UTF-8 (including wide characters) and CRLF files are show
- Multiple files in tabs, split view, file tree and fuzzy file finder
- Mouse: click, drag to select, double click selects a word, wheel scrolling, click on tabs
- System clipboard (wl-clipboard, xclip, xsel, pbcopy, Windows clipboard), copying falls back
  to OSC 52 so it works over ssh
- Themes and a JSON config (`:conf`)

---

## Keyboard shortcuts

| Keys | Action |
|---|---|
| Ctrl+S | Save (asks for a name for new buffers) |
| Ctrl+O / Ctrl+N | Open file / new buffer |
| Ctrl+W / Ctrl+Q | Close file / quit |
| Ctrl+Z / Ctrl+Y | Undo / redo |
| Ctrl+C / Ctrl+X / Ctrl+V | Copy / cut / paste (without a selection: the whole line) |
| Ctrl+A / Ctrl+L / Alt+W | Select all / select line / select word |
| Shift+arrows, Shift+Home/End, Ctrl+Shift+arrows | Extend selection |
| Ctrl+D / Ctrl+K | Duplicate / delete line(s) |
| Alt+Up / Alt+Down | Move line(s) |
| Ctrl+/ | Toggle comment |
| Tab / Shift+Tab | Indent / dedent (selection or line) |
| Ctrl+F | Find (arrows or Alt+N/Alt+P jump between matches, Enter selects the match) |
| Ctrl+R | Replace all |
| Ctrl+G | Go to line |
| Ctrl+B | Jump to matching bracket |
| Ctrl+Left/Right, Ctrl+Backspace/Delete | Word movement / deletion |
| Ctrl+Home/End, PgUp/PgDn, Ctrl+Up/Down | File start/end, page, scroll without moving the cursor |
| Alt+Left/Right, Ctrl+PgUp/PgDn, F1-F9, Alt+1-9 | Switch files |
| Ctrl+Tab | Previously used file |
| Ctrl+P / Ctrl+T | Fuzzy file finder / file tree |
| Alt+I | Go to definition (LSP) |
| Esc | Cancel find/selection, otherwise open the console |

## Console commands

Press Esc and type a command, `:help` lists them all. The most used ones:
`:o <path>`, `:w`, `:s <path>`, `:wa`, `:q`, `:q!`, `:c`, `:gt <line> [col]`, `:f <text>`,
`:r <find> <replace>`, `:set <option> [value]`, `:reload`, `:sw` (split view), `:tree`, `:theme`, `:conf`.

## Configuration

`config.json` lives in `~/.config/ccode-editor` (Linux) or `%LOCALAPPDATA%\ccode-editor` (Windows).
Besides the theme and LSP servers it accepts:

| Option | Default | |
|---|---|---|
| `tab_size` | 4 | |
| `use_tabs` | false | indent with tabs in new files (existing files are detected) |
| `line_numbers` / `relative_line_numbers` | true / false | |
| `auto_indent` / `auto_pair` | true / true | |
| `highlight_current_line` / `highlight_matching_bracket` | true / true | |
| `show_whitespace` | false | show tabs and trailing spaces |
| `trim_trailing_whitespace` | false | on save |
| `insert_final_newline` | true | on save |
| `scroll_margin` | 3 | lines kept visible around the cursor |

Options can also be changed for the session with `:set`, e.g. `:set relative_line_numbers on`.

## Command line

```
./main [-tree] file1 [file2 ...]      # file:line and file:line:column jump to a position
```

---

## Dependencies

This project uses the following libraries:

- [ncurses](https://invisible-island.net/ncurses/) (system package, wide-char `ncursesw`)
- [stb_ds.h](https://github.com/nothings/stb)
- [nob.h](https://github.com/tsoding/nob.h)
- [C-LSP-Client](https://github.com/jonasek369/C-LSP-Client)
- [C-JSON](https://github.com/jonasek369/C-JSON)
- [tiny_queue](https://github.com/jonasek369/tiny_queue)

---

## Running the project

Run the following commands in your terminal:

```bash
./download_dependencies.sh
./nob              # debug build (./main)
./nob -release     # optimised build
./nob -windows     # windows build (cross-compiles on linux into ./win-build, needs mingw)
./nob -no-embed-queries  # don't build the highlight queries into the binary,
                         # only <config>/scm_queries is used then
```

---

## Roadmap

- make LSP completion better
- rehaul the color scheme
- hover errors

---

## Latest Development

### Config, Theme & Theme selector

Json config and theme definition are now being loaded there is also new commands  
:conf (open config json file for changing, autoload when saved, default is loaded when the syntax is invalid)  
:theme (theme selector)   

![Themes](https://i.imgur.com/2SIqVjp.png)

---


### Support for more LSP was added

CCode editor now supports more than one lsp running at one  
(currenlty implemented are CLANGD and PYLSP)

---

### Type Hints & Errors

Type hints and error diagnostics are now displayed  
(currently using underline + color highlighting):

![Type hints](https://i.imgur.com/70LL4ER.png)

---

### File Tree Browser

A file tree view for file system navigation:

![Tree](https://i.imgur.com/TkJbatD.png)