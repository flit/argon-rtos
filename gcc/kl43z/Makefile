CHIP = KL43Z4
APP_NAME = argon_test
build ?= debug
LD_FILE_NAME = MKL43Z256xxx4_flash.ld

include ../mk/common.mk

SOURCES = $(PROJ_ROOT)/test/src/basic_test.cpp \
    $(PROJ_ROOT)/test/src/test_mutex_1.cpp \
    $(PROJ_ROOT)/test/src/test_queue_1.cpp \
    $(PROJ_ROOT)/test/src/test_sem_1.cpp \
    $(PROJ_ROOT)/test/src/test_sleep_1.cpp \
    $(PROJ_ROOT)/src/startup/startup.c \
    $(PROJ_ROOT)/src/startup/MKL43Z4/system_MKL43Z4.c \
    $(PROJ_ROOT)/src/startup/MKL43Z4/gcc/startup_MKL43Z4.S \
    $(PROJ_ROOT)/src/argon/src/ar_channel.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_kernel.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_mutex.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_queue.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_sem.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_thread.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_timer.cpp \
    $(PROJ_ROOT)/src/argon/cortex_m/ar_handlers_cm0p.S \
    $(PROJ_ROOT)/src/argon/cortex_m/ar_port.cpp \
    $(PROJ_ROOT)/src/argon/cortex_m/ar_main_thread.cpp \
    $(PROJ_ROOT)/src/drivers/nrf/nrf.cpp \
    $(PROJ_ROOT)/src/mbed/common/assert.c \
    $(PROJ_ROOT)/src/mbed/common/board.c \
    $(PROJ_ROOT)/src/mbed/common/BusIn.cpp \
    $(PROJ_ROOT)/src/mbed/common/BusOut.cpp \
    $(PROJ_ROOT)/src/mbed/common/CallChain.cpp \
    $(PROJ_ROOT)/src/mbed/common/CAN.cpp \
    $(PROJ_ROOT)/src/mbed/common/error.c \
    $(PROJ_ROOT)/src/mbed/common/exit.c \
    $(PROJ_ROOT)/src/mbed/common/FileBase.cpp \
    $(PROJ_ROOT)/src/mbed/common/FileLike.cpp \
    $(PROJ_ROOT)/src/mbed/common/FilePath.cpp \
    $(PROJ_ROOT)/src/mbed/common/FileSystemLike.cpp \
    $(PROJ_ROOT)/src/mbed/common/FunctionPointer.cpp \
    $(PROJ_ROOT)/src/mbed/common/gpio.c \
    $(PROJ_ROOT)/src/mbed/common/I2C.cpp \
    $(PROJ_ROOT)/src/mbed/common/I2CSlave.cpp \
    $(PROJ_ROOT)/src/mbed/common/InterruptIn.cpp \
    $(PROJ_ROOT)/src/mbed/common/InterruptManager.cpp \
    $(PROJ_ROOT)/src/mbed/common/LocalFileSystem.cpp \
    $(PROJ_ROOT)/src/mbed/common/mbed_interface.c \
    $(PROJ_ROOT)/src/mbed/common/pinmap_common.c \
    $(PROJ_ROOT)/src/mbed/common/RawSerial.cpp \
    $(PROJ_ROOT)/src/mbed/common/retarget.cpp \
    $(PROJ_ROOT)/src/mbed/common/rtc_time.c \
    $(PROJ_ROOT)/src/mbed/common/semihost_api.c \
    $(PROJ_ROOT)/src/mbed/common/Serial.cpp \
    $(PROJ_ROOT)/src/mbed/common/SerialBase.cpp \
    $(PROJ_ROOT)/src/mbed/common/SPI.cpp \
    $(PROJ_ROOT)/src/mbed/common/SPISlave.cpp \
    $(PROJ_ROOT)/src/mbed/common/Stream.cpp \
    $(PROJ_ROOT)/src/mbed/common/Ticker.cpp \
    $(PROJ_ROOT)/src/mbed/common/Timeout.cpp \
    $(PROJ_ROOT)/src/mbed/common/Timer.cpp \
    $(PROJ_ROOT)/src/mbed/common/TimerEvent.cpp \
    $(PROJ_ROOT)/src/mbed/common/us_ticker_api.c \
    $(PROJ_ROOT)/src/mbed/common/wait_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/analogin_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/analogout_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/gpio_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/i2c_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/pinmap.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/port_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/pwmout_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/rtc_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/sleep.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/us_ticker.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/TARGET_KL43Z/cmsis_nvic.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/TARGET_KL43Z/gpio_irq_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/TARGET_KL43Z/mbed_overrides.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/TARGET_KL43Z/PeripheralPins.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/TARGET_KL43Z/serial_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/TARGET_KL43Z/spi_api.c

DEFINES += -DCPU_MKL43Z256VLH4 -DTARGET_KL43Z -DTOOLCHAIN_GCC

INCLUDES = $(PROJ_ROOT)/test/src \
    $(PROJ_ROOT)/src \
    $(PROJ_ROOT)/src/include \
    $(PROJ_ROOT)/src/include/device \
    $(PROJ_ROOT)/src/include/device/MKL43Z4 \
    $(PROJ_ROOT)/src/startup \
    $(PROJ_ROOT)/src/startup/MKL43Z4 \
    $(PROJ_ROOT)/src/argon/cortex_m \
    $(PROJ_ROOT)/src/drivers \
    $(PROJ_ROOT)/src/mbed/hal \
    $(PROJ_ROOT)/src/mbed/api \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KLXX/TARGET_KL43Z

include $(PROJ_ROOT)/gcc/mk/targets.mk

