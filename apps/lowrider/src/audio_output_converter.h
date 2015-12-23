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
#if !defined(_AUDIO_OUT_CONVERTER_H_)
#define _AUDIO_OUT_CONVERTER_H_

#include "audio_output.h"
#include "audio_filter.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*!
 * @brief Output source to convert float buffers to the codec output format.
 */
class AudioOutputConverter : public AudioOutput::Source
{
public:
    AudioOutputConverter() : m_source(NULL) {}
    virtual ~AudioOutputConverter() {}

//     void set_buffer(float * buffer, size_t bufferSize);
    void set_buffer(AudioBuffer & buffer) { m_buffer = buffer; }
    void set_source(AudioFilter * source) { m_source = source; }

    virtual void fill_buffer(uint32_t bufferIndex, AudioOutput::Buffer & buffer);

protected:
//     float * m_buffer;
//     size_t m_bufferSize;
    AudioBuffer m_buffer;
    AudioFilter * m_source;
};

#endif // _AUDIO_OUT_CONVERTER_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
