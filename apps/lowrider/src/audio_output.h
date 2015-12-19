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
#if !defined(_AUDIO_OUT_H_)

#include "argon/argon.h"
#include "fsl_sai_edma.h"
#include "fsl_sgtl5000.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*!
 * @brief Audio output port.
 */
class AudioOutput
{
public:
    struct Buffer
    {
        uint8_t * data;
        size_t dataSize;
    };

    typedef void (*fill_callback_t)(uint32_t bufferIndex, Buffer * buffer);

    AudioOutput() {}
    ~AudioOutput() {}

    void init(const sai_transfer_format_t * format, I2C_Type * i2cBase, i2c_master_handle_t * i2c);
    void add_buffer(Buffer * newBuffer);
    void set_callback(fill_callback_t callback) { m_callback = callback; }

    void start();

    void dump_sgtl5000();

protected:

    enum {
        kMaxBufferCount = 2
    };

    sai_transfer_format_t m_format;
    sai_edma_handle_t m_txHandle;
    edma_handle_t m_dmaHandle;
    sgtl_handle_t m_codecHandle;
    Ar::Semaphore m_transferDone;
    Ar::ThreadWithStack<512> m_audioThread;
    Buffer m_buffers[kMaxBufferCount];
    uint32_t m_bufferCount;
    fill_callback_t m_callback;

    void audio_thread();

    static void sai_callback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData);

};

#endif // _AUDIO_OUT_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
