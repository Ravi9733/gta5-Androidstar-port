#include "AndroidModOverlay.h"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace fs = std::filesystem;

namespace fivestarport::mods {
namespace {

std::string lowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool safeRelative(std::string value, std::string& normalized) {
    std::replace(value.begin(), value.end(), '\\', '/');
    if (value.empty() || value.front() == '/' ||
        (value.size() > 1 && value[1] == ':')) return false;

    std::string part;
    normalized.clear();
    for (char c : value) {
        if (c == '/') {
            if (part.empty() || part == "." || part == "..") return false;
            if (!normalized.empty()) normalized.push_back('/');
            normalized += part;
            part.clear();
        } else {
            if (c == '\0') return false;
            part.push_back(c);
        }
    }
    if (part.empty() || part == "." || part == "..") return false;
    if (!normalized.empty()) normalized.push_back('/');
    normalized += part;
    return true;
}

bool isWithin(const fs::path& root, const fs::path& candidate) {
    auto r = root.begin();
    auto c = candidate.begin();
    for (; r != root.end(); ++r, ++c) {
        if (c == candidate.end() || *r != *c) return false;
    }
    return true;
}

} // namespace

bool AndroidModOverlay::rebuild(
    const std::vector<fs::path>& enabledModRoots,
    std::string* error) {
    files_.clear();

    for (const auto& suppliedRoot : enabledModRoots) {
        std::error_code ec;
        const fs::path root = fs::canonical(suppliedRoot, ec);
        if (ec || !fs::is_directory(root, ec)) {
            if (error) *error = "An enabled mod directory is missing or unreadable.";
            files_.clear();
            return false;
        }

        fs::recursive_directory_iterator it(
            root, fs::directory_options::skip_permission_denied, ec), end;
        if (ec) {
            if (error) *error = "Could not scan an enabled mod directory.";
            files_.clear();
            return false;
        }

        for (; it != end; it.increment(ec)) {
            if (ec) {
                ec.clear();
                continue;
            }

            const fs::file_status status = it->symlink_status(ec);
            if (ec || !fs::is_regular_file(status)) {
                ec.clear();
                continue; // Never follow or index symlinks.
            }

            const fs::path candidate = fs::canonical(it->path(), ec);
            if (ec || !isWithin(root, candidate)) {
                ec.clear();
                continue;
            }

            const fs::path relative = fs::relative(candidate, root, ec);
            if (ec) {
                ec.clear();
                continue;
            }

            std::string key = lowerAscii(relative.generic_string());
            // First enabled mod root has precedence over later roots.
            files_.emplace(std::move(key), candidate);
        }
    }

    return true;
}

std::optional<fs::path> AndroidModOverlay::resolve(
    const std::string& gameRelativePath) const {
    std::string normalized;
    if (!safeRelative(gameRelativePath, normalized)) return std::nullopt;

    const auto found = files_.find(lowerAscii(std::move(normalized)));
    if (found == files_.end()) return std::nullopt;
    return found->second;
}

} // namespace fivestarport::mods
