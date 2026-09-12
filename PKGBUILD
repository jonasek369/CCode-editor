# Maintainer: Jonáš Erlebach <jonaserlebach@email.cz>
pkgname=ccode-editor
pkgver=1.0.0
pkgrel=1
pkgdesc="Code editor made in C"
arch=('x86_64')
url="https://github.com/jonasek369/Ccode-editor"
license=('MIT')

depends=()
makedepends=(
    'git'
    'gcc'
)

source=(
    "ccode-editor::git+https://github.com/jonasek369/Ccode-editor.git"
    "stb_ds.h::https://raw.githubusercontent.com/nothings/stb/master/stb_ds.h"
    "nob.h::https://raw.githubusercontent.com/tsoding/nob.h/main/nob.h"
    "LSP.h::https://raw.githubusercontent.com/jonasek369/C-LSP-Client/main/LSP.h"
    "parser.h::https://raw.githubusercontent.com/jonasek369/C-JSON/main/parser.h"

    "PDCurses::git+https://github.com/wmcbrine/PDCurses.git"
    "tree-sitter::git+https://github.com/tree-sitter/tree-sitter.git"
    "tiny_queue::git+https://github.com/jonasek369/tiny_queue.git"

    "tree-sitter-c::git+https://github.com/tree-sitter/tree-sitter-c.git"
    "tree-sitter-python::git+https://github.com/tree-sitter/tree-sitter-python.git"
    "tree-sitter-json::git+https://github.com/tree-sitter/tree-sitter-json.git"
    "tree-sitter-c-sharp::git+https://github.com/tree-sitter/tree-sitter-c-sharp.git"
    "tree-sitter-rust::git+https://github.com/tree-sitter/tree-sitter-rust.git"
)

sha256sums=(
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
)

prepare() {
    cd "$srcdir/ccode-editor"

    cp "$srcdir/stb_ds.h" .
    cp "$srcdir/nob.h" .
    cp "$srcdir/LSP.h" .
    cp "$srcdir/parser.h" .

    cp -r "$srcdir/tiny_queue" .
    cp -r "$srcdir/PDCurses" .
    cp -r "$srcdir/tree-sitter" .

    mkdir -p tree-sitter-grammar

    cp -r "$srcdir/tree-sitter-c" tree-sitter-grammar/tree-sitter-c

    cp -r "$srcdir/tree-sitter-python" tree-sitter-grammar/tree-sitter-python

    cp -r "$srcdir/tree-sitter-json" tree-sitter-grammar/tree-sitter-json

    cp -r "$srcdir/tree-sitter-c-sharp" tree-sitter-grammar/tree-sitter-c-sharp

    cp -r "$srcdir/tree-sitter-rust" tree-sitter-grammar/tree-sitter-rust
}

build() {
	(
    	cd "$srcdir/PDCurses/x11"
    	./configure
    	make
	)
	(
    	cd "$srcdir/tree-sitter"
    	make
	)
	(
		cd "$srcdir/ccode-editor"

    	gcc nob.c -o nob -O2 -pipe
    	./nob
	)
}

package() {
    install -Dm755 \
        "$srcdir/ccode-editor/main" \
        "$pkgdir/usr/bin/ceditor"

    install -dm755 \
        "$pkgdir/usr/share/ccode-editor"

    if [[ -d "$srcdir/ccode-editor/themes" ]]; then
        cp -r \
            "$srcdir/ccode-editor/themes" \
            "$pkgdir/usr/share/ccode-editor/"
    fi

    install -dm755 \
        "$pkgdir/usr/share/ccode-editor/scm_queries"

    for grammar in \
        tree-sitter-c \
        tree-sitter-python \
        tree-sitter-json \
        tree-sitter-c-sharp \
        tree-sitter-rust
    do
        install -Dm644 \
            "$srcdir/$grammar/queries/highlights.scm" \
            "$pkgdir/usr/share/ccode-editor/scm_queries/$grammar.scm"
    done
}