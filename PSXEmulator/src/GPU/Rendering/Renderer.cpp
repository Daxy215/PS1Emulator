#include "Renderer.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

#include <GLFW/glfw3.h>
#include <glm/detail/func_geometric.inl>

#include "imgui.h"

#include "../Gpu.h"

//#define Test
// Ik I shouldn't do this but im lazy
enum {
    WIDTH = 1024,
    HEIGHT = 512
};

Emulator::Renderer::Renderer(Emulator::Gpu& gpu) : gpu(gpu), _rasterizer(gpu) {
    if(!glfwInit()) {
        std::cerr << "GLFW could not initialize: " << glewGetErrorString(0) << " \n";
        return;
    }
    
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Necessary on macOS
    //glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    
    //glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    printf("monitorCount: %d\n", monitorCount);
    
    auto monitor = monitors[monitorCount - 1];
    //const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    int mx, my;
    glfwGetMonitorPos(monitor, &mx, &my);
    
    window = glfwCreateWindow(WIDTH, HEIGHT, "PSX", nullptr, nullptr);
    glfwSetWindowPos(window, mx + 100, my + 100);
    
    if(window == nullptr) {
        glfwTerminate();
        
        return;
    }
    
    glfwMakeContextCurrent(window);
    
    // Init glew
    if(GLenum err = glewInit(); err != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(err) << '\n';
        glfwTerminate();
        
        return;
    }
    
    // Load and bind shaders
    {
        std::string vertexSource   = getShaderSource("../../Shaders/vertex.glsl");
        std::string fragmentSource = getShaderSource("../../Shaders/fragment.glsl");
        
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
        
        glVertexAttribIPointer(index, 1, GL_INT, 0, nullptr);
        
        // Uniforms
        offsetUni = glGetUniformLocation(program, "offset");
        setDrawingOffset(0, 0);
        
        drawingMinUni = glGetUniformLocation(program, "drawingAreaMin");
        glUniform2i(drawingMinUni, 1024, 512);
        
        drawingMaxUni = glGetUniformLocation(program, "drawingAreaMax");
        glUniform2i(drawingMaxUni, 1024, 512);
        
        textureWindowUni = glGetUniformLocation(program, "textureWindow");
        setTextureWindow(0, 0, 0, 0);
        
        semiTransparencyModeUni = glGetUniformLocation(program, "semiTransparencyMode");
        setSemiTransparencyMode(0);
    }
    
    glEnable(GL_BLEND);
    //glDisable(GL_BLEND);
    
    for (int i = 0; i < 2; i++)
        sceneFBO[i] = createFrameBuffer(WIDTH, HEIGHT, sceneTex[i]);
    
    // After sceneFBO creation
    for(int i = 0; i < 2; i++) {
        bloomFBO[i] = createFrameBuffer(WIDTH, HEIGHT, bloomTexture[i]);
        
        /*glBindTexture(GL_TEXTURE_2D, bloomTexture[i]);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/
    }
    
    {
        // Compile vertex shader first as,
        // it's used with other shaders
        //std::string postVertexSource   = preprocessShader(readFile("Shaders/bloomVertex.glsl"), "Shader");
        /*std::string postVertexSource   = getShaderSource("../Shaders/bloomVertex.glsl");
        postProcessVertexShader = compileShader(postVertexSource.c_str(), GL_VERTEX_SHADER);
        
        //std::string thresholdSource = preprocessShader(readFile("Shaders/bloom_threshold.frag"), "Shaders");
        std::string thresholdSource = getShaderSource("../Shaders/bloom_threshold.frag");
        //std::string blurSource = preprocessShader(readFile("Shaders/bloom_blur.frag"), "Shaders");
        std::string blurSource = getShaderSource("../Shaders/bloom_blur.frag");
        
        bloomThresholdProgram = compileShader(thresholdSource.c_str(), GL_FRAGMENT_SHADER);
        blurProgram = compileShader(blurSource.c_str(), GL_FRAGMENT_SHADER);
        
        bloomThresholdProgram = linkProgram(postProcessVertexShader, bloomThresholdProgram);
        blurProgram = linkProgram(postProcessVertexShader, blurProgram);
        
        //std::string postFragmentSource = preprocessShader(readFile("Shaders/bloomFragment.glsl"), "Shader");
        std::string postFragmentSource = getShaderSource("../Shaders/bloomFragment.glsl");
        
        postProcessFragmentShader = compileShader(postFragmentSource.c_str(), GL_FRAGMENT_SHADER);
        postProcessProgram = linkProgram(postProcessVertexShader, postProcessFragmentShader);*/
        
        const char *v = R"(
            #version 330 core
            
            layout(location = 0) in vec2 aPos;
            layout(location = 1) in vec2 aUV;
            
            out vec2 vUV;
            
            void main() {
                vUV = aUV;
                gl_Position = vec4(aPos, 0.0, 1.0);
            })";
        
        const char *f = R"(
            #version 330 core
            
            in vec2 vUV;
            out vec4 fragColor;
            
            uniform sampler2D screenTexture;
            
            void main() {
                fragColor = texture(screenTexture, vUV);
            }
            )";
        
        postProcessVertexShader   = compileShader(v,   GL_VERTEX_SHADER);
        postProcessFragmentShader = compileShader(f, GL_FRAGMENT_SHADER);
        
        postProcessProgram = linkProgram(postProcessVertexShader, postProcessFragmentShader);
        
        setupScreenQuad();
        
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
            if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
                 std::cerr << "GL DEBUG [" << id << "] Severity: " << severity << " Type: " << type << "\n" << message << '\n';
            }
        }, nullptr);
        
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        
        GLenum constructorError = glGetError();
        if(constructorError != GL_NO_ERROR) {
            std::cerr << "OpenGL Error during Renderer constructor: " << constructorError << '\n';
        }
    }
}

