# CCode Editor

A terminal-based code editor written in C, inspired by Vi.  
It features Tree-sitter parsing and LSP integration.

---

## Current Features

- Syntax highlighting for:
  - Python
  - JSON
  - C
  - C#
- Tree-sitter-based parsing
- Early LSP support (in progress)

---

## Dependencies

This project uses the following libraries:

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
./nob
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
:wconf (write config)  
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