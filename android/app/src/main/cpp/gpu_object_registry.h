#pragma once

#include "gpu_command_packets.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>

/*
 * Owns copies of CREATE_SHADER / CREATE_LAYOUT metadata. Command-ring memory is
 * transient, so no record may retain pointers into a consumed packet.
 * This registry is metadata only; it does not claim that a shader binary or
 * Vulkan pipeline has been loaded.
 */
class PGAndroidGpuObjectRegistry {
public:
    struct Shader {
        uint32_t stage = 0;
        uint32_t hashLow = 0;
        uint32_t hashHigh = 0;
        std::vector<uint32_t> inputSignatures;
    };
    struct Layout {
        std::vector<uint32_t> elements;
    };

    bool createShader(const uint32_t *words, uint32_t wordCount) {
        PGAndroidCreateShaderPacket packet{};
        if (!PGAndroidDecodeCreateShaderPacket(words, wordCount, packet) ||
            shaders_.count(packet.objectId) || layouts_.count(packet.objectId)) return false;
        Shader shader{};
        shader.stage = packet.stage;
        shader.hashLow = packet.hashLow;
        shader.hashHigh = packet.hashHigh;
        shader.inputSignatures.assign(packet.inputSignatures,
                                      packet.inputSignatures + packet.inputCount * 4u);
        shaders_.emplace(packet.objectId, std::move(shader));
        return true;
    }

    bool createLayout(const uint32_t *words, uint32_t wordCount) {
        PGAndroidCreateLayoutPacket packet{};
        if (!PGAndroidDecodeCreateLayoutPacket(words, wordCount, packet) ||
            shaders_.count(packet.objectId) || layouts_.count(packet.objectId)) return false;
        Layout layout{};
        layout.elements.assign(packet.elements,
                               packet.elements + packet.elementCount * 7u);
        layouts_.emplace(packet.objectId, std::move(layout));
        return true;
    }

    const Shader *shader(uint32_t objectId) const {
        const auto it = shaders_.find(objectId);
        return it == shaders_.end() ? nullptr : &it->second;
    }


    // iOS shader library keys are the 16-hex-digit hash with high word first.
    // Multiple object IDs may legitimately refer to the same compiled shader.
    const Shader *shaderByHash(uint32_t stage, uint32_t hashLow, uint32_t hashHigh) const {
        for (const auto &entry : shaders_) {
            const Shader &candidate = entry.second;
            if (candidate.stage == stage && candidate.hashLow == hashLow &&
                candidate.hashHigh == hashHigh) return &candidate;
        }
        return nullptr;
    }

    static std::string shaderHash(uint32_t hashLow, uint32_t hashHigh) {
        static constexpr char digits[] = "0123456789abcdef";
        std::string result(16, '0');
        const uint32_t words[2] = {hashHigh, hashLow};
        for (unsigned w = 0; w < 2; ++w) {
            for (unsigned n = 0; n < 8; ++n) {
                const unsigned shift = 28u - n * 4u;
                result[w * 8u + n] = digits[(words[w] >> shift) & 0xfu];
            }
        }
        return result;
    }

    const Layout *layout(uint32_t objectId) const {
        const auto it = layouts_.find(objectId);
        return it == layouts_.end() ? nullptr : &it->second;
    }

    bool destroy(uint32_t objectId) {
        const bool shaderRemoved = shaders_.erase(objectId) != 0;
        const bool layoutRemoved = layouts_.erase(objectId) != 0;
        return shaderRemoved || layoutRemoved;
    }

    void clear() {
        shaders_.clear();
        layouts_.clear();
    }

private:
    std::unordered_map<uint32_t, Shader> shaders_;
    std::unordered_map<uint32_t, Layout> layouts_;
};
