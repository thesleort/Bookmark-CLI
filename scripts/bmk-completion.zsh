# bmk — Zsh completion for the bookmark manager

_BMK_BIN=""
for candidate in \
	"$(cd "$(dirname "${(%):-%x}")" 2>/dev/null && pwd)/bmkbin" \
	"$HOME/.local/bmk/bmkbin" \
	"/usr/local/bmk/bmkbin"; do
	if [[ -x "$candidate" ]]; then
		_BMK_BIN="$candidate"
		break
	fi
done

_bmk() {
	local -a commands
	local bmks

	commands=(
		'ls:List all bookmarks'
		'go:Jump to bookmark'
		'add:Add current directory as bookmark'
		'rm:Remove a bookmark'
		'load:Source .bmkenv in current directory'
		'mkenv:Create a boilerplate .bmkenv file'
	)

	# Get bookmark names
	bmks=$(${_BMK_BIN:-bmk} ls 2>/dev/null | awk 'NR>2 {print $1}')

	# If we're completing the first argument (command)
	if [[ $CURRENT -eq 2 ]]; then
		_describe -t commands 'bmk command' commands
		return
	fi

	local cmd="${words[2]}"
	case "$cmd" in
		go|add|rm)
			# Complete with bookmark names
			compadd -M 'm:{a-z}={A-Z} M:{a-z}={A-Z}' -- $bmks
			;;
		*)
			# Complete commands
			compadd -- ${commands[@]:t}
			;;
	esac
}

compdef _bmk bmk
