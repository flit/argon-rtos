/*
 * Copyright (c) 2015 Chris Reed
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
#include "audio_output.h"
#include "audio_output_converter.h"
#include "audio_filter.h"
#include "audio_ramp.h"
#include "ar_envelope.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_fxos.h"
#include "arm_math.h"
#include <stdio.h>
#include <math.h>

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#define OVER_SAMPLE_RATE (384U)
#define BUFFER_SIZE (256)
#define CHANNEL_NUM (2)
#define BUFFER_NUM (2)

class SineGenerator : public AudioFilter
{
public:
    SineGenerator()
    :   m_sinFreq(0),
        m_currRadians(0),
        m_trigger("trig"),
        m_env()
    {
    }

    void set_freq(float freq) { m_sinFreq = freq; }
    void init();

    virtual void process(float * samples, uint32_t count);

    void trigger();

protected:
    float m_sinFreq; // 80 Hz
    float m_currRadians;
//     Ar::TypedChannel<int> m_trigger;
    volatile bool m_trigger;
    AREnvelope m_env;
};

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void accel_thread(void * arg);
void init_audio_out();
void init_board();

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

uint32_t g_xtal0Freq = 8000000U;
uint32_t g_xtal32Freq = 32768U;

float g_audioBuf[BUFFER_SIZE];
int16_t g_outBuf[BUFFER_NUM][BUFFER_SIZE * CHANNEL_NUM];

float g_sampleRate = 32000.0f; // 32kHz
float g_sinFreq = 80.0f; // 80 Hz
float g_currRadians = 0.0f;

AudioOutput g_audioOut;
AudioOutputConverter g_audioOutConverter;
SineGenerator g_sinGen;
i2c_master_handle_t g_i2cHandle;
fxos_handle_t g_fxos;

Ar::ThreadWithStack<512> g_accelThread("accel", accel_thread, 0, 120, kArSuspendThread);

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

void my_timer_fn(Ar::Timer * t, void * arg)
{
//     printf("x\n");

    GPIO_TogglePinsOutput(GPIOA, (1 << 1)|(1 << 2));
    GPIO_TogglePinsOutput(GPIOD, (1 << 5));

    g_sinGen.trigger();
}
Ar::Timer g_myTimer("mine", my_timer_fn, 0, kArPeriodicTimer, 1500);

void SineGenerator::init()
{
    m_env.set_sample_rate(get_sample_rate());
    m_env.set_curve_type(AREnvelope::kAttack, AudioRamp::kLinear);
    m_env.set_curve_type(AREnvelope::kRelease, AudioRamp::kLinear);
    m_env.set_length_in_seconds(AREnvelope::kAttack, 0.05f);
    m_env.set_length_in_seconds(AREnvelope::kRelease, 1.5f);
}

void SineGenerator::process(float * samples, uint32_t count)
{
    float delta = 2.0f * PI * (m_sinFreq / m_sampleRate);

    // Check for a trigger.
//     int trig;
//     ar_status_t status = m_trigger.receive(trig, kArNoTimeout);
    bool isTriggered = m_trigger; // status == kArSuccess && trig == 1;

    int i;
    float * sample = samples;
    for (i = 0; i < count; ++i)
    {
//         float f = sinf(m_currRadians);
        float f = arm_sin_f32(m_currRadians);
        *sample++ = f * m_env.next();

        m_currRadians += delta;
        if (m_currRadians >= 2.0f * PI)
        {
            m_currRadians = 0.0f;
        }
        if (isTriggered && m_currRadians == 0.0f)
        {
            m_env.reset();
            m_trigger = false;
        }
    }
}

void SineGenerator::trigger()
{
//     m_trigger.send(1);
    m_trigger = true;
}

void accel_thread(void * arg)
{
    memset(&g_fxos, 0, sizeof(g_fxos));
    g_fxos.base = I2C0;
    g_fxos.i2cHandle = &g_i2cHandle;
    g_fxos.xfer.slaveAddress = 0x1c;
    FXOS_Init(&g_fxos);

    while (1)
    {
        fxos_data_t data;
        status_t status = FXOS_ReadSensorData(&g_fxos, &data);
        if (status == kStatus_Success)
        {
//             printf("acc[x=%6d y=%6d z=%6d] mag[x=%6d y=%6d z=%6d]\r\n",
//                    data.accelX, data.accelY, data.accelZ,
//                    data.magX, data.magY, data.magZ);
        }

        Ar::Thread::sleep(20);
    }
}

void init_audio_out()
{
    // Configure the audio format
    sai_transfer_format_t format;
    format.bitWidth = kSAI_WordWidth16bits;
    format.channel = 0U;
    format.sampleRate_Hz = kSAI_SampleRate32KHz;
    format.masterClockHz = OVER_SAMPLE_RATE * format.sampleRate_Hz;
    format.protocol = kSAI_BusLeftJustified;
    format.stereo = kSAI_Stereo;
    format.watermark = FSL_FEATURE_SAI_FIFO_COUNT / 2U;

    // Configure Sgtl5000 I2C
    uint32_t i2cSourceClock = CLOCK_GetFreq(kCLOCK_BusClk);
    i2c_master_config_t i2cConfig = {0};
    I2C_MasterGetDefaultConfig(&i2cConfig);
    I2C_MasterInit(I2C0, &i2cConfig, i2cSourceClock);
    I2C_MasterCreateHandle(I2C0, &g_i2cHandle, NULL, NULL);

    g_audioOut.init(&format, I2C0, &g_i2cHandle);

    AudioOutput::Buffer buf;
    buf.dataSize = BUFFER_SIZE * CHANNEL_NUM * sizeof(int16_t);
    buf.data = (uint8_t *)&g_outBuf[0][0];
    g_audioOut.add_buffer(&buf);
    buf.data = (uint8_t *)&g_outBuf[1][0];
    g_audioOut.add_buffer(&buf);

    g_audioOut.set_source(&g_audioOutConverter);
    AudioBuffer audioBuf(&g_audioBuf[0], BUFFER_SIZE);
    g_audioOutConverter.set_buffer(audioBuf);
    g_audioOutConverter.set_source(&g_sinGen);

    g_sinGen.set_sample_rate(32000.0f);
    g_sinGen.set_freq(50.0f);
    g_sinGen.init();

//     g_audioOut.dump_sgtl5000();
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

int main(void)
{
    printf("Hello...\r\n");

    init_board();
    init_audio_out();

    g_myTimer.start();
    g_audioOut.start();
    g_accelThread.resume();

    Ar::Thread::getCurrent()->suspend();
    while (1)
    {
    }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
