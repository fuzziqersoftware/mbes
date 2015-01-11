#ifndef __GLTEXT_H
#define __GLTEXT_H

#include <stdio.h>

void draw_text(float x, float y, float r, float g, float b, float a,
    float aspect_ratio, float char_size, bool centered, const char* fmt, ...);

#endif // __GLTEXT_H
