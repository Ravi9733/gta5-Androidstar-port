#include "../gpu_spirv_validation.h"
#include <cassert>
#include <cstdint>
#include <vector>

static std::vector<uint32_t> module(uint32_t model) {
    return {
        0x07230203u, 0x00010000u, 0u, 16u, 0u,
        (5u << 16) | 15u, model, 1u, 0x6e69616du, 0u
    };
}

int main() {
    auto vertex = module(0);
    assert(pgandroid::validateSpirvForStage(vertex.data(), vertex.size(), 0));
    assert(!pgandroid::validateSpirvForStage(vertex.data(), vertex.size(), 1));
    assert(!pgandroid::validateSpirvForStage(vertex.data(), vertex.size(), 3));

    auto fragment = module(4);
    assert(pgandroid::validateSpirvForStage(fragment.data(), fragment.size(), 1));
    auto compute = module(5);
    assert(pgandroid::validateSpirvForStage(compute.data(), compute.size(), 2));

    auto truncated = vertex;
    truncated[5] = (7u << 16) | 15u;
    assert(!pgandroid::validateSpirvForStage(truncated.data(), truncated.size(), 0));

    auto unterminated = vertex;
    unterminated[9] = 0x41414141u;
    assert(!pgandroid::validateSpirvForStage(unterminated.data(), unterminated.size(), 0));

    auto badId = vertex;
    badId[7] = 16u;
    assert(!pgandroid::validateSpirvForStage(badId.data(), badId.size(), 0));

    auto decorated = vertex;
    decorated.insert(decorated.end(), {
        (4u << 16) | 71u, 2u, 33u, 0u,
        (4u << 16) | 71u, 2u, 34u, 0u
    });
    assert(pgandroid::validateSpirvForStage(decorated.data(), decorated.size(), 0));
    decorated.back() = 3u;
    assert(!pgandroid::validateSpirvForStage(decorated.data(), decorated.size(), 0));

    auto unmatched = vertex;
    unmatched.insert(unmatched.end(), {(4u << 16) | 71u, 2u, 33u, 0u});
    assert(!pgandroid::validateSpirvForStage(unmatched.data(), unmatched.size(), 0));
    return 0;
}
