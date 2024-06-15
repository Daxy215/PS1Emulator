#pragma once
#include <iostream>
#include <SDL_video.h>
#include <GL/glew.h>
//#include <GL/wglew.h>

#include "Buffer.h"

namespace Emulator {
    class Position;
    class Color;
}

namespace Emulator {
    class Renderer {
    public:
        Renderer();
        
        void display();
        void draw();
        
        void pushTriangle(Emulator::Position* positions, Emulator::Color* colors);
        void pushQuad(Emulator::Position* positions, Emulator::Color* colors);
        
        void setDrawingOffset(int32_t x, int32_t y) {
            //draw();
            
            glUniform2i(offsetUni, x, y);
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
    public:
        GLuint vertexShader;
        GLuint fragmentShader;
        GLuint program;
        
        // Vertex Array Object
        GLuint VAO;
        
        // Uniforms
        GLuint offsetUni;
        
        // Buffer contains the vertices positions
        Buffer<Position> positions;
        
        // Buffer contains the vertices colors
        Buffer<Color> colors;
        
        // Current number of vertices in the buffers
        uint32_t nVertices;
        
        SDL_GLContext sdl_context;
        SDL_Window* window;
    };
}
