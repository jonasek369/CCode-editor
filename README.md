
# CCode-editor
Terminal code editor made in C inspired by VI. With tree sitter parsing and LSP.
Currently supports (Python, Json, C, C#) syntax highlighting.

# Used dependencies
[stb_ds.h](https://github.com/nothings/stb)
[nob.h](https://github.com/tsoding/nob.h)
[C-LSP-Client](https://github.com/jonasek369/C-LSP-Client)
[C-JSON](https://github.com/jonasek369/C-JSON)
[tiny_queue](https://github.com/jonasek369/tiny_queue)

# Running the project
run ./download_dependencies.sh
then run ./nob


# Roadmap
currently working on making LSP Client and implementing it.


latest development:
type hints and errors are showing (only A_UNDERLINE and color for now)
![Type hints](https://i.imgur.com/70LL4ER.png)
tree for browsing files
![Tree](https://i.imgur.com/TkJbatD.png)