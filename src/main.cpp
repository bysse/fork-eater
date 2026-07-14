#include <thread>
#include <cmath>
#include <sstream>

#include "glad.h"
#include "glad.h"
#include <GLFW/glfw3.h>

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
#include "Settings.h"
#include "Timeline.h"
#include "GeneratedShaderLibraries.h"
#include <filesystem>
#include "RenderScaleMode.h"
#include "ShaderPreprocessor.h"
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

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
        // Initialize Settings early (before GLFW)
        Settings& settings = Settings::getInstance();
        settings.initialize();
        
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
        
        // Apply DPI scaling to window size
        float uiScale = settings.getUIScaleFactor();
        int scaledWidth = static_cast<int>(WINDOW_WIDTH * uiScale);
        int scaledHeight = static_cast<int>(WINDOW_HEIGHT * uiScale);
        
        // Create window
        m_window = glfwCreateWindow(
            scaledWidth,
            scaledHeight,
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

        // Initialize GLAD
        if (!gladLoadGL()) {
            LOG_ERROR("Failed to initialize GLAD");
            return false;
        }
        
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
        
        // Apply DPI scaling to ImGui
        settings.applyToImGui();
        
        // Setup settings change callback
        settings.onSettingsChanged = [this]() {
            Settings& s = Settings::getInstance();
            s.applyToImGui();
        };
        
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

        float lastTime = 0.0f;
        
        while (m_running && !glfwWindowShouldClose(m_window)) {
            float currentTime = glfwGetTime();
            float deltaTime = currentTime - lastTime;
            lastTime = currentTime;


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
            
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(m_window);

            // Dump framebuffer if requested
            if (m_dumpFramebuffer) {
                dumpFramebuffer(m_dumpPassName, m_dumpOutputPath);
                // Disable dump so we don't dump every frame if we continue running (though test mode will exit)
                m_dumpFramebuffer = false;
            }
            
            // Test mode: exit after first render loop
            if (m_testMode) {
                LOG_SUCCESS("Test mode: completed one render loop successfully");
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
                m_running = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    
    void setTestMode(bool enabled, int exitCode = 0) {
        m_testMode = enabled;
        m_testExitCode = exitCode;
    }
    
    void setRenderScaleFactor(float scale) {
        if (m_shaderEditor) {
            m_shaderEditor->setRenderScaleFactor(scale);
        }
    }
    
    void setDumpFramebuffer(const std::string& passName, const std::string& outputPath) {
        m_dumpFramebuffer = true;
        m_dumpPassName = passName;
        m_dumpOutputPath = outputPath;
    }
    
    int getTestExitCode() const {
        return m_testExitCode;
    }

    void dumpFramebuffer(const std::string& passName, const std::string& outputPath) {
        m_shaderEditor->dumpFramebuffer(passName, outputPath);
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
    std::unique_ptr<Timeline> m_timeline;
    
    // Dump framebuffer options
    bool m_dumpFramebuffer = false;
    std::string m_dumpPassName;
    std::string m_dumpOutputPath;
    
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

bool exportBufferToTZD(const ShaderProject& proj, const ShaderBuffer& buffer, const std::string& outputPath) {
    int components = 1;
    if (buffer.dataType == "vec2") components = 2;
    else if (buffer.dataType == "vec3") components = 3;
    else if (buffer.dataType == "vec4") components = 4;

    size_t length = buffer.data.size() / components;
    if (length == 0) {
        LOG_ERROR("Buffer '{}' has no data to export.", buffer.name);
        return false;
    }

    std::vector<uint16_t> packed(buffer.data.size(), 0);

    for (size_t i = 0; i < buffer.data.size(); ++i) {
        float val = buffer.data[i];
        
        // Detect if it is an integer (only zeroes as decimals)
        bool isInteger = false;
        if (!std::isnan(val) && !std::isinf(val)) {
            isInteger = (val == std::floor(val));
        }

        uint16_t packedVal = 0;
        if (isInteger) {
            // Bit 15: 1 (Integer)
            packedVal |= (1U << 15);

            // Bit 14: Sign
            bool isNegative = (val < 0.0f);
            if (isNegative) {
                packedVal |= (1U << 14);
            }

            // Bits 13-0: Magnitude
            float absVal = std::abs(val);
            uint32_t magnitude = static_cast<uint32_t>(absVal);
            if (magnitude > 16383) {
                magnitude = 16383; // Clamping to fit in 14 bits
            }
            packedVal |= (magnitude & 0x3FFF);
        } else {
            // Bit 15: 0 (Float)
            // Bit 14: Sign
            bool isNegative = (val < 0.0f);
            if (isNegative) {
                packedVal |= (1U << 14);
            }

            // Bits 13-0: Magnitude mapped to [0.0, 1.0] using 16384.0 scale
            float absVal = std::abs(val);
            if (absVal > 1.0f) {
                absVal = 1.0f; // Clamped to [0.0, 1.0]
            }
            
            uint32_t fractionVal = static_cast<uint32_t>(std::round(absVal * 16384.0f));
            if (fractionVal > 16383) {
                fractionVal = 16383; // Clamp to max 14-bit value
            }
            packedVal |= (fractionVal & 0x3FFF);
        }
        packed[i] = packedVal;
    }

    std::vector<uint8_t> outputBytes;
    outputBytes.reserve(packed.size() * 2);

    bool stripedCoords = buffer.exportStripedCoordinates;
    bool stripedBytes = buffer.exportStripedBytes;

    if (stripedBytes) {
        if (stripedCoords) {
            // Group high bytes of components first
            for (int c = 0; c < components; ++c) {
                for (size_t i = 0; i < length; ++i) {
                    uint16_t val = packed[i * components + c];
                    outputBytes.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
                }
            }
            // Group low bytes of components next
            for (int c = 0; c < components; ++c) {
                for (size_t i = 0; i < length; ++i) {
                    uint16_t val = packed[i * components + c];
                    outputBytes.push_back(static_cast<uint8_t>(val & 0xFF));
                }
            }
        } else {
            // Interleaved coordinates, but high bytes first, then low bytes
            for (size_t i = 0; i < packed.size(); ++i) {
                outputBytes.push_back(static_cast<uint8_t>((packed[i] >> 8) & 0xFF));
            }
            for (size_t i = 0; i < packed.size(); ++i) {
                outputBytes.push_back(static_cast<uint8_t>(packed[i] & 0xFF));
            }
        }
    } else {
        if (stripedCoords) {
            // Group coordinates by component, no byte striping (High byte, then Low byte)
            for (int c = 0; c < components; ++c) {
                for (size_t i = 0; i < length; ++i) {
                    uint16_t val = packed[i * components + c];
                    outputBytes.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
                    outputBytes.push_back(static_cast<uint8_t>(val & 0xFF));
                }
            }
        } else {
            // Interleaved coordinates, no byte striping (High byte, then Low byte)
            for (size_t i = 0; i < packed.size(); ++i) {
                outputBytes.push_back(static_cast<uint8_t>((packed[i] >> 8) & 0xFF));
                outputBytes.push_back(static_cast<uint8_t>(packed[i] & 0xFF));
            }
        }
    }

    std::filesystem::path fullOutputPath = std::filesystem::path(proj.getProjectPath()) / outputPath;
    
    try {
        std::filesystem::create_directories(fullOutputPath.parent_path());
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create directory structure for output: {}", e.what());
        return false;
    }

    std::ofstream out(fullOutputPath, std::ios::binary);
    if (!out.is_open()) {
        LOG_ERROR("Failed to open binary file for writing: {}", fullOutputPath.string());
        return false;
    }
    out.write(reinterpret_cast<const char*>(outputBytes.data()), outputBytes.size());
    out.close();

    LOG_SUCCESS("Successfully exported buffer '{}' to TZD binary: {} ({} bytes)", buffer.name, fullOutputPath.string(), outputBytes.size());
    return true;
}

// Function declarations
void printUsage(const char* programName);
void printTemplates();

int main(int argc, char* argv[]) {
    EmbeddedLibraries::initialize();
    Application app;
    bool testMode = false;
    bool newProject = false;
    bool exportLibs = false;
    int testExitCode = 0;
    bool debugMode = false;
    bool dumpFramebuffer = false;
    std::string dumpPassName;
    std::string dumpOutputPath;
    std::string shaderProjectPath;
    std::string templateName = "simple";
    
    // Preprocessor options
    bool preprocessMode = false;
    std::string preprocessInputPath;
    std::string outputPath;
    bool exportBufferMode = false;
    std::string exportBufferIndexStr;
    bool exportBuffersMode = false;
    int xres = 0;
    int yres = 0;
    std::string passName;

    // DPI scaling options
    bool overrideScaling = false;
    float customScale = 1.0f;
    bool disableDpiScaling = false;
    bool overrideRenderScaleMode = false;
    RenderScaleMode customRenderScaleMode = RenderScaleMode::Resolution;
    bool overrideRenderScale = false;
    float customRenderScale = 1.0f;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--debug" || arg == "-d") {
            debugMode = true;
        }
        else if (arg == "--render-scale-mode" && i + 1 < argc) {
            std::string modeStr = argv[i + 1];
            if (modeStr == "chunk") {
                customRenderScaleMode = RenderScaleMode::Chunk;
            } else if (modeStr == "resolution") {
                customRenderScaleMode = RenderScaleMode::Resolution;
            } else {
                LOG_ERROR("Invalid render scale mode: {}. Use 'chunk' or 'resolution'.", modeStr);
                return 1;
            }
            overrideRenderScaleMode = true;
            i++; // Skip mode argument
        }
        else if (arg == "--render-scale" && i + 1 < argc) {
             try {
                customRenderScale = std::stof(argv[i + 1]);
                if (customRenderScale <= 0.0f || customRenderScale > 1.0f) {
                    LOG_ERROR("Render scale must be between 0.0 and 1.0");
                    return 1;
                }
                overrideRenderScale = true;
                i++; // Skip the scale value
            } catch (const std::exception&) {
                LOG_ERROR("Invalid render scale: {}", argv[i + 1]);
                return 1;
            }
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
        }
        else if (arg == "--templates") {
            printTemplates();
            return 0;
        }
        else if (arg == "--new") {
            newProject = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                shaderProjectPath = argv[i + 1];
                i++;
                
                // Check for optional template flag -t
                if (i + 2 < argc && std::string(argv[i + 1]) == "-t") {
                    templateName = argv[i + 2];
                    i += 2;
                }
            }
        }
        else if (arg == "--export-libs") {
            exportLibs = true;
        }
        else if (arg == "--scale" && i + 1 < argc) {
            // Custom UI scaling factor
            try {
                customScale = std::stof(argv[i + 1]);
                if (customScale < 0.5f || customScale > 4.0f) {
                    LOG_ERROR("Scale factor must be between 0.5 and 4.0");
                    return 1;
                }
                overrideScaling = true;
                i++; // Skip the scale value
            } catch (const std::exception&) {
                LOG_ERROR("Invalid scale factor: {}", argv[i + 1]);
                return 1;
            }
        }
        else if (arg == "--no-dpi-scale") {
            // Disable DPI scaling
            disableDpiScaling = true;
        }
        else if (arg == "--dump-framebuffer" && i + 2 < argc) {
            dumpFramebuffer = true;
            dumpPassName = argv[i + 1];
            dumpOutputPath = argv[i + 2];
            i += 2;
        }
        else if (arg == "--preprocess" || arg == "-p") {
            preprocessMode = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                preprocessInputPath = argv[i + 1];
                i++;
            }
        }
        else if (arg == "--export-buffers") {
            exportBuffersMode = true;
        }
        else if (arg == "--export-buffer-header") {
            exportBufferMode = true;
            if (i + 1 < argc) {
                exportBufferIndexStr = argv[i + 1];
                i++;
            } else {
                LOG_ERROR("Missing buffer index for --export-buffer-header");
                return 1;
            }
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                outputPath = argv[i + 1];
                i++;
            } else {
                LOG_ERROR("Missing output file path for -o/--output");
                return 1;
            }
        }
        else if (arg == "-w" || arg == "--width") {
            if (i + 1 < argc) {
                try {
                    xres = std::stoi(argv[i + 1]);
                    i++;
                } catch (const std::exception&) {
                    LOG_ERROR("Invalid width: {}", argv[i + 1]);
                    return 1;
                }
            } else {
                LOG_ERROR("Missing width value");
                return 1;
            }
        }
        else if (arg == "-H" || arg == "--height") {
            if (i + 1 < argc) {
                try {
                    yres = std::stoi(argv[i + 1]);
                    i++;
                } catch (const std::exception&) {
                    LOG_ERROR("Invalid height: {}", argv[i + 1]);
                    return 1;
                }
            } else {
                LOG_ERROR("Missing height value");
                return 1;
            }
        }
        else if (arg == "--resolution") {
            if (i + 2 < argc) {
                try {
                    xres = std::stoi(argv[i + 1]);
                    yres = std::stoi(argv[i + 2]);
                    i += 2;
                } catch (const std::exception&) {
                    LOG_ERROR("Invalid resolution values: {} {}", argv[i + 1], argv[i + 2]);
                    return 1;
                }
            } else {
                LOG_ERROR("Missing values for --resolution (expected width and height)");
                return 1;
            }
        }
        else if (arg == "--pass") {
            if (i + 1 < argc) {
                passName = argv[i + 1];
                i++;
            } else {
                LOG_ERROR("Missing pass name for --pass");
                return 1;
            }
        }
        else if (!arg.empty() && arg[0] != '-') {
            // This is a shader project path
            if (shaderProjectPath.empty()) {
                shaderProjectPath = arg;
            } else {
                LOG_ERROR("Multiple shader paths specified. Only one is allowed.");
                printUsage(argv[0]);
                return 1;
            }
        }
        else {
            LOG_ERROR("Unknown argument: {}", arg);
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (!outputPath.empty() && !exportBufferMode) {
        preprocessMode = true;
    }

    // Initialize logger early so it can be used throughout
    Logger::getInstance().initialize(debugMode);
    LOG_INFO("Fork Eater - Compiled on {} at {}", __DATE__, __TIME__);

    if (exportBuffersMode) {
        if (shaderProjectPath.empty()) {
            shaderProjectPath = ".";
        }
        ShaderProject proj;
        LOG_INFO("Loading project from: {}", shaderProjectPath);
        if (!proj.loadFromDirectory(shaderProjectPath)) {
            LOG_ERROR("Failed to load project from: {}", shaderProjectPath);
            return 1;
        }

        bool success = true;
        int exportedCount = 0;
        for (const auto& buffer : proj.getManifest().buffers) {
            if (buffer.hasExport) {
                if (buffer.exportFormat == "tzd") {
                    if (!exportBufferToTZD(proj, buffer, buffer.exportOutputFile)) {
                        success = false;
                    } else {
                        exportedCount++;
                    }
                } else {
                    LOG_ERROR("Unsupported export format '{}' for buffer '{}'", buffer.exportFormat, buffer.name);
                    success = false;
                }
            }
        }

        if (!success) {
            LOG_ERROR("One or more buffers failed to export.");
            return 1;
        }

        if (exportedCount == 0) {
            LOG_WARN("No buffers configured for export in the manifest.");
        } else {
            LOG_SUCCESS("Successfully exported {} buffer(s).", exportedCount);
        }
        return 0;
    }

    if (exportBufferMode) {
        if (shaderProjectPath.empty()) {
            shaderProjectPath = ".";
        }
        ShaderProject proj;
        LOG_INFO("Loading project from: {}", shaderProjectPath);
        if (!proj.loadFromDirectory(shaderProjectPath)) {
            LOG_ERROR("Failed to load project from: {}", shaderProjectPath);
            return 1;
        }

        if (outputPath.empty()) {
            LOG_ERROR("Output path must be specified via -o/--output when using --export-buffer-header");
            return 1;
        }

        // Find buffer by index
        int bufferIndex = -1;
        try {
            bufferIndex = std::stoi(exportBufferIndexStr);
        } catch (const std::exception&) {
            LOG_ERROR("Invalid buffer index: {}", exportBufferIndexStr);
            return 1;
        }

        if (bufferIndex < 0 || bufferIndex >= static_cast<int>(proj.getManifest().buffers.size())) {
            LOG_ERROR("Buffer index {} out of range (project has {} buffers)", bufferIndex, proj.getManifest().buffers.size());
            return 1;
        }
        const ShaderBuffer* selectedBuffer = &proj.getManifest().buffers[bufferIndex];

        // Generate C-header content
        std::string indexStr = std::to_string(bufferIndex);
        std::string varName = "buffer_" + indexStr;
        std::string macroName = "BUFFER_" + indexStr;
        std::string guardName = macroName + "_H";

        int components = 1;
        if (selectedBuffer->dataType == "vec2") components = 2;
        else if (selectedBuffer->dataType == "vec3") components = 3;
        else if (selectedBuffer->dataType == "vec4") components = 4;

        size_t length = selectedBuffer->data.size() / components;

        std::stringstream header;
        header << "#ifndef " << guardName << "\n";
        header << "#define " << guardName << "\n\n";
        if (selectedBuffer->fixedPoint) {
            header << "#include <stdint.h>\n\n";
        }
        header << "#define " << macroName << "_LENGTH " << length << "\n";
        header << "#define " << macroName << "_NAME \"" << selectedBuffer->name << "\"\n\n";
        
        std::vector<float> exportData = selectedBuffer->data;
        if (selectedBuffer->striped) {
            exportData.resize(selectedBuffer->data.size());
            for (size_t i = 0; i < length; ++i) {
                for (int c = 0; c < components; ++c) {
                    exportData[c * length + i] = selectedBuffer->data[i * components + c];
                }
            }
        }

        if (selectedBuffer->fixedPoint) {
            std::string intType = "int16_t";
            int totalBits = (selectedBuffer->fixedPointSigned ? 1 : 0) + selectedBuffer->fixedPointM + selectedBuffer->fixedPointN;
            if (selectedBuffer->fixedPointSigned) {
                if (totalBits <= 8) intType = "int8_t";
                else if (totalBits <= 16) intType = "int16_t";
                else if (totalBits <= 32) intType = "int32_t";
                else intType = "int64_t";
            } else {
                if (totalBits <= 8) intType = "uint8_t";
                else if (totalBits <= 16) intType = "uint16_t";
                else if (totalBits <= 32) intType = "uint32_t";
                else intType = "uint64_t";
            }

            double scale = std::pow(2.0, selectedBuffer->fixedPointN);
            std::vector<int64_t> exportDataInt;
            exportDataInt.reserve(exportData.size());

            int shiftAmount = selectedBuffer->fixedPointM + selectedBuffer->fixedPointN;
            if (shiftAmount > 62) shiftAmount = 62;
            int64_t minVal = selectedBuffer->fixedPointSigned ? -(1LL << shiftAmount) : 0;
            int64_t maxVal = (1LL << shiftAmount) - 1;
            if (!selectedBuffer->fixedPointSigned && shiftAmount == 63) {
                maxVal = -1; // all bits set
            }

            for (float val : exportData) {
                int64_t intVal = 0;
                if (!std::isnan(val) && !std::isinf(val)) {
                    intVal = static_cast<int64_t>(std::round(val * scale));
                }
                if (intVal < minVal) intVal = minVal;
                if (intVal > maxVal) intVal = maxVal;
                exportDataInt.push_back(intVal);
            }

            header << "static const " << intType << " " << varName << "[" << exportDataInt.size() << "] = {\n    ";
            for (size_t idx = 0; idx < exportDataInt.size(); ++idx) {
                header << exportDataInt[idx];
                if (idx + 1 < exportDataInt.size()) {
                    header << ", ";
                    if ((idx + 1) % 8 == 0) {
                        header << "\n    ";
                    }
                }
            }
            header << "\n};\n\n";
        } else {
            header << "static const float " << varName << "[" << exportData.size() << "] = {\n    ";
            for (size_t idx = 0; idx < exportData.size(); ++idx) {
                float val = exportData[idx];
                std::string floatStr;
                if (std::isnan(val)) {
                    floatStr = "0.0";
                } else if (std::isinf(val)) {
                    floatStr = (val < 0.0f) ? "-3.40282347e+38" : "3.40282347e+38";
                } else {
                    std::ostringstream ss;
                    ss.precision(9);
                    ss << val;
                    floatStr = ss.str();
                    if (floatStr.find('.') == std::string::npos && floatStr.find('e') == std::string::npos && floatStr.find('E') == std::string::npos) {
                        floatStr += ".0";
                    }
                }
                header << floatStr << "f";
                if (idx + 1 < exportData.size()) {
                    header << ", ";
                    if ((idx + 1) % 8 == 0) {
                        header << "\n    ";
                    }
                }
            }
            header << "\n};\n\n";
        }

        if (selectedBuffer->striped || selectedBuffer->fixedPoint) {
            std::string funcName = "unpack_" + varName;
            header << "static inline const float* " << funcName << "() {\n";
            
            bool padTo4 = (selectedBuffer->type == "ubo");
            size_t destSize = length * (padTo4 ? 4 : components);
            header << "    static float dest[" << destSize << "];\n";
            
            if (selectedBuffer->fixedPoint) {
                double divisor = std::pow(2.0, selectedBuffer->fixedPointN);
                std::ostringstream divStr;
                divStr.precision(12);
                divStr << divisor;
                std::string dStr = divStr.str();
                if (dStr.find('.') == std::string::npos && dStr.find('e') == std::string::npos && dStr.find('E') == std::string::npos) {
                    dStr += ".0";
                }
                header << "    const float scale = 1.0f / " << dStr << "f;\n";
            }
            
            header << "    for (int i = 0; i < " << macroName << "_LENGTH; ++i) {\n";
            
            int destStride = padTo4 ? 4 : components;
            for (int c = 0; c < destStride; ++c) {
                header << "        dest[i * " << destStride << " + " << c << "] = ";
                if (c < components) {
                    std::string srcExpr;
                    if (selectedBuffer->striped) {
                        if (components == 1) {
                            srcExpr = varName + "[i]";
                        } else {
                            srcExpr = varName + "[" + std::to_string(c) + " * " + macroName + "_LENGTH + i]";
                        }
                    } else {
                        if (components == 1) {
                            srcExpr = varName + "[i]";
                        } else {
                            srcExpr = varName + "[i * " + std::to_string(components) + " + " + std::to_string(c) + "]";
                        }
                    }
                    
                    if (selectedBuffer->fixedPoint) {
                        header << "(float)" << srcExpr << " * scale;\n";
                    } else {
                        header << srcExpr << ";\n";
                    }
                } else {
                    header << "0.0f;\n";
                }
            }
            header << "    }\n";
            header << "    return dest;\n";
            header << "}\n\n";
        }

        header << "#endif // " << guardName << "\n";

        std::ofstream out(outputPath);
        if (!out.is_open()) {
            LOG_ERROR("Failed to write buffer header to: {}", outputPath);
            return 1;
        }
        out << header.str();
        out.close();

        LOG_SUCCESS("Successfully exported buffer '{}' to C-header: {}", selectedBuffer->name, outputPath);
        return 0;
    }

    if (preprocessMode) {
        if (preprocessInputPath.empty()) {
            if (!shaderProjectPath.empty()) {
                preprocessInputPath = shaderProjectPath;
            } else {
                preprocessInputPath = ".";
            }
        }
        
        if (xres <= 0 || yres <= 0) {
            LOG_ERROR("Error: Resolution width and height must be specified when using preprocessor mode (e.g. -w 1920 -h 1080 or --resolution 1920 1080)");
            return 1;
        }
        if (outputPath.empty()) {
            LOG_ERROR("Error: Output file path must be specified via -o or --output");
            return 1;
        }

        ShaderProject proj;
        bool isProject = false;
        std::string shaderToPreprocess = preprocessInputPath;
        
        // Check if the input path is a directory
        if (std::filesystem::is_directory(preprocessInputPath)) {
            isProject = true;
        } else {
            // If it's a file, check if there's a 4k-eater.project in its parent directories to load as a project
            std::filesystem::path p(preprocessInputPath);
            std::filesystem::path current = p.parent_path();
            while (!current.empty() && current != current.root_path()) {
                if (std::filesystem::exists(current / SHADER_PROJECT_MANIFEST_FILENAME)) {
                    preprocessInputPath = current.string();
                    isProject = true;
                    break;
                }
                current = current.parent_path();
            }
        }

        std::map<std::string, std::vector<float>> uniformValues;
        std::map<std::string, bool> switchStates;
        std::map<std::string, int> sliderStates;
        
        if (isProject) {
            LOG_INFO("Loading project from: {}", preprocessInputPath);
            if (!proj.loadFromDirectory(preprocessInputPath)) {
                LOG_ERROR("Failed to load project from: {}", preprocessInputPath);
                return 1;
            }
            
            // Find which shader pass to use
            const auto& passes = proj.getPasses();
            if (passes.empty()) {
                LOG_ERROR("Project has no shader passes");
                return 1;
            }
            
            const ShaderPass* selectedPass = nullptr;
            if (!passName.empty()) {
                for (const auto& pass : passes) {
                    if (pass.name == passName) {
                        selectedPass = &pass;
                        break;
                    }
                }
                if (!selectedPass) {
                    LOG_ERROR("Pass '{}' not found in project", passName);
                    return 1;
                }
            } else {
                // Default to first enabled pass, or just first pass
                for (const auto& pass : passes) {
                    if (pass.enabled) {
                        selectedPass = &pass;
                        break;
                    }
                }
                if (!selectedPass) {
                    selectedPass = &passes[0];
                }
            }
            
            // Get the absolute fragment shader path
            shaderToPreprocess = proj.getShaderPath(selectedPass->fragmentShader);
            passName = selectedPass->name;
            
            // Load uniforms file
            std::string uniformsPath = preprocessInputPath + "/uniforms.json";
            if (std::filesystem::exists(uniformsPath)) {
                std::ifstream file(uniformsPath);
                if (file.is_open()) {
                    try {
                        json j = json::parse(file);
                        if (j.contains("uniforms") && j["uniforms"].contains(passName)) {
                            for (auto& [name, val] : j["uniforms"][passName].items()) {
                                uniformValues[name] = val.get<std::vector<float>>();
                            }
                        }
                        if (j.contains("switches")) {
                            for (auto& [name, val] : j["switches"].items()) {
                                switchStates[name] = val.get<bool>();
                            }
                        }
                        if (j.contains("sliders")) {
                            for (auto& [name, val] : j["sliders"].items()) {
                                sliderStates[name] = val.get<int>();
                            }
                        }
                    } catch (const std::exception& e) {
                        LOG_WARN("Failed to parse uniforms.json: {}", e.what());
                    }
                }
            }
        } else {
            // If it's a standalone shader file, try to locate uniforms.json in the same directory or parent
            std::filesystem::path shaderPath(shaderToPreprocess);
            std::filesystem::path dir = shaderPath.parent_path();
            std::string uniformsPath;
            if (std::filesystem::exists(dir / "uniforms.json")) {
                uniformsPath = (dir / "uniforms.json").string();
            } else if (std::filesystem::exists(dir.parent_path() / "uniforms.json")) {
                uniformsPath = (dir.parent_path() / "uniforms.json").string();
            }
            
            if (!uniformsPath.empty()) {
                std::ifstream file(uniformsPath);
                if (file.is_open()) {
                    try {
                        json j = json::parse(file);
                        // For standalone file, we might not have a pass name. Let's merge all uniforms from all passes or look up directly.
                        if (j.contains("uniforms")) {
                            for (auto& [pass, uniforms] : j["uniforms"].items()) {
                                for (auto& [name, val] : uniforms.items()) {
                                    uniformValues[name] = val.get<std::vector<float>>();
                                }
                            }
                        }
                        if (j.contains("switches")) {
                            for (auto& [name, val] : j["switches"].items()) {
                                switchStates[name] = val.get<bool>();
                            }
                        }
                        if (j.contains("sliders")) {
                            for (auto& [name, val] : j["sliders"].items()) {
                                sliderStates[name] = val.get<int>();
                            }
                        }
                    } catch (const std::exception& e) {
                        LOG_WARN("Failed to parse uniforms.json: {}", e.what());
                    }
                }
            }
        }

        LOG_INFO("Preprocessing shader: {}", shaderToPreprocess);
        
        std::vector<ShaderPreprocessor::BufferInfo> preprocessBuffers;
        if (isProject) {
            for (const auto& buf : proj.getManifest().buffers) {
                ShaderPreprocessor::BufferInfo info;
                info.name = buf.name;
                info.type = buf.type;
                info.dataType = buf.dataType;
                info.size = buf.data.size();
                preprocessBuffers.push_back(info);
            }
        }

        ShaderPreprocessor preprocessor;
        std::string baked = preprocessor.preprocessAndBake(
            shaderToPreprocess,
            uniformValues,
            switchStates,
            sliderStates,
            preprocessBuffers,
            xres,
            yres
        );
        
        if (baked.empty() || baked.find("#error") != std::string::npos) {
            LOG_ERROR("Preprocessing failed or contains errors.");
        }
        
        std::ofstream out(outputPath);
        if (!out.is_open()) {
            LOG_ERROR("Failed to write to output file: {}", outputPath);
            return 1;
        }
        
        out << baked;
        out.close();
        
        LOG_SUCCESS("Successfully preprocessed and baked shader to: {}", outputPath);
        return (baked.find("#error") != std::string::npos) ? 1 : 0;
    }

    if (debugMode) {
        LOG_INFO("Debug mode enabled");
    }
    if (testMode) {
        LOG_INFO("Test mode enabled (exit code: {})", testExitCode);
    }
    if (newProject) {
        LOG_INFO("Creating new project in: {}", shaderProjectPath);
    }
    if (templateName != "simple") {
        LOG_INFO("Using template: {}", templateName);
    }
    if (overrideScaling) {
        LOG_INFO("Using custom UI scale: {}x", customScale);
    }
    if (disableDpiScaling) {
        LOG_INFO("DPI scaling disabled");
    }
    if (!shaderProjectPath.empty()) {
        LOG_INFO("Shader project path: {}", shaderProjectPath);
    }
    
    // Handle --new flag
    if (newProject) {
        ShaderProject newProj;
        if (!newProj.createNew(shaderProjectPath, "New Shader Project", templateName)) {
            LOG_ERROR("Failed to create new project in: {}", shaderProjectPath);
            return 1;
        }

        if (exportLibs) {
            newProj.exportLibraries();
        }

        LOG_IMPORTANT("Successfully created new shader project in: {}", shaderProjectPath);
        return 0; // Exit after creating project
    }
    
    // If no path specified and not in test mode, use current directory
    if (shaderProjectPath.empty() && !testMode) {
        shaderProjectPath = ".";
    }

    // Handle --export-libs for an existing project
    if (exportLibs && !newProject) {
        ShaderProject proj;
        if (proj.loadFromDirectory(shaderProjectPath)) {
            if (proj.exportLibraries()) {
                return 0;
            }
            return 1;
        } else {
            LOG_ERROR("Failed to load project from {} to export libraries", shaderProjectPath);
            return 1;
        }
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
        // In test mode, use the sample project if none specified (see project/test/)
        if (shaderProjectPath.empty()) {
            shaderProjectPath = "project/test";
        }
    }
    
    if (!shaderProjectPath.empty()) {
        app.setShaderProjectPath(shaderProjectPath);
    }
    
    // Apply command line DPI scaling settings before initialization
    Settings& settings = Settings::getInstance();
    
    if (overrideRenderScaleMode) {
        settings.setRenderScaleMode(customRenderScaleMode);
        LOG_INFO("Render scale mode set to {} via command line", 
                 (customRenderScaleMode == RenderScaleMode::Chunk ? "Chunk" : "Resolution"));
    }

    if (overrideRenderScale) {
        app.setRenderScaleFactor(customRenderScale);
        LOG_INFO("Render scale set to {} via command line", customRenderScale);
    }

    if (overrideScaling || disableDpiScaling) {
        
        if (disableDpiScaling) {
            settings.setDPIScaleMode(DPIScaleMode::Disabled);
            LOG_INFO("DPI scaling disabled via command line");
        } else if (overrideScaling) {
            settings.setDPIScaleMode(DPIScaleMode::Manual);
            settings.setUIScaleFactor(customScale);
            settings.setFontScaleFactor(customScale);
            LOG_INFO("UI scaling set to {}x via command line", customScale);
        }
    }
    
    if (!app.initialize()) {
        LOG_ERROR("Failed to initialize application");
        return 1;
    }

    if (dumpFramebuffer) {
        app.setDumpFramebuffer(dumpPassName, dumpOutputPath);
        // Force test mode to ensure we run one frame and exit
        app.setTestMode(true, testMode ? testExitCode : 0);
    }
    
    app.run();
    
    // Return test exit code if in test mode
    if (testMode || dumpFramebuffer) {
        return app.getTestExitCode();
    }
    
    return 0;
}

void printUsage(const char* programName) {
    LOG_INFO("Usage: {} [options] [shader_directory]", programName);
    LOG_INFO("Options:");
    LOG_INFO("  --new [path] [-t template]  Create new shader project");
    LOG_INFO("  --export-libs               Export bundled libraries to project's libs/ folder");
    LOG_INFO("  --templates                 List available shader templates");
    LOG_INFO("  --render-scale-mode MODE    Set render scale mode (chunk, resolution)");
    LOG_INFO("  --render-scale FACTOR       Set initial render scale factor (0.0 - 1.0)");
    LOG_INFO("  --preprocess, -p PATH       Preprocess shader file or project directory");
    LOG_INFO("  --export-buffers            Export all buffers configured in the manifest to their output files");
    LOG_INFO("  --export-buffer-header INDEX Export project buffer values (by manifest index) to a C-header file");
    LOG_INFO("  -o, --output PATH           Output baked/preprocessed shader or exported buffer header to path");
    LOG_INFO("  -w, --width VAL             Width (XRES) for resolution substitution");
    LOG_INFO("  -H, --height VAL            Height (YRES) for resolution substitution");
    LOG_INFO("  --resolution W H            Width and height for resolution substitution");
    LOG_INFO("  --pass NAME                 Pass name to preprocess (defaults to first pass)");
    LOG_INFO("  --test [exit_code]          Run in test mode (exit after one render loop)");
    LOG_INFO("  --debug, -d                 Enable debug output with colors");
    LOG_INFO("  --scale FACTOR              Set UI scale factor (e.g., 1.0, 1.5, 2.0)");
    LOG_INFO("  --no-dpi-scale              Disable DPI scaling (use 1.0x scaling)");
    LOG_INFO("  --help, -h                  Show this help message");
    LOG_INFO("  shader_directory            Path to shader project directory containing {} manifest", SHADER_PROJECT_MANIFEST_FILENAME);
    LOG_INFO("");
    LOG_INFO("Fork Eater - Real-time GLSL shader editor with hot reloading");
    LOG_INFO("");
    LOG_INFO("If no directory is specified, uses current directory.");
    LOG_INFO("Program will exit if no {} manifest is found (except in test mode).", SHADER_PROJECT_MANIFEST_FILENAME);
}

void printTemplates() {
    LOG_INFO("Available shader templates:");
    const auto& templateManager = ShaderTemplateManager::getInstance();
    const auto& templateNames = templateManager.getTemplateNames();
    
    for (const auto& name : templateNames) {
        const auto* templ = templateManager.getTemplate(name);
        if (templ) {
            LOG_INFO("  {} - {}", name, templ->description);
        }
    }
}
