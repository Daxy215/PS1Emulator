#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <GL/glew.h>

constexpr unsigned int VERTEX_BUFFER_LEN = 640 * 1024;

template<typename T>
struct Buffer {
    uint32_t object;
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
        
        GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
        
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, nullptr, access);
        map = static_cast<T*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize, access));
        
        std::memset(map, 0, bufferSize);
    }
    
    void set(uint32_t index, const T& value) const {
        if (index >= VERTEX_BUFFER_LEN) {
            throw std::runtime_error("buffer overflow!");
        }
        
        if (!map) {
            throw std::runtime_error("buffer not mapped");
            return;
        }
        
        map[index] = value;
        auto var = map[index];
    }
};