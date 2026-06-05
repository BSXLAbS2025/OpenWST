// Адрес фреймбуфера для J120F (нужно уточнить через документацию)
#define FB_ADDR 0xC0000000
#define WIDTH   480
#define HEIGHT  800

void put_pixel(int x, int y, unsigned short color) {
    unsigned short *fb = (unsigned short *)FB_ADDR;
    fb[y * WIDTH + x] = color;
}

void draw_rect(int x1, int y1, int x2, int y2, unsigned short color) {
    for (int y = y1; y < y2; y++)
        for (int x = x1; x < x2; x++)
            put_pixel(x, y, color);
}

void kernel_main(void) {
    // Рисуем белый экран
    draw_rect(0, 0, WIDTH, HEIGHT, 0xFFFF);
    
    // Рисуем красный квадрат посередине
    draw_rect(100, 100, 380, 700, 0xF800);
    
    // Бесконечный цикл (ядро "висит")
    while (1) {
        // Можно мигать LED, если найду GPIO
    }
}
