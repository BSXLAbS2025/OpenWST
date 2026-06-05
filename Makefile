# OpenWST Kernel Makefile
# Target: Samsung J120F (ARMv7, Exynos 3475)

ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabi-

# Путь к тулчейну (измени под свой)
TOOLCHAIN ?= /usr/bin

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
AS = $(CROSS_COMPILE)as
AR = $(CROSS_COMPILE)ar

# Оптимизация и отладка
CFLAGS = -O2 -g -Wall -Wextra -Wno-unused-parameter
CFLAGS += -fno-omit-frame-pointer
CFLAGS += -ffreestanding -fno-stack-protector
CFLAGS += -Iinclude

LDFLAGS = -T linker.ld -nostdlib

# Исходные файлы ядра
KERNEL_SRCS = \
    arch/arm/boot/start.S \
    arch/arm/boot/entry.c \
    kernel/main.c \
    kernel/syscalls.c \
    kernel/sched.c \
    kernel/mm.c \
    kernel/printk.c \
    drivers/mali_stub.c \
    drivers/ashmem_stub.c \
    drivers/binder_stub.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
KERNEL_OBJS := $(KERNEL_OBJS:.S=.o)

# Цель по умолчанию
all: kernel.bin

# Компиляция .S (ассемблер)
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция .c
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Линковка ядра
kernel.elf: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Конвертация в плоский бинарник (для прошивки)
kernel.bin: kernel.elf
	$(CROSS_COMPILE)objcopy -O binary $< $@

# Очистка
clean:
	rm -f $(KERNEL_OBJS) kernel.elf kernel.bin

# Установка на устройство (через fastboot/heimdall)
install: kernel.bin
	heimdall flash --KERNEL kernel.bin

.PHONY: all clean install
