/**
 * @file main.cpp
 * @author Troels Blicher Petersen
 * @brief 
 * @version 1.0
 * @date 2026-08-28
 * 
 * @copyright Copyright (c) 2026
 * 
 */


#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <map>
#include <algorithm>
#include <iomanip>
#include <sys/stat.h>
#include <vector>
#include <cstdlib>

namespace fs = std::filesystem;

// -- Config ------------------------------------------------------------------

static const char* DATA_DIR_ENV = "BMK_DATA_DIR";

/**
 * @brief Get the data directory path
 * 
 * @return fs::path 
 */
static fs::path get_data_dir() {
	const char* env = std::getenv(DATA_DIR_ENV);
	if (env && fs::exists(env)) {
		return fs::path(env);
	}
	const char* home = std::getenv("HOME");
	if (!home) {
		std::cerr << "Error: $HOME not set" << std::endl;
		exit(1);
	}
	return fs::path(home) / ".local" / "share" / "bmk";
}

/**
 * @brief Get the bookmarks directory path
 * 
 * @return fs::path 
 */
static fs::path get_bookmarks_dir() {
	auto data_dir = get_data_dir();
	fs::create_directories(data_dir / "bookmarks");
	return data_dir / "bookmarks";
}

// -- Helpers -----------------------------------------------------------------

/**
 * @brief Append .bmk extension to a bookmark name.
 */
static std::string bookmark_filename(const std::string& name) {
	return name + ".bmk";
}

/**
 * @brief Strip .bmk extension from a stored filename to get the display name.
 */
static std::string strip_bmk_extension(const std::string& filename) {
	if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".bmk") {
		return filename.substr(0, filename.size() - 4);
	}
	return filename;
}

/**
 * @brief Find the bookmark name that points to a given directory.
 *
 * Searches all bookmarks (recursively) for one whose content matches dir.
 * Returns empty string if no match found.
 *
 * @param dir
 * @return std::string
 */
static std::string find_bookmark_name_for_dir(const fs::path& dir) {
	auto bookmarks_dir = get_bookmarks_dir();
	if (!fs::exists(bookmarks_dir)) return {};

	fs::path canonical_dir = fs::weakly_canonical(dir);
	for (const auto& entry : fs::recursive_directory_iterator(bookmarks_dir)) {
		if (entry.is_regular_file()) {
			std::ifstream ifs(entry.path());
			std::string path;
			if (std::getline(ifs, path)) {
				try {
					if (fs::weakly_canonical(path) == canonical_dir) {
						return strip_bmk_extension(
							fs::relative(entry.path(), bookmarks_dir).string()
						);
					}
				} catch (...) {
					// skip entries that fail canonicalization
				}
			}
		}
	}
	return {};
}

/**
 * @brief Finds the .bmkenv environment file.
 *
 * Priority:
 *   1. Local .bmkenv in directory tree (up to root)
 *   2. Global .bmkenv stored alongside the bookmark file
 *      (only when bookmark_name is provided)
 *
 * @param dir
 * @param bookmark_name  optional bookmark name for global lookup
 * @return fs::path
 */
static fs::path find_bmkenv(const fs::path& dir, const std::string& bookmark_name = {}) {
	// 1. Check local first (existing behavior)
	fs::path current_path = fs::weakly_canonical(dir);
	while (true) {
		auto p = current_path / ".bmkenv";
		if (fs::exists(p)) return p;
		auto parent = current_path.parent_path();
		if (parent == current_path) break; // reached root
		current_path = parent;
	}
	// 2. Fallback to global .bmkenv next to the bookmark file
	if (!bookmark_name.empty()) {
		auto bookmarks_dir = get_bookmarks_dir();
		fs::path bmkenv_file = bookmarks_dir / (bookmark_name + ".bmkenv");
		if (fs::exists(bmkenv_file)) return bmkenv_file;
	}
	return {};
}

static std::string resolve_path(const std::string& input) {
	fs::path p(input);
	if (p.is_relative()) {
		const char* home = std::getenv("HOME");
		if (home) p = fs::path(home) / p;
		else {
			std::cerr << "Error: relative path with no $HOME" << std::endl;
			exit(1);
		}
	}
	return fs::weakly_canonical(p).string();
}

/**
 * @brief Get the editor command to use (default: nano).
 */
static std::string get_editor() {
	const char* env = std::getenv("BMK_EDITOR");
	if (env && std::string(env).length() > 0) {
		return std::string(env);
	}
	return "nano";
}

/**
 * @brief Open a file in the configured editor.
 */
static void open_editor(const fs::path& file) {
	std::string editor = get_editor();
	int ret = std::system((editor + " \"" + file.string() + "\"").c_str());
	(void)ret;
}

