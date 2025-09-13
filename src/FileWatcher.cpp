#include "FileWatcher.h"
#include <unistd.h>
#include <iostream>
#include <cstring>

FileWatcher::FileWatcher() : m_inotifyFd(-1), m_running(false) {}

FileWatcher::~FileWatcher() {
    stop();
}

bool FileWatcher::start() {
    if (m_running) {
        return true;
    }
    
    m_inotifyFd = inotify_init();
    if (m_inotifyFd == -1) {
        std::cerr << "Failed to initialize inotify" << std::endl;
        return false;
    }
    
    m_running = true;
    m_watchThread = std::thread(&FileWatcher::watchThread, this);
    
    return true;
}

void FileWatcher::stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    // Close inotify file descriptor to wake up the watch thread
    if (m_inotifyFd != -1) {
        close(m_inotifyFd);
        m_inotifyFd = -1;
    }
    
    if (m_watchThread.joinable()) {
        m_watchThread.join();
    }
    
    m_watches.clear();
    m_watchDescriptorToPath.clear();
}

bool FileWatcher::addWatch(const std::string& filePath, FileChangedCallback callback) {
    if (m_inotifyFd == -1) {
        std::cerr << "FileWatcher not started" << std::endl;
        return false;
    }
    
    // Remove existing watch if it exists
    removeWatch(filePath);
    
    int wd = inotify_add_watch(m_inotifyFd, filePath.c_str(), IN_MODIFY | IN_CLOSE_WRITE);
    if (wd == -1) {
        std::cerr << "Failed to add watch for: " << filePath << std::endl;
        return false;
    }
    
    WatchInfo info;
    info.watchDescriptor = wd;
    info.filePath = filePath;
    info.callback = callback;
    
    m_watches[filePath] = info;
    m_watchDescriptorToPath[wd] = filePath;
    
    return true;
}

void FileWatcher::removeWatch(const std::string& filePath) {
    auto it = m_watches.find(filePath);
    if (it == m_watches.end()) {
        return;
    }
    
    if (m_inotifyFd != -1) {
        inotify_rm_watch(m_inotifyFd, it->second.watchDescriptor);
    }
    
    m_watchDescriptorToPath.erase(it->second.watchDescriptor);
    m_watches.erase(it);
}

bool FileWatcher::isWatching() const {
    return m_running;
}

void FileWatcher::watchThread() {
    const size_t EVENT_SIZE = sizeof(struct inotify_event);
    const size_t BUF_LEN = 1024 * (EVENT_SIZE + 16);
    char buffer[BUF_LEN];
    
    while (m_running) {
        ssize_t length = read(m_inotifyFd, buffer, BUF_LEN);
        
        if (length < 0) {
            if (m_running) {
                std::cerr << "Error reading inotify events" << std::endl;
            }
            break;
        }
        
        processEvents(buffer, length);
    }
}

void FileWatcher::processEvents(char* buffer, ssize_t length) {
    ssize_t i = 0;
    
    while (i < length) {
        struct inotify_event* event = (struct inotify_event*)&buffer[i];
        
        if (event->mask & (IN_MODIFY | IN_CLOSE_WRITE)) {
            auto it = m_watchDescriptorToPath.find(event->wd);
            if (it != m_watchDescriptorToPath.end()) {
                const std::string& filePath = it->second;
                auto watchIt = m_watches.find(filePath);
                if (watchIt != m_watches.end() && watchIt->second.callback) {
                    watchIt->second.callback(filePath);
                }
            }
        }
        
        i += EVENT_SIZE + event->len;
    }
}