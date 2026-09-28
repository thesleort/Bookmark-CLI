# bmk — Bash completion for the bookmark manager

_BMK_BIN=""
for candidate in \
	"$(cd "$(dirname "${BASH_SOURCE[0]}")" 2>/dev/null && pwd)/bmkbin" \
	"$HOME/.local/bmk/bmkbin" \
	"/usr/local/bmk/bmkbin"; do
	if [[ -x "$candidate" ]]; then
		_BMK_BIN="$candidate"
		break
	fi
done

_bmk_completion() {
	local cur prev words cword
	_init_completion || return

	# Get list of bookmarks
	local bmks
	bmks=$("$_BMK_BIN" ls 2>/dev/null | awk 'NR>2 {print $1}')

	case "$prev" in
		go|add|rm)
			COMPREPLY=( $(compgen -W "$bmks" -- "$cur") )
			return 0
			;;
		rename)
			if [[ $cur == "" ]]; then
				COMPREPLY=( $(compgen -W "$bmks" -- "$cur") )
			else
				COMPREPLY=()
			fi
			return 0
			;;
		*)
			COMPREPLY=( $(compgen -W "ls go add rm rename load mkenv globalenv editenv --help -h" -- "$cur") )
			;;
	esac
}

complete -F _bmk_completion bmk
