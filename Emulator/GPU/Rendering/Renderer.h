#pragma once

#include <iostream>
#include <SDL_render.h>
#include <SDL_video.h>
#include <vector>
#include <GL/glew.h>

#include "Buffer.h"

namespace Emulator {
    struct Position;
    struct Color;
    
    class Renderer {
    public:
        Renderer();
        
        void display();

    private:
        void draw();

    public:
        void pushLine(Emulator::Position* positions, Emulator::Color* colors);
        void pushTriangle(Emulator::Position* positions, Emulator::Color* colors);
        void pushQuad(Emulator::Position* positions, Emulator::Color* colors);
        void pushRectangle(Emulator::Position* positions, Emulator::Color* colors);
        
        void setDrawingOffset(int32_t x, int32_t y) {
            glUniform2i(offsetUni, x, y);
        }
        
        void setDrawingArea(int32_t right, int32_t bottom) {
            glUniform2i(drawingUni, right, bottom);
        }
        
        GLuint compileShader(const char* source, GLenum shaderType);
        GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);
        GLuint getProgramAttrib(GLuint program, const std::string& attr) {
            GLint index = glGetAttribLocation(program, attr.c_str());
            
            if(index < 0) {
                std::cerr << "Attribute " << attr << " was not found in the program\n";
                return -1;
            }
            
            return static_cast<GLuint>(index);
        }
        
        std::string getShaderSource(const std::string& path);
        
        GLuint createFrameBuffer(GLsizei width, GLsizei height);
    public:
        // Shader parameters
        GLuint vertexShader;
        GLuint fragmentShader;
        GLuint program;
        
        // Frame buffers
        GLuint mainFramebuffer;
        GLuint vRamFramebuffer;
        
        // Vertex Array Object
        GLuint VAO;
        
        // Uniforms
        GLint offsetUni;
        GLint drawingUni;
        
        // Buffer contains the vertices positions
        Buffer<Position> positions;
        
        // Buffer contains the vertices colors
        Buffer<Color> colors;
        
        // Current number of vertices in the buffers
        uint32_t nVertices;
        
        SDL_Window* window;
        SDL_GLContext sdl_context;
        SDL_Renderer* renderer;
    };
}
