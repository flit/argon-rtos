#-------------------------------------------------------------------------------
# Copyright (c) 2012 Freescale Semiconductor, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# o Redistributions of source code must retain the above copyright notice, this list
#   of conditions and the following disclaimer.
#
# o Redistributions in binary form must reproduce the above copyright notice, this
#   list of conditions and the following disclaimer in the documentation and/or
#   other materials provided with the distribution.
#
# o Neither the name of Freescale Semiconductor, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Compiler flags
#-------------------------------------------------------------------------------

# Enables all warnings
C_FLAGS_WARNINGS = -Wall -Wno-format

# Turn on all warnings.
COMMON_FLAGS += $(C_FLAGS_WARNINGS)

# Don't use common symbols.  This is usually done in kernels.  Makes
# code size slightly larger and increases performance.
COMMON_FLAGS += -fno-common

#Place each function or data item into its own section in the output file
COMMON_FLAGS += -ffunction-sections -fdata-sections

# Use a freestanding build environment.  Standard for kernels, implies
# std library may not exist.
COMMON_FLAGS += -ffreestanding -fno-builtin

ARCH_FLAGS=-mthumb -mcpu=$(CPU)

CSTD=-std=gnu11
CXXSTD=-std=gnu++14

STRICT_ALIASING = -Wstrict-aliasing -fstrict-aliasing

# Use float-abi=softfp for soft floating point api with HW instructions.
# Alternatively, float-abi=hard for hw float instructions and pass float args in float regs.
# Here need to be modified.
ifeq "$(CPU)" "cortex-m4"

ifeq "$(CHOOSE_FLOAT)" "SOFT_FP"
FPU_FLAGS = -mfloat-abi=softfp -mfpu=fpv4-sp-d16
CC_LIB_POST := armv7e-m/softfp
else ifeq "$(CHOOSE_FLOAT)" "HARD_FP"
FPU_FLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CC_LIB_POST := armv7e-m/fpu
else
CC_LIB_POST := armv7e-m
endif

else ifeq "$(CPU)" "cortex-m0plus"
CC_LIB_POST := armv6-m
endif

# Debug setting
ifeq "$(DEBUG)" "1"
OPT_FLAGS += -O0
DEBUG_FLAGS = -g2 -gdwarf-2 -gstrict-dwarf
DEFINES += -D_DEBUG=1 -DDEBUG
else
OPT_FLAGS += -Os
DEBUG_FLAGS +=
DEFINES += -DNDEBUG
endif

COMMON_FLAGS += $(DEBUG_FLAGS) $(OPT_FLAGS) $(FPU_FLAGS)

USE_NANO = --specs=nano.specs
USE_NOHOST=--specs=nosys.specs

# -Os -flto -ffunction-sections -fdata-sections to compile for code size
CFLAGS=$(ARCH_FLAGS) $(COMMON_FLAGS)
CXXFLAGS=$(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS=$(ARCH_FLAGS)
LDFLAGS=$(ARCH_FLAGS) $(USE_NANO) $(USE_NOHOST) $(LDSCRIPTS) $(GC) $(FPU_FLAGS)

# Link for code size
GC=-Wl,--gc-sections


#-------------------------------------------------------------------------------
# Include paths
#-------------------------------------------------------------------------------

# These include paths have to be quoted because they may contain spaces,
# particularly under cygwin.
# LDINC += -L '$(LIBGCC_LDPATH)' -L '$(LIBC_LDPATH)'
#
# # Indicate gcc and newlib std includes as -isystem so gcc tags and
# # treats them as system directories.
# SYSTEM_INC = \
#     -isystem '$(CC_INCLUDE)' \
#     -isystem '$(CC_INCLUDE_FIXED)' \
#     -isystem '$(LIBC_INCLUDE)/c++/$(CC_VERSION)' \
#     -isystem '$(LIBC_INCLUDE)/c++/$(CC_VERSION)/$(CROSS_COMPILE_STRIP)/$(ARCH)' \
#     -isystem '$(LIBC_INCLUDE)'

# INCLUDES += \
#     $(SDK_ROOT)/platform \
#     $(SDK_ROOT)/platform/hal  \
#     $(SDK_ROOT)/platform/drivers  \
#     $(SDK_ROOT)/platform/utilities  \
#     $(SDK_ROOT)/platform/CMSIS/Include  \
#     $(SDK_ROOT)/platform/CMSIS/Include/device \
#     $(SDK_ROOT)/boards

