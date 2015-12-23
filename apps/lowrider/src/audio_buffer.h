/*
 * Copyright (c) 2015 Immo Software
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its contributors may
 *   be used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#if !defined(_AUDIO_BUFFER_H_)
#define _AUDIO_BUFFER_H_

#include <stddef.h>

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*!
 * @brief Represents a single channel audio buffer.
 *
 * Objects of this class only wrap the actual audio data buffers, they do not own them. Multiple
 * instances may reference the same underlying data buffer. If a data buffer is dynamically allocated,
 * it must be freed using a mechanism outside of this class, and any instances of this class that
 * reference the freed buffer must be properly updated or disposed.
 */
class AudioBuffer
{
public:
    AudioBuffer() : m_samples(NULL), m_count(0) {}
    AudioBuffer(float * samples, size_t count) : m_samples(samples), m_count(count) {}
    AudioBuffer(const AudioBuffer & other) : m_samples(other.m_samples), m_count(other.m_count) {}
    AudioBuffer & operator = (const AudioBuffer & other)
    {
        m_samples = other.m_samples;
        m_count = other.m_count;
        return *this;
    }

    ~AudioBuffer() {}

    void set(float * samples, size_t count)
    {
        m_samples = samples;
        m_count = count;
    }

    float * get_buffer() { return m_samples; }
    const float * get_buffer() const { return m_samples; }
    size_t get_count() const { return m_count; }

    void clear() { set_scalar(0.0f); }
    void set_scalar(float value);
    void multiply_scalar(float value);
    void multiply_vector(float * vector);

    operator float * () { return m_samples; }
    operator const float * () const { return m_samples; }
    operator bool () { return m_samples != NULL; }

//     class Cursor
//     {
//     public:
//         Cursor(AudioBuffer & buffer) : m_buffer(buffer) {}
//         ~Cursor() {}
//
//     protected:
//         AudioBuffer & m_buffer;
//     };
//
//     Cursor get_cursor() { return Cursor(*this); }

protected:
    float * m_samples;
    size_t m_count;
};

#endif // _AUDIO_BUFFER_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
