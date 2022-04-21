#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "drw.h"


#define ALIGN_UNSET INT_MIN
#define ALIGN_CENTER INT_MIN + 1

typedef struct Alignment {
    int top;
    int bottom;
    int left;
    int right;
} Alignment;


#define ALIGNMENT_ASSIGN_STR(alignment, parameter, str) \
    switch (str[0]) { \
        case 'C': alignment.parameter = ALIGN_CENTER; break; \
        case 'U': alignment.parameter = ALIGN_UNSET; break; \
        default: alignment.parameter = atoi(str); break; \
    }


void
calc_alignment_xy(const Alignment *alignment, int obj_w, int obj_h, int container_w, int container_h, int *x, int *y)
{
    *y = alignment->top == ALIGN_CENTER || alignment->bottom == ALIGN_CENTER
        ? (container_h - obj_h) / 2
        : alignment->top != ALIGN_UNSET
            ? alignment->top
            : alignment->bottom != ALIGN_UNSET
                ? container_h - obj_h - alignment->bottom
                : 0;

    *x = alignment->left == ALIGN_CENTER || alignment->right == ALIGN_CENTER
        ? (container_w - obj_w) / 2
        : alignment->left != ALIGN_UNSET
            ? alignment->left
            : alignment->right != ALIGN_UNSET
                ? container_w - obj_w - alignment->right
                : 0;
}


#include "config.h"

int
main (int argc, const char *argv[])
{
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Could not open display.\n");
        exit(EXIT_FAILURE);
    }

    const char **fonts = default_fonts;
    int fonts_len = sizeof(default_fonts) / sizeof(char*);
    const char *status_collecting_command = default_status_collecting_command;
    const char *cur_arg;
    const char *color_scheme[] = {
        default_text_color,
        default_background_color
    };

#ifdef USE_ARGS
    for (int i = 1; i < argc; i++) {
        cur_arg = argv[i];
        
        if (cur_arg[0] == '-') switch (cur_arg[1]) {
            // --<x>
            case '-':
                if (strcmp(cur_arg+2, "help") == 0) {
                    printf(
                        "Usage: light-status [flags]\n\n"
                        "Flags:\n"
                        "    --help              - display help\n"
                        "    -i <data-command>   - data collection command\n\n"
                        "        PANEL CONFIG\n"
                        "    -w <width>          - panel width\n"
                        "    -h <height>         - panel height\n"
                        "    -[l,r,t,b] <value>  - panel left, right, top and bottom alignment\n"
                        "    -c <color>          - panel color\n\n"
                        "        TEXT CONFIG\n"
                        "    -T[l,r,t,b] <value> - text left, right, top and bottom alignment\n"
                        "    -Tf <font>          - font pattern\n"
                        "    -Tc <color>         - text color\n\n"
                        "<data-command> is a command that will be executed with popen() to show its output.\n"
                        "    The command should periodically return a value, for example:\n"
                        "        \"while true; do echo `date`; sleep 1; done\"\n"
                        "    or\n"
                        "        \"slstatus -s\"\n\n"
                        "Alignment <value> can be:\n"
                        "    C - center\n"
                        "    U - unset\n"
                        "    <number> - offset in pixels\n\n"
                        "<color> should be in hex format with leading # (#000fff)\n\n"
                        "<font> should be in pattern: <font-name>[:size=<font-size>]\n"
                        "    <font-name> can be:\n"
                        "       actual name\n"
                        "       font family name - monospace, sans, etc.\n\n"
                    );
                    exit(0);
                }
                break;
            // -T<x>
            case 'T':
                switch (cur_arg[2]) {
                    case 'l':
                        cur_arg = argv[++i];
                        ALIGNMENT_ASSIGN_STR(text_alignment, left, cur_arg);
                        break;
                    case 'r':
                        cur_arg = argv[++i];
                        ALIGNMENT_ASSIGN_STR(text_alignment, right, cur_arg);
                        break;
                    case 't':
                        cur_arg = argv[++i];
                        ALIGNMENT_ASSIGN_STR(text_alignment, top, cur_arg);
                        break;
                    case 'b':
                        cur_arg = argv[++i];
                        ALIGNMENT_ASSIGN_STR(text_alignment, bottom, cur_arg);
                        break;
                    case 'f':
                        fonts_len = 1;
                        fonts = argv + (++i);
                        break;
                    case 'c':
                        cur_arg = argv[++i];
                        color_scheme[0] = cur_arg;
                        break;
                }
                break;
            // -<x>
            case 'l':
                cur_arg = argv[++i];
                ALIGNMENT_ASSIGN_STR(panel_alignment, left, cur_arg);
                break;
            case 'r':
                cur_arg = argv[++i];
                ALIGNMENT_ASSIGN_STR(panel_alignment, right, cur_arg);
                break;
            case 't':
                cur_arg = argv[++i];
                ALIGNMENT_ASSIGN_STR(panel_alignment, top, cur_arg);
                break;
            case 'b':
                cur_arg = argv[++i];
                ALIGNMENT_ASSIGN_STR(panel_alignment, bottom, cur_arg);
                break;
            case 'w':
                cur_arg = argv[++i];
                panel_width = atoi(cur_arg);
                break;
            case 'h':
                cur_arg = argv[++i];
                panel_height = atoi(cur_arg);
                break;
            case 'c':
                cur_arg = argv[++i];
                color_scheme[1] = cur_arg;
                break;
            case 'i':
                status_collecting_command = argv[++i];
                break;
        }
    }
