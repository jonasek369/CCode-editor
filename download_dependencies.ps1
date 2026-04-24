$ErrorActionPreference = "Stop"

function Clone-RepoIfNeeded {
    param (
        [string]$RepoUrl,
        [string]$RepoDir
    )

    if (-not (Test-Path $RepoDir)) {
        Write-Host "Cloning $RepoDir..."
        git clone $RepoUrl $RepoDir
    } else {
        Write-Host "Repo '$RepoDir' already exists, skipping clone."
    }
}

function Build-PDCurses {
    $repoUrl = "https://github.com/wmcbrine/PDCurses"
    $repoDir = "PDCurses"

    Write-Host "==> Building PDCurses..."

    Clone-RepoIfNeeded $repoUrl $repoDir

    Push-Location "$repoDir/wincon"

    if (-not (Test-Path "Makefile")) {
        if (Test-Path "configure") {
            bash configure
        }
    }

    make
    Rename-Item pdcurses.a libpdcurses.a
    
    Pop-Location

    Write-Host "==> PDCurses build complete."
}

function Build-TreeSitter {
    $repoUrl = "https://github.com/tree-sitter/tree-sitter"
    $repoDir = "tree-sitter"

    Write-Host "==> Building Tree-sitter..."

    Clone-RepoIfNeeded $repoUrl $repoDir

    Push-Location $repoDir
    make
    Pop-Location

    Write-Host "==> Tree-sitter build complete."
}

function Download-TreeSitterLanguage {
    param (
        [string]$RepoUrl,
        [string]$RepoDir
    )

    $targetDir = Join-Path "tree-sitter-grammar" $RepoDir

    Write-Host "==> Cloning $RepoDir..."
    Clone-RepoIfNeeded $RepoUrl $targetDir
    Write-Host "==> $RepoDir cloning complete."
}

function Download-File {
    param (
        [string]$Url,
        [string]$OutFile
    )

    Write-Host "Downloading $OutFile..."
    Invoke-WebRequest $Url -OutFile $OutFile
}

# ===== Download headers =====
Download-File "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_ds.h" "stb_ds.h"
Download-File "https://raw.githubusercontent.com/tsoding/nob.h/refs/heads/main/nob.h" "nob.h"
Download-File "https://raw.githubusercontent.com/jonasek369/C-LSP-Client/refs/heads/main/LSP.h" "LSP.h"
Download-File "https://raw.githubusercontent.com/jonasek369/C-JSON/refs/heads/main/parser.h" "parser.h"

Clone-RepoIfNeeded "https://github.com/jonasek369/tiny_queue" "tiny_queue"

# ===== Build dependencies =====
Build-PDCurses
Build-TreeSitter

# ===== Languages =====
New-Item -ItemType Directory -Force -Path "tree-sitter-grammar" | Out-Null

$languages = @(
    @{ url = "https://github.com/tree-sitter/tree-sitter-c";        dir = "tree-sitter-c" },
    @{ url = "https://github.com/tree-sitter/tree-sitter-python";   dir = "tree-sitter-python" },
    @{ url = "https://github.com/tree-sitter/tree-sitter-json";     dir = "tree-sitter-json" },
    @{ url = "https://github.com/tree-sitter/tree-sitter-c-sharp";  dir = "tree-sitter-c-sharp" }
)

foreach ($lang in $languages) {
    Download-TreeSitterLanguage $lang.url $lang.dir
}

# ===== Build nob =====
Write-Host "==> Building nob..."
gcc nob.c -o nob.exe

Write-Host "==> Run ./nob.exe to compile and start Ccode-editor"