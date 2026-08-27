#!/usr/bin/env bash
# bmk — Shell function for the bmk bookmark manager
# This file is sourced by bash/zsh to define the bmk function

# Find bmkbin: check same dir, then known locations
_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" 2>/dev/null && pwd)"
_BMK_BIN=""
for candidate in \
		"${_DIR}/bmkbin" \
		"${HOME}/.local/bmk/bmkbin" \
		"/usr/local/bmk/bmkbin"; do
		if [[ -x "$candidate" ]]; then
				_BMK_BIN="$candidate"
				break
		fi
done

if [[ -z "${_BMK_BIN:-}" ]]; then
		echo "Error: bmkbin not found. Run 'sudo make install' or 'make install'" >&2
		return 1 2>/dev/null || exit 1
fi

# Define bmk as a shell function so cd/load works in the parent shell
_bmk_impl() {
		local cmd="${1:-}"
		case "$cmd" in
				go|load)
						local result
						result=$("$_BMK_BIN" "$@" 2>&1)
						local status=$?
						if [[ $status -eq 0 ]] && [[ -n "$result" ]]; then
								eval "$result"
						else
								echo "$result" >&2
								return $status
						fi
						;;
				*)
						"$_BMK_BIN" "$@"
						;;
		esac
}

_bmk() {
		if [[ $# -eq 0 ]]; then
				"$_BMK_BIN" "$@"
				return $?
		fi
		case "$1" in
				go|load)
						_bmk_impl "$@"
						;;
				*)
						"$_BMK_BIN" "$@"
						;;
		esac
}

alias bmk=_bmk
