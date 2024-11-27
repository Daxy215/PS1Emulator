#pragma once

#include <iostream>

#include "Buffer.h"
#include "../Gpu.h"

/*#ifdef _WIN32
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#endif*/

namespace Emulator {
    class Gpu;
}

class GLFWwindow;

namespace Emulator {
    class Renderer {
    public:
        Renderer();
        
        void display();
        void displayVRam();
        
    private:
        void draw();
        
    public:
        void pushLine(Emulator::Gpu::Position* positions, Emulator::Gpu::Color* colors, Emulator::Gpu::UV* uvs, Emulator::Gpu::Attributes attributes);
        void pushTriangle(Emulator::Gpu::Position* positions, Emulator::Gpu::Color* colors, Emulator::Gpu::UV* uvs, Emulator::Gpu::Attributes attributes);
        void pushQuad(Emulator::Gpu::Position* positions, Emulator::Gpu::Color* colors, Emulator::Gpu::UV* uvs, Emulator::Gpu::Attributes attributes);
        void pushRectangle(Emulator::Gpu::Position* positions, Emulator::Gpu::Color* colors, Emulator::Gpu::UV* uvs, Emulator::Gpu::Attributes attributes);
        
        void setDrawingOffset(int16_t x, int16_t y);
        void setDrawingArea(int16_t right, int16_t bottom);
        void setTextureDepth(int textureDepth);
        
        void updateVramTextures(uint32_t texture4, uint32_t texture8, uint32_t texture16);
        
        GLuint compileShader(const char* source, GLenum shaderType);
        GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);
        GLuint getProgramAttrib(GLuint program, const std::string& attr);
        
        std::string getShaderSource(const std::string& path);
        
        GLuint createFrameBuffer(GLsizei width, GLsizei height, GLuint& textureId);
        
    private:
        // GLFW callbacks
        /*static void APIENTRY openglDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                            GLsizei length, const GLchar *message, const void *userParam);*/
        
    public:
        // Shader parameters
        GLuint vertexShader;
        GLuint fragmentShader;
        
        // TODO; For testing imma make this static
        static GLuint program;
        
        // Frame buffers
        GLuint mainFramebuffer;
        GLuint vRamFramebuffer;
        
        // Vertex Array Object
        GLuint VAO;
        
        // Uniforms
        GLint offsetUni;
        GLint drawingUni;
        GLint textureDepthUni;
        
        // Buffer contains the vertices positions
        Buffer<Gpu::Position> positions;
        
        // Buffer contains the vertices colors
        Buffer<Gpu::Color> colors;
        
        // Buffer contains the texture coordinates
        Buffer<Gpu::UV> uvs;
        
        // Buffer contains the vertices attributes
        Buffer<Gpu::Attributes> attributes;
        
        // Current number of vertices in the buffers
        uint32_t nVertices;
        
        GLFWwindow* window;
    };
}
