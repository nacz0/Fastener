#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <memory>

namespace fst {

struct ProfileEntry {
    std::string name;
    float startTime;
    float duration;
    int depth;
};

class Profiler {
public:
    Profiler();
    ~Profiler();

    void beginFrame();
    void endFrame();

    void beginSection(const std::string& name);
    void endSection();

    const std::vector<ProfileEntry>& getLastFrameEntries() const { return m_lastFrameEntries; }
    void getFrameHistory(float* outHistory, int count) const;
    float getAverageFrameTime() const;

private:
    struct Section {
        std::string name;
        std::chrono::steady_clock::time_point startTime;
    };

    std::vector<Section> m_sectionStack;
    std::vector<ProfileEntry> m_currentFrameEntries;
    std::vector<ProfileEntry> m_lastFrameEntries;

    static constexpr int HISTORY_SIZE = 128;
    float m_frameHistory[HISTORY_SIZE];
    int m_historyOffset = 0;
    
    std::chrono::steady_clock::time_point m_frameStartTime;
};

class ProfileScope {
public:
    ProfileScope(Profiler& profiler, const std::string& name) : m_profiler(profiler) {
        m_profiler.beginSection(name);
    }
    ~ProfileScope() {
        m_profiler.endSection();
    }
private:
    Profiler& m_profiler;
};

} // namespace fst
