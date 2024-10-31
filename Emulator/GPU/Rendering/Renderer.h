﻿#pragma once

#include <iostream>

#include "Buffer.h"

//#include <GL/glew.h>
//#define GLFW_INCLUDE_NONE
/*#include <GLFW/glfw3.h>*/

class GLFWwindow;

namespace Emulator {
    struct Position;
    struct Color;
    struct Attributes;
    
    class Renderer {
    public:
        Renderer();
        
        void display();
        
    private:
        void draw();
        
    public:
        void pushLine(Emulator::Position* positions, Emulator::Color* colors, Emulator::Attributes attributes);
        void pushTriangle(Emulator::Position* positions, Emulator::Color* colors, Emulator::Attributes attributes);
        void pushQuad(Emulator::Position* positions, Emulator::Color* colors, Emulator::Attributes attributes);
        void pushRectangle(Emulator::Position* positions, Emulator::Color* colors, Emulator::Attributes attributes);
        
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
        
        // Buffer contains the vertices attributes
        Buffer<Attributes> attributes;
        
        // Current number of vertices in the buffers
        uint32_t nVertices;
        
        GLFWwindow* window;
    };
}