// -- Commands ----------------------------------------------------------------

/**
 * @brief Add new bookmark
 * 
 * @param name 
 */
static void cmd_add(const std::string& name) {
	auto bookmarks_dir = get_bookmarks_dir();
	fs::path bookmark_file = bookmarks_dir / bookmark_filename(name);

	if (fs::exists(bookmark_file)) {
		std::cerr << "Error: bookmark '" << name << "' already exists" << std::endl;
		exit(1);
	}

	std::string target = fs::weakly_canonical(fs::current_path()).string();
	std::ofstream ofs(bookmark_file);
	if (!ofs) {
		std::cerr << "Error: cannot write to " << bookmark_file.string() << std::endl;
		exit(1);
	}
	ofs << target << "\n";
	std::cout << "Added bookmark '" << name << "' -> " << target << std::endl;

	// Open global .bmkenv in editor if it exists
	fs::path bmkenv_file = bookmarks_dir / (name + ".bmkenv");
	if (fs::exists(bmkenv_file)) {
		open_editor(bmkenv_file);
	}
}

/**
 * @brief Remove existing bookmark
 * 
 * @param name 
 */
static void cmd_rm(const std::string& name) {
	auto bookmarks_dir = get_bookmarks_dir();
	fs::path bookmark_file = bookmarks_dir / bookmark_filename(name);

	if (!fs::exists(bookmark_file)) {
		std::cerr << "Error: bookmark '" << name << "' not found" << std::endl;
		exit(1);
	}

	fs::remove(bookmark_file);
	// Also remove global .bmkenv if it exists
	fs::path bmkenv_file = bookmarks_dir / (name + ".bmkenv");
	if (fs::exists(bmkenv_file)) {
		fs::remove(bmkenv_file);
	}

	std::cout << "Removed bookmark '" << name << "'" << std::endl;
}

/**
 * @brief Rename existing bookmark
 * 
 * @param old_name 
 * @param new_name 
 */
static void cmd_rename(const std::string& old_name, const std::string& new_name) {
	auto bookmarks_dir = get_bookmarks_dir();
	fs::path old_bookmark_file = bookmarks_dir / bookmark_filename(old_name);
	fs::path new_bookmark_file = bookmarks_dir / bookmark_filename(new_name);

	if (!fs::exists(old_bookmark_file)) {
		std::cerr << "Error: bookmark '" << old_name << "' not found" << std::endl;
		exit(1);
	}

	if (fs::exists(new_bookmark_file)) {
		std::cerr << "Error: bookmark '" << new_name << "' already exists" << std::endl;
		exit(1);
	}

	// Create parent directories for new name (supports e.g. 'website/exhibitions')
	auto parent_dir = new_bookmark_file.parent_path();
	if (!parent_dir.empty() && parent_dir != bookmarks_dir) {
		fs::create_directories(parent_dir);
	}

	// Also rename global .bmkenv if it exists
	fs::path old_bmkenv_file = bookmarks_dir / (old_name + ".bmkenv");
	fs::path new_bmkenv_file = bookmarks_dir / (new_name + ".bmkenv");
	if (fs::exists(old_bmkenv_file)) {
		fs::rename(old_bmkenv_file, new_bmkenv_file);
	}

	fs::rename(old_bookmark_file, new_bookmark_file);
	std::cout << "Renamed bookmark '" << old_name << "' -> '" << new_name << "'" << std::endl;
}

/**
 * @brief List all current bookmarks
 * 
 */
static void cmd_ls() {
	auto bookmarks_dir = get_bookmarks_dir();
	if (!fs::exists(bookmarks_dir) || fs::is_empty(bookmarks_dir)) {
		std::cout << "No bookmarks found." << std::endl;
		return;
	}

	// Collect and sort — use recursive_iterator to handle subdirectory bookmarks
	std::vector<std::pair<std::string, std::string>> bookmarks;
	for (const auto& entry : fs::recursive_directory_iterator(bookmarks_dir)) {
		if (entry.is_regular_file()) {
			std::ifstream ifs(entry.path());
			std::string path;
			std::getline(ifs, path);
			// Use relative path from bookmarks dir as the bookmark name (strip .bmk)
			std::string name = strip_bmk_extension(
				fs::relative(entry.path(), bookmarks_dir).string()
			);
			bookmarks.push_back({name, path});
		}
	}
	std::sort(bookmarks.begin(), bookmarks.end());

	if (bookmarks.empty()) {
		std::cout << "No bookmarks found." << std::endl;
		return;
	}

	// Find max name length for alignment
	size_t max_name = 0;
	for (const auto& [name, _] : bookmarks) {
		max_name = std::max(max_name, name.size());
	}

	std::cout << std::left << std::setw(max_name + 2) << "NAME"
			  << "PATH" << std::endl;
	std::cout << std::string(max_name + 2, '-') << std::endl;

	for (const auto& [name, path] : bookmarks) {
		std::cout << std::left << std::setw(max_name + 2) << name << path << std::endl;
	}
}

