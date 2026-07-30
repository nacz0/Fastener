#include <gtest/gtest.h>
#include <fastener/core/log.h>
#include <atomic>
#include <thread>
#include <vector>

using namespace fst;

namespace {

class LogStateGuard {
public:
    LogStateGuard() : m_level(getMinLogLevel()) {}
    ~LogStateGuard() {
        setLogHandler(nullptr);
        setMinLogLevel(m_level);
    }

private:
    LogLevel m_level;
};

} // namespace

TEST(LogTest, HandlerCanReconfigureLoggingWithoutDeadlock) {
    LogStateGuard guard;
    std::atomic<int> calls{0};
    setMinLogLevel(LogLevel::Debug);
    setLogHandler([&](LogLevel, const char*, int, const char*) {
        ++calls;
        setLogHandler(nullptr);
    });

    logMessage(LogLevel::Info, __FILE__, __LINE__, "first");
    EXPECT_EQ(calls.load(), 1);
}

TEST(LogTest, ConcurrentMessagesReachTheInstalledHandler) {
    LogStateGuard guard;
    std::atomic<int> calls{0};
    setMinLogLevel(LogLevel::Debug);
    setLogHandler([&](LogLevel, const char*, int, const char*) {
        calls.fetch_add(1, std::memory_order_relaxed);
    });

    constexpr int threadCount = 4;
    constexpr int messagesPerThread = 250;
    std::vector<std::thread> threads;
    for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        threads.emplace_back([&] {
            for (int messageIndex = 0;
                 messageIndex < messagesPerThread;
                 ++messageIndex) {
                logMessage(
                    LogLevel::Info,
                    __FILE__,
                    __LINE__,
                    "concurrent");
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(calls.load(), threadCount * messagesPerThread);
}
