// Ранний framebuffer для отладки (пример для J120F)
static unsigned int *fb = (unsigned int *)0xc0000000; // адрес фреймбуфера

void putchar_fb(char c) {
    static int x = 0, y = 0;
    if (c == '\n') {
        x = 0;
        y++;
    } else {
        fb[y * 800 + x] = c;  // упрощённо, по факту нужен шрифт
        x++;
    }
}
