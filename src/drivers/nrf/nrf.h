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

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

//! @brief nRF24L01 commands.
enum {
    knRFCommand_R_REGISTER = 0x00,
    knRFCommand_W_REGISTER = 0x20,
    knRFCommand_R_RX_PAYLOAD = 0x61,
    knRFCommand_W_TX_PAYLOAD = 0xa0,
    knRFCommand_FLUSH_TX = 0xe1,
    knRFCommand_FLUSH_RX = 0xe2,
    knRFCommand_REUSE_TX_PL = 0xe3,
    knRFCommand_ACTIVATE = 0x50,
    knRFCommand_R_RX_PL_WID = 0x60,
    knRFCommand_W_ACK_PAYLOAD = 0xa7,
    knRFCommand_W_TX_PAYLOAD_NOACK = 0xb0,
    knRFCommand_NOP = 0xff,
};

//! @brief nRF24L01 register addresses.
enum {
    knRFRegisterAddressMask = 0x1f,
    knRFRegister_CONFIG = 0x00,
    knRFRegister_EN_AA = 0x01,
    knRFRegister_EN_RXADDR = 0x02,
    knRFRegister_SETUP_AW = 0x03,
    knRFRegister_SETUP_RETR = 0x04,
    knRFRegister_RF_CH = 0x05,
    knRFRegister_RF_SETUP = 0x06,
    knRFRegister_STATUS = 0x07,
    knRFRegister_OBSERVE_TX = 0x08,
    knRFRegister_CD = 0x09,
    knRFRegister_RX_ADDR_P0 = 0x0a,
    knRFRegister_TX_ADDR = 0x10,
    knRFRegister_RX_PW_P0 = 0x11,
    knRFRegister_FIFO_STATUS = 0x17,
    knRFRegister_DYNPD = 0x1c,
    knRFRegister_FEATURE = 0x1d,
};

//! @brief nRF24L01 CONFIG register bitfield masks.
enum {
    knRF_CONFIG_MASK_RX_DR = (1 << 6),
    knRF_CONFIG_MASK_TX_DS = (1 << 5),
    knRF_CONFIG_MASK_MAX_RT = (1 << 4),
    knRF_CONFIG_EN_CRC = (1 << 3),
    knRF_CONFIG_CRCO = (1 << 2),
    knRF_CONFIG_PWR_UP = (1 << 1),
    knRF_CONFIG_PRIM_RX = (1 << 0),
};

//! @brief nRF24L01 RF_SETUP register bitfield masks.
enum {
    knRF_RF_SETUP_PLL_LOCK = (1 << 4),
    knRF_RF_SETUP_RF_DR = (1 << 3),
    knRF_RF_SETUP_RF_PWR = (3 << 1),
    knRF_RF_SETUP_LNA_HCURR = (1 << 0),
};

//! @brief nRF24L01 STATUS register bitfield masks.
enum {
    knRF_STATUS_RX_DR = (1 << 6),
    knRF_STATUS_TX_DS = (1 << 5),
    knRF_STATUS_MAX_RT = (1 << 4),
    knRF_STATUS_RX_P_NO = (7 << 1),
    knRF_STATUS_TX_FULL = (1 << 0),
};

//! @brief nRF24L01 FEATURE register bitfield masks.
enum {
    knRF_FEATURE_EN_DPL = (1 << 2),
    knRF_FEATURE_EN_ACK_PAY = (1 << 1),
    knRF_FEATURE_EN_DYN_ACK = (1 << 0),
};

/*!
 * @brief Driver for Nordic Semiconductor nRF24L01
 */
class NordicRadio
{
public:
    NordicRadio(PinName ce, PinName cs, PinName sck, PinName mosi, PinName miso, PinName irq);
    virtual ~NordicRadio();

    void init(uint32_t address);
    void setChannel(uint8_t channel);
    uint8_t getChannel() const { return m_channel; }

    uint8_t receive(uint8_t * buffer, uint32_t timeout_ms=0);
    bool send(uint32_t address, const uint8_t * buffer, uint32_t count, uint32_t timeout_ms=0);

    void dump();

protected:
    SPI m_spi;
    DigitalOut m_cs;
    DigitalOut m_ce;
    InterruptIn m_irq;
    uint32_t m_stationAddress;
    uint8_t m_channel;

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
