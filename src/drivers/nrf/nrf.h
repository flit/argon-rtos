/*
 * Copyright (c) 2014 Immo Software
 * All rights reserved.
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
#if !defined(_NRF_H_)
#define _NRF_H_

#include "mbed.h"
#include "argon/argon.h"

//------------------------------------------------------------------------------
// Classes
//------------------------------------------------------------------------------

/*!
 * @brief Driver for Nordic Semiconductor nRF24L01
 */
class NordicRadio
{
public:
    NordicRadio(PinName ce, PinName cs, PinName sck, PinName mosi, PinName miso, PinName irq);
    virtual ~NordicRadio();

    void init(uint32_t address);
    void setReceiveSem(Ar::Semaphore * sem) { m_receiveSem = sem; }
    void setChannel(uint8_t channel);
    uint8_t getChannel() const { return m_channel; }

    uint32_t receive(uint8_t * buffer, uint32_t timeout_ms=0);
    bool send(uint32_t address, const uint8_t * buffer, uint32_t count, uint32_t timeout_ms=0);

    void startReceive();
    void stopReceive();
    uint32_t readPacket(uint8_t * buffer);
    bool isPacketAvailable();
    void clearReceiveFlag();

    void dump();

protected:
    SPI m_spi;
    DigitalOut m_cs;
    DigitalOut m_ce;
    InterruptIn m_irq;
    uint32_t m_stationAddress;
    uint8_t m_channel;

    enum mode_t {
        kPRX = 0,
        kPTX = 1
    };

    mode_t m_mode;
    Ar::Semaphore * m_receiveSem;

    void readRegister(uint8_t address, uint8_t * data, uint32_t count);
    void writeRegister(uint8_t address, const uint8_t * data, uint32_t count);
    uint8_t readRegister(uint8_t address);
    void writeRegister(uint8_t address, uint8_t data);

    void readCommand(uint8_t command, uint8_t * buffer, uint32_t count);
    void writeCommand(uint8_t command, const uint8_t * buffer=NULL, uint32_t count=0);

    void irqHandler(void);
};

#endif // _NRF_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
