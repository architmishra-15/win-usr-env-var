#include "path.h"

using namespace Colors;

class PathManager {
private:
  std::vector<std::string> userPaths;
  std::vector<std::string> systemPaths;

  std::string getEnvironmentVariable(const std::string &varName,
                                     HKEY hive = HKEY_CURRENT_USER) {
    DWORD bufSize = 128;
    std::string buffer;
    DWORD copied = 0;

    // Try environment variable first
    for (;;) {
      buffer.resize(bufSize);
      copied = GetEnvironmentVariableA(varName.c_str(), buffer.data(), bufSize);
      if (copied == 0)
        break;
      if (copied < bufSize) {
        buffer.resize(copied);
        return buffer;
      }
      bufSize = copied + 1;
    }

    // If not found, try registry
    HKEY hKey;
    std::string regPath = (hive == HKEY_CURRENT_USER)
                              ? "Environment"
                              : "SYSTEM\\CurrentControlSet\\Control\\Session "
                                "Manager\\Environment";

    LONG res = RegOpenKeyExA(hive, regPath.c_str(), 0, KEY_READ, &hKey);
    if (res == ERROR_SUCCESS) {
      bufSize = 32768; // Larger buffer for registry
      buffer.resize(bufSize);
      DWORD type;
      res = RegQueryValueExA(hKey, varName.c_str(), nullptr, &type,
                             reinterpret_cast<BYTE *>(buffer.data()), &bufSize);
      RegCloseKey(hKey);

      if (res == ERROR_SUCCESS && bufSize > 0) {
        buffer.resize(bufSize - 1); // Remove null terminator
        return buffer;
      }
    }

    return "";
  }

public:
  void loadPaths() {
    std::string userPath = getEnvironmentVariable("PATH", HKEY_CURRENT_USER);
    std::string systemPath = getEnvironmentVariable("PATH", HKEY_LOCAL_MACHINE);
    if (userPath == systemPath) {
      userPaths = splitPath(userPath);
      systemPaths.clear(); // don’t double‑count
    } else {
      userPaths = splitPath(userPath);
      systemPaths = splitPath(systemPath);
    }
  }

  std::vector<std::string> splitPath(const std::string &path) {
    std::vector<std::string> parts;
    std::stringstream iss(path);
    std::string token;

    while (std::getline(iss, token, ';')) {
      if (!token.empty()) {
        // Expand environment variables
        std::string expanded = expandEnvironmentStrings(token);
        parts.push_back(expanded);
      }
    }
    return parts;
  }

  std::string expandEnvironmentStrings(const std::string &str) {
    char buffer[32768];
    DWORD result =
        ExpandEnvironmentStringsA(str.c_str(), buffer, sizeof(buffer));
    if (result > 0 && result <= sizeof(buffer)) {
      return std::string(buffer);
    }
    return str;
  }

  bool isequals(const std::string &a, const std::string &b) {
    if (a.size() != b.size())
      return false;
    for (size_t i = 0; i < a.size(); i++)
      if (tolower(a[i]) != tolower(b[i]))
        return false;
    return true;
  }

  bool directoryExists(const std::string &path) {
    try {
      return std::filesystem::exists(path) &&
             std::filesystem::is_directory(path);
    } catch (...) {
      return false;
    }
  }

  std::string getShortenedPath(const std::string &path, size_t maxLength = 60) {
    if (path.length() <= maxLength)
      return path;
    size_t start = path.length() - maxLength + 3;
    return "..." + path.substr(start);
  }

