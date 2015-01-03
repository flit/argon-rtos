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

#include "nrf.h"
#include <stdio.h>
#include <assert.h>

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

inline void word_to_array(uint32_t value, uint8_t * buf)
{
    buf[0] = value & 0xff;
    buf[1] = (value >> 8) & 0xff;
    buf[2] = (value >> 16) & 0xff;
    buf[3] = (value >> 24) & 0xff;
}

NordicRadio::NordicRadio(PinName ce, PinName cs, PinName sck, PinName mosi, PinName miso, PinName irq)
:   m_spi(mosi, miso, sck),
    m_cs(cs),
    m_ce(ce),
    m_irq(irq),
    m_stationAddress(0),
    m_channel(0)
{
    m_spi.format(8, 0);
    m_spi.frequency();
    m_cs = 1;
    m_ce = 0;
    m_irq.mode(PullUp);
    m_irq.fall(this, &NordicRadio::irqHandler);
}

NordicRadio::~NordicRadio()
{
}

void NordicRadio::init(uint32_t address)
{
    uint8_t buf[4];

    // Enable dynamic payload size. It's done in this loop to handle the case
    // where the nRF was previously configured. If we send the ACTIVATE command again,
    // it will disable the features, which is not what we want. So we have to test
    // for the FEATURE register to be active first.
    //
    // A for loop is used so it's impossible to get stuck here.
    int i;
    for (i=0; i < 2; ++i)
    {
        // First try writing the feature register and reading it back.
        writeRegister(knRFRegister_FEATURE, knRF_FEATURE_EN_DPL);
        uint8_t feature = readRegister(knRFRegister_FEATURE);

        // Activate features if the feature register is disabled and reads as 0.
        if (feature == 0)
        {
            buf[0] = 0x73;
            writeCommand(knRFCommand_ACTIVATE, buf, 1);
        }
        else
        {
            break;
        }
    }

    // Set address width to 4 bytes.
    writeRegister(knRFRegister_SETUP_AW, 0x02);

    // Set tx and rx addresses to the same.
    m_stationAddress = address;
    word_to_array(address, buf);
    writeRegister(knRFRegister_TX_ADDR, buf, 4);
    writeRegister(knRFRegister_RX_ADDR_P0, buf, 4);

    // Configure and enable pipe 0 with dynamic payload and set its width to max.
    writeRegister(knRFRegister_DYNPD, 1);
    writeRegister(knRFRegister_EN_RXADDR, 1);
    writeRegister(knRFRegister_RX_PW_P0, 32);

    // Set default channel. This also clears PLOS_CNT.
    setChannel(2);

    // Disable interrupts.
    uint8_t config = readRegister(knRFRegister_CONFIG);
    config |= knRF_CONFIG_MASK_RX_DR | knRF_CONFIG_MASK_TX_DS | knRF_CONFIG_MASK_MAX_RT;
    writeRegister(knRFRegister_CONFIG, config);

    // Flush FIFOs.
    writeCommand(knRFCommand_FLUSH_TX);
    writeCommand(knRFCommand_FLUSH_RX);

    // Clear all flags.
    writeRegister(knRFRegister_STATUS, knRF_STATUS_RX_DR | knRF_STATUS_TX_DS | knRF_STATUS_MAX_RT);

    // Power up.
    writeRegister(knRFRegister_CONFIG, config | knRF_CONFIG_PWR_UP);

    // Wait until radio is powered up (>=1.5ms)
    wait_ms(2);
}

void NordicRadio::setChannel(uint8_t channel)
{
    m_channel = channel & 0x7f;
    writeRegister(knRFRegister_RF_CH, channel);
}

uint8_t NordicRadio::receive(uint8_t * buffer, uint32_t timeout_ms)
{
    // Write RX address.
    uint8_t addressBuf[4];
    word_to_array(m_stationAddress, addressBuf);
    writeRegister(knRFRegister_RX_ADDR_P0, addressBuf, 4);

    // Set RX mode.
    uint8_t config = readRegister(knRFRegister_CONFIG);
    writeRegister(knRFRegister_CONFIG, config | knRF_CONFIG_PRIM_RX);

    // Enable reception.
    m_ce = 1;

    // Spin until we get a packet.
    Timer timeout;
    timeout.start();
    while (timeout_ms == 0 || timeout.read_ms() <= timeout_ms)
    {
        uint8_t status = readRegister(knRFRegister_STATUS);
        if (status & knRF_STATUS_RX_DR)
        {
            break;
        }
    }

    // Disable reception.
    m_ce = 0;

    // Read received byte count.
    uint8_t count;
    readCommand(knRFCommand_R_RX_PL_WID, &count, 1);

    // Flush RX FIFO if count is invalid.
    if (count > 32)
    {
        writeCommand(knRFCommand_FLUSH_RX);
        count = 0;
    }
    else
    {
        // Read packet.
        readCommand(knRFCommand_R_RX_PAYLOAD, buffer, count);
    }

    // Clear rx flag.
    writeRegister(knRFRegister_STATUS, knRF_STATUS_RX_DR);

    return count;
}

