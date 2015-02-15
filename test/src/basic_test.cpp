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
#include "kernel_tests.h"
#include "mbed.h"
#include "drivers/nrf/nrf.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#define TEST_CASE_CLASS TestMutex1

#if TARGET_K22F
#define ROLE_TRANSMITTER 0
#define ROLE_RECEIVER 1
#else
#define ROLE_TRANSMITTER 1
#define ROLE_RECEIVER 0
#endif

enum {
    kRadioAddress = 0xdeadbeef
};

#define ADCR_VDD                (65535.0f)    /*! Maximum value when use 16b resolution */
#define V_BG                    (1000.0f)     /*! BANDGAP voltage in mV (trim to 1.0V) */
#define V_TEMP25                (716.0f)      /*! Typical VTEMP25 in mV */
#define M                       (1620.0f)     /*! Typical slope: (mV x 1000)/oC */
#define STANDARD_TEMP           (25.0f)

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void main_thread(void * arg);
void x_thread(void * arg);
void y_thread(void * arg);
void rf_thread(void * arg);

class Foo
{
public:

    void my_entry() //void * param)
    {
        while (1)
        {
            printf("hi from Foo!\n");
            Ar::Thread::sleep(1000);
        }
    }
};

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

extern serial_t stdio_uart;

#if !defined(__ICCARM__)
Ar::ThreadWithStack<512> g_mainThread("main", main_thread, 0, 128);
#endif

// TEST_CASE_CLASS g_testCase;

Ar::ThreadWithStack<512> g_xThread("x", x_thread, 0, 130);
Ar::ThreadWithStack<512> g_yThread("y", y_thread, 0, 120);

Ar::Thread * g_fpThread1;
Ar::Thread * g_fpThread2;
Ar::TypedChannel<float> g_fchan("f");
Ar::TypedChannel<float> g_avgTempChan("avgtemp");

Ar::TypedChannel<int> g_chan("c");

Ar::Thread g_fooThread;

Ar::Thread * g_dyn;

// (ce, cs, sck, mosi, miso, irq)
#if TARGET_K22F
NordicRadio g_nrf(PTC11, PTD4, PTD1, PTD2, PTD3, PTD0);
#else
NordicRadio g_nrf(D0, D10, D13, D11, D12, D1);
#endif

Ar::Semaphore g_receiveSem("rx");
Ar::Thread * g_rfThread;

float adcrTemp25 = 0;             /*! Calibrated ADCR_TEMP25 */
float adcr100m = 0;

Ar::Mutex g_adcMutex("adc");

#if ROLE_TRANSMITTER
DigitalOut g_led(LED_RED);
#endif

Ticker g_ticker;

Ar::Mutex g_someMutex("foobar");

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

#if defined(__MICROLIB)
#pragma weak __aeabi_assert
extern "C" void __aeabi_assert(const char *s, const char *f, int l)
{
    Ar::_halt();
}
#endif

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

void get_vdd()
{
    Ar::Mutex::Guard guard(g_adcMutex);

    // Enable bandgap.
    HW_PMC_REGSC(PMC_BASE).B.BGBE = 1;

    AnalogIn bandgap(ADC0_BANDGAP);

    // Get VDD value measured in mV: VDD = (ADCR_VDD x V_BG) / ADCR_BG
    float vdd = ADCR_VDD * V_BG / (float)bandgap.read_u16();
    printf("vdd=%.3f mV\n", vdd);

    // Calibrate ADCR_TEMP25: ADCR_TEMP25 = ADCR_VDD x V_TEMP25 / VDD
    adcrTemp25 = ADCR_VDD * V_TEMP25 / vdd;

    // ADCR_100M = ADCR_VDD x M x 100 / VDD
    adcr100m = (ADCR_VDD * M) / (vdd * 10);

    // Disable bandgap.
    HW_PMC_REGSC(PMC_BASE).B.BGBE = 0;
}

