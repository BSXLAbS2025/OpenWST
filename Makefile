# OpenWST Kernel Makefile
# Target: Samsung J120F (ARMv7, Exynos 3475)

ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabi-

TOOLCHAIN ?= /usr/bin
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
AS = $(CROSS_COMPILE)as
AR = $(CROSS_COMPILE)ar

# Для Exynos 3475 (ARM Cortex-A7, ARMv7-A)
CFLAGS = -O2 -g -Wall -Wextra -Wno-unused-parameter
CFLAGS += -fno-omit-frame-pointer
CFLAGS += -ffreestanding -fno-stack-protector
CFLAGS += -Iinclude
CFLAGS += -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard
CFLAGS += -mcpu=cortex-a7

LDFLAGS = -T linker.ld -nostdlib

KERNEL_SRCS = \
    arch/arm/boot/start.S \
    arch/arm/boot/entry.c \
    kernel/main.c \
    kernel/printk.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
KERNEL_OBJS := $(KERNEL_OBJS:.S=.o)

all: kernel.bin

%.o: %.S
	$(CC) $(CFLAGS) -march=armv7-a -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

kernel.bin: kernel.elf
	$(CROSS_COMPILE)objcopy -O binary $< $@

clean:
	rm -f $(KERNEL_OBJS) kernel.elf kernel.bin

install: kernel.bin
	heimdall flash --KERNEL kernel.bin

.PHONY: all clean install
