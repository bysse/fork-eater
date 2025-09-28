#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <sys/inotify.h>

class FileWatcher {
public:
    using FileChangedCallback = std::function<void(const std::string&)>;
    
    FileWatcher();
    ~FileWatcher();
    
    // Start watching for file changes
    bool start();
    
    // Stop watching for file changes
    void stop();
    
    // Add file to watch list
    bool addWatch(const std::string& filePath, FileChangedCallback callback);
    
    // Remove file from watch list
    void removeWatch(const std::string& filePath);

    // Clear all watches
    void clearWatches();
    
    // Check if currently watching
    bool isWatching() const;

private:
    struct WatchInfo {
        int watchDescriptor;
        std::string filePath;
        FileChangedCallback callback;
    };
    
    int m_inotifyFd;
    std::atomic<bool> m_running;
    std::thread m_watchThread;
    std::unordered_map<std::string, WatchInfo> m_watches;
    std::unordered_map<int, std::string> m_watchDescriptorToPath;
    
    // Thread function for watching files
    void watchThread();
    
    // Process inotify events
    void processEvents(char* buffer, ssize_t length);
};