/**
 * @brief Go to specified bookmark
 * 
 * @param name 
 */
static void cmd_go(const std::string& name) {
	auto bookmark_dir = get_bookmarks_dir();
	fs::path bookmark_file = bookmark_dir / bookmark_filename(name);

	if (!fs::exists(bookmark_file)) {
		std::cerr << "Error: bookmark '" << name << "' not found" << std::endl;
		exit(1);
	}

	std::ifstream ifs(bookmark_file);
	std::string target;
	if (!std::getline(ifs, target) || target.empty()) {
		std::cerr << "Error: bookmark '" << name << "' is empty" << std::endl;
		exit(1);
	}

	// Canonicalize
	try {
		target = fs::weakly_canonical(target).string();
	} catch (...) {
		// If canonicalization fails, use as-is
	}

	if (!fs::exists(target)) {
		std::cerr << "Warning: directory '" << target << "' does not exist" << std::endl;
	}

	// Find .bmkenv in target directory or its parents
	std::string bm_name = find_bookmark_name_for_dir(target);
	fs::path bmkenv_path = find_bmkenv(target, bm_name);

	std::cout << "cd '" << target << "'";
	if (!bmkenv_path.empty()) {
		std::cout << " && source '" << bmkenv_path.string() << "'";
	}
	std::cout << std::endl;
}

/**
 * @brief Loads the bmkenv in the current directory.
 * 
 * This is useful if the user has not navigated to the directory using bmk.
 * 
 */
static void cmd_load() {
	std::string current_path;
	try {
		current_path = fs::current_path().string();
	} catch (const std::exception& e) {
		std::cerr << "Error: cannot determine current directory" << std::endl;
		exit(1);
	}

	// Find bookmark name for this directory (for global .bmkenv fallback)
	std::string bm_name = find_bookmark_name_for_dir(current_path);

	fs::path bmkenv_path = find_bmkenv(current_path, bm_name);
	if (bmkenv_path.empty()) {
		std::cout << "# No .bmkenv found in " << current_path << std::endl;
		return;
	}

	std::cout << "source '" << bmkenv_path.string() << "'" << std::endl;
}

/**
 * @brief Creates a new bmkenv in the current directory
 * 
 */
static void cmd_mkenv() {
	fs::path bmkenv_path = fs::current_path() / ".bmkenv";

	if (fs::exists(bmkenv_path)) {
		std::cerr << "Error: .bmkenv already exists in " << fs::current_path().string() << std::endl;
		exit(1);
	}

	std::ofstream ofs(bmkenv_path);
	if (!ofs) {
		std::cerr << "Error: cannot create .bmkenv" << std::endl;
		exit(1);
	}

	ofs << "# .bmkenv — Localized environment for this project\n";
	ofs << "# Generated by bmk mkenv — edit as needed\n\n";
	ofs << "# ── Aliases ──\n";
	ofs << "# alias ll='ls -la'\n";
	ofs << "# alias dev='make dev'\n\n";
	ofs << "# ── Environment Variables ──\n";
	ofs << "# export MY_PROJECT_DIR='" << fs::current_path().string() << "'\n";
	ofs << "# export DEBUG=1\n\n";
	ofs << "# ── Prompt ──\n";
	ofs << "# PS1='['\"$(basename '\"$PWD\"')\"'] '\"$PS1\"'\n";
	ofs << "\n";

	std::cout << "Created .bmkenv in " << fs::current_path().string() << std::endl;

	// Open the newly created .bmkenv in editor
	open_editor(bmkenv_path);
}

/**
 * @brief Creates a global .bmkenv file next to the matching bookmark.
 *
 * Searches all bookmarks for one pointing to the current directory,
 * then creates <bookmarks_dir>/<name>.bmkenv alongside it.
 * 
 */