  static std::string pad(const std::string &s, int width) {
    if ((int)s.size() >= width) return s.substr(0, width);
    return s + std::string(width - s.size(), ' ');
}

void printHeader(const std::string &title) {
    std::cout << "\n"
              << Colors::text::purple << Colors::bold
              << "╔════════════════════════════════════════════════════════════"
                 "══════════════════╗\n"
              << "║" << Colors::text::bright_yellow << title
              << std::setw((80 + title.length()) / 2 - title.length())
              << Colors::text::purple << Colors::bold << "║\n"
              << Colors::text::purple << Colors::bold
              << "╚════════════════════════════════════════════════════════════"
                 "══════════════════╝\n\n"
              << Colors::reset;
}

void printTableHeader() {
    std::cout << Colors::text::purple << "│" << Colors::reset << Colors::bold << " #  " << Colors::reset
              << Colors::text::purple << "│" << Colors::reset << Colors::bold << " ✓/✗ " << Colors::reset
              << Colors::text::purple << "│" << Colors::reset << Colors::bold << " Type " << Colors::reset
              << Colors::text::purple << "│ " << Colors::reset << Colors::bold << "Path" << Colors::reset
              << std::string(56, ' ') << Colors::text::purple << "│\n"
              << Colors::text::purple
              << "├────┼─────┼──────┼──────────────────────────────────────────"
                 "────────────────────┤\n"
              << Colors::reset;
}

void printSeparator() {
    std::cout << Colors::text::purple
              << "├────┼─────┼──────┼──────────────────────────────────────────"
                 "────────────────────┤\n"
              << Colors::reset;
}

void listAllPaths() {
    loadPaths();
    printHeader("COMPLETE PATH ANALYSIS");

    // Combine & analyze
    std::vector<std::pair<std::string, std::string>> allPaths;
    for (auto &p : userPaths)
      allPaths.emplace_back(p, "USER");
    for (auto &p : systemPaths)
      allPaths.emplace_back(p, "SYS ");
    int valid = 0, invalid = 0, dup = 0;
    std::set<std::string> seen;
    for (auto &e : allPaths) {
      auto low = e.first;
      std::transform(low.begin(), low.end(), low.begin(), ::tolower);
      if (seen.count(low))
        dup++;
      seen.insert(low);
      directoryExists(e.first) ? ++valid : ++invalid;
    }

    // Summary
    std::cout << Colors::text::bright_yellow << "📊 SUMMARY:\n"
              << Colors::text::white << "   Total entries: " << Colors::text::bright_cyan
              << allPaths.size() << Colors::text::white
              << " (User: " << Colors::text::bright_cyan << userPaths.size()
              << Colors::text::white << ", System: " << Colors::text::bright_cyan
              << systemPaths.size() << ")\n"
              << "   " << Colors::text::bright_green << "✅ Valid paths: " << valid
              << "\n"
              << "   " << Colors::text::bright_red << "❌ Invalid paths: " << invalid
              << "\n"
              << "   " << Colors::text::bright_magenta
              << "🔄 Potential duplicates: " << dup << "\n\n"
              << Colors::reset;

    if (allPaths.empty()) {
      std::cout << Colors::text::bright_black << "🔍 No PATH entries found.\n\n"
                << Colors::reset;
      return;
    }

    // Table
    std::cout << Colors::text::purple
              << "┌────┬─────┬──────┬──────────────────────────────────────────"
                 "────────────────────┐\n"
              << Colors::reset;
    printTableHeader();

    for (size_t i = 0; i < allPaths.size(); ++i) {
        auto &e = allPaths[i];
        bool ok = directoryExists(e.first);

        // Build padded fields with proper widths
        std::string idx = pad(std::to_string(i+1), 3);
        std::string mark = ok ? "✅" : "❌";
        std::string type = pad(e.second, 5); // Changed from 4 to 5 to match "Type " header
        std::string path = pad(getShortenedPath(e.first, 60), 60); // Increased path width

        // Print with colors around padded fields
        std::cout
            << Colors::text::purple << "│" << Colors::reset
            << Colors::text::bright_cyan << idx << Colors::reset
            << Colors::text::purple << " │ " << Colors::reset
            << (ok ? Colors::text::bright_green : Colors::text::bright_red) << mark << Colors::reset
            << Colors::text::purple << " │ " << Colors::reset;

        if (e.second == "USER") {
            std::cout << Colors::text::teal;
        } else {
            std::cout << Colors::text::turquoise;
        }
        std::cout << type << Colors::reset
                  << Colors::text::purple << " │ " << Colors::reset
                  << Colors::text::white << path << Colors::reset
                  << Colors::text::purple << " │\n" << Colors::reset;

        if (i+1 < allPaths.size()) printSeparator();
    }

    std::cout << Colors::text::purple
              << "└────┴─────┴──────┴──────────────────────────────────────────"
                 "────────────────────┘\n"
              << Colors::reset;

    printLegend();
}

void printLegend() {
    std::cout << "\n"
              << Colors::text::bright_yellow << "📋 LEGEND:\n"
              << Colors::reset << "   " << Colors::text::bright_green << "✅" << Colors::reset
              << " = Directory exists    " << Colors::text::bright_red << "❌" << Colors::reset
              << " = Directory missing\n"
              << "   " << Colors::text::teal << "USER" << Colors::reset
              << " = User PATH         " << Colors::text::cyan << "SYS" << Colors::reset
              << " = System PATH\n"
              << "   ... = Path truncated for display\n\n";
}


