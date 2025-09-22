#include <GLFW/glfw3.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <iostream>
#include <memory>
#include <chrono>
#include <cstdlib>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "ShaderManager.h"
#include "FileWatcher.h"
#include "ShaderEditor.h"
#include "ShaderProject.h"
#include "ShaderTemplates.h"
#include "Logger.h"
#include <filesystem>

// Constants
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* WINDOW_TITLE = "Fork Eater - Shader Editor";

class Application {
public:
    Application() : m_window(nullptr), m_running(false), m_testMode(false), m_testExitCode(0), m_testStartTime(), m_imguiInitialized(false) {}
    
    ~Application() {
        cleanup();
    }
    
    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            LOG_ERROR("Failed to initialize GLFW");
            return false;
        }
        
        // Set OpenGL version and profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        
        // Create window
        m_window = glfwCreateWindow(
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            WINDOW_TITLE,
            nullptr,
            nullptr
        );
        
        if (!m_window) {
            LOG_ERROR("Failed to create GLFW window");
            glfwTerminate();
            return false;
        }
        
        // Make OpenGL context current
        glfwMakeContextCurrent(m_window);
        
        // Set up callbacks
        glfwSetWindowUserPointer(m_window, this);
        glfwSetKeyCallback(m_window, keyCallback);
        glfwSetWindowCloseCallback(m_window, windowCloseCallback);
        
        // Enable vsync
        glfwSwapInterval(1);
        
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        // Disable navigation to prevent focus jumping and pane selection
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
        
        // Setup ImGui style
        ImGui::StyleColorsDark();
        
        // Setup platform/renderer backends
        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        m_imguiInitialized = true;
        
        // Initialize components
        m_shaderManager = std::make_shared<ShaderManager>();
        m_fileWatcher = std::make_shared<FileWatcher>();
        m_shaderEditor = std::make_unique<ShaderEditor>(m_shaderManager, m_fileWatcher);
        
        if (!m_shaderEditor->initialize()) {
            LOG_ERROR("Failed to initialize shader editor");
            return false;
        }
        
        // Load shader project if specified
        if (!m_shaderProjectPath.empty()) {
            m_shaderEditor->openProject(m_shaderProjectPath);
        }
        
        if (!m_fileWatcher->start()) {
            LOG_ERROR("Failed to start file watcher");
            return false;
        }
        
        // Set up file watching for the loaded project (now that FileWatcher is started)
        if (!m_shaderProjectPath.empty()) {
            m_shaderEditor->setupFileWatching();
        }
        
        LOG_SUCCESS("Fork Eater initialized successfully!");
        LOG_INFO("OpenGL Version: {}", (const char*)glGetString(GL_VERSION));
        LOG_INFO("GLSL Version: {}", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
        
        return true;
    }
    
    void run() {
        m_running = true;
        
        if (m_testMode) {
            m_testStartTime = std::chrono::steady_clock::now();
        }
        
        while (m_running && !glfwWindowShouldClose(m_window)) {
            // Test mode timeout (5 seconds max)
            if (m_testMode) {
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_testStartTime).count();
                if (duration > 5) {
                    LOG_WARN("Test mode: timeout reached, forcing exit");
                    m_running = false;
                    break;
                }
            }
            // Poll events
            glfwPollEvents();
            
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Render the shader editor
            m_shaderEditor->render();
            
            // Exit handling now uses immediate std::exit() to avoid cleanup hanging
            
            // Rendering
            ImGui::Render();
            
            int display_w, display_h;
            glfwGetFramebufferSize(m_window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(m_window);
            
            // Test mode: exit after first render loop
            if (m_testMode) {
                LOG_SUCCESS("Test mode: completed one render loop successfully");
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
                m_running = false;
            }
        }
    }
    
    void setTestMode(bool enabled, int exitCode = 0) {
        m_testMode = enabled;
        m_testExitCode = exitCode;
    }
    
    int getTestExitCode() const {
        return m_testExitCode;
    }
    
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
        if (app) {
            app->handleKey(key, scancode, action, mods);
        }
    }
    
    static void windowCloseCallback(GLFWwindow* window) {
        Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
        if (app) {
            LOG_INFO("Window close requested");
            std::exit(0);  // Immediate exit to avoid cleanup hanging
        }
    }
    
    void handleKey(int key, int scancode, int action, int mods) {
        // Let ShaderEditor handle shortcuts first
        if (m_shaderEditor && m_shaderEditor->handleKeyPress(key, scancode, action, mods)) {
            return; // Key was handled by shortcuts
        }
        
        // Handle application-level keys
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_ESCAPE) {
                LOG_INFO("ESC pressed - exiting");
                std::exit(0);  // Immediate exit to avoid cleanup hanging
            }
        }
    }

private:
    GLFWwindow* m_window;
    bool m_running;
    bool m_testMode;
    int m_testExitCode;
    std::chrono::steady_clock::time_point m_testStartTime;
    bool m_imguiInitialized;
    
    std::shared_ptr<ShaderManager> m_shaderManager;
    std::shared_ptr<FileWatcher> m_fileWatcher;
    std::unique_ptr<ShaderEditor> m_shaderEditor;
    
    // Shader project path
    std::string m_shaderProjectPath;
    
public:
    void setShaderProjectPath(const std::string& path) {
        m_shaderProjectPath = path;
    }
    
    const std::string& getShaderProjectPath() const {
        return m_shaderProjectPath;
    }
    
