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

#include "audio_output.h"
#include "fsl_dmamux.h"
#include <stdio.h>

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

void AudioOutput::init(const sai_transfer_format_t * format, I2C_Type * i2cBase, i2c_master_handle_t * i2c)
{
    m_format = *format;
    m_bufferCount = 0;
    m_transferDone.init("txdone", 0);
    m_source = NULL;

    // Create EDMA handle
    edma_config_t dmaConfig = {0};
    EDMA_GetDefaultConfig(&dmaConfig);
    EDMA_Init(DMA0, &dmaConfig);
    EDMA_CreateHandle(&m_dmaHandle, DMA0, 0);

    DMAMUX_Init(DMAMUX0);
    DMAMUX_SetSource(DMAMUX0, 0, kDmaRequestMux0I2S0Tx & 0xff);
    DMAMUX_EnableChannel(DMAMUX0, 0);

    // Init SAI module
    sai_config_t saiConfig;
    SAI_TxGetDefaultConfig(&saiConfig);
    saiConfig.protocol = kSAI_BusLeftJustified;
    saiConfig.masterSlave = kSAI_Master;
    SAI_TxInit(I2S0, &saiConfig);
    SAI_TransferTxCreateHandleEDMA(I2S0, &m_txHandle, sai_callback, &m_transferDone, &m_dmaHandle);

    uint32_t mclkSourceClockHz = CLOCK_GetFreq(kCLOCK_CoreSysClk);
    SAI_TransferTxSetFormatEDMA(I2S0, &m_txHandle, &m_format, mclkSourceClockHz, m_format.masterClockHz);

    // Configure the sgtl5000.
    sgtl_config_t sgtlConfig = {
        .route = kSGTL_RoutePlayback,
        .bus = kSGTL_BusLeftJustified,
        .master_slave = false,
    };
    m_codecHandle.base = i2cBase;
    m_codecHandle.i2cHandle = i2c;
    SGTL_Init(&m_codecHandle, &sgtlConfig);
    SGTL_ConfigDataFormat(&m_codecHandle, m_format.masterClockHz, m_format.sampleRate_Hz, m_format.bitWidth);

    // Create audio thread.
    m_audioThread.init("audio", this, &AudioOutput::audio_thread, 180, kArSuspendThread);
}

void AudioOutput::add_buffer(Buffer * newBuffer)
{
    m_buffers[m_bufferCount] = *newBuffer;
    ++m_bufferCount;
    assert(m_bufferCount <= kMaxBufferCount);
}

void AudioOutput::start()
{
    m_audioThread.resume();
}

void AudioOutput::audio_thread()
{
    sai_transfer_t xfer;
    uint32_t currentBuffer = 0;
    Buffer buf;

    // Pre-fill and queue all buffers.
    int i;
    for (i = 0; i < m_bufferCount; i ++)
    {
        buf.data = m_buffers[i].data;
        buf.dataSize = m_buffers[i].dataSize;
        if (m_source)
        {
            m_source->fill_buffer(i, buf);
        }

        xfer.data = buf.data;
        xfer.dataSize = buf.dataSize;
        SAI_TransferSendEDMA(I2S0, &m_txHandle, &xfer);
    }

    // Wait for buffers to complete, then refill and enqueue them.
    while (1)
    {
        // Block until the semaphore is put by the SAI transfer complete callback.
        m_transferDone.get();

        buf.data = m_buffers[currentBuffer].data;
        buf.dataSize = m_buffers[currentBuffer].dataSize;
        if (m_source)
        {
            m_source->fill_buffer(currentBuffer, buf);
        }

        xfer.data = buf.data;
        xfer.dataSize = buf.dataSize;
        SAI_TransferSendEDMA(I2S0, &m_txHandle, &xfer);

        currentBuffer = (currentBuffer + 1) % m_bufferCount;
    }
}

void AudioOutput::sai_callback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData)
{
    Ar::Semaphore * sem = (Ar::Semaphore *)userData;
    assert(sem);
    sem->put();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
