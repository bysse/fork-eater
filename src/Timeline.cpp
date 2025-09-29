#include "Timeline.h"
#include "imgui/imgui.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include "Settings.h"

float Timeline::defaultHeightDIP() { return TIMELINE_HEIGHT; }

Timeline::Timeline()
    : m_currentTime(0.0f)
    , m_duration(120.0f) // Default 120 seconds
    , m_playbackSpeed(1.0f)
    , m_isPlaying(false)
    , m_isLooping(true) // Default to looping
    , m_wasDragging(false)
    , m_useBPM(true)
    , m_bpm(120.0f)
    , m_beatsPerBar(4)
    , m_timeSliceDuration(0.25f) // Each slice represents 250ms
{
    // Initialize fpsData with a size based on duration and timeSliceDuration
    // and fill with -1.0f to indicate no data recorded yet
    m_fpsData.resize(static_cast<size_t>(m_duration / m_timeSliceDuration), -1.0f);
}

void Timeline::addFPS(float time, float fps) {
    if (time < 0.0f || time >= m_duration) {
        return; // Time is out of bounds
    }
    size_t index = static_cast<size_t>(time / m_timeSliceDuration);
    if (index < m_fpsData.size()) {
        m_fpsData[index] = fps;
    }
}

void Timeline::clearFPSData() {
    std::fill(m_fpsData.begin(), m_fpsData.end(), -1.0f);
}

