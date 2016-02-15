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
#include "kernel_test.h"
#include <stdio.h>
#include "fsl_device_registers.h"
#include "fsl_adc16.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#define TEST_CASE_CLASS TestMutex1

#define ADCR_VDD                (65535.0f)    /*! Maximum value when use 16b resolution */
#define V_BG                    (1000.0f)     /*! BANDGAP voltage in mV (trim to 1.0V) */
#define V_TEMP25                (716.0f)      /*! Typical VTEMP25 in mV */
#define M                       (1620.0f)     /*! Typical slope: (mV x 1000)/oC */
#define STANDARD_TEMP           (25.0f)

enum {
    ADC0_TEMP = 26,     // s.e. ch 26
    ADC0_BANDGAP = 27,  // s.e. ch 27
};

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void main_thread(void * arg);
void x_thread(void * arg);
void y_thread(void * arg);
void rf_thread(void * arg);
void my_timer_fn(Ar::Timer * t, void * arg);
void creator_thread(void * arg);
void rl_thread(void * arg);
void rl2_thread(void * arg);

class Foo
{
public:
    void my_entry();
};

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

// TEST_CASE_CLASS g_testCase;

// Ar::ThreadWithStack<512> g_xThread("x", x_thread, 0, 130, kArSuspendThread);
// Ar::ThreadWithStack<512> g_yThread("y", y_thread, 0, 120, kArSuspendThread);

Ar::Thread * g_fpThread1;
Ar::Thread * g_fpThread2;
Ar::TypedChannel<float> g_fchan("f");
Ar::TypedChannel<float> g_avgTempChan("avgtemp");

Ar::TypedChannel<int> g_chan("c");

Ar::Thread g_fooThread;

Ar::Thread * g_dyn;

float adcrTemp25 = 0;             /*! Calibrated ADCR_TEMP25 */
float adcr100m = 0;

Ar::Mutex g_adcMutex("adc");

Ar::Mutex g_someMutex("foobar");

Ar::TypedChannel<uint32_t> g_adcResult("adcresult");

Ar::TypedChannel<float> g_tempchan("temp");

Ar::Timer g_myTimer("mine", my_timer_fn, 0, kArPeriodicTimer, 1500);
Ar::Thread * g_timerFeederThread;

Ar::Thread g_creatorThread("creator", creator_thread, 0, 512, 96, kArSuspendThread);

Ar::ThreadWithStack<1024> g_rlThread("rl", rl_thread, 0, 100, kArSuspendThread);
// Ar::ThreadWithStack<1024> g_rl2Thread("rl2", rl2_thread, 0, 70, kArSuspendThread);

Ar::StaticQueue<uint32_t, 4> g_q("q");

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

uint32_t us_ticker_read()
{
    return ar_get_tick_count();
}

void say_hi(void * arg)
{
    printf("[%d] say_hi says hi %d\n", us_ticker_read(), int(arg));
}

void Foo::my_entry()
{
    int n = 0;
    while (1)
    {
//         printf("hi from Foo!\n");

        ar_runloop_t * rl = ar_thread_get_runloop(&g_rlThread);
        ar_runloop_perform(rl, say_hi, (void *)(n));

        Ar::Thread::sleep(700);
        ++n;
    }
}

void x_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();
    ar_status_t status;

    while (1)
    {
        printf("[%d:%s] receiving on channel\n", us_ticker_read(), myName);
        int foo = g_chan.receive();
        printf("[%d:%s] received from channel (foo=%d)\n", us_ticker_read(), myName, foo);

        printf("[%d:%s] sleeping a bit\n", us_ticker_read(), myName);
        Ar::Thread::sleep(1000);

        printf("[%d:%s] receiving on channel\n", us_ticker_read(), myName);
        foo = g_chan.receive();
        printf("[%d:%s] received from channel (foo=%d)\n", us_ticker_read(), myName, foo);

        printf("[%d:%s] receiving on channel (will timeout)\n", us_ticker_read(), myName);
        status = g_chan.receive(foo, 500);
        if (status != kArTimeoutError)
        {
            printf("[%d:%s] receiving didn't timeout!\n", us_ticker_read(), myName);
        }
        else
        {
            printf("[%d:%s] receiving timed out successfully\n", us_ticker_read(), myName);
        }

        printf("[%d:%s] sleeping a bit\n", us_ticker_read(), myName);
        Ar::Thread::sleep(4000);
    }
}