bool NordicRadio::send(uint32_t address, const uint8_t * buffer, uint32_t count, uint32_t timeout_ms)
{
    assert(count <= 32);

    // Write TX and RX addresses.
    uint8_t addressBuf[4];
    word_to_array(address, addressBuf);
    writeRegister(knRFRegister_TX_ADDR, addressBuf, 4);
    writeRegister(knRFRegister_RX_ADDR_P0, addressBuf, 4);

    // Clear tx flags.
    writeRegister(knRFRegister_STATUS, knRF_STATUS_TX_DS | knRF_STATUS_MAX_RT);

    // Set TX mode.
    uint8_t config = readRegister(knRFRegister_CONFIG);
    writeRegister(knRFRegister_CONFIG, config & ~knRF_CONFIG_PRIM_RX);

    // Fill the TX FIFO.
    writeCommand(knRFCommand_W_TX_PAYLOAD, buffer, count);

    // Pulse CE to send the packet.
    m_ce = 1;
    wait_ms(10);
    m_ce = 0;

    // Wait until we get an ack or timeout.
    Timer timeout;
    timeout.start();
    uint8_t status = 0;
    while (timeout_ms == 0 || timeout.read_ms() <= timeout_ms)
    {
        status = readRegister(knRFRegister_STATUS);
        if (status & (knRF_STATUS_TX_DS | knRF_STATUS_MAX_RT))
        {
            break;
        }
    }

    // Clear the tx flags.
    writeRegister(knRFRegister_STATUS, knRF_STATUS_TX_DS | knRF_STATUS_MAX_RT);

    return (status & knRF_STATUS_TX_DS);
}

void NordicRadio::readRegister(uint8_t address, uint8_t * data, uint32_t count)
{
    readCommand(knRFCommand_R_REGISTER | (address & knRFRegisterAddressMask), data, count);
}

void NordicRadio::writeRegister(uint8_t address, const uint8_t * data, uint32_t count)
{
    writeCommand(knRFCommand_W_REGISTER | (address & knRFRegisterAddressMask), data, count);
}

uint8_t NordicRadio::readRegister(uint8_t address)
{
    uint8_t value;
    readRegister(address, &value, 1);
    return value;
}

void NordicRadio::writeRegister(uint8_t address, uint8_t data)
{
    writeRegister(address, &data, 1);
}

void NordicRadio::readCommand(uint8_t command, uint8_t * buffer, uint32_t count)
{
    assert(count <= 32);

    m_cs = 0;
    m_spi.write(command);
    while (count--)
    {
        *buffer++ = m_spi.write(0x00);
    }
    m_cs = 1;
}

void NordicRadio::writeCommand(uint8_t command, const uint8_t * buffer, uint32_t count)
{
    assert(count <= 32);

    m_cs = 0;
    m_spi.write(command);
    while (count--)
    {
        m_spi.write(*buffer++);
    }
    m_cs = 1;
}

void NordicRadio::irqHandler(void)
{
}

void NordicRadio::dump()
{
#if DEBUG
    uint8_t config = readRegister(knRFRegister_CONFIG);
    uint8_t en_aa = readRegister(knRFRegister_EN_AA);
    uint8_t en_rxaddr = readRegister(knRFRegister_EN_RXADDR);
    uint8_t setup_aw = readRegister(knRFRegister_SETUP_AW);
    uint8_t setup_retr = readRegister(knRFRegister_SETUP_RETR);
    uint8_t rf_ch = readRegister(knRFRegister_RF_CH);
    uint8_t rf_setup = readRegister(knRFRegister_RF_SETUP);
    uint8_t status = readRegister(knRFRegister_STATUS);
    uint8_t observe_tx = readRegister(knRFRegister_OBSERVE_TX);
    uint8_t cd = readRegister(knRFRegister_CD);
    uint8_t rx_pw_p0 = readRegister(knRFRegister_RX_PW_P0);
    uint8_t fifo_status = readRegister(knRFRegister_FIFO_STATUS);
    uint8_t dynpd = readRegister(knRFRegister_DYNPD);
    uint8_t feature = readRegister(knRFRegister_FEATURE);

    uint8_t rx_addr_p0[5];
    readRegister(knRFRegister_RX_ADDR_P0, rx_addr_p0, sizeof(rx_addr_p0));

    uint8_t tx_addr[5];
    readRegister(knRFRegister_TX_ADDR, tx_addr, sizeof(tx_addr));

    printf(
"CONFIG=0x%02x      EN_AA=0x%02x        EN_RXADDR=0x%02x\n"
"SETUP_AW=0x%02x    SETUP_RETR=0x%02x   RF_CH=0x%02x\n"
"RF_SETUP=0x%02x    STATUS=0x%02x       OBSERVE_TX=0x%02x\n"
"CD=0x%02x          RX_PW_P0=0x%02x     FIFO_STATUS=0x%02x\n"
"DYNPD=0x%02x       FEATURE=0x%02x\n",
        config, en_aa, en_rxaddr,
        setup_aw, setup_retr, rf_ch,
        rf_setup, status, observe_tx,
        cd, rx_pw_p0, fifo_status,
        dynpd, feature);

    printf("RX_ADDR_P0={%02x %02x %02x %02x %02x}\nTX_ADDR={%02x %02x %02x %02x %02x}\n",
        rx_addr_p0[0], rx_addr_p0[1], rx_addr_p0[2], rx_addr_p0[3], rx_addr_p0[4],
        tx_addr[0], tx_addr[1], tx_addr[2], tx_addr[3], tx_addr[4]);
#endif // DEBUG
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
