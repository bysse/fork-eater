#include "Timeline.h"
#include "imgui/imgui.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

Timeline::Timeline()
    : m_currentTime(0.0f)
    , m_duration(120.0f) // Default 120 seconds
    , m_playbackSpeed(1.0f)
    , m_isPlaying(false)
    , m_isLooping(true) // Default to looping
    , m_wasDragging(false) {
}

Timeline::~Timeline() {
    // Nothing to cleanup
}

void Timeline::update(float deltaTime) {
    if (m_isPlaying) {
        float newTime = m_currentTime + deltaTime * m_playbackSpeed;
        
        if (newTime >= m_duration) {
            if (m_isLooping) {
                // Loop back to start
                newTime = fmod(newTime, m_duration);
                setCurrentTime(newTime);
            } else {
                // Stop at end
                setCurrentTime(m_duration);
                m_isPlaying = false;
                if (onPlayStateChanged) {
                    onPlayStateChanged(m_isPlaying);
                }
            }
        } else {
            setCurrentTime(newTime);
        }
    }
}

void Timeline::reset() {
    setCurrentTime(0.0f);
    m_isPlaying = false;
    if (onReset) {
        onReset();
    }
    if (onPlayStateChanged) {
        onPlayStateChanged(m_isPlaying);
    }
}

void Timeline::render() {
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    
    // Use available height directly without extra padding
    ImGui::BeginChild("Timeline", ImVec2(windowSize.x, windowSize.y), true, ImGuiWindowFlags_NoScrollbar);
    
    // Split into four sections: controls, current time, timeline bar, speed control
    float controlsWidth = 180.0f;  // Reduced since no speed control here
    float currentTimeWidth = 80.0f;
    float speedControlWidth = 100.0f;  // For speed slider
    float timelineBarWidth = std::max(200.0f, windowSize.x - controlsWidth - currentTimeWidth - speedControlWidth - 30.0f);
    
    // Use the full available height minus a small margin
    float childHeight = windowSize.y - 10.0f;
    
    // Playback controls (without speed)
    ImGui::BeginChild("Controls", ImVec2(controlsWidth, childHeight), false);
    renderPlaybackControls();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Current time display
    ImGui::BeginChild("CurrentTime", ImVec2(currentTimeWidth, childHeight), false);
    renderCurrentTime();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Timeline bar
    ImGui::BeginChild("TimelineBar", ImVec2(timelineBarWidth, childHeight), false);
    renderTimelineBar();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Speed control
    ImGui::BeginChild("SpeedControl", ImVec2(speedControlWidth, childHeight), false);
    renderSpeedControl();
    ImGui::EndChild();
    
    ImGui::EndChild();
}

