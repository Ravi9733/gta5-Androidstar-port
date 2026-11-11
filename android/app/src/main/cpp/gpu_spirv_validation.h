#pragma once
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace pgandroid {

inline bool validateSpirvForStage(const uint32_t *words, size_t wordCount,
                                  uint32_t stageIndex) {
    if (!words || wordCount < 5 || stageIndex >= 3 ||
        words[0] != 0x07230203u || words[1] < 0x00010000u ||
        words[1] > 0x00010600u || words[3] == 0 || words[4] != 0)
        return false;

    const uint32_t expectedModel = stageIndex == 0 ? 0u : stageIndex == 1 ? 4u : 5u;
    const uint32_t idBound = words[3];
    std::unordered_map<uint32_t, uint32_t> bindings;
    std::unordered_map<uint32_t, uint32_t> sets;
    bool hasEntryPoint = false;

    for (size_t at = 5; at < wordCount;) {
        const uint32_t instruction = words[at];
        const uint32_t count = instruction >> 16;
        const uint32_t opcode = instruction & 0xffffu;
        if (count == 0 || count > wordCount - at) return false;

        // OpEntryPoint: execution model, function id, and a null-terminated name.
        if (opcode == 15u) {
            if (count < 4u || words[at + 2] == 0 || words[at + 2] >= idBound)
                return false;
            if (words[at + 1] != expectedModel) return false;
            bool terminated = false;
            for (uint32_t i = 3; i < count; ++i) {
                const uint32_t value = words[at + i];
                if ((value & 0xffu) == 0 || (value & 0xff00u) == 0 ||
                    (value & 0xff0000u) == 0 || (value & 0xff000000u) == 0) {
                    terminated = true;
                    break;
                }
            }
            if (!terminated) return false;
            hasEntryPoint = true;
        }

        // OpDecorate: ensure descriptor bindings and sets can be paired safely.
        if (opcode == 71u && count >= 3u) {
            const uint32_t target = words[at + 1];
            const uint32_t decoration = words[at + 2];
            if (target == 0 || target >= idBound) return false;
            if ((decoration == 33u || decoration == 34u) && count != 4u)
                return false;
            if (decoration == 33u) {
                if (!bindings.emplace(target, words[at + 3]).second) return false;
            } else if (decoration == 34u) {
                if (!sets.emplace(target, words[at + 3]).second) return false;
            }
        }
        at += count;
    }

    if (!hasEntryPoint || bindings.size() != sets.size()) return false;
    for (const auto &entry : bindings)
        if (sets.find(entry.first) == sets.end()) return false;
    for (const auto &entry : sets)
        if (bindings.find(entry.first) == bindings.end() || entry.second > 2u) return false;
    return true;
}

} // namespace pgandroid
