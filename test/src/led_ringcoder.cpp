/*
 * Copyright (c) 2013-2014 Immo Software
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

#include "os/ar_kernel.h"
#include "debug_uart.h"
#include "mbed.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

enum
{
    kEncoderAPin = D2,  // PTD4
    kEncoderBPin = D3,  // PTA12
    kRedLEDPin = D5,    // PTA5
    kBlueLEDPin = D6,   // PTC8
    kGreenLEDPin = D9,  // PTD5
    kEncoderSwitchPin = D7, // PTC9
    kRingDataPin = A4, //D14, // PTE0
    kRingClearPin = D10,    // PTD0
    kRingClockPin = D11,    // PTD2
    kRingLatchPin = D12,    // PTD3
    kRingEnablePin = D15    // PTD1
};

enum
{
    kLedFadeRate = 100
};

class Fader
{
private:
    float m_value;
    float m_delta;
    
public:
    Fader(float delta);
    
    float next();
    
    void setDelta(float delta) { m_delta = delta; }
    
    operator float () { return next(); }
};

class RotaryDecoder
{
public:
    RotaryDecoder();
    
    int decode(uint8_t a, uint8_t b);

private:
    static const int s_encoderStates[];
    uint8_t m_oldState;
};

class ShiftRegister
{
public:
    ShiftRegister(PinName en, PinName latch, PinName clk, PinName clr, PinName dat);

    void clear();

    template <uint8_t B>
    void set(uint32_t bits);

private:
    DigitalOut m_en;
    DigitalOut m_latch;
    DigitalOut m_clk;
    DigitalOut m_clr;
    DigitalOut m_dat;
};

enum
{
    kRedChannel,
    kGreenChannel,
    kBlueChannel
};

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void init_thread(void * arg);
void encoder_handler();
// void blue_thread(void * arg);

void blinker(Ar::Timer * timer, void * arg);
void green_blinker(Ar::Timer * timer, void * arg);

void encoder_debounce(Ar::Timer * timer, void * arg);

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

const int RotaryDecoder::s_encoderStates[] = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };

Ar::ThreadWithStack<512> g_initThread("main", init_thread, 0, 56);

Ar::Timer g_blinker("blinker", blinker, 0, Ar::Timer::kPeriodicTimer, 1000);
Ar::Timer g_green_blinker("green_blinker", green_blinker, 0, Ar::Timer::kPeriodicTimer, 1500);

Ar::Timer g_encoderDebounceTimer("encoder_debounce", encoder_debounce, 0, Ar::Timer::kOneShotTimer, 5);

PwmOut g_r(LED_RED);
PwmOut g_g(LED_GREEN);
PwmOut g_bl(LED_BLUE);

PwmOut g_red((PinName)kRedLEDPin);
PwmOut g_green((PinName)kGreenLEDPin);
PwmOut g_blue((PinName)kBlueLEDPin);

RotaryDecoder g_decoder;
InterruptIn g_a((PinName)kEncoderAPin);
InterruptIn g_b((PinName)kEncoderBPin);
int g_value = 0;

ShiftRegister g_ledRing((PinName)kRingEnablePin, (PinName)kRingLatchPin, (PinName)kRingClockPin, (PinName)kRingClearPin, (PinName)kRingDataPin);

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

Fader::Fader(float delta)
:   m_value(0.0),
    m_delta(delta)
{
}

float Fader::next()
{
    m_value += m_delta;
    if (m_value < 0.000001f)
    {
        m_value = 0.0f;
        m_delta = -m_delta;
    }
    else if (m_value > 0.99999f)
    {
        m_value = 1.0f;
        m_delta = -m_delta;
    }
    
    return m_value;
}

ShiftRegister::ShiftRegister(PinName en, PinName latch, PinName clk, PinName clr, PinName dat)
:   m_en(en),
    m_latch(latch),
    m_clk(clk),
    m_clr(clr),
    m_dat(dat)
{
    m_en = 0;
    m_clr = 1;
    m_clk = 0;
    m_latch = 0;
    m_dat = 0;
}

void ShiftRegister::clear()
{
    m_clr = 0;
    Ar::Thread::sleep(10);
    m_clr = 1;
    m_latch = 1;
    Ar::Thread::sleep(10);
    m_latch = 0;
}

template <uint8_t B>
void ShiftRegister::set(uint32_t bits)
{
    m_latch = 0;
    m_clk = 0;
    
    unsigned i;
    for (i=B; i; --i)
    {
        uint32_t b = (bits >> (i-1)) & 1;
        
        m_clk = 0;
//         m_latch = 0;
        m_dat = b;
//         Ar::Thread::sleep(20);
        wait_us(50);
        m_clk = 1;
//         m_latch = 1;
//         Ar::Thread::sleep(20);
        wait_us(50);
    }
    
//     Ar::Thread::sleep(20);
    wait_us(50);
    m_latch = 1;
//     Ar::Thread::sleep(20);
    wait_us(50);
    m_latch = 0;
}

void blinker(Ar::Timer * timer, void * arg)
{
    g_blue = !g_blue;
}

void green_blinker(Ar::Timer * timer, void * arg)
{
    g_green = !g_green;
}

void update_led_ring(unsigned value)
{
    unsigned n = value * 16 / 100;
    uint32_t bits = 0;
    unsigned i;
    for (i=0; i < n; ++i)
    {
        bits = (bits << 1) | 1;
    }
    
//    bits = ((bits & 0xff) << 8) | ((bits & 0xff00) >> 8);
    g_ledRing.set<16>(bits);
}

void update_rgb_led(unsigned r, unsigned g, unsigned b)
{
    g_r = 1.0f - ((float)r / 100.0f);
    g_g = 1.0f - ((float)g / 100.0f);
    g_bl = 1.0f - ((float)b / 100.0f);

//     g_red = 1.0f - ((float)r / 100.0f);
//     g_green = 1.0f - ((float)g / 100.0f);
//     g_blue = 1.0f - ((float)b / 100.0f);
}

void init_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();
    
    printf("[%s] Init thread is running\r\n", myName);
    
    g_red = 1;
    g_green = 1;
    g_blue = 1;
    
//     g_blinker.start();
//     g_green_blinker.start();
    
    g_a.mode(PullUp);
    g_b.mode(PullUp);
    g_a.rise(encoder_handler);
    g_a.fall(encoder_handler);
    g_b.rise(encoder_handler);
    g_b.fall(encoder_handler);
    
    g_r.period_us(20);
    g_g.period_us(20);
    
    g_ledRing.clear();
//     g_ledRing.set<16>(0x5555);
    
    int colorChannel = kRedChannel;
    g_red = 0;
    int colorValues[3] = {0};
    update_rgb_led(colorValues[kRedChannel], colorValues[kGreenChannel], colorValues[kBlueChannel]);
    
    DigitalIn switchPin((PinName)kEncoderSwitchPin);
//     int lastValue = g_value;
    while (true)
    {
//         if (switchPin)
//         {
//             g_red = 0; //1.0f;
//         }
//         else
//         {
//             g_red = 1; //0.0f;
//         }

        if (switchPin)
        {
            ++colorChannel;
            if (colorChannel > kBlueChannel)
            {
                colorChannel = kRedChannel;
            }
            
            switch (colorChannel)
            {
                case kRedChannel:
                    g_red = 0;
                    g_green = 1;
                    g_blue = 1;
                    break;
                case kGreenChannel:
                    g_red = 1;
                    g_green = 0;
                    g_blue = 1;
                    break;
                case kBlueChannel:
                    g_red = 1;
                    g_green = 1;
                    g_blue = 0;
                    break;
            }

            g_value = colorValues[colorChannel];
            update_led_ring(g_value);
            
            while (switchPin)
            {
                Ar::Thread::sleep(100);
            }
        }

        if (colorValues[colorChannel] != g_value)
        {
            printf("value=%d\r\n", g_value);
            colorValues[colorChannel] = g_value;
            update_led_ring(g_value);
            
            update_rgb_led(colorValues[kRedChannel], colorValues[kGreenChannel], colorValues[kBlueChannel]);
        }
    }
    
//     printf("[%s] goodbye!\r\n", myName);
}

RotaryDecoder::RotaryDecoder()
:   m_oldState(0)
{
}

int RotaryDecoder::decode(uint8_t a, uint8_t b)
{
    uint8_t newState = ((b & 1) << 1) | (a & 1);
    
    m_oldState = ((m_oldState & 3) << 2) | newState;
    
    return s_encoderStates[m_oldState];
    
    // First, find the newEncoderState. This'll be a 2-bit value
    // the msb is the state of the B pin. The lsb is the state
    // of the A pin on the encoder.
//     newEncoderState = (digitalRead(bPin)<<1) | (digitalRead(aPin));

    // Now we pair oldEncoderState with new encoder state
    // First we need to shift oldEncoder state left two bits.
    // This'll put the last state in bits 2 and 3.
//     oldEncoderState <<= 2;
    
    // Mask out everything in oldEncoderState except for the previous state
//     oldEncoderState &= 0xC0;
    
    // Now add the newEncoderState. oldEncoderState will now be of
    // the form: 0b0000(old B)(old A)(new B)(new A)
//     oldEncoderState |= newEncoderState; // add filteredport value
}

void encoder_debounce(Ar::Timer * timer, void * arg)
{
    int a = g_a;
    int b = g_b;
//     printf("encoder: a=%d, b=%d\r\n", a, b);
    
    int delta = g_decoder.decode(a, b);
    g_value += delta;
    if (g_value < 0)
    {
        g_value = 0;
    }
    else if (g_value > 100)
    {
        g_value = 100;
    }
}

void encoder_handler()
{
    g_encoderDebounceTimer.start();
}

void main(void)
{
//     debug_init();

    SIM->SCGC5 |= ( SIM_SCGC5_PORTA_MASK
                  | SIM_SCGC5_PORTB_MASK
                  | SIM_SCGC5_PORTC_MASK
                  | SIM_SCGC5_PORTD_MASK
                  | SIM_SCGC5_PORTE_MASK );
    
    printf("Running test...\r\n");
    
    g_initThread.resume();
    
    Ar::Kernel::run();

    Ar::_halt();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
