/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
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
#ifndef _FSL_OSC_H_
#define _FSL_OSC_H_

#include "fsl_common.h"

/*! @addtogroup osc */
/*! @{*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define FSL_OSC_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0. */

/*! @brief Oscillator capacitor load setting.*/
enum _osc_cap_load
{
    kOSC_Cap2P  = OSC_CR_SC2P_MASK,  /*!< 2  pF capacitor load */
    kOSC_Cap4P  = OSC_CR_SC4P_MASK,  /*!< 4  pF capacitor load */
    kOSC_Cap8P  = OSC_CR_SC8P_MASK,  /*!< 8  pF capacitor load */
    kOSC_Cap16P = OSC_CR_SC16P_MASK  /*!< 16 pF capacitor load */
};

/*! @brief OSCERCLK enable mode. */
enum _oscer_enable_mode
{
    kOSC_ErClkEnable       = OSC_CR_ERCLKEN_MASK, /*!< Enable.              */
    kOSC_ErClkEnableInStop = OSC_CR_EREFSTEN_MASK /*!< Enable in stop mode. */
};

/*! @brief OSC configuration for OSCERCLK. */
typedef struct _oscer_config
{
    uint8_t enableMode; /*!< OSCERCLK enable mode. OR'ed value of \ref _oscer_enable_mode. */

#if (defined(FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER) && FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER)
    uint8_t erclkDiv; /*!< Divider for OSCERCLK.*/
#endif
} oscer_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief Configure the OSC external reference clock (OSCERCLK).
 *
 * This function configures the OSC external reference clock (OSCERCLK).
 * For example, to enable the OSCERCLK in normal mode and stop mode, also set
 * the output divider to 1, plese use like this:
 *
   @code
   oscer_config_t config =
   {
       .enableMode = kOSC_ErClkEnable | kOSC_ErClkEnableInStop,
       .erclkDiv   = 1U,
   };

   OSC_SetExtRefClkConfig(OSC, &config);
   @endcode
 *
 * @param base   OSC peripheral address.
 * @param config Pointer to the configuration structure.
 */
static inline void OSC_SetExtRefClkConfig(OSC_Type *base, oscer_config_t const *config)
{
    uint8_t reg = base->CR;

    reg &= ~(OSC_CR_ERCLKEN_MASK | OSC_CR_EREFSTEN_MASK);
    reg |= config->enableMode;

    base->CR = reg;

#if (defined(FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER) && FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER)
    base->DIV = OSC_DIV_ERPS(config->erclkDiv);
#endif
}

/*!
 * @brief Sets the capacitor load configuration for the oscillator.
 *
 * This function sets the specified capacitors configuration for the oscillator.
 * This should be done in the early system level initialization function call
 * based on the system configuration.
 *
 * @param base   OSC peripheral address.
 * @param capLoad OR'ed value for the capacitor load option, see \ref _osc_cap_load.
 *
 * Example:
   @code
   // To enable only 2 pF and 8 pF capacitor load, please use like this.
   OSC_SetCapLoad(OSC, kOSC_Cap2P | kOSC_Cap8P);
   @endcode
 */
static inline void OSC_SetCapLoad(OSC_Type *base, uint8_t capLoad)
{
    uint8_t reg = base->CR;

    reg &= ~(OSC_CR_SC2P_MASK | OSC_CR_SC4P_MASK | OSC_CR_SC8P_MASK | OSC_CR_SC16P_MASK);
    reg |= capLoad;

    base->CR = reg;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*! @}*/

#endif /* _FSL_OSC_H_*/
