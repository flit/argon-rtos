/*
 * Copyright (c) 2013 Immo Software
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

#include "argon/argon.h"
#include "fsl_dmamux.h"
#include "fsl_sai_edma.h"
#include "fsl_sgtl5000.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "arm_math.h"
#include <stdio.h>
#include "music.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#define OVER_SAMPLE_RATE (512U)
#define BUFFER_SIZE (256)
#define CHANNEL_NUM (2)
#define BUFFER_NUM (2)

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void sai_callback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData);
void init_audio_out();
void init_board();

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

uint32_t g_xtal0Freq = 8000000U;
uint32_t g_xtal32Freq = 32768U;

sai_edma_handle_t txHandle;
edma_handle_t dmaHandle;
sgtl_handle_t codecHandle;
i2c_master_handle_t i2cHandle;

float g_audioBuf[BUFFER_NUM][BUFFER_SIZE * CHANNEL_NUM];
int16_t g_outBuf[BUFFER_NUM][BUFFER_SIZE * CHANNEL_NUM];

Ar::Semaphore g_saiTransferDone("sai_done", 0);

float g_sampleRate = 16000.0f; // 16kHz
float g_sinFreq = 200.0f; // 200 Hz
float g_currRadians = 0.0f;
uint32_t g_currPhase = 0;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

void my_timer_fn(Ar::Timer * t, void * arg)
{
//     printf("x\n");

    GPIO_TogglePinsOutput(GPIOA, (1 << 1)|(1 << 2));
    GPIO_TogglePinsOutput(GPIOD, (1 << 5));
}
Ar::Timer g_myTimer("mine", my_timer_fn, 0, kArPeriodicTimer, 1500);

void fill_sine(float * samples, uint32_t count)
{
    float delta = 2.0f * PI * (g_sinFreq / g_sampleRate);

    int i;
    for (i = 0; i < count; ++i)
    {
        float f = arm_sin_f32(g_currRadians);
        *samples++ = f; // L channel
        *samples++ = f; // R channel

        g_currRadians += delta;
        if (g_currRadians >= 2.0f * PI)
        {
            g_currRadians = 0.0f;
        }
    }
}

void fill_square(int16_t * samples, uint32_t count)
{
    uint32_t halfwaveCount = g_sampleRate / g_sinFreq / 2;

    int i;
    for (i = 0; i < count; ++i)
    {
        if (g_currPhase < halfwaveCount)
        {
            *samples++ = 32000;
            *samples++ = 32000;
        }
        else
        {
            *samples++ = -32000;
            *samples++ = -32000;
        }

        ++g_currPhase;
        if (g_currPhase > halfwaveCount * 2)
        {
            g_currPhase = 0;
        }
    }
}

void convert_and_interleave(uint32_t bufIndex)
{
//     int i;
//     for (i = 0; i < BUFFER_SIZE; ++i)
//     {
//         float samples[CHANNEL_COUNT];
//         samples[0] = g_audioBuf[bufIndex][0][i];
//         samples[1] = g_audioBuf[bufIndex][1][i];
//         q16_t * out = &g_outBuf[bufIndex][i * CHANNEL_NUM];
//         arm_float_to_q15(&sample, out, 2);
//     }

    arm_float_to_q15(&g_audioBuf[bufIndex][0], &g_outBuf[bufIndex][0], BUFFER_SIZE * CHANNEL_NUM);
}

void sai_callback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData)
{
    g_saiTransferDone.put();
}

void dump_sgtl5000()
{
    const struct {
        uint16_t addr;
        const char * name;
    } kRegsToDump[] = {
        { CHIP_ID, "CHIP_ID" },
        { CHIP_DIG_POWER, "CHIP_DIG_POWER" },
        { CHIP_CLK_CTRL, "CHIP_CLK_CTRL" },
        { CHIP_I2S_CTRL, "CHIP_I2S_CTRL" },
        { CHIP_SSS_CTRL, "CHIP_SSS_CTRL" },
        { CHIP_ADCDAC_CTRL, "CHIP_ADCDAC_CTRL" },
        { CHIP_DAC_VOL, "CHIP_DAC_VOL" },
        { CHIP_PAD_STRENGTH, "CHIP_PAD_STRENGTH" },
        { CHIP_ANA_ADC_CTRL, "CHIP_ANA_ADC_CTRL" },
        { CHIP_ANA_HP_CTRL, "CHIP_ANA_HP_CTRL" },
        { CHIP_ANA_CTRL, "CHIP_ANA_CTRL" },
        { CHIP_LINREG_CTRL, "CHIP_LINREG_CTRL" },
        { CHIP_REF_CTRL, "CHIP_REF_CTRL" },
        { CHIP_MIC_CTRL, "CHIP_MIC_CTRL" },
        { CHIP_LINE_OUT_CTRL, "CHIP_LINE_OUT_CTRL" },
        { CHIP_LINE_OUT_VOL, "CHIP_LINE_OUT_VOL" },
        { CHIP_ANA_POWER, "CHIP_ANA_POWER" },
        { CHIP_PLL_CTRL, "CHIP_PLL_CTRL" },
        { CHIP_CLK_TOP_CTRL, "CHIP_CLK_TOP_CTRL" },
        { CHIP_ANA_STATUS, "CHIP_ANA_STATUS" },
//         { CHIP_ANA_TEST2, "CHIP_ANA_TEST2" },
//         { CHIP_SHORT_CTRL, "CHIP_SHORT_CTRL" },
    };

    int i;
    for (i = 0; i < ARRAY_SIZE(kRegsToDump); ++i)
    {
        uint16_t val;
        status_t status = SGTL_ReadReg(&codecHandle, kRegsToDump[i].addr, &val);
        if (status == kStatus_Success)
        {
            printf("%s = 0x%04x\r\n", kRegsToDump[i].name, val);
        }
        else
        {
            printf("%s = (failed to read!)\r\n", kRegsToDump[i].name);
        }
    }
}

void init_audio_out()
{
    // Configure the audio format
    sai_transfer_format_t format;
    format.bitWidth = kSAI_WordWidth16bits;
    format.channel = 0U;
    format.sampleRate_Hz = kSAI_SampleRate16KHz;
    format.masterClockHz = OVER_SAMPLE_RATE * format.sampleRate_Hz;
    format.protocol = kSAI_BusI2S; //kSAI_BusLeftJustified;
    format.stereo = kSAI_Stereo;
    format.watermark = FSL_FEATURE_SAI_FIFO_COUNT / 2U;

    // Create EDMA handle
    edma_config_t dmaConfig = {0};
    EDMA_GetDefaultConfig(&dmaConfig);
    EDMA_Init(DMA0, &dmaConfig);
    EDMA_CreateHandle(&dmaHandle, DMA0, 0);

    DMAMUX_Init(DMAMUX0);
    DMAMUX_SetSource(DMAMUX0, 0, kDmaRequestMux0I2S0Tx & 0xff);
    DMAMUX_EnableChannel(DMAMUX0, 0);

    // Init SAI module
    sai_config_t saiConfig;
    SAI_TxGetDefaultConfig(&saiConfig);
    saiConfig.protocol = kSAI_BusI2S;
    saiConfig.masterSlave = kSAI_Slave;
    SAI_TxInit(I2S0, &saiConfig);
    SAI_TxCreateHandleEDMA(I2S0, &txHandle, sai_callback, NULL, &dmaHandle);

    uint32_t mclkSourceClockHz = CLOCK_GetFreq(kCLOCK_CoreSysClk);
    SAI_TxSetTransferFormatEDMA(I2S0, &txHandle, &format, mclkSourceClockHz, format.masterClockHz);

    // Configure Sgtl5000 I2C
    uint32_t i2cSourceClock = CLOCK_GetFreq(kCLOCK_BusClk);
    i2c_master_config_t i2cConfig = {0};
    I2C_MasterGetDefaultConfig(&i2cConfig);
    I2C_MasterInit(I2C0, &i2cConfig, i2cSourceClock);
    I2C_MasterCreateHandle(I2C0, &i2cHandle, NULL, NULL);

    // Configure the sgtl5000.
    sgtl_config_t sgtlConfig = {
        .route = kSGTL_RoutePlayback,
        .bus = kSGTL_BusI2S, //kSGTL_BusLeftJustified,
        .master_slave = true,
    };
    codecHandle.base = I2C0;
    codecHandle.i2cHandle = &i2cHandle;
    SGTL_Init(&codecHandle, &sgtlConfig);
    SGTL_ConfigDataFormat(&codecHandle, format.masterClockHz, format.sampleRate_Hz, format.bitWidth);

    dump_sgtl5000();
}

void init_board()
{
    CLOCK_EnableClock(kCLOCK_PortA);
    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);
    CLOCK_EnableClock(kCLOCK_PortD);

    // I2C0 pins
    const port_pin_config_t pinConfig = {
        .pullSelect = kPORT_PullDisable,
        .slewRate = kPORT_FastSlewRate,
        .passiveFilterEnable = kPORT_PassiveFilterDisable,
        .openDrainEnable = kPORT_OpenDrainEnable,
        .driveStrength = kPORT_LowDriveStrength,
        .mux = kPORT_MuxAlt2,
    };
    PORT_SetMultiplePinsConfig(PORTB, (1 << 3)|(1 << 2), &pinConfig);

    // SAI pins
    PORT_SetPinMux(PORTC, 8, kPORT_MuxAlt4);
    PORT_SetPinMux(PORTA, 5, kPORT_MuxAlt6);
    PORT_SetPinMux(PORTA, 12, kPORT_MuxAlt6);
    PORT_SetPinMux(PORTA, 13, kPORT_MuxAlt6);
    PORT_SetPinMux(PORTC, 5, kPORT_MuxAlt4);

    // LED pins
    PORT_SetPinMux(PORTA, 1, kPORT_MuxAsGpio);
    PORT_SetPinMux(PORTA, 2, kPORT_MuxAsGpio);
    PORT_SetPinMux(PORTD, 5, kPORT_MuxAsGpio);

    const gpio_pin_config_t gpioOut = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0,
    };
    GPIO_PinInit(GPIOA, 1, &gpioOut);
    GPIO_PinInit(GPIOA, 2, &gpioOut);
    GPIO_PinInit(GPIOD, 5, &gpioOut);
}

void send_audio()
{
    sai_transfer_t xfer;
    uint32_t currentBuffer = 0;

    xfer.dataSize = BUFFER_SIZE;

    printf("starting transfer...\r\n");

    // Start transfer
    int i;
    for (i = 0; i < BUFFER_NUM; i ++)
    {
//         fill_sine(&g_audioBuf[i][0], BUFFER_SIZE);
        fill_square(&g_outBuf[i][0], BUFFER_SIZE);
//         convert_and_interleave(i);

        xfer.data = (uint8_t *)&g_outBuf[i][0];
        SAI_SendEDMA(I2S0, &txHandle, &xfer);
    }

    while (1)
    {
        // One block finished
        g_saiTransferDone.get();

//         fill_sine(&g_audioBuf[currentBuffer][0], BUFFER_SIZE);
        fill_square(&g_outBuf[currentBuffer][0], BUFFER_SIZE);
//         convert_and_interleave(currentBuffer);

        // Add another buffer
        xfer.data = (uint8_t *)&g_outBuf[currentBuffer][0];
        SAI_SendEDMA(I2S0, &txHandle, &xfer);

        printf("transferred buf %d...\r\n", currentBuffer);

        currentBuffer = (currentBuffer + 1) % BUFFER_NUM;
    }
}

void play_music()
{
    int userIndex = 0;
    int i;
    sai_transfer_t xfer;

    /* First copy the buffer full */
    memcpy(g_outBuf, music, BUFFER_SIZE * BUFFER_NUM);

    xfer.dataSize = BUFFER_SIZE;

    /* Start transfer */
    for (i = 0; i < BUFFER_NUM; i ++)
    {
        xfer.data = (uint8_t *)&g_outBuf[0][0] + i * BUFFER_SIZE;
        SAI_SendEDMA(I2S0, &txHandle, &xfer);
    }

    for (i = 0; i < (MUSIC_LEN/BUFFER_SIZE - BUFFER_NUM); i ++)
    {
        /* One block finished */
        g_saiTransferDone.get();

        /* Copy data to buffer */
        memcpy((uint8_t *)&g_outBuf[0][0] + userIndex * BUFFER_SIZE, &music[(i + 2) * BUFFER_SIZE], BUFFER_SIZE);
        userIndex = (userIndex + 1)%BUFFER_NUM;

        /* Add another buffer */
        xfer.data = (uint8_t *)&g_outBuf[0][0] + userIndex * BUFFER_SIZE;
        SAI_SendEDMA(I2S0, &txHandle, &xfer);
    }
}

int main(void)
{
    printf("Hello...\r\n");

    init_board();
    init_audio_out();

    g_myTimer.start();

//     send_audio();
    while (1) { play_music(); }

    Ar::Thread::getCurrent()->suspend();
    while (1)
    {
    }

//     return 0;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
