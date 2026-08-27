# bmk — Bookmark Manager

A minimal C++ CLI tool to bookmark directories and quickly navigate to them with automatic environment loading.

## Features

- **Quick navigation**: Register directories as bookmarks and jump to them instantly
- **Automatic environment loading**: Source `.bmkenv` files when navigating to bookmarked directories
- **Custom aliases**: Define shell aliases per-project in `.bmkenv`
- **Zero dependencies**: Pure C++ with only the standard library
- **Simple storage**: One file per bookmark, easy to understand and edit

## Installation

### Build from source

```bash
make build
make install
```

This installs two files to `~/.local/bmk/`:
- `bmkbin` — the compiled C++ binary
- `bmk` — a shell wrapper that delegates to `bmkbin`

### Add to PATH (if not already)

```bash
export PATH="$HOME/.local/bmk:$PATH"
```

### Tab completion (optional)

For bash, add to `~/.bashrc`:
```bash
source ~/.local/bmk/completion.bash
```

For zsh, add to `~/.zshrc`:
```bash
autoload -Uz compinit && compinit
source ~/.local/bmk/completion.zsh
```

## Usage

```bash
# List all bookmarks
bmk ls

# Add current directory as a bookmark
bmk add myproject

# Jump to a bookmark (auto-evals, no eval needed)
bmk go myproject

# Remove a bookmark
bmk rm myproject

# Rename a bookmark
bmk rename oldname newname

# Load .bmkenv in current directory (auto-evals)
bmk load

# Create a boilerplate .bmkenv file
bmk mkenv
```

## Storage

Bookmarks are stored in `~/.local/share/bmk/bookmarks/`:

```
~/.local/share/bmk/bookmarks/
├── myproject
├── work
└── server
```

Each file contains the absolute path to the bookmarked directory.

## .bmkenv Format

Create a `.bmkenv` file in any directory to load environment variables, aliases, and prompt customizations when navigating there:

```bash
# .bmkenv — Localized environment for this project

# ── Aliases ──
alias ll='ls -la'
alias dev='make dev'

# ── Environment Variables ──
export MY_PROJECT_DIR='/home/user/myproject'
export DEBUG=1

# ── Prompt ──
PS1='[myproject] '"$PS1"
```

Use `bmk mkenv` to create a boilerplate `.bmkenv` file in the current directory.

## Environment Variables

| Variable        | Description                           | Default                          |
|-----------------|---------------------------------------|----------------------------------|
| `BMK_DATA_DIR`  | Override the data directory location  | `~/.local/share/bmk`            |

## Directory Layout

```
~/.local/bmk/
├── bmk           ← shell wrapper (your "bmk" command)
├── bmkbin        ← compiled C++ binary
└── completion.bash  ← bash tab completion
```
| `BMK_BIN_PATH`  | Path to bmk binary (used in shell)    | Auto-detected                    |

## Build Requirements

- **C++17** compiler (g++ 7+, clang++ 5+)
- **CMake** 3.10+
- **Linux** (tested on Ubuntu)

No external dependencies — uses only the C++ standard library.
