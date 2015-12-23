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

#include "audio_output_converter.h"

#define NUM_CHANNELS (2)

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

// void AudioOutputConverter::set_buffer(float * buffer, size_t bufferSize)
// {
//     m_buffer = buffer;
//     m_bufferSize = bufferSize;
// }

void AudioOutputConverter::fill_buffer(uint32_t bufferIndex, AudioOutput::Buffer & buffer)
{
    assert(m_source);
    m_source->process(m_buffer); //, m_bufferSize);

    int16_t * out = (int16_t *)buffer.data;
    int sampleCount = buffer.dataSize / sizeof(int16_t) / NUM_CHANNELS;
    assert(sampleCount <= m_buffer.get_count());

    int i;
    for (i = 0; i < sampleCount; ++i)
    {
        float sample = m_buffer[i];
        int16_t intSample = int16_t(sample * 32767.0);
        *out++ = intSample;
        *out++ = intSample;
    }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
