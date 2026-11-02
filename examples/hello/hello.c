#include "../../sdk/flareos.h"

FLARE_APP();

static void draw_word(flare_i32 col) {
    static const char word[] = "Hello, FlareOS";
    for (flare_u32 c = 0; c < 80; c++)
        flare_put_cell(12, c, ' ', 0x00);
    for (flare_u32 i = 0; word[i]; i++) {
        flare_i32 x = col + (flare_i32)i;
        if (x >= 0 && x < 80)
            flare_put_cell(12, (flare_u32)x, (flare_u8)word[i], 0x0B);
    }
}

void app_main(void) {
    flare_clear();
    flare_print("hello app: slide-in starting\n");
    flare_tone(784, 160);
    flare_anim_start(0, FLARE_EASE_OUT_CUBIC, 40, -10, 30);
    for (flare_u32 f = 0; f < 50; f++) {
        draw_word(flare_anim_value(0));
        flare_sleep(1);
    }
    flare_print("hello app: done\n");
}
