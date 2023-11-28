#include <stdint.h>

// undef to stop handling cmdline arguments
#define USE_ARGS

uint32_t
    max_status_len = 2048;

Rect panel_rect = {
    .x = 0,
    .y = 0,
    .w = 1000,
    .h = 70,
};

// Alignment.[top, bottom, left, right]: int | ALIGN_CENTER | ALIGN_UNSET 
Alignment panel_alignment = {
    .top = ALIGN_CENTER,
    .bottom = ALIGN_UNSET,
    .left = ALIGN_CENTER,
    .right = ALIGN_UNSET,
};

Alignment text_alignment = {
    .top = ALIGN_UNSET,
    .bottom = ALIGN_CENTER,
    .left = ALIGN_UNSET,
    .right = ALIGN_CENTER,
};

const char *default_fonts[] = {"monospace:size=20"};
const char default_text_color[] = "#ffffff";
const char default_background_color[] = "#000000";

// should repeatedly put a string witn \n at the end
const char default_status_collecting_command[] = "/usr/local/bin/slstatus -s";
