﻿#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <GL/glew.h>

constexpr unsigned int VERTEX_BUFFER_LEN = 64 * 1024;

template<typename T>
struct Buffer {
    GLuint object;
    T* map;
    
    Buffer() : object(0), map(nullptr) {
        
    }
    
    ~Buffer() {
        if (object != 0) {
            glBindBuffer(GL_ARRAY_BUFFER, object);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glDeleteBuffers(1, &object);
        }
    }
    
    void create() {
        // Generate and bind the buffer object
        glGenBuffers(1, &object);
        glBindBuffer(GL_ARRAY_BUFFER, object);
        
        // Compute the size of the buffer
        size_t elementSize = sizeof(T);
        GLsizeiptr bufferSize = elementSize * VERTEX_BUFFER_LEN;
        
        // Write only persistent mapping. Not coherent
        glBufferStorage(GL_ARRAY_BUFFER,
            bufferSize, nullptr,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
        
        map = static_cast<T*>(glMapBufferRange(GL_ARRAY_BUFFER, 0,
            bufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT));
        
        // Check for errors
        GLenum err = glGetError();
        if (err!= GL_NO_ERROR) {
            std::cerr << "OpenGL Error: " << err << std::endl;
        }
        
        // Rest the buffer to avoid bugs if uninitialized  memory is accessed
        memset(map, 0, bufferSize);
        /*std::vector<T> defaultValues(VERTEX_BUFFER_LEN, T());
        glBufferStorage(GL_ARRAY_BUFFER, 0, bufferSize, defaultValues.data());*/
    }
    
    void set(uint32_t index, const T& value) {
        if (index >= VERTEX_BUFFER_LEN) {
            throw std::runtime_error("buffer overflow!");
        }
        
        if (!map) {
            throw std::runtime_error("buffer not mapped");
        }
        
        map[index] = value;
    }
};