void fp1_thread(void * arg)
{
    Ar::TypedChannel<float> & chan = *reinterpret_cast<Ar::TypedChannel<float> *>(arg);
    get_vdd();
    AnalogIn adc(ADC0_TEMP);

    while (1)
    {
//         float v = adc.read();

        float currentTemperature;
        {
            Ar::Mutex::Guard guard(g_adcMutex);

            // Temperature = 25 - (ADCR_T - ADCR_TEMP25) * 100 / ADCR_100M
            currentTemperature = (STANDARD_TEMP - ((float)adc.read_u16() - adcrTemp25) * 100.0f / adcr100m);
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

#if ROLE_TRANSMITTER
        avg >> g_avgTempChan;
#endif

        Ar::Thread::sleep(1000);
    }
}

void dyn_test(void * arg)
{
    printf("hi from dyn_test\n");
    printf("__cplusplus = '%d'\n", __cplusplus);
}

void rf_thread(void * arg)
{
    g_nrf.init(kRadioAddress);
    g_nrf.setReceiveSem(&g_receiveSem);
    g_nrf.dump();

#if ROLE_TRANSMITTER

    while (1)
    {
        float temp;
        temp <<= g_avgTempChan;

        g_led = 0;

        uint8_t buf[32];
        memcpy(buf, &temp, sizeof(temp));
        g_nrf.send(kRadioAddress, buf, sizeof(temp));

        g_led = 1;

        Ar::Thread::sleep(2000);
    }

#else

    while (1)
    {
        g_nrf.startReceive();

        // Wait up to 1 sec to receive a packet.
        g_receiveSem.get(1000);

        g_nrf.stopReceive();

        // Read packets from the rx FIFO.
        while (g_nrf.isPacketAvailable())
        {
            uint8_t buf[32];
            uint32_t count = g_nrf.readPacket(buf);
            if (count)
            {
                float temp;
                memcpy(&temp, buf, sizeof(temp));
                printf("Remote temperature = %.3f deg\n", temp);
            }

            g_nrf.clearReceiveFlag();
        }

        Ar::Thread::sleep(1500);
    }

#endif
}

Ar::Thread g_spinner;
// Ar::Semaphore sem("s", 0);
void sem_spin_thread(void * arg)
{
    Ar::Semaphore sem("s", 0);

    while (1)
    {
        g_someMutex.get();
        sem.put();
        sem.get();
        g_someMutex.put();
    }
}

Ar::Semaphore tsem("tsem", 0);
void ticker_thread(void * arg)
{
    while (1)
    {
        tsem.get();
    }
}

Ar::Thread g_tickThread("ticker", ticker_thread, 0, 1024, 90);

void ticker_handler(void)
{
//     printf("ticker called\n");
    tsem.put();

}

// #if TARGET_K22F
// int g_bytesSent = 0;
// Ar::StaticQueue<int, 8> g_countQ("count");
// void uart_tx_handler(uint32_t id, SerialIrq event)
// {
//     ++g_bytesSent;
//     g_countQ.send(g_bytesSent, kArNoTimeout);
// }
//
// void uart_thread(void * arg)
// {
//     while (1)
//     {
//         int n = g_countQ.receive();
//
//         if (n % 20)
//         {
//             printf("sent %d bytes\n", n);
//         }
//     }
// }
// Ar::Thread g_uartThread("uart_rx", uart_thread, 0, 1024, 92);
// #endif // TARGET_K22F

Ar::TypedChannel<float> g_tempchan("temp");
void my_timer_fn(Ar::Timer * t, void * arg)
{
    float temp = g_tempchan.receive(kArNoTimeout);
}
Ar::Timer g_myTimer("mine", my_timer_fn, 0, kArPeriodicTimer, 1500);
Ar::Thread * g_timerFeederThread;

void created_thread(void * arg)
{
    Ar::Semaphore * doneSem = reinterpret_cast<Ar::Semaphore *>(arg);
    printf("hello from a created thread\n");
    doneSem->put();
}

void creator_thread(void * arg)
{
    while (1)
    {
        Ar::Semaphore * doneSem = new Ar::Semaphore("done", 0);
        Ar::Thread * created = new Ar::Thread("created", created_thread, doneSem, 512, 97);

        printf("creator thread: waiting on done sem\n");
        doneSem->get();
        printf("creator thread: got done sem\n");

        delete created;
        delete doneSem;

        Ar::Thread::sleep(1250);
    }
}
Ar::Thread g_creatorThread("creator", creator_thread, 0, 512, 96);

void main_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();

    printf("[%d:%s] Main thread is running\r\n", us_ticker_read(), myName);

    g_myTimer.start();

// #if TARGET_K22F
//     // Hook up uart rx irq.
//     serial_irq_handler(&stdio_uart, uart_tx_handler, 1);
//     serial_irq_set(&stdio_uart, TxIrq, true);
// #endif // TARGET_K22F

#if ROLE_TRANSMITTER
    // Turn off LED
    g_led = 1;
#endif

//     while (1)
//     {
//         printf("hi\n");
//         Ar::Thread::sleep(1000);
//     }

//     g_testCase.init();
//     g_testCase.run();

    Foo * foo = new Foo;
    g_fooThread.init("foo", foo, &Foo::my_entry, NULL, 512, 120);

    g_dyn = new Ar::Thread("dyn", dyn_test, 0, 512, 120);

// #if ROLE_TRANSMITTER
    g_fpThread1 = new Ar::Thread("fp1", fp1_thread, &g_fchan, 1024, 100);
    g_fpThread2 = new Ar::Thread("fp2", fp2_thread, 0, 1024, 100);
// #endif

    g_timerFeederThread = new Ar::Thread("temp2", fp1_thread, &g_tempchan, 512, 100);

    g_rfThread = new Ar::Thread("rf", rf_thread, 0, 1536, 80);

    g_spinner.init("spinner", sem_spin_thread, 0, NULL, 512, 70);

    g_ticker.attach_us(ticker_handler, 250000);

    printf("[%d:%s] goodbye!\r\n", us_ticker_read(), myName);

}

int main(void)
{
    us_ticker_init();

    printf("Running test...\r\n");

#if 0
    printf("sizeof(Thread)=%d\r\n", sizeof(Ar::Thread));
    printf("sizeof(Channel)=%d\r\n", sizeof(Ar::Channel));
    printf("sizeof(Semaphore)=%d\r\n", sizeof(Ar::Semaphore));
    printf("sizeof(Mutex)=%d\r\n", sizeof(Ar::Mutex));
    printf("sizeof(Queue)=%d\r\n", sizeof(Ar::Queue));
    printf("sizeof(Timer)=%d\r\n", sizeof(Ar::Timer));
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
