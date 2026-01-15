#include "limbo/debug/GPUTimer.hpp"
#include "limbo/debug/Log.hpp"

#include <glad/gl.h>

#include <cstring>

namespace limbo {

GPUTimer::~GPUTimer() {
    if (m_initialized) {
        shutdown();
    }
}

void GPUTimer::init() {
    if (m_initialized) {
        return;
    }

    if (!isSupported()) {
        LIMBO_LOG_RENDER_WARN("GPU timer queries not supported on this hardware");
        return;
    }

    // Create query objects for all timers
    for (u32 i = 0; i < MaxTimers; ++i) {
        glGenQueries(BufferCount, m_timers[i].queryIds);
    }

    m_initialized = true;
    m_currentBuffer = 0;
    m_timerCount = 0;
    m_totalTimeMs = 0.0;

    LIMBO_LOG_RENDER_INFO("GPU timer initialized");
}

void GPUTimer::shutdown() {
    if (!m_initialized) {
        return;
    }

    // Delete query objects
    for (u32 i = 0; i < MaxTimers; ++i) {
        glDeleteQueries(BufferCount, m_timers[i].queryIds);
        m_timers[i].queryIds[0] = 0;
        m_timers[i].queryIds[1] = 0;
        m_timers[i].active = false;
    }

    m_initialized = false;
    m_timerCount = 0;
}

bool GPUTimer::isSupported() {
    // GL_TIME_ELAPSED is core since OpenGL 3.3
    // Check for the extension or version
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    return (major > 3) || (major == 3 && minor >= 3);
}

void GPUTimer::beginFrame() {
    if (!m_initialized) {
        return;
    }

    // Collect results from the previous frame (other buffer)
    collectResults();

    // Swap buffers
    m_currentBuffer = (m_currentBuffer + 1) % BufferCount;

    // Reset timer count for this frame
    m_timerCount = 0;
    m_frameActive = true;
}

void GPUTimer::endFrame() {
    if (!m_initialized || !m_frameActive) {
        return;
    }

    // Make sure any active timer is ended
    if (m_activeTimer < MaxTimers) {
        end();
    }

    m_frameActive = false;
}

void GPUTimer::begin(const char* name) {
    if (!m_initialized || !m_frameActive) {
        return;
    }

    // End any currently active timer
    if (m_activeTimer < MaxTimers) {
        end();
    }

    if (m_timerCount >= MaxTimers) {
        LIMBO_LOG_RENDER_WARN("GPU timer limit reached ({})", MaxTimers);
        return;
    }

    // Set up timer data
    TimerData& timer = m_timers[m_timerCount];
    std::strncpy(timer.name, name, sizeof(timer.name) - 1);
    timer.name[sizeof(timer.name) - 1] = '\0';
    timer.active = true;

    // Begin query
    glBeginQuery(GL_TIME_ELAPSED, timer.queryIds[m_currentBuffer]);

    m_activeTimer = m_timerCount;
    ++m_timerCount;
}

void GPUTimer::end() {
    if (!m_initialized || m_activeTimer >= MaxTimers) {
        return;
    }

    glEndQuery(GL_TIME_ELAPSED);
    m_activeTimer = MaxTimers;
}

void GPUTimer::collectResults() {
    if (!m_initialized) {
        return;
    }

    // Read from the other buffer (previous frame)
    u32 const readBuffer = (m_currentBuffer + 1) % BufferCount;

    m_totalTimeMs = 0.0;

    for (u32 i = 0; i < MaxTimers; ++i) {
        TimerData& timer = m_timers[i];
        if (!timer.active) {
            continue;
        }

        GLuint64 timeNs = 0;

        // Check if result is available
        GLint available = 0;
        glGetQueryObjectiv(timer.queryIds[readBuffer], GL_QUERY_RESULT_AVAILABLE, &available);

        if (available != 0) {
            glGetQueryObjectui64v(timer.queryIds[readBuffer], GL_QUERY_RESULT, &timeNs);
            timer.timeMs = static_cast<f64>(timeNs) / 1000000.0;  // ns to ms
            m_totalTimeMs += timer.timeMs;
        }
    }
}

f64 GPUTimer::getTimeMs(const char* name) const {
    for (u32 i = 0; i < MaxTimers; ++i) {
        if (m_timers[i].active && std::strcmp(m_timers[i].name, name) == 0) {
            return m_timers[i].timeMs;
        }
    }
    return 0.0;
}

const char* GPUTimer::getTimerName(u32 index) const {
    if (index < MaxTimers && m_timers[index].active) {
        return m_timers[index].name;
    }
    return nullptr;
}

f64 GPUTimer::getTimerTimeMs(u32 index) const {
    if (index < MaxTimers && m_timers[index].active) {
        return m_timers[index].timeMs;
    }
    return 0.0;
}

}  // namespace limbo
