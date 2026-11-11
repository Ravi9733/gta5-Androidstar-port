#include "NativeGraphics.h"
#include "gpu_command_ring.h"

#include <android/log.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <mutex>

extern "C" bool PGAndroidGpuIsReady(void);
extern "C" uint32_t PGGraphicsNotify(uint64_t address) __attribute__((weak));
extern "C" uint32_t PGGraphicsWait(uint64_t address, uint32_t expected,
                                    int64_t timeoutNs) __attribute__((weak));

namespace {
constexpr const char *kTag = "GTA5Android";
constexpr uint32_t kRingDataOffset = PG_ANDROID_GPU_RING_HEADER_BYTES;
constexpr uint32_t kBatchLimit = 512;
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, kTag, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)

std::atomic<bool> gWorkerStarted{false};
std::atomic<bool> gWorkerStop{false};
std::mutex gWorkerMutex;
std::thread gWorker;

uint32_t loadAcquire(uint8_t *address) {
    auto *word = reinterpret_cast<uint32_t *>(address);
    return __atomic_load_n(word, __ATOMIC_ACQUIRE);
}
void storeRelease(uint8_t *address, uint32_t value) {
    auto *word = reinterpret_cast<uint32_t *>(address);
    __atomic_store_n(word, value, __ATOMIC_RELEASE);
}
void addExecuted(uint8_t *address, uint32_t value) {
    auto *word = reinterpret_cast<uint32_t *>(address);
    __atomic_fetch_add(word, value, __ATOMIC_RELAXED);
}

void runRingWorker(uint8_t *memory, uint64_t memorySize, uint64_t ringOffset,
                   uint32_t ringSize) {
    (void)memorySize;
    uint8_t *ring = memory + ringOffset;
    storeRelease(ring + 4, kRingDataOffset);
    storeRelease(ring + 8, 1);
    PGAndroidGpuRingCursor cursor{kRingDataOffset};
    LOGI("Original iOS GPU ring attached: offset=%llu size=%u",
         static_cast<unsigned long long>(ringOffset), ringSize);

    while (!gWorkerStop.load(std::memory_order_acquire)) {
        const uint32_t head = loadAcquire(ring);
        if (head == cursor.tail || !PGAndroidGpuIsReady()) {
            storeRelease(ring + 20, 1);
            if (PGGraphicsWait) {
                (void)PGGraphicsWait(ringOffset, cursor.tail, 100000000ll);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            storeRelease(ring + 20, 0);
            continue;
        }

        const uint32_t previousTail = cursor.tail;
        uint32_t executed = 0;
        const int ok = PGAndroidConsumeGpuRing(
            ring, ringSize, head, &cursor, kBatchLimit, &executed);

        if (executed) addExecuted(ring + 12, executed);
        if (cursor.tail != previousTail || executed)
            storeRelease(ring + 4, cursor.tail);
        if (cursor.tail != previousTail || executed) {
            if (PGGraphicsNotify) (void)PGGraphicsNotify(ringOffset + 4);
        }

        if (!ok) {
            // Never acknowledge a command the Vulkan backend did not execute.
            storeRelease(ring + 8, 2);
            LOGE("GPU command ring paused at opcode packet tail=%u head=%u; "
                 "the command is malformed or not implemented yet",
                 cursor.tail, head);
            break;
        }
    }
    gWorkerStarted.store(false, std::memory_order_release);
}
} // namespace

extern "C" void PGStartGraphics(void *memory, uint64_t memorySize,
                                  uint64_t ringOffset, uint64_t ringSize,
                                  uint64_t debug) {
    (void)debug;
    if (!memory || ringOffset > memorySize || ringSize > memorySize - ringOffset ||
        ringSize < kRingDataOffset + 8 || ringSize > UINT32_MAX ||
        (ringOffset & 3u) || (ringSize & 3u) ||
        (reinterpret_cast<uintptr_t>(memory) & 3u)) {
        LOGE("Refusing invalid engine GPU ring mapping");
        return;
    }
    std::lock_guard<std::mutex> workerLock(gWorkerMutex);
    if (gWorkerStarted.load(std::memory_order_acquire)) {
        LOGE("GPU ring worker already started; duplicate start ignored");
        return;
    }
    if (gWorker.joinable()) gWorker.join();
    gWorkerStop.store(false, std::memory_order_release);
    gWorkerStarted.store(true, std::memory_order_release);
    gWorker = std::thread(runRingWorker, static_cast<uint8_t *>(memory), memorySize,
                          ringOffset, static_cast<uint32_t>(ringSize));
}

extern "C" void PGStopGraphics(void) {
    gWorkerStop.store(true, std::memory_order_release);
    std::lock_guard<std::mutex> workerLock(gWorkerMutex);
    if (gWorker.joinable()) gWorker.join();
    gWorkerStarted.store(false, std::memory_order_release);
}