void Timeline::renderTimelineBar(float renderScaleFactor) {
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    float uiScale = Settings::getInstance().getUIScaleFactor();
    // Ensure minimum height for the timeline bar
    float barHeight = std::max(20.0f * uiScale, canvas_size.y - 10.0f * uiScale);
    float barY = canvas_pos.y + (canvas_size.y - barHeight) * 0.5f;
    
    // Timeline background
    ImVec2 bar_start = ImVec2(canvas_pos.x + 10.0f * uiScale, barY);
    ImVec2 bar_end = ImVec2(canvas_pos.x + canvas_size.x - 10.0f * uiScale, barY + barHeight);
    float bar_width = bar_end.x - bar_start.x;
    
    draw_list->AddRectFilled(bar_start, bar_end, IM_COL32(40, 40, 40, 255));

    // FPS graph
    float fpsGraphHeight = barHeight * 0.25f;
    ImVec2 fpsGraphStart = ImVec2(bar_start.x, bar_end.y - fpsGraphHeight);
    
    float segmentWidth = bar_width / m_fpsData.size();

    for (size_t i = 0; i < m_fpsData.size(); ++i) {
        float fps = m_fpsData[i];
        ImColor color;

        if (fps < 0.0f) { // Not recorded yet
            color = ImColor(128, 128, 128); // Grey
        } else {
            if (fps < Settings::getInstance().getLowFPSThreshold()) {
                color = ImColor(255, 0, 0); // Red
            } else if (fps > Settings::getInstance().getHighFPSThreshold()) {
                color = ImColor(0, 255, 0); // Green
            } else {
                // Interpolate between red and green for intermediate values
                float t = (fps - Settings::getInstance().getLowFPSThreshold()) /
                          (Settings::getInstance().getHighFPSThreshold() - Settings::getInstance().getLowFPSThreshold());
                t = std::clamp(t, 0.0f, 1.0f);
                color = ImColor(static_cast<int>(255 * (1 - t)), static_cast<int>(255 * t), 0); // Red to Green
            }
        }
        
        float x = fpsGraphStart.x + (float)i * segmentWidth;
        draw_list->AddRectFilled(ImVec2(x, fpsGraphStart.y), ImVec2(x + segmentWidth, bar_end.y), color);
    }

    // Render scale factor overlay
    if (renderScaleFactor < 1.0f) {
        ImColor overlayColor;
        if (renderScaleFactor == 0.5f) {
            overlayColor = ImColor(255, 165, 0, 100); // Orange with transparency
        } else if (renderScaleFactor == 0.25f) {
            overlayColor = ImColor(255, 0, 0, 100); // Red with transparency
        }
        draw_list->AddRectFilled(bar_start, bar_end, overlayColor);
    }

    draw_list->AddRect(bar_start, bar_end, IM_COL32(100, 100, 100, 255));
    
    // Progress bar
    float progress = m_duration > 0.0f ? m_currentTime / m_duration : 0.0f;
    if (progress > 0.0f) {
        ImVec2 progress_end = ImVec2(bar_start.x + bar_width * progress, bar_end.y-fpsGraphHeight);
        draw_list->AddRectFilled(bar_start, progress_end, IM_COL32(60, 150, 60, 255));
    }
    
    // Time markers
    if (m_useBPM) {
        // Draw beat markers
        float beatsPerSecond = getBeatsPerSecond();
        float totalBeats = m_duration * beatsPerSecond;
        
        for (float beat = 0.0f; beat <= totalBeats; beat += 1.0f) {
            float time = beat / beatsPerSecond;
            float marker_progress = time / m_duration;
            float marker_x = bar_start.x + bar_width * marker_progress;
            
            bool isMajorBeat = (fmod(beat, m_beatsPerBar) < 0.1f);
            
            // Draw tick mark
            draw_list->AddLine(
                ImVec2(marker_x, bar_start.y),
                ImVec2(marker_x, bar_start.y + (isMajorBeat ? 8.0f * uiScale : 5.0f * uiScale)),
                isMajorBeat ? IM_COL32(200, 200, 200, 255) : IM_COL32(150, 150, 150, 255),
                isMajorBeat ? 2.0f * uiScale : 1.0f * uiScale
            );
            
            // Draw bar numbers for major beats
            if (isMajorBeat && fmod(beat, m_beatsPerBar * 4) < 0.1f) { // Every 4 bars
                int bar = (int)(beat / m_beatsPerBar) + 1;
                std::stringstream ss;
                ss << bar;
                draw_list->AddText(
                    ImVec2(marker_x - 8.0f * uiScale, bar_start.y - 20.0f * uiScale),
                    IM_COL32(200, 200, 200, 255),
                    ss.str().c_str()
                );
            }
        }
    } else {
        // Draw time markers (every 10 seconds)
        for (float time = 0.0f; time <= m_duration; time += 10.0f) {
            float marker_progress = time / m_duration;
            float marker_x = bar_start.x + bar_width * marker_progress;
            
            // Draw tick mark
            draw_list->AddLine(
                ImVec2(marker_x, bar_start.y),
                ImVec2(marker_x, bar_start.y + 5.0f * uiScale),
                IM_COL32(150, 150, 150, 255),
                1.0f * uiScale
            );
            
            // Draw time label for major markers
            if (fmod(time, 30.0f) < 0.1f) { // Every 30 seconds
                char timeText[16];
                formatTime(time, timeText, sizeof(timeText));
                draw_list->AddText(
                    ImVec2(marker_x - 15.0f * uiScale, bar_start.y - 20.0f * uiScale),
                    IM_COL32(200, 200, 200, 255),
                    timeText
                );
            }
        }
    }
    
    // Current time indicator
    float indicator_x = bar_start.x + bar_width * progress;
    draw_list->AddLine(
        ImVec2(indicator_x, bar_start.y - 5.0f * uiScale),
        ImVec2(indicator_x, bar_end.y + 5.0f * uiScale),
        IM_COL32(255, 255, 255, 255),
        2.0f * uiScale
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
    clearFPSData(); // Clear FPS data on reset
    if (onReset) {
        onReset();
    }
    if (onPlayStateChanged) {
        onPlayStateChanged(m_isPlaying);
    }
}

void Timeline::render(float renderScaleFactor) {
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    
    // Use available height directly without extra padding, ensure no scrollbars
    ImGui::BeginChild("Timeline", ImVec2(windowSize.x, windowSize.y), true, 
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    // Get actual content region after the child window border
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    
    // Split into four sections: controls, current time, timeline bar, speed control
    float uiScale = Settings::getInstance().getUIScaleFactor();
    float controlsWidth = 180.0f * uiScale;
    float currentTimeWidth = 80.0f * uiScale;
    float speedControlWidth = 100.0f * uiScale;
    float spacing = ImGui::GetStyle().ItemSpacing.x * 3; // 3 SameLine() calls (already scaled via style)
    float timelineBarWidth = std::max(200.0f * uiScale, contentSize.x - controlsWidth - currentTimeWidth - speedControlWidth - spacing);
    
    // Use the allocated content height to avoid requesting more than the pane provides
    float childHeight = contentSize.y;
    
    // Push style to prevent any potential scrollbars in child windows and disable focus
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
    
    // Define common flags to disable navigation and focus
    ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | 
                                  ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs;
    
    // Playback controls (without speed)
    ImGui::BeginChild("Controls", ImVec2(controlsWidth, childHeight), false, childFlags);
    renderPlaybackControls();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Current time display
    ImGui::BeginChild("CurrentTime", ImVec2(currentTimeWidth, childHeight), false, childFlags);
    renderCurrentTime();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Timeline bar
    ImGui::BeginChild("TimelineBar", ImVec2(timelineBarWidth, childHeight), false, childFlags);
    renderTimelineBar(renderScaleFactor);
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Speed control
    ImGui::BeginChild("SpeedControl", ImVec2(speedControlWidth, childHeight), false, childFlags);
    renderSpeedControl();
    ImGui::EndChild();
    
    ImGui::PopStyleVar(); // ScrollbarSize
    ImGui::EndChild();
}

void Timeline::renderPlaybackControls() {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

    float uiScale = Settings::getInstance().getUIScaleFactor();
    
    // Play/Pause button
    const char* playPauseText = m_isPlaying ? "Pause" : "Play";
    ImGui::PushStyleColor(ImGuiCol_Button, m_isPlaying ? ImVec4(0.8f, 0.5f, 0.2f, 1.0f) : ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button(playPauseText, ImVec2(50.0f * uiScale, 0.0f))) {
        handlePlayPause();
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    // Stop button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Stop", ImVec2(40.0f * uiScale, 0.0f))) {
        handleStop();
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    // Loop toggle
    ImGui::PushStyleColor(ImGuiCol_Button, m_isLooping ? ImVec4(0.2f, 0.6f, 0.8f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    if (ImGui::Button("Loop", ImVec2(40.0f * uiScale, 0.0f))) {
        m_isLooping = !m_isLooping;
    }
    ImGui::PopStyleColor();
    
    ImGui::PopStyleVar(2);
}



void Timeline::renderCurrentTime() {
    char currentTimeText[16];
    
    if (m_useBPM) {
        formatTimeBPM(m_currentTime, currentTimeText, sizeof(currentTimeText));
        ImGui::Text("Beat:");
    } else {
        formatTime(m_currentTime, currentTimeText, sizeof(currentTimeText));
        ImGui::Text("Time:");
    }
    
    ImGui::Text("%s", currentTimeText);
}

void Timeline::renderSpeedControl() {
    float uiScale = Settings::getInstance().getUIScaleFactor();
    ImGui::Text("Speed:");
    ImGui::SetNextItemWidth(80.0f * uiScale);
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
    
    std::stringstream ss;
    ss << minutes << ":" << (seconds < 10 ? "0" : "") << seconds << "." << (centiseconds < 10 ? "0" : "") << centiseconds;
    strncpy(buffer, ss.str().c_str(), bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
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

// Direct playback control methods
void Timeline::play() {
    if (!m_isPlaying) {
        m_isPlaying = true;
        
        // If we're at the end and not looping, reset to start when playing
        if (m_currentTime >= m_duration && !m_isLooping) {
            setCurrentTime(0.0f);
        }
        
        if (onPlayStateChanged) {
            onPlayStateChanged(m_isPlaying);
        }
    }
}

void Timeline::pause() {
    if (m_isPlaying) {
        m_isPlaying = false;
        
        if (onPlayStateChanged) {
            onPlayStateChanged(m_isPlaying);
        }
    }
}

void Timeline::stop() {
    handleStop();
}

// BPM support methods
void Timeline::setBPM(float bpm, int beatsPerBar) {
    m_bpm = bpm;
    m_beatsPerBar = beatsPerBar;
    m_useBPM = (bpm > 0);
}

float Timeline::getBeatsPerSecond() const {
    return m_bpm / 60.0f;
}

float Timeline::getBarsPerSecond() const {
    return getBeatsPerSecond() / m_beatsPerBar;
}

float Timeline::secondsToBeats(float seconds) const {
    return seconds * getBeatsPerSecond();
}

float Timeline::beatsToSeconds(float beats) const {
    return beats / getBeatsPerSecond();
}

void Timeline::formatTimeBPM(float timeSeconds, char* buffer, size_t bufferSize) {
    float beats = secondsToBeats(timeSeconds);
    int bar = static_cast<int>(beats / m_beatsPerBar) + 1;
    int beat = static_cast<int>(fmod(beats, m_beatsPerBar)) + 1;
    float subBeat = fmod(beats, 1.0f);
    
    std::stringstream ss;
    ss << bar << ":" << beat << "." << static_cast<int>(subBeat * 100);
    strncpy(buffer, ss.str().c_str(), bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
}
