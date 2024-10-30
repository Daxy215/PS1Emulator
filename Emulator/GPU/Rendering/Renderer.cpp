#include "Renderer.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <GL/glew.h>
#include "SDL.h"

#include "../Gpu.h"

Emulator::Renderer::Renderer() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL could not initialize SDL_Error: " << SDL_GetError() << '\n';
        return;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    
    window = SDL_CreateWindow("PSX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 512, SDL_WINDOW_OPENGL);
    
    if (window == nullptr) {
        std::cerr << "Window could not be created SDL_Error: " << SDL_GetError() << '\n';
        return;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, GL_CONTEXT_FLAG_DEBUG_BIT);
    
    sdl_context = SDL_GL_CreateContext(window);
    if (sdl_context == nullptr) {
        std::cerr << "OpenGL context could not be created SDL_Error: " << SDL_GetError() << '\n';
        return;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    
    glClearColor(1, 0, 0, 1.0f);
    SDL_GL_SwapWindow(window);
    
    GLenum err = glewInit();
    if (err!= GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << '\n';
        return;
    }
    
    SDL_Surface* screenSurface = SDL_GetWindowSurface(window);
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
    SDL_UpdateWindowSurface(window);
    
    // Load and bind shaders
    std::string vertexSource   = getShaderSource("Shaders/vertex.glsl");
    std::string fragmentSource = getShaderSource("Shaders/fragment.glsl");
    
    vertexShader = compileShader(vertexSource.c_str(), GL_VERTEX_SHADER);
    fragmentShader = compileShader(fragmentSource.c_str(), GL_FRAGMENT_SHADER);
    
    program = linkProgram(vertexShader, fragmentShader);
    
    // Generate buffers & arrays
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // Position buffer
    positions.create();
    
    GLuint index = getProgramAttrib(program, "vertexPosition");
    
    // Enable the attrib
    glEnableVertexAttribArray(index);
    
    // Link the buffer and the given index.
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(Position), nullptr);
    
    // Color buffer
    colors.create();
    
    index = getProgramAttrib(program, "vertexColor");
    
    // Enable the attrib
    glEnableVertexAttribArray(index);
    
    // Link the buffer and the given index.
    glVertexAttribIPointer(index, 3, GL_UNSIGNED_BYTE, 0, nullptr);
    
    glUseProgram(program);
    
    // Uniforms
    offsetUni = glGetUniformLocation(program, "offset");
    glUniform2i(offsetUni, 0, 0);
    
    drawingUni = glGetUniformLocation(program, "drawingArea");
    glUniform2i(drawingUni, 1024, 512);
    
    GLenum ersr = glGetError();
    if (ersr!= GL_NO_ERROR) {
        std::cerr << "OpenGL Error con: " << ersr << '\n';
    }
}

void Emulator::Renderer::display() {
    //glClearColor(1, 0, 0, 1.0f);
    
    draw();
    
    SDL_GL_SwapWindow(window);
    
    GLenum ersr = glGetError();
    if (ersr!= GL_NO_ERROR) {
        std::cerr << "OpenGL Error con: " << ersr << '\n';
    }
}

void Emulator::Renderer::draw() {
    // Make sure all the data is flushed to the buffer
    //glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(program);
    //glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(nVertices));
    
    // Wait for GPU to complete
    /*auto sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    
    while(true) {
        auto r = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
        
        if(r == GL_ALREADY_SIGNALED || r == GL_CONDITION_SATISFIED)
            // Drawing done
                break;
    }
    */
    
    // Reset the buffers
    //nVertices = 0;
}

void Emulator::Renderer::pushLine(Emulator::Position* positions, Emulator::Color* colors) {
    // TODO;
    throw std::runtime_error("Error; Unsupported line rendering.");
}

void Emulator::Renderer::pushTriangle(Emulator::Position* positions, Emulator::Color* colors) {
    if(nVertices + 3 > VERTEX_BUFFER_LEN) {
        // Reset the buffer size
        //nVertices = 0;
        
        display();
    }
    
    for(int i = 0; i < 3; i++) {
        this->positions.set(nVertices, positions[i]);
        this->colors.set(nVertices, colors[i]);
        nVertices++;
    }
}

void Emulator::Renderer::pushQuad(Emulator::Position* positions, Emulator::Color* colors) {
    if(nVertices + 6 > VERTEX_BUFFER_LEN) {
        // Reset the buffer size
        //nVertices = 0;
        
        display();
    }
    
    // First triangle
    // [2, 3, 0]
    this->positions.set(nVertices, positions[2]);
    this->colors.set(nVertices, colors[2]);
    nVertices++;
    
    this->positions.set(nVertices, positions[3]);
    this->colors.set(nVertices, colors[3]);
    nVertices++;
    
    this->positions.set(nVertices, positions[0]);
    this->colors.set(nVertices, colors[0]);
    nVertices++;
    
    // [3, 0, 1]
    this->positions.set(nVertices, positions[3]);
    this->colors.set(nVertices, colors[3]);
    nVertices++;
    
    this->positions.set(nVertices, positions[0]);
    this->colors.set(nVertices, colors[0]);
    nVertices++;
    
    this->positions.set(nVertices, positions[1]);
    this->colors.set(nVertices, colors[1]);
    nVertices++;
    
    /*for(int i = 0; i < 3; i++) {
        this->positions.set(nVertices, positions[i]);
        this->colors.set(nVertices, colors[i]);
        nVertices++;
    }
    
    // Second triangle
    for(int i = 1; i < 4; i++) {
        this->positions.set(nVertices, positions[i]);
        this->colors.set(nVertices, colors[i]);
        nVertices++;
    }*/
}

void Emulator::Renderer::pushRectangle(Emulator::Position* positions, Emulator::Color* colors) {
    /*
     * From my knowledgeable, PS1 doesn't split,
     * rectangles into 2 trinagles, however,
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
        //nVertices = 0;
        
        display();
    }
    
    // First triangle
    // [0, 1, 2]
    this->positions.set(nVertices, positions[0]);
    this->colors.set(nVertices, colors[0]);
    nVertices++;
    
    this->positions.set(nVertices, positions[1]);
    this->colors.set(nVertices, colors[1]);
    nVertices++;
    
    this->positions.set(nVertices, positions[2]);
    this->colors.set(nVertices, colors[2]);
    nVertices++;
    
    // First triangle
    // [0, 2, 3]
    this->positions.set(nVertices, positions[0]);
    this->colors.set(nVertices, colors[0]);
    nVertices++;

    this->positions.set(nVertices, positions[2]);
    this->colors.set(nVertices, colors[2]);
    nVertices++;
    
    this->positions.set(nVertices, positions[3]);
    this->colors.set(nVertices, colors[3]);
    nVertices++;
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
    
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << '\n';
    }
    
    return program;
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

GLuint Emulator::Renderer::createFrameBuffer(GLsizei width, GLsizei height) {
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    
    GLuint texture;
    glGenTextures(1, &texture);
    
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << '\n';
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return framebuffer;
}
