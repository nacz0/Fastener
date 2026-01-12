#include "fastener/core/profiler.h"
#include <numeric>

namespace fst {

Profiler::Profiler() {
    for (int i = 0; i < HISTORY_SIZE; ++i) m_frameHistory[i] = 0.0f;
}

Profiler::~Profiler() {}

void Profiler::beginFrame() {
    m_frameStartTime = std::chrono::steady_clock::now();
    m_currentFrameEntries.clear();
    m_sectionStack.clear();
}

void Profiler::endFrame() {
    auto now = std::chrono::steady_clock::now();
    float frameTime = std::chrono::duration<float, std::milli>(now - m_frameStartTime).count();
    
    m_frameHistory[m_historyOffset] = frameTime;
    m_historyOffset = (m_historyOffset + 1) % HISTORY_SIZE;
    
    m_lastFrameEntries = std::move(m_currentFrameEntries);
}

void Profiler::beginSection(const std::string& name) {
    Section s;
    s.name = name;
    s.startTime = std::chrono::steady_clock::now();
    m_sectionStack.push_back(s);
}

void Profiler::endSection() {
    if (m_sectionStack.empty()) return;

    auto now = std::chrono::steady_clock::now();
    const auto& s = m_sectionStack.back();
    
    ProfileEntry entry;
    entry.name = s.name;
    entry.startTime = std::chrono::duration<float, std::milli>(s.startTime - m_frameStartTime).count();
    entry.duration = std::chrono::duration<float, std::milli>(now - s.startTime).count();
    entry.depth = static_cast<int>(m_sectionStack.size()) - 1;
    
    m_currentFrameEntries.push_back(entry);
    m_sectionStack.pop_back();
}

void Profiler::getFrameHistory(float* outHistory, int count) const {
    for (int i = 0; i < count; ++i) {
        int idx = (m_historyOffset - count + i + HISTORY_SIZE) % HISTORY_SIZE;
        outHistory[i] = m_frameHistory[idx];
    }
}

float Profiler::getAverageFrameTime() const {
    float sum = std::accumulate(std::begin(m_frameHistory), std::end(m_frameHistory), 0.0f);
    return sum / HISTORY_SIZE;
}

} // namespace fst
