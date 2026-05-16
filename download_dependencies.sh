#!/usr/bin/env bash
set -euo pipefail

JOBS="$(nproc)"
MAX_PARALLEL=$JOBS

clone_repo_if_needed() {
    local repo_url="$1"
    local repo_dir="$2"

    if [[ ! -d "$repo_dir" ]]; then
        git clone --depth 1 "$repo_url" "$repo_dir"
    fi
}

run_limited() {
    # limits background jobs to MAX_PARALLEL
    while (( $(jobs -rp | wc -l) >= MAX_PARALLEL )); do
        wait -n
    done
}

build_pdcurses() {
    (
        echo "==> Building PDCurses..."

        clone_repo_if_needed "https://github.com/wmcbrine/PDCurses" "PDCurses"

        cd PDCurses/x11

        [[ ! -f Makefile ]] && ./configure

        make -j"$JOBS"

        echo "==> PDCurses done"
    ) &
}

build_tree_sitter() {
    (
        echo "==> Building Tree-sitter..."

        clone_repo_if_needed "https://github.com/tree-sitter/tree-sitter" "tree-sitter"

        cd tree-sitter
        make -j"$JOBS"

        echo "==> Tree-sitter done"
    ) &
}

download_tree_sitter_language() {
    local repo_url="$1"
    local repo_dir="$2"

    run_limited

    (
        clone_repo_if_needed "$repo_url" "tree-sitter-grammar/$repo_dir"
    ) &
}

echo "==> downloading headers in parallel"

wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_ds.h -O stb_ds.h &
wget https://raw.githubusercontent.com/tsoding/nob.h/refs/heads/main/nob.h -O nob.h &
wget https://raw.githubusercontent.com/jonasek369/C-LSP-Client/refs/heads/main/LSP.h -O LSP.h &
wget https://raw.githubusercontent.com/jonasek369/C-JSON/refs/heads/main/parser.h -O parser.h &

wait

clone_repo_if_needed "https://github.com/jonasek369/tiny_queue" "tiny_queue" &

# start builds ASAP (parallel)
build_pdcurses
build_tree_sitter

mkdir -p tree-sitter-grammar

languages=(
    "https://github.com/tree-sitter/tree-sitter-c tree-sitter-c"
    "https://github.com/tree-sitter/tree-sitter-python tree-sitter-python"
    "https://github.com/tree-sitter/tree-sitter-json tree-sitter-json"
    "https://github.com/tree-sitter/tree-sitter-c-sharp tree-sitter-c-sharp"
)

echo "==> cloning grammars in parallel (limited concurrency)"

for entry in "${languages[@]}"; do
    repo_url="${entry%% *}"
    repo_dir="${entry##* }"
    download_tree_sitter_language "$repo_url" "$repo_dir"
done

wait

echo "==> building nob"
gcc nob.c -o nob -O2 -march=native -pipe

wait

echo "==> run ./nob to compile and start Ccode-editor"