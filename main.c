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
#include "geometry.h"
#include "util.h"


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


typedef struct MonitorSpec {
    char *name;
    int index;
    Rect rect;
    struct MonitorSpec *next;
} MonitorSpec;


#define E_MONITOR_SPEC_PARSE_WRONG_FORMAT -1

int monitor_spec_from_str(const char *str, int str_len, MonitorSpec ** monitor_spec) {
    int name_len = strcspn(str, ":");

    if (name_len == 0 || name_len == str_len) {
        return E_MONITOR_SPEC_PARSE_WRONG_FORMAT;
    }

    Rect rect;
    int index;
    if (sscanf(str + name_len + 1, "%d:%d:%d:%d:%d", &index, &rect.w, &rect.h, &rect.x, &rect.y) != 5) {
        return E_MONITOR_SPEC_PARSE_WRONG_FORMAT;
    }

    MonitorSpec *spec = malloc(sizeof(MonitorSpec));
    spec->name = malloc(name_len + 1);
    memcpy(spec->name, str, name_len);
    spec->name[name_len] = '\0';
    spec->index = index;
    spec->rect = rect;
    spec->next = NULL;

    *monitor_spec = spec;
    return 0;
}

int parse_monitors(const char *str, MonitorSpec ** monitors) {
    MonitorSpec ** head = monitors;

    int i = 0;
    while (true) {
        int part_len = strcspn(str, ",");
        if (part_len == 0)
            break;
        
        if (monitor_spec_from_str(str, part_len, head) != 0)
            return E_MONITOR_SPEC_PARSE_WRONG_FORMAT;
        i++;
        head = &(*head)->next;
        
        if (str[part_len] == ',')
            str += part_len + 1;
        else
            break;
    }

    return i;
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
decide_screen_rect(Display *dpy, int default_screen, int preferred_screen, MonitorSpec *monitors)
{
    Rect rect;
    bool xineramaIsOk = false;

    if (monitors) {
        rect = monitors->rect;

        MonitorSpec *current_monitor = monitors;
        if (preferred_screen >= 0) {
            default_screen = preferred_screen;
        }

        while (current_monitor) {
            if (current_monitor->index == default_screen) {
                rect = current_monitor->rect;
                break;
            }
            current_monitor = current_monitor->next;
        }

        return rect;
    }

#ifdef USE_XINERAMA
    int _dummy1, _dummy2;

    Window root_window = XDefaultRootWindow(dpy);
    Window window_returned;
    int mouse_x, mouse_y;
    unsigned int mask_return;
    int current_screen_index = default_screen;

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
        current_screen_index = preferred_screen;
    } else {
        Rect screen_space;
        Rect current_screen;

        for (int i = 0; i < heads; i++) {
            screen = screens + i;
            current_screen.x = screen->x_org;
            current_screen.y = screen->y_org;
            current_screen.w = screen->width;
            current_screen.h = screen->height;

            if (i == 0) {
                screen_space = current_screen;
            } else {
                add_rect(&screen_space, &current_screen);
            }
        }

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
                current_screen_index = i;
                break;
            }
        }
    }

    screen = screens + current_screen_index;
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
print_help(const char *program_name)
{
    printf(
        C_BLUE "Usage: " C_RESET "%s [flags]\n\n"
        "Things marked !W or !X are only for Wayland or Xorg setups.\n\n"
        C_GREEN "Flags:\n" C_RESET
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
        "    -Xn <name>             - window name\n"
        "    -Xc <class>            - window class\n"
        "    -Xm <monitor index>    - monitor number\n"
        "    -Xd <monitor spec>[,<monitor spec>...] - monitor spec\n"
        C_GREEN "<data-command>" C_RESET " is a command that will be executed with popen() to show its output.\n"
        "    The command should periodically return a value, for example:\n"
        "        \"while true; do echo `date`; sleep 1; done\"\n"
        "    or\n"
        "        \"slstatus -s\"\n\n"
        C_GREEN "<align>" C_RESET " can be:\n"
        "    C - center\n"
        "    U - unset (default)\n"
        "    <number> - offset in pixels\n\n"
        C_GREEN "<color>" C_RESET " should be in hex format with leading # (#000fff)\n\n"
        C_GREEN "<font>" C_RESET " should be in pattern: <font-name>[:size=<font-size>]\n"
        "    " C_GREEN "<font-name>" C_RESET " can be:\n"
        "       actual name\n"
        "       font family name - monospace, sans, etc.\n\n"
        C_GREEN "<monitor index>" C_RESET " can be:\n"
        "    0 - primary monitor\n"
        "    <number> - other monitors\n"
        "    F - focused monitor, deduced from mouse position\n\n"
        C_GREEN "<monitor spec>" C_RESET " is:\n"
        "    <name>:<index>:<w>:<h>:<x>:<y> - monitor name, index, width, height, x, y\n\n",
        program_name
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

    MonitorSpec *monitors = NULL;

#ifdef USE_ARGS
    for (int i = 1; i < argc; i++) {
        cur_arg = argv[i];
        
        if (cur_arg[0] == '-') switch (cur_arg[1]) {
            // --<x>
            case '-':
                if (strcmp(cur_arg+2, "help") == 0) {
                    print_help(argv[0]);
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
                    case 'd':
                        cur_arg = argv[++i];
                        if (parse_monitors(cur_arg, &monitors) == E_MONITOR_SPEC_PARSE_WRONG_FORMAT) {
                            printf("Wrong monitor spec format, expected <name>:<index>:<w>:<h>:<x>:<y>\n");
                            exit(1);
                        }
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

    Rect screen_rect = decide_screen_rect(dpy, screen, monitor, monitors);
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