void y_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();
    ar_status_t status;

    while (1)
    {
        printf("[%d:%s] sending to channel\n", us_ticker_read(), myName);
        g_chan.send(128);
        printf("[%d:%s] sent to channel\n", us_ticker_read(), myName);

        printf("[%d:%s] sending to channel\n", us_ticker_read(), myName);
        g_chan.send(256);
        printf("[%d:%s] sent to channel\n", us_ticker_read(), myName);

        printf("[%d:%s] sleeping a bit\n", us_ticker_read(), myName);
        Ar::Thread::sleep(2000);

        printf("[%d:%s] sending on channel (will timeout)\n", us_ticker_read(), myName);
        status = g_chan.send(1, 2);
        if (status != kArTimeoutError)
        {
            printf("[%d:%s] sending didn't timeout!\n", us_ticker_read(), myName);
        }
        else
        {
            printf("[%d:%s] sending timed out successfully\n", us_ticker_read(), myName);
        }
    }
}

extern "C" void ADC0_IRQHandler()
{
    uint32_t result = ADC16_GetChannelConversionValue(ADC0, 0);
    g_adcResult.send(result, kArNoTimeout);
}

uint32_t read_adc_channel(uint32_t channel)
{
    adc16_channel_config_t channelConfig;
    channelConfig.channelNumber = channel;
    channelConfig.enableInterruptOnConversionCompleted = true;
    channelConfig.enableDifferentialConversion = false;

    ADC16_SetChannelConfig(ADC0, 0, &channelConfig);

    uint32_t result = g_adcResult.receive();
    return result;
}

void init_adc()
{
    adc16_config_t config;
    ADC16_GetDefaultConfig(&config);
    config.resolution = kADC16_Resolution16Bit;
    ADC16_Init(ADC0, &config);
    ADC16_DoAutoCalibration(ADC0);
    ADC16_SetHardwareAverage(ADC0, kADC16_HardwareAverageCount16);
    EnableIRQ(ADC0_IRQn);
}

void get_vdd()
{
    Ar::Mutex::Guard guard(g_adcMutex);

    // Enable bandgap.
    PMC->REGSC |= PMC_REGSC_BGBE_MASK;

    // Get VDD value measured in mV: VDD = (ADCR_VDD x V_BG) / ADCR_BG
    float vdd = ADCR_VDD * V_BG / (float)read_adc_channel(ADC0_BANDGAP);
    printf("vdd=%.3f mV\n", vdd);

    // Calibrate ADCR_TEMP25: ADCR_TEMP25 = ADCR_VDD x V_TEMP25 / VDD
    adcrTemp25 = ADCR_VDD * V_TEMP25 / vdd;

    // ADCR_100M = ADCR_VDD x M x 100 / VDD
    adcr100m = ADCR_VDD * M * 100.0f / vdd;
//     adcr100m = (ADCR_VDD * M) / (vdd * 10);

    // Disable bandgap.
    PMC->REGSC &= ~PMC_REGSC_BGBE_MASK;
}

void fp1_thread(void * arg)
{
    Ar::TypedChannel<float> & chan = *reinterpret_cast<Ar::TypedChannel<float> *>(arg);
    get_vdd();

    while (1)
    {
//         float v = adc.read();

        float currentTemperature;
        {
            Ar::Mutex::Guard guard(g_adcMutex);

            // Temperature = 25 - (ADCR_T - ADCR_TEMP25) * 100 / ADCR_100M
            uint32_t t = read_adc_channel(ADC0_TEMP);
            currentTemperature = (STANDARD_TEMP - ((float)t - adcrTemp25) * 100.0f / adcr100m);
        }

        printf("sending temp=%.3f\n", currentTemperature);
        currentTemperature >> chan; // g_fchan;
    }
}

void fp2_thread(void * arg)
{
    const int kHistoryCount = 10;
    float history[kHistoryCount];
    int count = 0;
    while (1)
    {
        float v;
        v <<= g_fchan;
        printf("received temp=%.3f\n", v);

        if (count >= kHistoryCount)
        {
            memcpy(&history[0], &history[1], sizeof(float)*(kHistoryCount-1));
            --count;
        }
        history[count] = v;
        ++count;

        float sum = 0;
        int i;
        for (i=0; i < count; ++i)
        {
            sum += history[i];
        }
        float avg = sum / float(count);

        printf("average temp=%.3f over last %d samples\n", avg, count);

        Ar::Thread::sleep(1000);
    }
}

void dyn_test(void * arg)
{
    printf("hi from dyn_test\n");
    printf("__cplusplus = '%d'\n", __cplusplus);
}

void my_timer_fn(Ar::Timer * t, void * arg)
{
    float temp = g_tempchan.receive(kArNoTimeout);
}

void created_thread(void * arg)
{
    Ar::Semaphore * doneSem = reinterpret_cast<Ar::Semaphore *>(arg);
    printf("hello from a created thread\n");
    doneSem->put();
}

