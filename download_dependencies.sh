#!/usr/bin/env bash
set -euo pipefail

build_pdcurses() {
    local repo_url="https://github.com/wmcbrine/PDCurses"
    local repo_dir="PDCurses"

    echo "==> Building PDCurses..."

    if [ ! -d "$repo_dir" ]; then
        git clone "$repo_url"
    else
        echo "Repo already exists, skipping clone."
    fi

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

	if [ ! -d "$repo_dir" ]; then
        git clone "$repo_url"
    else
        echo "Repo already exists, skipping clone."
    fi

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

    if [ ! -d "$target_dir" ]; then
        git clone "$repo_url" "$target_dir"
    else
        echo "Repo already exists, skipping clone."
    fi

    echo "==> $repo_dir cloning complete."
}

languages=(
    "https://github.com/tree-sitter/tree-sitter-c tree-sitter-c"
    "https://github.com/tree-sitter/tree-sitter-python tree-sitter-python"
    "https://github.com/tree-sitter/tree-sitter-json tree-sitter-json"
    "https://github.com/tree-sitter/tree-sitter-c-sharp tree-sitter-c-sharp"
)

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
echo "==> run ./nob for to compile and start Ccode-editor"