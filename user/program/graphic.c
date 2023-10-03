//
// Created by endaytrer on 2023/10/3.
//
#include <lib.h>
#include <fcntl.h>
#define SCREEN_WIDTH 1280

#define FONT_WIDTH 12
#define FONT_HEIGHT 16
#define BITMAP_COLS 16
#define BITMAP_WIDTH (FONT_WIDTH * BITMAP_COLS)

#define RGBA(r, g, b, a) ((((a) & 255) << 24) | (((r) & 255) << 16) | (((g) & 255) << 8) | ((b) & 255))
#define BG_COLOR RGBA(0, 0, 0, 0)



int main(int argc, const char *argv[]) {
    
    int fd_bitmap = open("/font.bitmap", O_RDONLY, 0644);
    int fd_fb = open("/dev/framebuffer", O_RDWR, 0666);
    int x = 0, y = 0, cols = 80;
    srand(get_time());
    char buf[] = "Hello world!";
    u32 FONT_COLOR = RGBA(rand() % 255, rand() % 255, rand() % 255, 255);
    for (char *ptr = buf; *ptr != '\0'; ++ptr) {
        if (*ptr == '\n') {
            x = 0;
            y++;
        }
        int char_x = *ptr % BITMAP_COLS;
        int char_y = *ptr / BITMAP_COLS;
        int bitmap_x = char_x * FONT_WIDTH;
        int bitmap_y = char_y * FONT_HEIGHT;

        char row_buf[(FONT_WIDTH >> 3) + ((FONT_WIDTH & 0x7) ? 1 : 0)];
        for (int i = 0; i < FONT_HEIGHT; i++) {
            u32 row_bitmap[FONT_WIDTH];
            int pos = (BITMAP_WIDTH * (bitmap_y + i) + bitmap_x);
            int pos_byte = pos >> 3;
            int pos_offset = pos & 0x7;
            int pos_top = pos + FONT_WIDTH;
            int pos_top_offset = pos_top & 0x7;
            int pos_top_byte = (pos_top >> 3) + (pos_top_offset ? 1 : 0);
            lseek(fd_bitmap, pos_byte, SEEK_SET);
            read(fd_bitmap, row_buf, pos_top_byte - pos_byte);
            for (int j = 0; j < FONT_WIDTH; j++) {
                if ((row_buf[(pos_offset + j) >> 3] >> (7 - ((pos_offset + j) & 0x7))) & 1) {
                    row_bitmap[j] = FONT_COLOR;
                } else {
                    row_bitmap[j] = BG_COLOR;
                }
            }
            lseek(fd_fb, 4 * ((y * FONT_HEIGHT + i) * SCREEN_WIDTH + x * FONT_WIDTH), SEEK_SET);
            write(fd_fb, (const char *) row_bitmap, sizeof(row_bitmap));
        }
        x++;
        if (x == cols) {
            x = 0;
            y++;
        }
        read(fd_fb, NULL, 0);
        FONT_COLOR++;
    }
    close(fd_bitmap);
    close(fd_fb);
    return 0;
}