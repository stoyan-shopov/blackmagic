CROSS_COMPILE ?= arm-none-eabi-
ST_BOOTLOADER ?=
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

OPT_FLAGS = -Os
CFLAGS += -mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard \
	-DSTM32F7 -DDISCOVERY_STLINK -I../libopencm3/include \
	-I platforms/stm32
LDFLAGS_BOOT := $(LDFLAGS) -mfpu=fpv5-sp-d16 -mfloat-abi=hard \
	--specs=nano.specs -lopencm3_stm32f7 \
	-Wl,-T,platforms/stlinkv3-mini/stlinkv3-mini.ld -nostartfiles -lc \
	-Wl,-Map=mapfile -mthumb -mcpu=cortex-m7 -Wl,-gc-sections \
	-L../libopencm3/lib
ifeq ($(ST_BOOTLOADER), 1)
$(info  Load address 0x08004000 for original ST-LinkV2 Bootloader)
CFLAGS += -DST_BOOTLOADER
LDFLAGS = $(LDFLAGS_BOOT) -Wl,-Ttext=0x8004000
else
# TODO: relocate this address when debugging is finished
LDFLAGS = $(LDFLAGS_BOOT) -Wl,-Ttext=0x8000000
endif

ifeq ($(ENABLE_DEBUG), 1)
LDFLAGS += --specs=rdimon.specs
else
LDFLAGS += --specs=nosys.specs
endif

SRC += stlinkv3-mini-swdptap.c

VPATH += platforms/stm32
HAS_SWDPTAP_LL = 1

SRC += 	cdcacm.c	\
	serialno.c	\
	timing.c	\
	timing_stm32.c	\

ifeq ($(ST_BOOTLOADER), 1)
all:	blackmagic.bin
else
all:	blackmagic.bin blackmagic_dfu.bin blackmagic_dfu.hex dfu_upgrade.bin dfu_upgrade.hex
endif
blackmagic_dfu.elf: usbdfu.o dfucore.o dfu_f1.o stlink_common.o
	@echo "  LD      $@"
	$(Q)$(CC) $^ -o $@ $(LDFLAGS_BOOT)

dfu_upgrade.elf: dfu_upgrade.o dfucore.o dfu_f1.o  stlink_common.o
	@echo "  LD      $@"
	$(Q)$(CC) $^ -o $@ $(LDFLAGS)

host_clean:
	-$(Q)$(RM) *.bin *elf *hex

