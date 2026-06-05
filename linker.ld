ENTRY(_start)

SECTIONS {
    . = 0xC0000000;  /* Адрес загрузки ядра (для ARM) */
    
    .text : {
        *(.text.boot)
        *(.text*)
    }
    
    .rodata : {
        *(.rodata*)
    }
    
    .data : {
        *(.data*)
    }
    
    .bss : {
        sbss = .;
        *(.bss*)
        ebss = .;
    }
    
    . = ALIGN(4096);
    . = . + 0x4000;  /* стек 16KB */
    stack_top = .;
}
