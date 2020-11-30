CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

NO_BOOTLOADER ?=
OPT_FLAGS = -Os
CFLAGS += -mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard \
	-DSTM32F7 -DDISCOVERY_STLINK -I../libopencm3/include \
	-I platforms/stm32
LDFLAGS_BOOT := $(LDFLAGS) -mfpu=fpv5-sp-d16 -mfloat-abi=hard \
	--specs=nano.specs -lopencm3_stm32f7 \
	-Wl,-T,platforms/stlinkv3-mini/stlinkv3-mini.ld -nostartfiles -lc \
	-Wl,-Map=mapfile -mthumb -mcpu=cortex-m7 -Wl,-gc-sections \
	-L../libopencm3/lib

# TODO: relocate this address when debugging is finished
LDFLAGS = $(LDFLAGS_BOOT)
CFLAGS +=  -DUSE_ST_SERIAL
ifneq ($(NO_BOOTLOADER), 1)
LDFLAGS +=  -Wl,-Ttext=0x8004000
endif

ifeq ($(ENABLE_DEBUG), 1)
LDFLAGS += --specs=rdimon.specs
else
LDFLAGS += --specs=nosys.specs
endif

#SRC += stlinkv3-mini-swdptap.c

VPATH += platforms/stm32
#HAS_SWDPTAP_LL = 1

SRC += 	usbuart.c	\
	cdcacm.c	\
	serialno.c	\
	timing.c	\
	timing_stm32.c	\
	traceswoasync.c	\
	traceswodecode.c	\

ifeq ($(NO_BOOTLOADER), 1)
all:	blackmagic.bin
else
all:	blackmagic.bin blackmagic_dfu.bin blackmagic_dfu.hex

blackmagic_dfu.elf: usbdfu.o dfucore.o dfu_f4.o
	@echo "  LD      $@"
	$(Q)$(CC) $^ -o $@ $(LDFLAGS_BOOT)
endif
host_clean:
	-$(Q)$(RM) *.bin *elf *hex *.o

