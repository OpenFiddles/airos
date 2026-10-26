#include "../kernel/kernel.h"

static int cursor_driver_init(void);

/* Global Variable Instantiations - Stripped static to allow extern matching */
u32 cursor_x = 10;
u32 cursor_y = 10;

static struct driver cursor_driver = {
    .name = "AirOS Input Vector",
    .type = DRV_TYPE_UNKNOWN, 
    .init = cursor_driver_init,
    .shutdown = 0
};

void cursor_move(i32 delta_x, i32 delta_y) {
    i32 next_x = (i32)cursor_x + delta_x;
    i32 next_y = (i32)cursor_y + delta_y;
    
    if (next_x < 0) next_x = 0;
    if (next_y < 0) next_y = 0;
    if (next_x > 78) next_x = 78; 
    if (next_y > 23) next_y = 23;

    cursor_x = (u32)next_x;
    cursor_y = (u32)next_y;
    
    draw_sqr(cursor_x, cursor_y);
}

static int cursor_driver_init(void) {
    draw_sqr(cursor_x, cursor_y);
    return 1;
}

void cursor_driver_bootstrap(void) {
    driver_register(&cursor_driver);
}
