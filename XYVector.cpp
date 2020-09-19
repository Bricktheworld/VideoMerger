#include "XYVector.h"

int *get_side(XYVector *vector, Side side)
{
    if (side == Side::width)
    {
        return &(vector->x);
    }
    else if (side == Side::height)
    {
        return &(vector->y);
    }

    return nullptr;
}

void get_offset(XYVector *out, XYVector *dimensions, Side side, int vWidth, int vHeight)
{
    if (side == Side::width)
    {
        out->x = (vWidth - dimensions->x) / 2;
    }
    else if (side == Side::height)
    {
        out->y = (vHeight - dimensions->y) / 2;
    }
}