  void findDuplicates() {
    loadPaths();
    printHeader("DUPLICATE PATH ANALYSIS");

    std::map<std::string, std::vector<std::pair<std::string, std::string>>>
        pathMap;

    // Group paths by normalized version
    for (const auto &p : userPaths) {
      std::string normalized = p;
      std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                     ::tolower);
      pathMap[normalized].push_back({p, "USER"});
    }

    for (const auto &p : systemPaths) {
      std::string normalized = p;
      std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                     ::tolower);
      pathMap[normalized].push_back({p, "SYSTEM"});
    }

    bool foundDuplicates = false;
    for (const auto &entry : pathMap) {
      if (entry.second.size() > 1) {
        foundDuplicates = true;
        std::cout << "🔄 Duplicate found:\n";
        for (const auto &path : entry.second) {
          std::cout << "   [" << path.second << "] " << path.first << "\n";
        }
        std::cout << "\n";
      }
    }

    if (!foundDuplicates) {
      std::cout << "✅ No duplicates found!\n\n";
    }
  }

  void cleanupInvalidPaths() {
    loadPaths();
    printHeader("PATH CLEANUP");

    std::vector<std::string> validUserPaths;
    std::vector<std::string> removedPaths;

    for (const auto &path : userPaths) {
      if (directoryExists(path)) {
        validUserPaths.push_back(path);
      } else {
        removedPaths.push_back(path);
      }
    }

    if (removedPaths.empty()) {
      std::cout << "✅ All user PATH entries are valid!\n\n";
      return;
    }

    std::cout << "❌ Found " << removedPaths.size() << " invalid path(s):\n";
    for (const auto &path : removedPaths) {
      std::cout << "   • " << path << "\n";
    }

    std::cout << "\nDo you want to remove these invalid paths? (y/N): ";
    std::string response;
    std::getline(std::cin, response);

    if (response == "y" || response == "Y") {
      // Build new PATH
      std::string newPath;
      for (size_t i = 0; i < validUserPaths.size(); ++i) {
        if (i > 0)
          newPath += ";";
        newPath += validUserPaths[i];
      }

      if (setUserPath(newPath)) {
        std::cout << "✅ Successfully cleaned up PATH! Removed "
                  << removedPaths.size() << " invalid entries.\n";
      }
    } else {
      std::cout << "❌ Cleanup cancelled.\n";
    }
    std::cout << "\n";
  }

  void exportPath() {
    loadPaths();

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::string filename = "path_backup_" + std::to_string(time_t) + ".log";
    std::ofstream file(filename);

    if (!file) {
      std::cerr << "❌ Failed to create backup file: " << filename << "\n";
      return;
    }

    file << "# PATH Backup created at " << std::ctime(&time_t);
    file << "# User PATH entries:\n";
    for (const auto &path : userPaths) {
      file << path << "\n";
    }

    file << "\n# System PATH entries:\n";
    for (const auto &path : systemPaths) {
      file << path << "\n";
    }

    file.close();
    std::cout << "💾 PATH exported to: " << filename << "\n\n";
  }

  void searchInPath(const std::string &searchTerm) {
    loadPaths();
    printHeader("PATH SEARCH RESULTS");

    std::cout << "🔍 Searching for: \"" << searchTerm << "\"\n\n";

    bool found = false;
    std::vector<std::pair<std::string, std::string>> matches;

    // Search user paths
    for (const auto &path : userPaths) {
      std::string lowerPath = path;
      std::string lowerTerm = searchTerm;
      std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                     ::tolower);
      std::transform(lowerTerm.begin(), lowerTerm.end(), lowerTerm.begin(),
                     ::tolower);

      if (lowerPath.find(lowerTerm) != std::string::npos) {
        matches.push_back({path, "USER"});
        found = true;
      }
    }

    // Search system paths
    for (const auto &path : systemPaths) {
      std::string lowerPath = path;
      std::string lowerTerm = searchTerm;
      std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                     ::tolower);
      std::transform(lowerTerm.begin(), lowerTerm.end(), lowerTerm.begin(),
                     ::tolower);

      if (lowerPath.find(lowerTerm) != std::string::npos) {
        matches.push_back({path, "SYSTEM"});
        found = true;
      }
    }

    if (found) {
      for (const auto &match : matches) {
        bool exists = directoryExists(match.first);
        std::cout << "[" << match.second << "] " << (exists ? "✅" : "❌")
                  << " " << match.first << "\n";
      }
    } else {
      std::cout << "❌ No matches found.\n";
    }
    std::cout << "\n";
  }

  bool setUserPath(const std::string &newPath) {
    HKEY hKey;
    LONG res = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_SET_VALUE,
                             &hKey);
    if (res != ERROR_SUCCESS) {
      std::cerr << "❌ Failed to open registry key (code " << res << ")\n";
      return false;
    }

    res = RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
                         reinterpret_cast<const BYTE *>(newPath.c_str()),
                         static_cast<DWORD>(newPath.size() + 1));
    RegCloseKey(hKey);

    if (res != ERROR_SUCCESS) {
      std::cerr << "❌ Failed to set registry value (code " << res << ")\n";
      return false;
    }

    // Broadcast the change
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        reinterpret_cast<LPARAM>("Environment"),
                        SMTO_ABORTIFHUNG, 5000, nullptr);
    return true;
  }

  void addToUserPath(const std::string &newDir) {
    loadPaths();

    // Check if already exists
    for (const auto &p : userPaths) {
      if (isequals(p, newDir)) {
        std::cout << Colors::text::teal << "✅ \"" << newDir
                  << "\" is already in your user PATH.\n\n"
                  << Colors::reset;
        return;
      }
    }

    // Check if directory exists
    if (!directoryExists(newDir)) {
      std::cout << Colors::text::yellow << "⚠️  WARNING: Directory \"" << newDir
                << "\" does not exist.\n"
                << Colors::reset;
      std::cout << "Do you want to add it anyway? (y/N): ";
      std::string response;
      std::getline(std::cin, response);
      if (response != "y" && response != "Y") {
        std::cout << Colors::text::red << "❌ Operation cancelled.\n\n"
                  << Colors::reset;
        return;
      }
    }

    // Build new path
    std::string oldPath = getEnvironmentVariable("PATH", HKEY_CURRENT_USER);
    std::string newPath = oldPath;
    if (!newPath.empty() && newPath.back() != ';')
      newPath += ';';
    newPath += newDir;

    if (setUserPath(newPath)) {
      std::cout << Colors::text::teal << "✅ Successfully added \"" << newDir
                << "\" to your user PATH.\n"
                << Colors::reset;
      std::cout << Colors::text::yellow
                << "🔄 Note: You may need to restart applications for the "
                   "change to take effect.\n\n"
                << Colors::reset;
    }
  }

  void removeFromUserPath(const std::string &targetDir) {
    loadPaths();

    std::string expanded = expandEnvironmentStrings(targetDir);

    namespace fs = std::filesystem;

    std::error_code ec;
    fs::path tpath = fs::absolute(fs::path(expanded));
    fs::path tcanon = fs::weakly_canonical(tpath, ec);
    std::string normTarget = ec.value() ? tpath.string() : tcanon.string();

    bool found = false;
    std::vector<std::string> newPaths;

    for (const auto &p : userPaths) {

      fs::path up = fs::absolute(fs::path(expandEnvironmentStrings(p)));
      fs::path ucanon = fs::weakly_canonical(up, ec);
      std::string normStored = ec.value() ? up.string() : ucanon.string();

      if (std::equal(normStored.begin(), normStored.end(), normTarget.begin(),
                     normTarget.end(),
                     [](char a, char b) { return tolower(a) == tolower(b); })) {
        found = true;
      } else {
        newPaths.push_back(p);
      }
    }

    if (!found) {
      std::cout << "❌ \"" << targetDir << Colors::text::red
                << "\" not found in user PATH.\n\n"
                << Colors::reset;
      return;
    }

    std::string rebuilt;
    for (size_t i = 0; i < newPaths.size(); ++i) {
      if (i > 0)
        rebuilt += ';';
      rebuilt += newPaths[i];
    }

    if (setUserPath(rebuilt)) {
      std::cout << "✅ Successfully removed \"" << targetDir
                << "\" from user PATH.\n\n";
    }
  }
};

