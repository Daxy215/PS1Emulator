﻿#include "Renderer.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <GLFW/glfw3.h>

#include "../Gpu.h"

//#define Test
// Ik I shouldn't do this but im lazy
#define WIDTH  1024
#define HEIGHT 512

GLuint Emulator::Renderer::program = 0;

Emulator::Renderer::Renderer(Emulator::Gpu& gpu) : gpu(gpu), _rasterizer(gpu) {
    if(!glfwInit()) {
        std::cerr << "GLFW could not initialize: " << glewGetErrorString(0) << " \n";
        return;
    }
    
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Necessary on macOS
    //glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    
    //glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    
    window = glfwCreateWindow(WIDTH, HEIGHT, "PSX", nullptr, nullptr);
    
    if(window == nullptr) {
        glfwTerminate();
        return;
    }
    
    glfwMakeContextCurrent(window);
    
    /*if (GL_ARB_debug_output) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Ensures errors are raised as soon as they occur
        glDebugMessageCallback(openglDebugCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }*/
    
    //glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    
    if(err != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(err) << '\n';
        glfwTerminate();
        return;
    }
    
    // Load and bind shaders
    std::string vertexSource = getShaderSource("Shaders/vertex.glsl");
    std::string fragmentSource = getShaderSource("Shaders/fragment.glsl");
    
    vertexShader = compileShader(vertexSource.c_str(), GL_VERTEX_SHADER);
    fragmentShader = compileShader(fragmentSource.c_str(), GL_FRAGMENT_SHADER);
    
    program = linkProgram(vertexShader, fragmentShader);
    glUseProgram(program);
    
    // Generate buffers & arrays
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // Position buffer
    positions.create();
    
    GLuint index = getProgramAttrib(program, "vertexPosition");
    
    // Enable the attrib
    glEnableVertexAttribArray(index);
    
    // Link the buffer and the given index.
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(Gpu::Position), nullptr);
    
    // Color buffer
    colors.create();
    
    index = getProgramAttrib(program, "vertexColor");
    
    // Enable the attrib
    glEnableVertexAttribArray(index);
    
    // Link the buffer and the given index.
    glVertexAttribIPointer(index, 3, GL_UNSIGNED_BYTE, 0, nullptr);
    
    // UV Buffer
    uvs.create();
    
    index = getProgramAttrib(program, "texCoords");
    
    // Enable the attributes in the shader
    glEnableVertexAttribArray(index);
    
    // Link the buffer and the given index.
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(Gpu::UV), nullptr);
    
    // Attributes buffer
    attributes.create();
    
    index = getProgramAttrib(program, "attributes");

    // Enable the attributes in the shader
    glEnableVertexAttribArray(index);
    
    // Link the attributes to the buffer with offsets
    // Needs to be split up
    glVertexAttribIPointer(index, 1, GL_INT, 0, nullptr);
    
    // Uniforms
    offsetUni = glGetUniformLocation(program, "offset");
    setDrawingOffset(0, 0);
    
    drawingUni = glGetUniformLocation(program, "drawingArea");
    glUniform2i(drawingUni, 1024, 512);
    
    textureWindowUni = glGetUniformLocation(program, "textureWindow");
    setTextureWindow(0, 0, 0, 0);
    
    glDisable(GL_BLEND);
    
    // TODO; Enabling this does mess up textures a bit
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //glfwSwapInterval(0);
    
    // Create frame buffers
    //mainFramebuffer = createFrameBuffer(1024, 512, mainTexture);
    //offscreenFramebuffer = createFrameBuffer(1024, 512, offscreenTexture);
    
    GLenum ersr = glGetError();
    if(ersr != GL_NO_ERROR) {
        std::cerr << "OpenGLSS Error con: " << ersr << '\n';
    }
}

void Emulator::Renderer::display() {
    //auto displayX = gpu.drawingAreaRight - gpu.drawingAreaLeft;
    //auto displayY = gpu.drawingAreaBottom - gpu.drawingAreaTop;
    
    int width = 0;
    int height = 0;
    
    glfwGetWindowSize(window, &width, &height);
    
    glViewport(0, 0, width, height);
    //setDrawingArea(gpu.hres.getResolution(), gpu.vres == VerticalRes::Y240Lines ? 240 : 480);
    //setDrawingOffset(gpu.displayVramXStart, gpu.displayVramYStart);
    
    draw();
}

void Emulator::Renderer::draw() const {
    if(nVertices == 0)
        return;
    
    gpu.vram->endTransfer();
    
    //glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT/* | GL_DEPTH_BUFFER_BIT*/);
    
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(nVertices));
    
    glDisable(GL_BLEND);
    
    /*glfwSwapBuffers(window);
    glfwPollEvents();*/
    //nVertices = 0;
}