static bool isRendering = false;
static bool useShaders = false;
void Emulator::Renderer::display() {
    if(isRendering)
       return;
    
    isRendering = true;
    
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO[curTex]);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    int winWidth = WIDTH, winHeight = HEIGHT;
    glfwGetFramebufferSize(window, &winWidth, &winHeight);
    
    glViewport(0, 0, WIDTH, HEIGHT);
    //glViewport(0, 0, winWidth, winHeight);
    
    // Render scene
    glEnable(GL_BLEND);
    //glDisable(GL_BLEND);
    draw();
    glDisable(GL_BLEND);
    
    if (!useShaders) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        int winWidth, winHeight;
        glfwGetFramebufferSize(window, &winWidth, &winHeight);
        glViewport(0, 0, winWidth, winHeight);
        
        glUseProgram(postProcessProgram);
        
        glBindVertexArray(quadVAO);
        
        glActiveTexture(GL_TEXTURE0);
        
        if (!renderVRAM)
            glBindTexture(GL_TEXTURE_2D, sceneTex[curTex]);
        else
            glBindTexture(GL_TEXTURE_2D, gpu.vram->tex);
        
        glUniform1i(glGetUniformLocation(postProcessProgram, "screenTexture"), 0);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glBindVertexArray(0);
        
        isRendering = false;
        
        return;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Bloom threshold
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[0]);
    glViewport(0, 0, WIDTH, HEIGHT);
    
    glEnable(GL_BLEND);
    
    glUseProgram(bloomThresholdProgram);
    glBindVertexArray(quadVAO);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex[curTex]);
    
    glUniform1i(glGetUniformLocation(bloomThresholdProgram, "thresholdImg"), 0);
    glUniform1f(glGetUniformLocation(bloomThresholdProgram, "threshold"), threshold);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[1]);
    glViewport(0, 0, WIDTH, HEIGHT);
    
    glEnable(GL_BLEND);
    
    glUseProgram(blurProgram);
    
    for(int i = 0; i < bloomPasses; i++) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bloomTexture[horizontal]);
        //glGenerateMipmap(GL_TEXTURE_2D);
        glUniform1i(glGetUniformLocation(blurProgram, "image"), 0);
        glUniform1i(glGetUniformLocation(blurProgram, "horizontal"), horizontal ? 1 : 0);
        glUniform1f(glGetUniformLocation(blurProgram, "blurRadius"), blurRadius);
        
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        
        horizontal = !horizontal;
    }
    
    // If the number of passes were uneven,
    // it'd cause the screen to flicker
    horizontal = true;
    
    glDisable(GL_BLEND);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    //int winWidth = WIDTH, winHeight = HEIGHT;
    //glfwGetFramebufferSize(window, &winWidth, &winHeight);
    
    // Render scene
    glViewport(0, 0, winWidth, winHeight);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(postProcessProgram);
    glBindVertexArray(quadVAO);
    
    // Bind scene texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex[curTex]);
    //glGenerateMipmap(GL_TEXTURE_2D);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Bind bloom texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomTexture[1]);
    //glGenerateMipmap(GL_TEXTURE_2D);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    {
        // Set uniforms
        glUniform1i(glGetUniformLocation(postProcessProgram, "screenTexture"), 0);
        glUniform1i(glGetUniformLocation(postProcessProgram, "bloomTexture"), 1);
        glUniform2f(glGetUniformLocation(postProcessProgram, "uTextureSize"), WIDTH, HEIGHT);
        glUniform2f(glGetUniformLocation(postProcessProgram, "uOutputSize"), 3840.0f, 1920.0f);
        
        // Sensible defaults for PS1-style rendering
        glUniform1f(glGetUniformLocation(postProcessProgram, "uKernelB"), kernelB);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uKernelC"), kernelC);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uSharpness"), sharpness);
        glUniform1i(glGetUniformLocation(postProcessProgram, "uSampleRadius"), sampleRadius);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uLODBias"), lodBias);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uEdgeThreshold"), edgeThreshold);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uDitherStrength"), ditherStrength);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uNoiseStrength"), noiseStrength);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uContrast"), contrast);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uSaturation"), saturation);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uHalation"), halation);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uTime"), glfwGetTime());
        glUniform1f(glGetUniformLocation(postProcessProgram, "uEnableAdaptiveSharpening"), enableAdaptiveSharpening);
        
        glUniform1f(glGetUniformLocation(postProcessProgram, "uGamma"), gamma);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uScanline"), scanline);
        glUniform1f(glGetUniformLocation(postProcessProgram, "uBloomIntensity"), bloomIntensity);
        
        glUniform1i(glGetUniformLocation(postProcessProgram, "uEnableBloom"),     enableBloom ? 1 : 0);
        glUniform1i(glGetUniformLocation(postProcessProgram, "uEnableUpscaling"), enableUpscaling ? 1 : 0);
    }
    
    // Draw final quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindVertexArray(0);
    
    //glfwSwapBuffers(window);
    
    isRendering = false;
    
    /*positions.endFrame();
    colors.endFrame();
    uvs.endFrame();
    attributes.endFrame();*/
}

