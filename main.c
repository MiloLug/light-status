#include <X11/X.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#ifdef USE_XINERAMA
    #include <X11/extensions/Xinerama.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>

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


#define MONITOR_FOCUSED -1

#define MONITOR_ASSIGN_STR(monitor, str) \
    switch (str[0]) { \
        case 'F': monitor = MONITOR_FOCUSED; break; \
        default: monitor = atoi(str); break; \
    }


void
set_alignment(const Alignment *alignment, Rect * obj, Rect * container)
{
    obj->y = alignment->top == ALIGN_CENTER || alignment->bottom == ALIGN_CENTER
        ? (container->h - obj->h) / 2
        : alignment->top != ALIGN_UNSET
            ? alignment->top
            : alignment->bottom != ALIGN_UNSET
                ? container->h - obj->h - alignment->bottom
                : 0;

    obj->x = alignment->left == ALIGN_CENTER || alignment->right == ALIGN_CENTER
        ? (container->w - obj->w) / 2
        : alignment->left != ALIGN_UNSET
            ? alignment->left
            : alignment->right != ALIGN_UNSET
                ? container->w - obj->w - alignment->right
                : 0;
}


#include "config.h"

static volatile FILE *status_data_pipe = NULL;


static void
on_close(void)
{
    if (status_data_pipe != NULL) {
        pclose((FILE *)status_data_pipe);
        status_data_pipe = NULL;
    }
}

static void
sig_handler(int sig)
{
    on_close();
    kill(0, 15);
    exit(0);
}


Rect
get_screen_rect(Display *dpy, int default_screen, int preferred_screen)
{
    Rect rect;
    bool xineramaIsOk = false;

#ifdef USE_XINERAMA
    int _dummy1, _dummy2;

    Window root_window = XDefaultRootWindow(dpy);
    Window window_returned;
    int mouse_x, mouse_y;
    unsigned int mask_return;
    int current_screen = default_screen;

    XineramaScreenInfo * screens = NULL;
    XineramaScreenInfo * screen = NULL;
    int heads=0;

    if (!XineramaQueryExtension(dpy, &_dummy1, &_dummy2)) {
        printf("Xinerama not supported\n");
        goto xinerama_end;
    }
    if (!XineramaIsActive(dpy)) {
        printf("Xinerama not active\n");
        goto xinerama_end;
    }
    
    screens = XineramaQueryScreens(dpy, &heads);
    if (!screens) {
        printf("XineramaQueryScreens failed\n");
        goto xinerama_end;
    }
    if (heads < 1) {
        printf("No screens found\n");
        goto xinerama_end;
    }

    if (preferred_screen >= 0 && preferred_screen < heads) {
        current_screen = preferred_screen;
    } else {
        if (!XQueryPointer(
            dpy, root_window, &window_returned,
            &window_returned, &mouse_x, &mouse_y, &_dummy1, &_dummy2,
            &mask_return
        )) {
            printf("XQueryPointer failed\n");
            goto xinerama_end;
        }

        for (int i = 0; i < heads; i++) {
            screen = screens + i;
            if (
                mouse_x >= screen->x_org && mouse_x < screen->x_org + screen->width &&
                mouse_y >= screen->y_org && mouse_y < screen->y_org + screen->height
            ) {
                current_screen = i;
                break;
            }
        }
    }

    screen = screens + current_screen;
    rect.x = screen->x_org;
    rect.y = screen->y_org;
    rect.w = screen->width;
    rect.h = screen->height;
    xineramaIsOk = true;

xinerama_end:
    
    if (screens) {
        XFree(screens);
    }
#endif

    if (!xineramaIsOk) {
        rect.w = DisplayWidth(dpy, default_screen);
        rect.h = DisplayHeight(dpy, default_screen);
        rect.x = 0;
        rect.y = 0;
    }

    return rect;
}


int
normalize_u8_string(signed char *str, size_t len)
{
    int i = 0;
    
    while (str[i] > 0) {
ascii:
        if (str[i] == '\n') {
            if (i == len - 1) {
                str[i] = '\0';
                len--;
                return len;
            }else {
                str[i] = ' ';
            }
        }
		i++;
	}

	while (str[i]) {
		if (str[i] > 0) {
			goto ascii;
		} else {
			switch (0xF0 & str[i]) {
			case 0xE0:
				i += 3;
				break;
			case 0xF0:
				i += 4;
				break;
			default:
				i += 2;
				break;
			}
		}
	}
	
	return len;
}