void Emulator::Renderer::displayVRam() {
    Gpu::Position positions[] = {
        Gpu::Position(0.0f, 0),   // Bottom left
        Gpu::Position(640.0f, 0), // Bottom right
        Gpu::Position(0.0f, 481.0f),   // Top left 
        Gpu::Position(640.0f, 481.0f), // Top right
    };
    
    Gpu::Color colors[] = {
        Gpu::Color(0x80, 0x80, 0x80),
        Gpu::Color(0x80, 0x80, 0x80),
        Gpu::Color(0x80, 0x80, 0x80),
        Gpu::Color(0x80, 0x80, 0x80),
    };
    
    Gpu::UV uvs[] = {
        Gpu::UV(0.0f, 0.0f), // Bottom-left
        Gpu::UV(1.0f, 0.0f), // Bottom-right
        Gpu::UV(0.0f, 1.0f), // Top-left
        Gpu::UV(1.0f, 1.0f), // Top-right
    };
    
    nVertices = 0;
    
    // reset drawing offset
    /*GLint value[2];
    glGetUniformiv(program, drawingUni, value);
    
    setDrawingArea(0, 0);*/
    //pushQuad(positions, colors, uvs, { 0, 1, Emulator::Gpu::TextureMode::TestVRAM });
    
    Gpu::Attributes attributes = { 0, 1, Emulator::Gpu::TextureMode::TestVRAM };
    
    // First triangle
    // [2, 3, 0]
    this->positions.set(nVertices, positions[2]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[2]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[2]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[3]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[3]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[3]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[0]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[0]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[0]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    // [3, 0, 1]
    this->positions.set(nVertices, positions[3]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[3]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[3]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[0]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[0]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[0]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[1]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[1]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[1]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    display();
    glfwPollEvents();
    
    //setDrawingArea(value[0], value[1]);
}

void Emulator::Renderer::clear() {
    //nVertices = 0;
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Emulator::Renderer::pushLine(Emulator::Gpu::Position positions[], Emulator::Gpu::Color colors[], Emulator::Gpu::UV uvs[], Gpu::Attributes attributes) {
    // TODO;
    throw std::runtime_error("Error; Unsupported line rendering.");
}

void Emulator::Renderer::pushTriangle(Emulator::Gpu::Position positions[], Emulator::Gpu::Color colors[], Emulator::Gpu::UV uvs[], Gpu::Attributes attributes) {
    if(nVertices + 3 > VERTEX_BUFFER_LEN) {
        // Reset the buffer size
        nVertices = 0;
        
        //display();
    }
    
    _rasterizer.drawTriangle(positions, colors, uvs, attributes);
    
    int length[] = { 0, 1, 2 };
    if(!checkIfWithin(positions, length)) {
        return;
    }
    
    for(int i = 0; i < 3; i++) {
        this->positions.set(nVertices, positions[i]);
        if(attributes.usesColor()) this->colors.set(nVertices, colors[i]);
        if(attributes.useTextures()) this->uvs.set(nVertices, uvs[i]);
        this->attributes.set(nVertices, attributes);
        nVertices++;
    }
}

void Emulator::Renderer::pushQuad(Emulator::Gpu::Position positions[], Emulator::Gpu::Color colors[], Emulator::Gpu::UV uvs[], Gpu::Attributes attributes) {
    if(nVertices + 6 > VERTEX_BUFFER_LEN) {
        // Reset the buffer size
        nVertices = 0;
        
        //display();
    }
    
    _rasterizer.drawQuad(positions, colors, uvs, attributes);
    //displayVRam();
    
    // First triangle
    // [2, 3, 0]

    int length[] = { 2, 3, 0 };
    if(!checkIfWithin(positions, length)) {
        return;
    }
    
    this->positions.set(nVertices, positions[2]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[2]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[2]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[3]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[3]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[3]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[0]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[0]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[0]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    // [3, 0, 1]
    int length1[] = { 3, 0, 1 };
    if(!checkIfWithin(positions, length1)) {
        return;
    }
    
    this->positions.set(nVertices, positions[3]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[3]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[3]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[0]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[0]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[0]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[1]);
    if(attributes.usesColor()) this->colors.set(nVertices, colors[1]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[1]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
}

void Emulator::Renderer::pushRectangle(Emulator::Gpu::Position positions[], Emulator::Gpu::Color colors[], Emulator::Gpu::UV uvs[], Gpu::Attributes attributes) {
    /*
     * From my knowledgeable, PS1 doesn't split,
     * rectangles into 2 triangles, however,
     * I don't understand SDL enough..
     * So, my goal in the future is to use,
     * a different library because I hate SDL.
     * 
     * Hence, why this is a different function,
     * from pushQuad, even though my goal isn't,
     * to make this emulator anywhere near accurate,
     * but it's fun to learn new stuff
     */
    
    if (nVertices + 6 > VERTEX_BUFFER_LEN) {
        // Reset the buffer size
        nVertices = 0;
        
        //display();
    }
    
    _rasterizer.drawRectangle(positions, colors, uvs, attributes);
    
    // First triangle
    // [0, 1, 2]
    
    int length[] = { 0, 1, 2 };
    if(!checkIfWithin(positions, length)) {
        return;
    }
    
    this->positions.set(nVertices, positions[0]);
    this->colors.set(nVertices, colors[0]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[0]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[1]);
    this->colors.set(nVertices, colors[1]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[1]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[2]);
    this->colors.set(nVertices, colors[2]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[2]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    // First triangle
    // [1, 2, 3]
    
    int length0[] = { 1, 2, 3 };
    if(!checkIfWithin(positions, length0)) {
        return;
    }
    
    this->positions.set(nVertices, positions[1]);
    this->colors.set(nVertices, colors[1]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[1]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[2]);
    this->colors.set(nVertices, colors[2]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[2]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
    
    this->positions.set(nVertices, positions[3]);
    this->colors.set(nVertices, colors[3]);
    if(attributes.useTextures()) this->uvs.set(nVertices, uvs[3]);
    this->attributes.set(nVertices, attributes);
    nVertices++;
}

bool Emulator::Renderer::checkIfWithin(Emulator::Gpu::Position positions[], int length[]) {
    for(size_t i = 0; i < 3; i++) {
        int index = i;//length[i];
        
        auto v0 = positions[index];
        auto v1 = positions[(index + 1) % 3];
        
        float dis0 = abs(v0.x - v1.x);
        float dis1 = abs(v0.y - v1.y);
        
        // Size Restriction: The maximum distance between two vertices is 1023 horizontally, and 511 vertically. 
        if(dis0 >= 1024 || dis1 >= 512) {
            return false;
        }
        
        /*auto width = gpu.hres.getResolution();
        auto height = gpu.vres == VerticalRes::Y240Lines ? 240 : 480;
        
        // Check if within screen bounds
        if(v0.x < 0 || v0.x > 512 || v0.y < 0 || v0.y > 512) {
            return false;
        }*/
    }
    
    return true;
}

void Emulator::Renderer::setDrawingOffset(int16_t x, int16_t y) {
    /**
     * Because my renderer is still...
     * isn't completed I need to manually do this,
     * for every game as otherwise, it causes,
     * the entire screen to constantly flicker
     */
    
    //glUniform2f(offsetUni, x, y);
    //lUniform2i(offsetUni, 0, 256);
    //glUniform2i(offsetUni, 0, 0); // GEX
    //glUniform2i(offsetUni, 120, 160); // Pepsi man
    //glUniform2i(offsetUni, 0, 0); // Ridge Racer
    //glUniform2i(offsetUni, 256, 128); // Crash Bandicoot
}

void Emulator::Renderer::setDrawingArea(int16_t right, int16_t bottom) {
    // 839, 479
    //glUniform2i(drawingUni, 640, 480);
    //glUniform2i(drawingUni, 512, 255);
    
    drawingArea.x = right;
    drawingArea.y = bottom;
    
    //glUniform2i(drawingUni, WIDTH, HEIGHT);
    glUniform2i(drawingUni, right, bottom);
}

void Emulator::Renderer::setTextureWindow(uint8_t textureWindowXMask, uint8_t textureWindowYMask, uint8_t textureWindowXOffset, uint8_t textureWindowYOffset) {
    glUniform4f(textureWindowUni, textureWindowXMask, textureWindowYMask, textureWindowXOffset, textureWindowYOffset);
}

void Emulator::Renderer::bindFrameBuffer(GLuint buf) {
    glBindFramebuffer(GL_FRAMEBUFFER, buf);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

GLuint Emulator::Renderer::compileShader(const char* source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << '\n';
    }
    
    return shader;
}

GLuint Emulator::Renderer::linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (success != GL_TRUE) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << '\n';
    }
    
    return program;
}

GLuint Emulator::Renderer::getProgramAttrib(GLuint program, const std::string& attr) {
    GLint index = glGetAttribLocation(program, attr.c_str());
    
    if (index < 0) {
        std::cerr << "Attribute " << attr << " was not found in the program\n";
        return -1;
    }
    
    return static_cast<GLuint>(index);
}

std::string Emulator::Renderer::getShaderSource(const std::string& path) {
    // Open the file in binary mode
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file." << '\n';
        return {};
    }
    
    // Read the file contents into a string
    std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    
    // Close the file
    file.close();
    
    return content;
}

GLuint Emulator::Renderer::createFrameBuffer(GLsizei width, GLsizei height, GLuint& textureId) {
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    
    glGenTextures(1, &textureId);
    
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << '\n';
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return framebuffer;
}

/*
void APIENTRY Emulator::Renderer::openglDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                GLsizei length, const GLchar* message, const void* userParam) {
    
     // Filter out certain messages, if desired
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    
    std::cerr << "OpenGL Debug Message (" << id << "): " << message << std::endl;
    
    switch (source) {
        case GL_DEBUG_SOURCE_API:             std::cerr << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cerr << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cerr << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cerr << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cerr << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cerr << "Source: Other"; break;
    }
    std::cerr << std::endl;
    
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               std::cerr << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cerr << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cerr << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         std::cerr << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cerr << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cerr << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cerr << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cerr << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cerr << "Type: Other"; break;
    }
    std::cerr << std::endl;
    
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         std::cerr << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cerr << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cerr << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cerr << "Severity: notification"; break;
    }
    
    std::cerr << std::endl << std::endl;
}
*/