void Emulator::Renderer::renderFrame() {
    /*uint32_t fbX = gpu.displayVramXStart;
    uint32_t fbY = gpu.displayVramYStart;
    uint32_t fbW = gpu.hres.getResolution();                  // 256/320/368/512/640
    uint32_t fbH = (gpu.vres == VerticalRes::Y480Lines) 
             ? 480 
             : (gpu.display LineEnd - gpu.displayLineStart);*/
    
    //gpu.vram->copyToTexture(0, 0, 1024, 512, sceneTex[curTex]);
    
    display();
    
    curTex = 0;
    
    nVertices = 0;
}

void Emulator::Renderer::draw() {
    if(nVertices == 0) return;
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );
    
    glUseProgram(program);
    glBindVertexArray(VAO);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gpu.vram->tex);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sceneTex[0]);
    
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(nVertices));
    
    glBindVertexArray(0);
    glUseProgram(0);
}

void Emulator::Renderer::clear() {
    /*glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);*/
}

void Emulator::Renderer::pushLine(Emulator::Gpu::Position positions[], Emulator::Gpu::Color colors[], Emulator::Gpu::UV uvs[], Gpu::Attributes attributes) {
    if (nVertices + 2 > VERTEX_BUFFER_LEN) {
        nVertices = 0;
    }
    
    //_rasterizer.drawLine(positions, colors, uvs, attributes);
    
    float dx = positions[1].x - positions[0].x;
    float dy = positions[1].y - positions[0].y;
    
    float len = sqrtf(dx*dx + dy*dy);
    if (len == 0) {
        assert(false && "Uhhh len is 0?? (pushLine)");
        return;
    }
    
    dx /= len;
    dy /= len;
    
    float px = -dy;
    float py = dx;
    
    float thickness = 1;
    float ox = px * (thickness * 0.5f);
    float oy = py * (thickness * 0.5f);
    
    Emulator::Gpu::Position v0{ positions[0].x + ox, positions[0].y + oy };
    Emulator::Gpu::Position v1{ positions[1].x + ox, positions[1].y + oy };
    Emulator::Gpu::Position v2{ positions[1].x - ox, positions[1].y - oy };
    Emulator::Gpu::Position v3{ positions[0].x - ox, positions[0].y - oy };
    
    Emulator::Gpu::Color c0 = colors[0];
    Emulator::Gpu::Color c1 = colors[0];
    Emulator::Gpu::Color c2 = colors[1];
    Emulator::Gpu::Color c3 = colors[1];
    
    {
        Gpu::Position pos[3] = { v0, v1, v2 };
        Gpu::Color    col[3] = { c0, c1, c2 };
        pushTriangle(pos, col, {}, attributes);
    }
    
    {
        Gpu::Position pos[3] = { v2, v3, v0 };
        Gpu::Color    col[3] = { c2, c3, c0 };
        pushTriangle(pos, col, {}, attributes);
    }
}

