#include "SideEnum.h"

#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

struct XYVector
{
    int x;
    int y;
};

int *get_side(XYVector *vector, Side side);

void get_offset(XYVector *out, XYVector *dimensions, Side side, int vWidth, int vHeight);

#endif