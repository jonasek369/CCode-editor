#!/usr/bin/env bash
set -euo pipefail

clone_repo_if_needed() {
    local repo_url="$1"
    local repo_dir="$2"

    if [ ! -d "$repo_dir" ]; then
        git clone "$repo_url" "$repo_dir"
    else
        echo "Repo '$repo_dir' already exists, skipping clone."
    fi
}

build_pdcurses() {
    local repo_url="https://github.com/wmcbrine/PDCurses"
    local repo_dir="PDCurses"

    echo "==> Building PDCurses..."

    clone_repo_if_needed "$repo_url" "$repo_dir"

    pushd "$repo_dir/x11" > /dev/null

    if [ ! -f Makefile ]; then
        ./configure
    fi

    make -j"$(nproc)"

    popd > /dev/null

    echo "==> PDCurses build complete."
}

build_tree_sitter() {
    local repo_url="https://github.com/tree-sitter/tree-sitter"
    local repo_dir="tree-sitter"

    echo "==> Building Tree-sitter..."

    clone_repo_if_needed "$repo_url" "$repo_dir"

    pushd "$repo_dir" > /dev/null

    make -j"$(nproc)"

    popd > /dev/null

    echo "==> Tree-sitter build complete."
}

download_tree_sitter_language() {
    local repo_url="$1"
    local repo_dir="$2"

    local target_dir="tree-sitter-grammar/$repo_dir"

    echo "==> Cloning $repo_dir..."

    clone_repo_if_needed "$repo_url" "$target_dir"

    echo "==> $repo_dir cloning complete."
}

languages=(
    "https://github.com/tree-sitter/tree-sitter-c tree-sitter-c"
    "https://github.com/tree-sitter/tree-sitter-python tree-sitter-python"
    "https://github.com/tree-sitter/tree-sitter-json tree-sitter-json"
    "https://github.com/tree-sitter/tree-sitter-c-sharp tree-sitter-c-sharp"
)

# nob.h and stb_ds.h are stable wont really need to change
wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_ds.h -O stb_ds.h
wget https://raw.githubusercontent.com/tsoding/nob.h/refs/heads/main/nob.h -O nob.h
wget https://raw.githubusercontent.com/jonasek369/C-LSP-Client/refs/heads/main/LSP.h -O LSP.h
wget https://raw.githubusercontent.com/jonasek369/C-JSON/refs/heads/main/parser.h -O parser.h
clone_repo_if_needed "https://github.com/jonasek369/tiny_queue" "tiny_queue"

build_pdcurses
build_tree_sitter

mkdir -p tree-sitter-grammar

for entry in "${languages[@]}"; do
    repo_url="${entry%% *}"
    repo_dir="${entry##* }"

    download_tree_sitter_language "$repo_url" "$repo_dir"
done

echo "==> building nob"
gcc nob.c -o nob
echo "==> run ./nob to compile and start Ccode-editor"
