#include <stdio.h>
#include <stdlib.h>
#include "geometry.h"


void
set_alignment(const Alignment *alignment, Rect *obj, Rect *container)
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

void
add_rect(Rect *base, Rect *rect)
{
    if (rect->x + rect->w > base->x + base->w) {
        base->w = rect->x + rect->w - base->x;
    }
    if (rect->y + rect->h > base->y + base->h) {
        base->h = rect->y + rect->h - base->y;
    }
    if (rect->x < base->x) {
        base->x = rect->x;
    }
    if (rect->y < base->y) {
        base->y = rect->y;
    }
}

void
position_to_00(Position *position, Rect *rect)
{
    switch (position->center) {
        case -1:
            break;
        case 00:
            position->x = rect->x + position->x;
            position->y = rect->y + position->y;
            break;
        case 10:
            position->x = rect->x + rect->w / 2 + position->x;
            position->y = rect->y + position->y;
            break;
        case 01:
            position->x = rect->x + position->x;
            position->y = rect->y + rect->h / 2 - position->y;
            break;
        case 11:
            position->x = rect->x + rect->w / 2 + position->x;
            position->y = rect->y + rect->h / 2 - position->y;
            break;
    }
} 

const char *rect_to_str(Rect *rect)
{
    static char buffer[100];
    snprintf(buffer, sizeof(buffer), "Rect { w = %d, h = %d, x = %d, y = %d }", rect->w, rect->h, rect->x, rect->y);
    return buffer;
}

const char *position_to_str(Position *position)
{
    static char buffer[100];
    snprintf(buffer, sizeof(buffer), "Position { x = %d, y = %d, center = %d }", position->x, position->y, position->center);
    return buffer;
}
