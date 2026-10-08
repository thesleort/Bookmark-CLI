#!/usr/bin/env bash
# bmg — Shell function for "bmk go" shorthand
# This file is sourced by bash/zsh to define the bmg function

# Find bmkbin: check same dir, then known locations
_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" 2>/dev/null && pwd)"
_BMG_BIN=""
for candidate in \
		"${_DIR}/bmkbin" \
		"${HOME}/.local/bmk/bmkbin" \
		"/usr/local/bmk/bmkbin"; do
		if [[ -x "$candidate" ]]; then
				_BMG_BIN="$candidate"
				break
		fi
done

if [[ -z "${_BMG_BIN:-}" ]]; then
		echo "Error: bmkbin not found. Run 'sudo make install' or 'make install'" >&2
		return 1 2>/dev/null || exit 1
fi

_bmg() {
		if [[ $# -eq 0 ]]; then
				echo "Usage: bmg <bookmark_name>" >&2
				return 1
		fi
		local result
		result=$("$_BMG_BIN" go "$@" 2>&1)
		local status=$?
		if [[ $status -eq 0 ]] && [[ -n "$result" ]]; then
				eval "$result"
		else
				echo "$result" >&2
				return $status
		fi
}

alias bmg=_bmg