#endif

    int screen = DefaultScreen(dpy);
    Visual *visual = DefaultVisual(dpy, screen);
    int depth  = DefaultDepth(dpy, screen);
    Window root_window = DefaultRootWindow(dpy);

    int dpy_width = DisplayWidth(dpy, screen);
    int dpy_height = DisplayHeight(dpy, screen);
    int panel_x, panel_y;
    calc_alignment_xy(&panel_alignment, panel_width, panel_height, dpy_width, dpy_height, &panel_x, &panel_y);

    XSetWindowAttributes window_attributes = {
        .override_redirect = True,
    };

    Window window = XCreateWindow(
        dpy,
        root_window,  // parent
        panel_x, panel_y,  // x, y
        panel_width, panel_height,  // width, height
        0,  // border width
        depth,  // depth
        InputOutput,  // class
        visual,  // visual
        CWOverrideRedirect,  // value mask
        &window_attributes
    );
    XMapWindow(dpy, window);

    Drw *drw = drw_create(dpy, screen, root_window, panel_width, panel_height);
    drw_fontset_create(drw, fonts, fonts_len);
    Clr *scheme = drw_scm_create(drw, color_scheme, sizeof(color_scheme) / sizeof(char*));
    drw_setscheme(drw, scheme);
    
    FILE *slstatus_pipe;
    char status[max_status_len];
    size_t status_len;
    unsigned int status_row_h, status_row_w;
    int status_row_x, status_row_y;

    /* Open the command for reading. */
    slstatus_pipe = popen(status_collecting_command, "r");
    if (slstatus_pipe == NULL) {
        printf("Failed to run command\n");
        return 1;
    }

    /* Read the output a line at a time - output it. */
    while (fgets(status, max_status_len, slstatus_pipe) != NULL) {
        status_len = strlen(status);
        drw_font_getexts(drw->fonts, status, status_len, &status_row_w, &status_row_h);
        calc_alignment_xy(&text_alignment, status_row_w, status_row_h, panel_width, panel_height, &status_row_x, &status_row_y);

        for (int i = 0; i < status_len; i++) {
            if (status[i] == '\n') {
                status[i] = ' ';
            }
        }
        
        XClearWindow(dpy, window);
        drw_rect(drw, 0, 0, panel_width, panel_height, true, true);
        drw_text(
            drw,
            status_row_x, status_row_y,  // x, y
            status_row_w, status_row_h,  // width, height
            0,  // align
            status,
            false  // invert color
        );
        drw_map(drw, window, 0, 0, panel_width, panel_height);
        
        XFlush(dpy);
    }

    pclose(slstatus_pipe);
 
    drw_free(drw);
    free(scheme);

    XDestroyWindow(dpy, window);
    XCloseDisplay(dpy);

    return 0;
}
