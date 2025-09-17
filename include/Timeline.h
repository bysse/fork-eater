#pragma once

#include <functional>

class Timeline {
public:
    Timeline();
    ~Timeline();
    
    // Render the timeline UI
    void render();
    
    // Get current timeline time
    float getCurrentTime() const { return m_currentTime; }
    
    // Get timeline duration
    float getDuration() const { return m_duration; }
    
    // Set timeline duration
    void setDuration(float duration) { m_duration = duration; }
    
    // BPM support
    void setBPM(float bpm, int beatsPerBar = 4);
    float getBPM() const { return m_bpm; }
    int getBeatsPerBar() const { return m_beatsPerBar; }
    bool isBPMMode() const { return m_useBPM; }
    
    // Playback state
    bool isPlaying() const { return m_isPlaying; }
    bool isLooping() const { return m_isLooping; }
    float getPlaybackSpeed() const { return m_playbackSpeed; }
    
    // Update timeline (call every frame)
    void update(float deltaTime);
    
    // Reset timeline to start
    void reset();
    
    // Keyboard shortcuts
    void togglePlayPause();
    void jumpTime(float seconds);
    void jumpToStart();
    void jumpToEnd();
    void adjustSpeed(float delta);
    
    // Direct playback control
    void play();
    void pause();
    void stop();
    
    // Callbacks for timeline events
    std::function<void(float)> onTimeChanged;
    std::function<void(bool)> onPlayStateChanged;
    std::function<void()> onReset;

private:
    float m_currentTime;
    float m_duration;
    float m_playbackSpeed;
    bool m_isPlaying;
    bool m_isLooping;
    bool m_wasDragging;
    
    // BPM support
    bool m_useBPM;
    float m_bpm;
    int m_beatsPerBar;
    
    // UI constants
    static constexpr float TIMELINE_HEIGHT = 65.0f;  // Increased by 5px
    static constexpr float BUTTON_SIZE = 22.0f;      // Slightly smaller buttons
    static constexpr float MIN_SPEED = 0.1f;
    static constexpr float MAX_SPEED = 4.0f;
    
    // Private methods
    void renderPlaybackControls();
    void renderTimelineBar();
    void renderCurrentTime();
    void renderSpeedControl();
    void handlePlayPause();
    void handleStop();
    void setCurrentTime(float time);
    void formatTime(float timeSeconds, char* buffer, size_t bufferSize);
    
    // BPM helper methods
    float getBeatsPerSecond() const;
    float getBarsPerSecond() const;
    float secondsToBeats(float seconds) const;
    float beatsToSeconds(float beats) const;
    void formatTimeBPM(float timeSeconds, char* buffer, size_t bufferSize);
};