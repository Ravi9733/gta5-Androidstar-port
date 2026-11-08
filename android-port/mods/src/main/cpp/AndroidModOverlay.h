#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fivestarport::mods {

// Read-only lookup layer for the Android game-file host.
// roots are ordered by priority: the first root wins on duplicate game paths.
class AndroidModOverlay {
public:
    bool rebuild(const std::vector<std::filesystem::path>& enabledModRoots,
                 std::string* error = nullptr);

    // gameRelativePath is a PC-style path such as
    // "x64/levels/gta5/vehicles.rpf/oracle.yft".
    // Returns the mod file to read, or nullopt when no mod replaces it.
    std::optional<std::filesystem::path> resolve(
        const std::string& gameRelativePath) const;

    std::size_t size() const noexcept { return files_.size(); }

private:
    std::unordered_map<std::string, std::filesystem::path> files_;
};

} // namespace fivestarport::mods