void creator_thread(void * arg)
{
    uint32_t n = 1;
    while (1)
    {
//         Ar::Semaphore * doneSem = new Ar::Semaphore("done", 0);
//         Ar::Thread * created = new Ar::Thread("created", created_thread, doneSem, 512, 97);
//
//         printf("creator thread: waiting on done sem\n");
//         doneSem->get();
//         printf("creator thread: got done sem\n");
//
//         delete created;
//         delete doneSem;

        g_q.send(n);

        Ar::Thread::sleep(1250);
        ++n;
    }
}

void rl_thread(void * arg)
{
    ar_runloop_t rl;
    ar_runloop_create(&rl, "rl", ar_thread_get_current());

// ar_status_t ar_runloop_perform(ar_runloop_t * runloop, ar_runloop_function_t function, void * param);

    Ar::Timer myTimer("mine2", my_timer_fn, 0, kArPeriodicTimer, 500);
    ar_runloop_add_timer(&rl, &myTimer);
    myTimer.start();

    ar_runloop_add_queue(&rl, &g_q, NULL, 0);

    while (true)
    {
        ar_runloop_status_t result;
        void * obj;
        result = ar_runloop_run(&rl, kArInfiniteTimeout, &obj);
        printf("ar_runloop_run returned %d, obj=%x\r\n", result, obj);

        switch (result)
        {
            case kArRunLoopError:
                break;
            case kArRunLoopStopped:
                break;
            case kArRunLoopQueueReceived:
                printf("runloop queue received\n");
                if (obj == static_cast<ar_queue_t *>(&g_q))
                {
                    uint32_t v = g_q.receive();
                    printf("  value = %d\n", v);
                }
                break;
            case kArRunLoopChannelReceived:
                break;
            default:
                break;
        }
    }
}

void rl2_thread(void * arg)
{
    Ar::RunLoop rl("rl2", ar_thread_get_current());

    while (true)
    {
        ar_runloop_status_t result = rl.run();
    }
}

void main_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();
    printf("[%d:%s] Main thread is running\r\n", us_ticker_read(), myName);

    init_adc();

//     g_xThread.resume();
//     g_yThread.resume();

    g_rlThread.resume();
//     g_rl2Thread.resume();

    Foo * foo = new Foo;
    g_fooThread.init("foo", foo, &Foo::my_entry, NULL, 512, 120);

//     g_dyn = new Ar::Thread("dyn", dyn_test, 0, 512, 120);

//     g_fpThread1 = new Ar::Thread("fp1", fp1_thread, &g_fchan, 1024, 100);
//     g_fpThread2 = new Ar::Thread("fp2", fp2_thread, 0, 1024, 100);

//     g_timerFeederThread = new Ar::Thread("temp2", fp1_thread, &g_tempchan, 512, 100);

//     g_spinner.init("spinner", sem_spin_thread, 0, NULL, 512, 70);

    g_creatorThread.resume();

    printf("[%d:%s] goodbye!\r\n", us_ticker_read(), myName);
}

int main(void)
{
//     us_ticker_init();

    printf("Running test...\r\n");

#if 1
    printf("sizeof(ar_thread_t)=%d; sizeof(Thread)=%d\r\n", sizeof(ar_thread_t), sizeof(Ar::Thread));
    printf("sizeof(ar_channel_t)=%d; sizeof(Channel)=%d\r\n", sizeof(ar_channel_t), sizeof(Ar::Channel));
    printf("sizeof(ar_semaphore_t)=%d; sizeof(Semaphore)=%d\r\n", sizeof(ar_semaphore_t), sizeof(Ar::Semaphore));
    printf("sizeof(ar_mutex_t)=%d; sizeof(Mutex)=%d\r\n", sizeof(ar_mutex_t), sizeof(Ar::Mutex));
    printf("sizeof(ar_queue_t)=%d; sizeof(Queue)=%d\r\n", sizeof(ar_queue_t), sizeof(Ar::Queue));
    printf("sizeof(ar_timer_t)=%d; sizeof(Timer)=%d\r\n", sizeof(ar_timer_t), sizeof(Ar::Timer));
    printf("sizeof(ar_runloop_t)=%d; sizeof(RunLoop)=%d\r\n", sizeof(ar_runloop_t), 0 /*sizeof(Ar::RunLoop)*/);
#endif

#if defined(__ICCARM__)
    main_thread(0);
    Ar::Thread::getCurrent()->suspend();
#else
//     g_mainThread.init("main", main_thread, 0, 56);
//     g_mainThread.resume();
    ar_kernel_run();
    Ar::_halt();
#endif

    return 0;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
