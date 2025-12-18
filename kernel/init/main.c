#include <stdint.h>

extern const uint8_t image_map[307200 * 3];
extern const uint8_t pbar_map[20000 * 4];

__attribute__((noreturn))
void main(uint8_t *fb, int pitch)
{
    for (int i = 0; i < 480; i++) {
        for (int j = 0; j < 640; j++) {
            fb[i * pitch + j * 3] = image_map[(i * 640 + j) * 3];
            fb[i * pitch + j * 3 + 1] = image_map[(i * 640 + j) * 3 + 1];
            fb[i * pitch + j * 3 + 2] = image_map[(i * 640 + j) * 3 + 2];
        }
    }

    int center_x = 320;
    int center_y = 375;

    for (int f = 0;; f = (f + 1) % 8) {
        for (int i = center_y - 25; i < center_y + 25; i++) {
            for (int j = center_x - 25; j < center_x + 25; j++) {
                int alpha = pbar_map[((i - center_y + 25) * 400 + (j - center_x + 25 + f * 50)) * 4 + 3];
                int r = pbar_map[((i - center_y + 25) * 400 + (j - center_x + 25 + f * 50)) * 4 + 0];
                int g = pbar_map[((i - center_y + 25) * 400 + (j - center_x + 25 + f * 50)) * 4 + 1];
                int b = pbar_map[((i - center_y + 25) * 400 + (j - center_x + 25 + f * 50)) * 4 + 2];
                int bg_r = image_map[(i * 640 + j) * 3];
                int bg_g = image_map[(i * 640 + j) * 3 + 1];
                int bg_b = image_map[(i * 640 + j) * 3 + 2];

                if (!alpha) {
                    fb[i * pitch + j * 3] = bg_r;
                    fb[i * pitch + j * 3 + 1] = bg_g;
                    fb[i * pitch + j * 3 + 2] = bg_b;
                } else if (alpha == 255) {
                    fb[i * pitch + j * 3] = r;
                    fb[i * pitch + j * 3 + 1] = g;
                    fb[i * pitch + j * 3 + 2] = b;
                } else {
                    fb[i * pitch + j * 3] = bg_r * (255 - alpha) / 255 + r * alpha / 255;
                    fb[i * pitch + j * 3 + 1] = bg_g * (255 - alpha) / 255 + g * alpha / 255;
                    fb[i * pitch + j * 3 + 2] = bg_b * (255 - alpha) / 255 + b * alpha / 255;
                }
            }
        }

        for (volatile int i = 0; i < 1024 * 1024 * 16; i++) {}
    }
}
