/* mbed Microcontroller Library
 * Copyright (c) 2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "spi_api.h"

#include <math.h>

#include "cmsis.h"
#include "pinmap.h"
#include "error.h"

static const PinMap PinMap_SPI_SCLK[] = {
    {PTC5, SPI_0, 2},
    {PTD1, SPI_0, 2},
    {NC  , NC   , 0}
};

static const PinMap PinMap_SPI_MOSI[] = {
    {PTD2, SPI_0, 2},
    {PTC6, SPI_0, 2},
    {NC  , NC   , 0}
};

static const PinMap PinMap_SPI_MISO[] = {
    {PTD3, SPI_0, 2},
    {PTC7, SPI_0, 2},
    {NC  , NC   , 0}
};

static const PinMap PinMap_SPI_SSEL[] = {
    {PTD0, SPI_0, 2},
    {PTC4, SPI_0, 2},
    {NC  , NC   , 0}
};

void spi_init(spi_t *obj, PinName mosi, PinName miso, PinName sclk, PinName ssel) {
    // determine the SPI to use
    SPIName spi_mosi = (SPIName)pinmap_peripheral(mosi, PinMap_SPI_MOSI);
    SPIName spi_miso = (SPIName)pinmap_peripheral(miso, PinMap_SPI_MISO);
    SPIName spi_sclk = (SPIName)pinmap_peripheral(sclk, PinMap_SPI_SCLK);
    SPIName spi_ssel = (SPIName)pinmap_peripheral(ssel, PinMap_SPI_SSEL);
    SPIName spi_data = (SPIName)pinmap_merge(spi_mosi, spi_miso);
    SPIName spi_cntl = (SPIName)pinmap_merge(spi_sclk, spi_ssel);

    obj->spi = (SPI_Type*)pinmap_merge(spi_data, spi_cntl);
    if ((int)obj->spi == NC) {
        error("SPI pinout mapping failed");
    }

    SIM->SCGC5 |= (1 << 11) | (1 << 12); // PortC & D
    SIM->SCGC6 |= 1 << 12;               // spi clocks

    // halted state
    obj->spi->MCR = SPI_MCR_HALT_MASK;

    // set default format and frequency
    if (ssel == NC) {
        spi_format(obj, 8, 0, 0);  // 8 bits, mode 0, master
    } else {
        spi_format(obj, 8, 0, 1);  // 8 bits, mode 0, slave
    }
    spi_frequency(obj, 1000000);

    // not halt in the debug mode
    obj->spi->SR |= SPI_SR_EOQF_MASK;
    // enable SPI
    obj->spi->MCR &= (~SPI_MCR_HALT_MASK);

    // pin out the spi pins
    pinmap_pinout(mosi, PinMap_SPI_MOSI);
    pinmap_pinout(miso, PinMap_SPI_MISO);
    pinmap_pinout(sclk, PinMap_SPI_SCLK);
    if (ssel != NC) {
        pinmap_pinout(ssel, PinMap_SPI_SSEL);
    }
}

void spi_free(spi_t *obj) {
    // [TODO]
}
void spi_format(spi_t *obj, int bits, int mode, int slave) {
    if ((bits != 8) && (bits != 16)) {
        error("Only 8/16 bits SPI supported");
    }

    if ((mode < 0) || (mode > 3)) {
        error("SPI mode unsupported");
    }

    uint32_t polarity = (mode & 0x2) ? 1 : 0;
    uint32_t phase = (mode & 0x1) ? 1 : 0;

    // set master/slave
    obj->spi->MCR &= ~SPI_MCR_MSTR_MASK;
    obj->spi->MCR |= ((!slave) << SPI_MCR_MSTR_SHIFT);

    // CTAR0 is used
    obj->spi->CTAR[0] &= ~(SPI_CTAR_CPHA_MASK | SPI_CTAR_CPOL_MASK);
    obj->spi->CTAR[0] |= (polarity << SPI_CTAR_CPOL_SHIFT) | (phase << SPI_CTAR_CPHA_SHIFT);
}

void spi_frequency(spi_t *obj, int hz) {
    uint32_t error = 0;
    uint32_t p_error = 0xffffffff;
    uint32_t ref = 0;
    uint32_t spr = 0;
    uint32_t ref_spr = 0;
    uint32_t ref_prescaler = 0;

    // bus clk
    uint32_t PCLK = 48000000u;
    uint32_t prescaler = 1;
    uint32_t divisor = 2;

    for (prescaler = 1; prescaler <= 8; prescaler++) {
        divisor = 2;
        for (spr = 0; spr <= 8; spr++, divisor *= 2) {
            ref = PCLK / (prescaler*divisor);
            if (ref > (uint32_t)hz)
                continue;
            error = hz - ref;
            if (error < p_error) {
                ref_spr = spr;
                ref_prescaler = prescaler - 1;
                p_error = error;
            }
        }
    }

    // set SPPR and SPR
    obj->spi->CTAR[0] = ((ref_prescaler & 0x7) << 4) | (ref_spr & 0xf);
}

static inline int spi_writeable(spi_t * obj) {
    return (obj->spi->SR & SPI_SR_TCF_MASK) ? 1 : 0;
}

static inline int spi_readable(spi_t * obj) {
    return (obj->spi->SR & SPI_SR_TFFF_MASK) ? 1 : 0;
}

int spi_master_write(spi_t *obj, int value) {
    // wait tx buffer empty
    while(!spi_writeable(obj));
    obj->spi->PUSHR = SPI_PUSHR_TXDATA(value & 0xff);

    // wait rx buffer full
    while (!spi_readable(obj));
    return obj->spi->POPR;
}

int spi_slave_receive(spi_t *obj) {
    return spi_readable(obj);
}

int spi_slave_read(spi_t *obj) {
    return obj->spi->POPR;
}

void spi_slave_write(spi_t *obj, int value) {
    while (!spi_writeable(obj));
}