static void cmd_globalenv() {
	fs::path current = fs::weakly_canonical(fs::current_path());
	std::string bm_name = find_bookmark_name_for_dir(current);

	if (bm_name.empty()) {
		std::cerr << "Error: no bookmark found for '" << current.string() << "'" << std::endl;
		std::cerr << "       : run 'bmk add <name>' first" << std::endl;
		exit(1);
	}

	auto bookmarks_dir = get_bookmarks_dir();
	fs::path bmkenv_file = bookmarks_dir / (bm_name + ".bmkenv");

	if (fs::exists(bmkenv_file)) {
		std::cerr << "Error: global .bmkenv already exists for '" << bm_name << "'" << std::endl;
		std::cerr << "       : " << bmkenv_file.string() << std::endl;
		exit(1);
	}

	std::ofstream ofs(bmkenv_file);
	if (!ofs) {
		std::cerr << "Error: cannot create global .bmkenv" << std::endl;
		exit(1);
	}

	ofs << "# .bmkenv — Global environment for " << current.string() << "\n";
	ofs << "# Generated by bmk globalenv — edit as needed\n\n";
	ofs << "# ── Aliases ──\n";
	ofs << "# alias ll='ls -la'\n";
	ofs << "# alias dev='make dev'\n\n";
	ofs << "# ── Environment Variables ──\n";
	ofs << "# export MY_PROJECT_DIR='" << current.string() << "'\n";
	ofs << "# export DEBUG=1\n\n";
	ofs << "# ── Prompt ──\n";
	ofs << "# PS1='['\"$(basename '\"$PWD\"')\"'] '\"$PS1\"'\n";
	ofs << "\n";

	std::cout << "Created global .bmkenv for '" << bm_name << "' in " << bmkenv_file.string() << std::endl;
}

/**
 * @brief Opens the global .bmkenv for the current directory in the editor.
 *
 * Finds the bookmark pointing to the current directory, then opens
 * <bookmarks_dir>/<name>.bmkenv in $BMK_EDITOR (default: nano).
 * 
 */
static void cmd_editenv() {
	fs::path current = fs::weakly_canonical(fs::current_path());
	std::string bm_name = find_bookmark_name_for_dir(current);

	if (bm_name.empty()) {
		std::cerr << "Error: no bookmark found for '" << current.string() << "'" << std::endl;
		std::cerr << "       : run 'bmk add <name>' first" << std::endl;
		exit(1);
	}

	auto bookmarks_dir = get_bookmarks_dir();
	fs::path bmkenv_file = bookmarks_dir / (bm_name + ".bmkenv");

	if (!fs::exists(bmkenv_file)) {
		std::cerr << "Error: no global .bmkenv found for '" << bm_name << "'" << std::endl;
		std::cerr << "       : run 'bmk globalenv' first" << std::endl;
		exit(1);
	}

	open_editor(bmkenv_file);
	std::cout << "Edited global .bmkenv for '" << bm_name << "' in " << bmkenv_file.string() << std::endl;
}

static void print_usage() {
	std::cout << R"(bmk — Bookmark Manager

Usage:
  bmk ls              List all bookmarks
  bmk go <name>       Jump to bookmark (use: eval $(bmk go <name>))
  bmk add <name>      Add current directory as bookmark
  bmk rm <name>       Remove a bookmark
  bmk rename <old> <new>  Rename a bookmark
  bmk load            Source .bmkenv in current directory
  bmk mkenv           Create a local boilerplate .bmkenv file
  bmk globalenv       Create a global .bmkenv next to the matching bookmark
  bmk editenv         Edit the global .bmkenv for the current directory

Notes:
  - Bookmarks are stored in: ~/.local/share/bmk/bookmarks/
  - Each bookmark is a <name>.bmk file containing the target directory path
  - Global .bmkenv files are stored as <bookmarks_dir>/<name>.bmkenv
  - .bmkenv lookup: local first (in directory tree), then global fallback
  - Use eval $(bmk go <name>) to change directory and load .bmkenv)";
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		print_usage();
		return 0;
	}

	std::string cmd = argv[1];

	if (cmd == "ls") {
		cmd_ls();
	} else if (cmd == "add" && argc >= 3) {
		cmd_add(argv[2]);
	} else if (cmd == "rm" && argc >= 3) {
		cmd_rm(argv[2]);
	} else if (cmd == "rename" && argc >= 4) {
		cmd_rename(argv[2], argv[3]);
	} else if (cmd == "go" && argc >= 3) {
		cmd_go(argv[2]);
	} else if (cmd == "load") {
		cmd_load();
	} else if (cmd == "mkenv") {
		cmd_mkenv();
	} else if (cmd == "globalenv") {
		cmd_globalenv();
	} else if (cmd == "editenv") {
		cmd_editenv();
	} else if (cmd == "--help" || cmd == "-h") {
		print_usage();
	} else {
		std::cerr << "Unknown command: " << cmd << std::endl;
		std::cerr << "Run 'bmk' without arguments for usage." << std::endl;
		return 1;
	}

	return 0;
}
