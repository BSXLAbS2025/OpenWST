.section .text.boot
.global _start

_start:
    @ Переключиться в режим Supervisor (SVC)
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x13  @ SVC mode
    msr cpsr, r0
    
    @ Установить стек
    ldr sp, =stack_top
    
    @ Очистить BSS
    ldr r0, =sbss
    ldr r1, =ebss
    sub r1, r1, r0
    mov r2, #0
    bl memset
    
    @ Перейти в C-код
    bl kernel_main
    
    @ Если kernel_main вернулся — зависнуть
hang:
    nop          @ вместо wfi (не все ARM поддерживают wfi)
    b hang

@ Простая реализация memset
memset:
    cmp r1, #0
    beq 2f
1:
    strb r2, [r0], #1
    subs r1, r1, #1
    bne 1b
2:
    bx lr