void
print_help(void)
{
    printf(
        "Usage: light-status [flags]\n\n"
        "Flags:\n"
        "    --help              - display help\n"
        "    -i <data-command>   - data collection command\n\n"
        "        PANEL CONFIG\n"
        "    -w <width>          - panel width\n"
        "    -h <height>         - panel height\n"
        "    -[l,r,t,b] <align>  - panel left, right, top and bottom alignment\n"
        "    -c <color>          - panel color\n\n"
        "        TEXT CONFIG\n"
        "    -T[l,r,t,b] <value> - text left, right, top and bottom alignment\n"
        "    -Tf <font>          - font pattern\n"
        "    -Tc <color>         - text color\n\n"
        "        XORG PROPERTIES\n"
        "    -Xn <name>          - window name\n"
        "    -Xc <class>         - window class\n"
        "    -Xm <monitor>       - monitor number\n\n"
        "<data-command> is a command that will be executed with popen() to show its output.\n"
        "    The command should periodically return a value, for example:\n"
        "        \"while true; do echo `date`; sleep 1; done\"\n"
        "    or\n"
        "        \"slstatus -s\"\n\n"
        "<align> can be:\n"
        "    C - center\n"
        "    U - unset (default)\n"
        "    <number> - offset in pixels\n\n"
        "<color> should be in hex format with leading # (#000fff)\n\n"
        "<font> should be in pattern: <font-name>[:size=<font-size>]\n"
        "    <font-name> can be:\n"
        "       actual name\n"
        "       font family name - monospace, sans, etc.\n\n"
        "<monitor> can be:\n"
        "    0 - primary monitor\n"
        "    <number> - other monitors\n"
        "    F - focused monitor\n\n"
    );
}


int
main (int argc, const char *argv[])
{
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGKILL, sig_handler);

    const char **fonts = default_fonts;
    int fonts_len = sizeof(default_fonts) / sizeof(char*);
    const char *status_collecting_command = default_status_collecting_command;
    const char *cur_arg;
    const char *color_scheme[] = {
        default_text_color,
        default_background_color
    };
    char * window_name = default_window_name;
    char * window_class = default_window_class;

#ifdef USE_ARGS
    for (int i = 1; i < argc; i++) {
        cur_arg = argv[i];
        
        if (cur_arg[0] == '-') switch (cur_arg[1]) {
            // --<x>
            case '-':
                if (strcmp(cur_arg+2, "help") == 0) {
                    print_help();
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
            // -X<x>
            case 'X':
                switch (cur_arg[2]) {
                    case 'n':
                        window_name = (char*)argv[++i];
                        break;
                    case 'c':
                        window_class = (char*)argv[++i];
                        break;
                    case 'm':
                        cur_arg = argv[++i];
                        MONITOR_ASSIGN_STR(monitor, cur_arg);
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
                panel_rect.w = atoi(cur_arg);
                break;
            case 'h':
                cur_arg = argv[++i];
                panel_rect.h = atoi(cur_arg);
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

    status_data_pipe = popen(status_collecting_command, "r");
    if (status_data_pipe == NULL) {
        printf("Failed to run the command\n");
        return 1;
    }

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Could not open display.\n");
        return 1;
    }

    int screen = DefaultScreen(dpy);
    Visual *visual = DefaultVisual(dpy, screen);
    int depth  = DefaultDepth(dpy, screen);
    Window root_window = DefaultRootWindow(dpy);

    Rect screen_rect = get_screen_rect(dpy, screen, monitor);
    Rect text_rect;
    char status[max_status_len];

    set_alignment(&panel_alignment, &panel_rect, &screen_rect);
    panel_rect.x += screen_rect.x;
    panel_rect.y += screen_rect.y;

    /* Create a simple window with a drawable. */
    XSetWindowAttributes window_attributes = {
        .override_redirect = True,
    };
    Window window = XCreateWindow(
        dpy,
        root_window,  // parent
        panel_rect.x, panel_rect.y,
        panel_rect.w, panel_rect.h,
        0,  // border width
        depth,  // depth
        InputOutput,  // class
        visual,  // visual
        CWOverrideRedirect,  // value mask
        &window_attributes
    );
    
    /* set the name and class hints for the window manager to use */
    XStoreName(dpy, window, window_name);
    XClassHint * class_hint = XAllocClassHint();
    if (class_hint) {
        class_hint->res_name = window_name;
        class_hint->res_class = window_class;
    }
    Atom win_type_atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom win_utility_atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_UTILITY", False);
    XSetClassHint(dpy, window, class_hint);
    XChangeProperty(
        dpy, window,
        win_type_atom, XA_ATOM, 32,
        PropModeReplace,
        (unsigned char *)&win_utility_atom, 1
    );
    
    XMapWindow(dpy, window);

    Drw *drw = drw_create(dpy, screen, root_window, panel_rect.w, panel_rect.h);
    drw_fontset_create(drw, fonts, fonts_len);
    Clr *scheme = drw_scm_create(drw, color_scheme, sizeof(color_scheme) / sizeof(char*));
    drw_set_scheme(drw, scheme);

    /* Read the output a line at a time - output it. */
    while (fgets(status, max_status_len, (FILE *)status_data_pipe) != NULL) {
        normalize_u8_string((signed char *)status, strlen(status));
        
        text_rect.w = 0;
        text_rect.h = 0;
        get_text_rect(drw, status, &text_rect);
        set_alignment(&text_alignment, &text_rect, &panel_rect);
        
        XClearWindow(dpy, window);
        drw_rect(drw, 0, 0, panel_rect.w, panel_rect.h, true, true);
        drw_text(
            drw,
            text_rect.x, text_rect.y,
            text_rect.w, text_rect.h,
            0,  // align
            status,
            false  // invert color
        );
        drw_map(drw, window, 0, 0, panel_rect.w, panel_rect.h);
        
        XFlush(dpy);
    }

    drw_free(drw);
    free(scheme);

    XDestroyWindow(dpy, window);
    XCloseDisplay(dpy);

    on_close();
    return 0;
}
