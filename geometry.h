#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <stdint.h>
#include <limits.h>
#include <X11/Xlib.h>

#define ALIGN_UNSET INT_MIN
#define ALIGN_CENTER INT_MIN + 1

typedef struct Rect {
    int w, h, x, y;
} Rect;

typedef struct Alignment {
    int top;
    int bottom;
    int left;
    int right;
} Alignment;

typedef struct Position {
    int x;
    int y;
    int center;
} Position;



/**
 * Set the alignment of an object within a container
 * 
 * @param alignment The alignment settings
 * @param obj The object to align
 * @param container The container to align within
 */
void set_alignment(const Alignment *alignment, Rect *obj, Rect *container);

/**
 * Increase the base rect's size to include the second rect
 * 
 * @param base The base rect
 * @param rect The rect to add
 */
void add_rect(Rect *base, Rect *rect);

/**
 * Convert a position to absolute coordinates based on center type
 * 
 * @param position The position to convert
 * @param rect The reference rectangle
 */
void position_to_00(Position *position, Rect *rect);

const char *rect_to_str(Rect *rect);
const char *position_to_str(Position *position);

#endif /* GEOMETRY_H */