void showUsage(const char *programName) {
  using namespace Colors;
  std::cout << "\n";
  std::cout << text::bright_cyan // a cool cyan for framing
            << bold
            << "╔══════════════════════════════════════════════════════════════"
               "════════════════╗\n"
            << "║" << text::bright_white // white text on cyan border
            << italic << std::setw(74) << std::left << " PATH MANAGER " << bold
            << text::bright_cyan << "    ║\n"
            << "╚══════════════════════════════════════════════════════════════"
               "════════════════╝\n"
            << reset;
  std::cout << "\n";
  std::cout << text::yellow // yellow “📖 USAGE:”
            << bold << "📖 USAGE:\n"
            << reset;

  // Commands themselves
  std::cout << text::white                 // normal white commands
            << "   " << text::bright_green // green for verbs
            << "add-path" << text::white << " show" <<
            "                                 # Display all PATH entries\n"
            << "   " << text::bright_green << "add-path add" << text::white
            << " <directory>" << 
                            "                      # Add directory to user PATH\n"
            << "   " << text::bright_green << "add-path remove" << text::white
            << " <directory>                   # Remove directory from user PATH\n"
            << "   " << text::bright_green << "add-path search" << text::white
            << " <term>                        # Search PATH for term\n"
            << "   " << text::bright_green << "add-path clean" << text::white
            << "                                # Cleanup invalid user PATH entries\n"
            << "   " << text::bright_green << "add-path duplicates" << text::white
            << "                           # Find duplicate PATH entries\n"
            << "   " << text::bright_green << "add-path export" << text::white
            << "                               # Export current PATH to a log file\n"
            << reset;

  // Examples label
  std::cout << "\n" << text::yellow << bold << "💡 EXAMPLES:\n" << reset;

  std::cout << text::white << "   " << text::bright_green << "add-path"
            << text::white << " show\n"
            << "   " << text::bright_green << "add-path" << text::white
            << " add \"C:\\MyTools\\bin\"\n"
            << "   " << text::bright_green << "add-path" << text::white
            << " remove \"C:\\OldTool\"\n"
            << "   " << text::bright_green << "add-path" << text::white
            << " search python\n"
            << reset;
  std::cout << "\n";
}

int main(int argc, char *argv[]) {
  SetConsoleOutputCP(CP_UTF8);
  if (argc < 2) {
    showUsage(argv[0]);
    return 1;
  }

  PathManager pm;
  std::string cmd = argv[1];
  std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

  if (cmd == "show" || cmd == "list") {
    pm.listAllPaths();
  } else if (cmd == "add" && argc == 3) {
    pm.addToUserPath(argv[2]);
  } else if (cmd == "remove" && argc == 3) {
    pm.removeFromUserPath(argv[2]);
  } else if (cmd == "clean") {
    pm.cleanupInvalidPaths();
  } else if (cmd == "duplicates" || cmd == "dups") {
    pm.findDuplicates();
  } else if (cmd == "export" || cmd == "backup") {
    pm.exportPath();
  } else if (cmd == "search" && argc == 3) {
    pm.searchInPath(argv[2]);
  } else if (cmd == "version" || cmd == "--version" || cmd == "-v") {
    std::cout << "\n" << Colors::bold << VERSION << Colors::reset << "\n\n";
  } else {
    showUsage(argv[0]);
    return 1;
  }

  return 0;
}