void Emulator::Renderer::pushTriangle(Emulator::Gpu::Position positions[], Emulator::Gpu::Color colors[], Emulator::Gpu::UV uvs[], Gpu::Attributes attributes) {
    if(nVertices + 3 > VERTEX_BUFFER_LEN) {
        // Reset the buffer size
        nVertices = 0;
        
        //display();
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

void Emulator::Renderer::setDrawingOffset(int16_t x, int16_t y) {
    /**
     * Because my renderer is still...
     * isn't completed I need to manually do this,
     * for every game as otherwise, it causes,
     * the entire screen to constantly flicker
     */
    
    glUseProgram(program);
    
    //int16_t f = 256;
    //int16_t d = 128;
    
    //glUniform2f(offsetUni, x, y);
    glUniform2f(offsetUni, 0, 0);
    //lUniform2i(offsetUni, 0, 256);
    //glUniform2i(offsetUni, 0, 0); // GEX
    //glUniform2i(offsetUni, 120, 160); // Pepsi man
    //glUniform2i(offsetUni, 0, 0); // Ridge Racer
    //glUniform2f(offsetUni, f, d); // Crash Bandicoot
}

void Emulator::Renderer::setDrawingArea(const uint16_t left, const uint16_t right, const uint16_t top, const uint16_t bottom) const {
    glUseProgram(program);
    
    glUniform2i(drawingMinUni, left, top);
    glUniform2i(drawingMaxUni, right, bottom);
}

void Emulator::Renderer::setTextureWindow(uint8_t textureWindowXMask, uint8_t textureWindowYMask, uint8_t textureWindowXOffset, uint8_t textureWindowYOffset) {
    glUseProgram(program);
    
    glUniform4f(textureWindowUni, textureWindowXMask, textureWindowYMask, textureWindowXOffset, textureWindowYOffset);
}

void Emulator::Renderer::setSemiTransparencyMode(uint8_t semiTransparencyMode) const {
    glUseProgram(program);
    
    //printf("Got; %d\n", semiTransparencyMode);
    glUniform1i(semiTransparencyModeUni, semiTransparencyMode);
}

void Emulator::Renderer::bindFrameBuffer(GLuint buf) {
    glBindFramebuffer(GL_FRAMEBUFFER, buf);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

GLuint Emulator::Renderer::compileShader(const char* source, const GLenum shaderType) {
    const GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        
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
    std::filesystem::path absPath = std::filesystem::absolute(path);
    
    std::ifstream file(absPath/*, std::ios::in | std::ios::binary*/);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file at path: " << absPath << '\n';
        return {};
    }
    
    std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    
    // Close the file
    file.close();
    
    // Print loaded file full path
    std::cout << "Loaded shader from: " << absPath << '\n';
    
    return content;
}

GLuint Emulator::Renderer::createFrameBuffer(GLsizei width, GLsizei height, GLuint& textureId) {
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    
    glGenTextures(1, &textureId);
    
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    //glGenerateMipmap(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, /*GL_RGBA8*/GL_RGB5_A1, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << '\n';
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return framebuffer;
}

void Emulator::Renderer::setupScreenQuad() {
    float fullscreenQuad[] = {
        // positions   // texCoords
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
        
        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), fullscreenQuad, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}