void Timeline::renderPlaybackControls() {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
    
    // Play/Pause button
    const char* playPauseText = m_isPlaying ? "Pause" : "Play";
    ImGui::PushStyleColor(ImGuiCol_Button, m_isPlaying ? ImVec4(0.8f, 0.5f, 0.2f, 1.0f) : ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button(playPauseText, ImVec2(50, BUTTON_SIZE))) {
        handlePlayPause();
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    // Stop button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Stop", ImVec2(40, BUTTON_SIZE))) {
        handleStop();
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    // Loop toggle
    ImGui::PushStyleColor(ImGuiCol_Button, m_isLooping ? ImVec4(0.2f, 0.6f, 0.8f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    if (ImGui::Button("Loop", ImVec2(40, BUTTON_SIZE))) {
        m_isLooping = !m_isLooping;
    }
    ImGui::PopStyleColor();
    
    ImGui::PopStyleVar(2);
}

void Timeline::renderTimelineBar() {
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Ensure minimum height for the timeline bar
    float barHeight = std::max(20.0f, canvas_size.y - 10.0f);
    float barY = canvas_pos.y + (canvas_size.y - barHeight) * 0.5f;
    
    // Timeline background
    ImVec2 bar_start = ImVec2(canvas_pos.x + 10.0f, barY);
    ImVec2 bar_end = ImVec2(canvas_pos.x + canvas_size.x - 10.0f, barY + barHeight);
    float bar_width = bar_end.x - bar_start.x;
    
    draw_list->AddRectFilled(bar_start, bar_end, IM_COL32(40, 40, 40, 255));
    draw_list->AddRect(bar_start, bar_end, IM_COL32(100, 100, 100, 255));
    
    // Progress bar
    float progress = m_duration > 0.0f ? m_currentTime / m_duration : 0.0f;
    if (progress > 0.0f) {
        ImVec2 progress_end = ImVec2(bar_start.x + bar_width * progress, bar_end.y);
        draw_list->AddRectFilled(bar_start, progress_end, IM_COL32(60, 150, 60, 255));
    }
    
    // Time markers (every 10 seconds)
    for (float time = 0.0f; time <= m_duration; time += 10.0f) {
        float marker_progress = time / m_duration;
        float marker_x = bar_start.x + bar_width * marker_progress;
        
        // Draw tick mark
        draw_list->AddLine(
            ImVec2(marker_x, bar_start.y),
            ImVec2(marker_x, bar_start.y + 5.0f),
            IM_COL32(150, 150, 150, 255),
            1.0f
        );
        
        // Draw time label for major markers
        if (fmod(time, 30.0f) < 0.1f) { // Every 30 seconds
            char timeText[16];
            formatTime(time, timeText, sizeof(timeText));
            draw_list->AddText(
                ImVec2(marker_x - 15.0f, bar_start.y - 20.0f),
                IM_COL32(200, 200, 200, 255),
                timeText
            );
        }
    }
    
    // Current time indicator
    float indicator_x = bar_start.x + bar_width * progress;
    draw_list->AddLine(
        ImVec2(indicator_x, bar_start.y - 5.0f),
        ImVec2(indicator_x, bar_end.y + 5.0f),
        IM_COL32(255, 255, 255, 255),
        2.0f
    );
    
    // Handle scrubbing
    ImGui::SetCursorScreenPos(bar_start);
    ImGui::InvisibleButton("TimelineBar", ImVec2(bar_width, barHeight));
    
    if (ImGui::IsItemActive()) {
        if (!m_wasDragging) {
            m_wasDragging = true;
            // Pause playback when starting to drag
            m_isPlaying = false;
            if (onPlayStateChanged) {
                onPlayStateChanged(m_isPlaying);
            }
        }
        
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float normalized_pos = (mouse_pos.x - bar_start.x) / bar_width;
        normalized_pos = std::clamp(normalized_pos, 0.0f, 1.0f);
        setCurrentTime(normalized_pos * m_duration);
    } else if (m_wasDragging) {
        m_wasDragging = false;
    }
}

void Timeline::renderCurrentTime() {
    char currentTimeText[16];
    formatTime(m_currentTime, currentTimeText, sizeof(currentTimeText));
    
    ImGui::Text("Time:");
    ImGui::Text("%s", currentTimeText);
}

void Timeline::renderSpeedControl() {
    ImGui::Text("Speed:");
    ImGui::SetNextItemWidth(80.0f);
    if (ImGui::SliderFloat("##Speed", &m_playbackSpeed, MIN_SPEED, MAX_SPEED, "%.1fx")) {
        m_playbackSpeed = std::clamp(m_playbackSpeed, MIN_SPEED, MAX_SPEED);
    }
}

void Timeline::handlePlayPause() {
    m_isPlaying = !m_isPlaying;
    
    // If we're at the end and not looping, reset to start when playing
    if (m_isPlaying && m_currentTime >= m_duration && !m_isLooping) {
        setCurrentTime(0.0f);
    }
    
    if (onPlayStateChanged) {
        onPlayStateChanged(m_isPlaying);
    }
}

void Timeline::handleStop() {
    m_isPlaying = false;
    setCurrentTime(0.0f);
    
    if (onPlayStateChanged) {
        onPlayStateChanged(m_isPlaying);
    }
}

void Timeline::setCurrentTime(float time) {
    float oldTime = m_currentTime;
    m_currentTime = std::clamp(time, 0.0f, m_duration);
    
    if (oldTime != m_currentTime && onTimeChanged) {
        onTimeChanged(m_currentTime);
    }
}

void Timeline::formatTime(float timeSeconds, char* buffer, size_t bufferSize) {
    int minutes = static_cast<int>(timeSeconds) / 60;
    int seconds = static_cast<int>(timeSeconds) % 60;
    int centiseconds = static_cast<int>((timeSeconds - static_cast<int>(timeSeconds)) * 100);
    
    snprintf(buffer, bufferSize, "%d:%02d.%02d", minutes, seconds, centiseconds);
}

// Keyboard shortcut implementations
void Timeline::togglePlayPause() {
    handlePlayPause();
}

void Timeline::jumpTime(float seconds) {
    float newTime = m_currentTime + seconds;
    setCurrentTime(newTime);
}

void Timeline::jumpToStart() {
    setCurrentTime(0.0f);
}

void Timeline::jumpToEnd() {
    setCurrentTime(m_duration);
}

void Timeline::adjustSpeed(float delta) {
    m_playbackSpeed += delta;
    m_playbackSpeed = std::clamp(m_playbackSpeed, MIN_SPEED, MAX_SPEED);
}