private:
    
    void cleanup() {
        if (m_testMode) {
            // In test mode, skip all cleanup to avoid hanging and exit immediately
            std::exit(m_testExitCode);
        }
        
        // For regular mode, also use immediate exit to avoid ImGui cleanup hanging
        // This is a workaround for ImGui cleanup issues in certain environments
        LOG_INFO("Exiting Fork Eater...");
        std::exit(0);
        
        if (m_shaderEditor) {
            m_shaderEditor.reset();
        }
        
        if (m_fileWatcher) {
            m_fileWatcher->stop();
            m_fileWatcher.reset();
        }
        
        m_shaderManager.reset();
        
        // Cleanup ImGui (only if it was initialized)
        if (m_imguiInitialized) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }
        
        // Cleanup GLFW
        if (m_window) {
            glfwDestroyWindow(m_window);
        }
        
        glfwTerminate();
    }
};

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] [shader_directory]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --new [path] [-t template]  Create new shader project" << std::endl;
    std::cout << "  --templates                 List available shader templates" << std::endl;
    std::cout << "  --test [exit_code]          Run in test mode (exit after one render loop)" << std::endl;
    std::cout << "  --debug, -d                 Enable debug output with colors" << std::endl;
    std::cout << "  --help, -h                  Show this help message" << std::endl;
    std::cout << "  shader_directory            Path to shader project directory containing " << SHADER_PROJECT_MANIFEST_FILENAME << " manifest" << std::endl;
    std::cout << std::endl;
    std::cout << "Fork Eater - Real-time GLSL shader editor with hot reloading" << std::endl;
    std::cout << std::endl;
    std::cout << "If no directory is specified, uses current directory." << std::endl;
    std::cout << "Program will exit if no " << SHADER_PROJECT_MANIFEST_FILENAME << " manifest is found (except in test mode)." << std::endl;
}

void printTemplates() {
    std::cout << "Available shader templates:" << std::endl;
    const auto& templateManager = ShaderTemplateManager::getInstance();
    const auto& templateNames = templateManager.getTemplateNames();
    
    for (const auto& name : templateNames) {
        const auto* templ = templateManager.getTemplate(name);
        if (templ) {
            std::cout << "  " << name << " - " << templ->description << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    Application app;
    bool testMode = false;
    int testExitCode = 0;
    bool newProject = false;
    bool debugMode = false;
    std::string shaderProjectPath;
    std::string templateName = "basic";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--debug" || arg == "-d") {
            debugMode = true;
            // Note: Logger not initialized yet, so using std::cout here
            std::cout << "Debug mode enabled" << std::endl;
        }
        else if (arg == "--test") {
            testMode = true;
            // Check if next argument is an exit code
            if (i + 1 < argc) {
                try {
                    testExitCode = std::stoi(argv[i + 1]);
                    i++; // Skip the next argument as it's the exit code
                } catch (const std::exception&) {
                    // If next argument is not a number, use default exit code 0
                }
            }
            std::cout << "Test mode enabled (exit code: " << testExitCode << ")" << std::endl;
        }
        else if (arg == "--templates") {
            printTemplates();
            return 0;
        }
        else if (arg == "--new") {
            newProject = true;
            // Check if next argument is a path
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                shaderProjectPath = argv[i + 1];
                i++; // Skip the next argument as it's the path
            } else {
                shaderProjectPath = "."; // Use current directory
            }
            // Note: Logger not initialized yet, so using std::cout here
            std::cout << "Creating new project in: " << shaderProjectPath << std::endl;
        }
        else if (arg == "-t" && i + 1 < argc) {
            // Template selection (only valid with --new)
            templateName = argv[i + 1];
            i++; // Skip the template name
            // Note: Logger not initialized yet, so using std::cout here
            std::cout << "Using template: " << templateName << std::endl;
        }
        else if (!arg.empty() && arg[0] != '-') {
            // This is a shader project path
            if (shaderProjectPath.empty()) {
                shaderProjectPath = arg;
                // Note: Logger not initialized yet, so using std::cout here
                std::cout << "Shader project path: " << shaderProjectPath << std::endl;
            } else {
                std::cerr << "Multiple shader paths specified. Only one is allowed." << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Initialize logger early so it can be used throughout
    Logger::getInstance().initialize(debugMode);
    
    // Handle --new flag
    if (newProject) {
        ShaderProject newProj;
        if (!newProj.createNew(shaderProjectPath, "New Shader Project", templateName)) {
            LOG_ERROR("Failed to create new project in: {}", shaderProjectPath);
            return 1;
        }
        LOG_SUCCESS("Successfully created new shader project in: {}", shaderProjectPath);
        return 0; // Exit after creating project
    }
    
    // If no path specified and not in test mode, use current directory
    if (shaderProjectPath.empty() && !testMode) {
        shaderProjectPath = ".";
    }
    
    // If not in test mode, validate that manifest exists
    if (!testMode && !shaderProjectPath.empty()) {
        std::string manifestPath = shaderProjectPath + "/" + SHADER_PROJECT_MANIFEST_FILENAME;
        if (!std::filesystem::exists(manifestPath)) {
            LOG_ERROR("No {} manifest found in: {}", SHADER_PROJECT_MANIFEST_FILENAME, shaderProjectPath);
            LOG_ERROR("Use --new to create a new project, or specify a directory with a {} manifest.", SHADER_PROJECT_MANIFEST_FILENAME);
            return 1;
        }
    }
    
    if (testMode) {
        app.setTestMode(true, testExitCode);
        // In test mode, use basic shader if no project specified
        if (shaderProjectPath.empty()) {
            shaderProjectPath = "shaders/basic";
        }
    }
    
    if (!shaderProjectPath.empty()) {
        app.setShaderProjectPath(shaderProjectPath);
    }
    
    if (!app.initialize()) {
        LOG_ERROR("Failed to initialize application");
        return 1;
    }
    
    app.run();
    
    // Return test exit code if in test mode
    if (testMode) {
        return app.getTestExitCode();
    }
    
    return 0;
}
