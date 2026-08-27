#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <map>
#include <algorithm>
#include <iomanip>
#include <sys/stat.h>
#include <vector>

namespace fs = std::filesystem;

// ── Config ──────────────────────────────────────────────────────────────────
static const char* DATA_DIR_ENV = "BMK_DATA_DIR";
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

static fs::path get_bookmarks_dir() {
    auto d = get_data_dir();
    fs::create_directories(d / "bookmarks");
    return d / "bookmarks";
}

// ── Helpers ─────────────────────────────────────────────────────────────────
static void ensure_bmkenv_dir() {
    auto d = get_data_dir();
    fs::create_directories(d / "templates");
}

static fs::path find_bmkenv(const fs::path& dir) {
    fs::path current = fs::weakly_canonical(dir);
    while (true) {
        auto p = current / ".bmkenv";
        if (fs::exists(p)) return p;
        auto parent = current.parent_path();
        if (parent == current) break; // reached root
        current = parent;
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

// ── Commands ────────────────────────────────────────────────────────────────

static void cmd_add(const std::string& name) {
    auto bdir = get_bookmarks_dir();
    fs::path bmfile = bdir / name;

    if (fs::exists(bmfile)) {
        std::cerr << "Error: bookmark '" << name << "' already exists" << std::endl;
        exit(1);
    }

    std::string target = fs::weakly_canonical(fs::current_path()).string();
    std::ofstream ofs(bmfile);
    if (!ofs) {
        std::cerr << "Error: cannot write to " << bmfile.string() << std::endl;
        exit(1);
    }
    ofs << target << "\n";
    std::cout << "Added bookmark '" << name << "' -> " << target << std::endl;
}

static void cmd_rm(const std::string& name) {
    auto bdir = get_bookmarks_dir();
    fs::path bmfile = bdir / name;

    if (!fs::exists(bmfile)) {
        std::cerr << "Error: bookmark '" << name << "' not found" << std::endl;
        exit(1);
    }

    fs::remove(bmfile);
    std::cout << "Removed bookmark '" << name << "'" << std::endl;
}

static void cmd_rename(const std::string& old_name, const std::string& new_name) {
    auto bdir = get_bookmarks_dir();
    fs::path old_bmfile = bdir / old_name;
    fs::path new_bmfile = bdir / new_name;

    if (!fs::exists(old_bmfile)) {
        std::cerr << "Error: bookmark '" << old_name << "' not found" << std::endl;
        exit(1);
    }

    if (fs::exists(new_bmfile)) {
        std::cerr << "Error: bookmark '" << new_name << "' already exists" << std::endl;
        exit(1);
    }

    // Create parent directories for new name (supports e.g. 'website/exhibitions')
    auto parent_dir = new_bmfile.parent_path();
    if (!parent_dir.empty() && parent_dir != bdir) {
        fs::create_directories(parent_dir);
    }

    fs::rename(old_bmfile, new_bmfile);
    std::cout << "Renamed bookmark '" << old_name << "' -> '" << new_name << "'" << std::endl;
}

static void cmd_ls() {
    auto bdir = get_bookmarks_dir();
    if (!fs::exists(bdir) || fs::is_empty(bdir)) {
        std::cout << "No bookmarks found." << std::endl;
        return;
    }

    // Collect and sort — use recursive_iterator to handle subdirectory bookmarks
    std::vector<std::pair<std::string, std::string>> bookmarks;
    for (const auto& entry : fs::recursive_directory_iterator(bdir)) {
        if (entry.is_regular_file()) {
            std::ifstream ifs(entry.path());
            std::string path;
            std::getline(ifs, path);
            // Use relative path from bookmarks dir as the bookmark name
            std::string name = fs::relative(entry.path(), bdir).string();
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

static void cmd_go(const std::string& name) {
    auto bdir = get_bookmarks_dir();
    fs::path bmfile = bdir / name;

    if (!fs::exists(bmfile)) {
        std::cerr << "Error: bookmark '" << name << "' not found" << std::endl;
        exit(1);
    }

    std::ifstream ifs(bmfile);
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
    fs::path bmkenv_path = find_bmkenv(target);

    std::cout << "cd '" << target << "'";
    if (!bmkenv_path.empty()) {
        std::cout << " && source '" << bmkenv_path.string() << "'";
    }
    std::cout << std::endl;
}

static void cmd_load() {
    std::string current;
    try {
        current = fs::current_path().string();
    } catch (const std::exception& e) {
        std::cerr << "Error: cannot determine current directory" << std::endl;
        exit(1);
    }

    fs::path bmkenv_path = find_bmkenv(current);
    if (bmkenv_path.empty()) {
        std::cout << "# No .bmkenv found in " << current << std::endl;
        return;
    }

    std::cout << "source .bmkenv" << std::endl;
}

static void cmd_mkenv() {
    ensure_bmkenv_dir();

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
  bmk mkenv           Create a boilerplate .bmkenv file

Notes:
  - Bookmarks are stored in: ~/.local/share/bmk/bookmarks/
  - Each bookmark is a file containing the target directory path
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
    } else if (cmd == "--help" || cmd == "-h") {
        print_usage();
    } else {
        std::cerr << "Unknown command: " << cmd << std::endl;
        std::cerr << "Run 'bmk' without arguments for usage." << std::endl;
        return 1;
    }

    return 0;
}
