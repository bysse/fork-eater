#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <iostream>
#include <memory>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "ShaderManager.h"
#include "FileWatcher.h"
#include "ShaderEditor.h"

// Constants
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* WINDOW_TITLE = "Fork Eater - Shader Editor";

class Application {
public:
    Application() : m_window(nullptr), m_running(false) {}
    
    ~Application() {
        cleanup();
    }
    
    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
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
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        // Make OpenGL context current
        glfwMakeContextCurrent(m_window);
        
        // Enable vsync
        glfwSwapInterval(1);
        
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        
        // Setup ImGui style
        ImGui::StyleColorsDark();
        
        // Setup platform/renderer backends
        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        // Initialize components
        m_shaderManager = std::make_shared<ShaderManager>();
        m_fileWatcher = std::make_shared<FileWatcher>();
        m_shaderEditor = std::make_unique<ShaderEditor>(m_shaderManager, m_fileWatcher);
        
        if (!m_shaderEditor->initialize()) {
            std::cerr << "Failed to initialize shader editor" << std::endl;
            return false;
        }
        
        if (!m_fileWatcher->start()) {
            std::cerr << "Failed to start file watcher" << std::endl;
            return false;
        }
        
        std::cout << "Fork Eater initialized successfully!" << std::endl;
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        
        return true;
    }
    
    void run() {
        m_running = true;
        
        while (m_running && !glfwWindowShouldClose(m_window)) {
            // Poll events
            glfwPollEvents();
            
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Render the shader editor
            m_shaderEditor->render();
            
            // Rendering
            ImGui::Render();
            
            int display_w, display_h;
            glfwGetFramebufferSize(m_window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(m_window);
        }
    }

private:
    GLFWwindow* m_window;
    bool m_running;
    
    std::shared_ptr<ShaderManager> m_shaderManager;
    std::shared_ptr<FileWatcher> m_fileWatcher;
    std::unique_ptr<ShaderEditor> m_shaderEditor;
    
    void cleanup() {
        if (m_shaderEditor) {
            m_shaderEditor.reset();
        }
        
        if (m_fileWatcher) {
            m_fileWatcher->stop();
            m_fileWatcher.reset();
        }
        
        m_shaderManager.reset();
        
        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        // Cleanup GLFW
        if (m_window) {
            glfwDestroyWindow(m_window);
        }
        
        glfwTerminate();
    }
};

int main(int argc, char* argv[]) {
    Application app;
    
    if (!app.initialize()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    app.run();
    
    return 0;
}