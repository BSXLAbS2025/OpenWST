ENTRY(_start)

SECTIONS {
    . = 0xC0000000;
    
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
    . = . + 0x4000;
    stack_top = .;
}
