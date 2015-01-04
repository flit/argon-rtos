CHIP = K22F51212
CHOOSE_FLOAT = HARD_FP
APP_NAME = argon_test
build ?= debug
LD_FILE_NAME = MK22FN512xxx12_flash.ld

include ../mk/common.mk

SOURCES = $(PROJ_ROOT)/test/src/basic_test.cpp \
    $(PROJ_ROOT)/test/src/test_mutex_1.cpp \
    $(PROJ_ROOT)/test/src/test_queue_1.cpp \
    $(PROJ_ROOT)/test/src/test_sem_1.cpp \
    $(PROJ_ROOT)/test/src/test_sleep_1.cpp \
    $(PROJ_ROOT)/src/startup/startup.c \
    $(PROJ_ROOT)/src/startup/MK22F51212/system_MK22F51212.c \
    $(PROJ_ROOT)/src/startup/MK22F51212/gcc/startup_MK22F51212.S \
    $(PROJ_ROOT)/src/argon/src/ar_channel.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_kernel.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_mutex.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_queue.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_sem.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_thread.cpp \
    $(PROJ_ROOT)/src/argon/src/ar_timer.cpp \
    $(PROJ_ROOT)/src/argon/cortex_m/ar_handlers_cm4.S \
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
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/analogin_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/analogout_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/gpio_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/gpio_irq_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/i2c_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/pinmap.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/port_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/pwmout_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/rtc_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/serial_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/sleep.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/spi_api.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/us_ticker.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_K22F/cmsis_nvic.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_K22F/mbed_overrides.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_K22F/PeripheralPins.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_K22F/MK22F51212/fsl_clock_K22F51212.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_K22F/MK22F51212/fsl_sim_hal_K22F51212.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/drivers/clock/fsl_clock_manager.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/drivers/pit/src/fsl_pit_driver.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/drivers/pit/src/fsl_pit_irq.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/drivers/pit/common/fsl_pit_common.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/adc/fsl_adc_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/dac/fsl_dac_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/dspi/fsl_dspi_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/flextimer/fsl_ftm_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/gpio/fsl_gpio_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/i2c/fsl_i2c_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/lptmr/fsl_lptmr_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/lpuart/fsl_lpuart_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/mcg/fsl_mcg_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/mcg/fsl_mcg_hal_modes.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/osc/fsl_osc_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/pdb/fsl_pdb_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/pit/fsl_pit_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/pmc/fsl_pmc_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/port/fsl_port_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/rcm/fsl_rcm_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/rtc/fsl_rtc_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/sai/fsl_sai_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/sdhc/fsl_sdhc_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/sim/fsl_sim_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/smc/fsl_smc_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/uart/fsl_uart_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/wdog/fsl_wdog_hal.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/utilities/src/fsl_misc_utilities.c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/utilities/src/fsl_os_abstraction_mbed.c

DEFINES += -DCPU_MK22FN512VLH12 -DTARGET_K22F -DTOOLCHAIN_GCC

INCLUDES = $(PROJ_ROOT)/test/src \
    $(PROJ_ROOT)/src \
    $(PROJ_ROOT)/src/include \
    $(PROJ_ROOT)/src/include/device \
    $(PROJ_ROOT)/src/include/device/MK22F51212 \
    $(PROJ_ROOT)/src/startup \
    $(PROJ_ROOT)/src/startup/MK22F51212 \
    $(PROJ_ROOT)/src/argon/cortex_m \
    $(PROJ_ROOT)/src/drivers \
    $(PROJ_ROOT)/src/mbed/hal \
    $(PROJ_ROOT)/src/mbed/api \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_K22F \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_K22F/MK22F51212 \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/utilities \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/adc \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/dac \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/dspi \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/enet \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/flextimer \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/gpio \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/i2c \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/llwu \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/lptmr \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/mcg \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/osc \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/pdb \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/pit \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/pmc \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/port \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/rtc \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/sai \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/sdhc \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/sim \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/smc \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/uart \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/hal/wdog \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/drivers/clock \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/drivers/enet \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/drivers/interrupt \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/drivers/pit \
    $(PROJ_ROOT)/src/mbed/targets/TARGET_KPSDK_MCUS/TARGET_KPSDK_CODE/drivers/pit/common

include $(PROJ_ROOT)/gcc/mk/targets.mk

