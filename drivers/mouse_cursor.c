#include "../kernel/kernel.h"

static int cursor_driver_init(void);

/* Current tracking coordinates for the tracking block */
static u32 cursor_x = 100;
static u32 cursor_y = 100;

/* Define the Input Type if not explicitly in your enum */
#define DRV_TYPE_INPUT 7 

static struct driver cursor_driver = {
    .name = "AirOS Input Vector",
    .type = DRV_TYPE_UNKNOWN, // Maps to your enum; can alter to custom type
    .init = cursor_driver_init,
    .shutdown = 0
};

/* Renders your 4x4 coordinate square using your native header architecture */
void render_cursor(u32 x, u32 y) {
    // Uses your native engine's draw primitive to paint the block layout
    for (u32 row = 0; row < 4; row++) {
        for (u32 col = 0; col < 4; col++) {
            draw_sqr(x + col, y + row);
        }
    }
}

/* API Hook: Moves the cursor securely based on hardware interrupts */
void cursor_move(i32 delta_x, i32 delta_y) {
    // 1. Erase or overwrite tracking if you implement a clear layer
    // 2. Compute modern layout boundary offsets 
    i32 next_x = (i32)cursor_x + delta_x;
    i32 next_y = (i32)cursor_y + delta_y;
    
    // 3. Keep coordinates clamped safely within standard limits
    if (next_x < 0) next_x = 0;
    if (next_y < 0) next_y = 0;
    
    cursor_x = (u32)next_x;
    cursor_y = (u32)next_y;
    
    // 4. Paint the fresh screen block position
    render_cursor(cursor_x, cursor_y);
}

static int cursor_driver_init(void) {
    render_cursor(cursor_x, cursor_y);
    return 1; // Success vector status
}

void cursor_driver_bootstrap(void) {
    driver_register(&cursor_driver);
}
