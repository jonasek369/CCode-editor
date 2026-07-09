#!/usr/bin/env bash
set -euo pipefail

echo "==> Cleaning project dependencies..."

remove_if_exists() {
    local path="$1"

    if [ -e "$path" ]; then
        echo "Removing $path"
        rm -rf "$path"
    else
        echo "Skipping $path (not found)"
    fi
}

# Repos
remove_if_exists "PDCurses"
remove_if_exists "tree-sitter"
remove_if_exists "tiny_queue"

# Tree-sitter grammars
remove_if_exists "tree-sitter-grammar"

# Downloaded headers
remove_if_exists "stb_ds.h"
remove_if_exists "nob.h"
remove_if_exists "LSP.h"
remove_if_exists "parser.h"

# Built binaries
remove_if_exists "nob"
remove_if_exists "main"
remove_if_exists "nob.old"
remove_if_exists "build_grammar"
remove_if_exists "build_grammar.old"

remove_if_exists "compile_flags.txt"

echo "==> Cleanup complete."