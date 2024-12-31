#pragma once

#include "Buffer.h"
#include "Rasterizer.h"

#include "glm/vec2.hpp"

/*#ifdef _WIN32
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#endif*/

class GLFWwindow;

namespace Emulator {
    class Renderer {
    public:
        Renderer(Emulator::Gpu& gpu);
        
        void display();
        void displayVRam();
        void clear();
        
    private:
        void draw() const;
        
    public:
        void pushLine(Emulator::Gpu::Position positions[], Emulator::Gpu::Color colors[], Emulator::Gpu::UV uvs[], Emulator::Gpu::Attributes attributes);
        void pushTriangle(Emulator::Gpu::Position positions[], Emulator::Gpu::Color colors[], Emulator::Gpu::UV uvs[], Emulator::Gpu::Attributes attributes);
        void pushQuad(Emulator::Gpu::Position positions[], Emulator::Gpu::Color colors[], Emulator::Gpu::UV uvs[], Emulator::Gpu::Attributes attributes);
        void pushRectangle(Emulator::Gpu::Position positions[], Emulator::Gpu::Color colors[], Emulator::Gpu::UV uvs[], Emulator::Gpu::Attributes attributes);
        
    private:
        bool checkIfWithin(Emulator::Gpu::Position positions[], int length[]);
        
    public:
        void setDrawingOffset(int16_t x, int16_t y);
        void setDrawingArea(int16_t right, int16_t bottom);
        void setTextureWindow(uint8_t textureWindowXMask, uint8_t textureWindowYMask, uint8_t textureWindowXOffset, uint8_t textureWindowYOffset);
        
        void bindFrameBuffer(GLuint buf);
        
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
        
        // Vertex Array Object
        GLuint VAO;
        
        // Uniforms
        GLint offsetUni;
        GLint drawingUni;
        //GLint textureDepthUni;
        GLint textureWindowUni;
        
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
        
    private:
        Emulator::Gpu& gpu;
        
        Rasterizer _rasterizer;
    };
}
