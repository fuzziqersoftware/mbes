#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <GLFW/glfw3.h>

#include "gl_text_font.hh"

const float cell_size = 1;
const float cell_division_size = 0;
const float char_space_size = 0.3;

void draw_text(float x, float y, float r, float g, float b, float a,
    float aspect_ratio, float char_size, bool centered, const char* fmt, ...) {

  char* s = NULL;
  va_list va;
  va_start(va, fmt);
  int len = vasprintf(&s, fmt, va);
  va_end(va);
  if (!s)
    return;

  if (len == 0)
    return;

  if (centered) {
    int num_spaces = len - 1;
    float totalWidth = ((5 * cell_size + 4 * cell_division_size) * len * char_size
                       + char_space_size * char_size * num_spaces) / aspect_ratio;
    float totalHeight = (7 * cell_size + 6 * cell_division_size) * char_size;
    x -= totalWidth / 2;
    y += totalHeight / 2;
  }

  glBegin(GL_QUADS);
  glColor4f(r, g, b, a);

  double total_size = 5 * cell_size + 4 * cell_division_size + char_space_size;
  double currentX = x, currentY = y;
  for (; *s; s++) {
    if ((unsigned char)*s > 0x20) {
      for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 5; x++) {
          if (!font[(unsigned char)*s - 0x20][y * 5 + x])
            continue;
          glVertex2f(currentX + ((cell_size + cell_division_size) * x) * char_size / aspect_ratio,
                     currentY - ((cell_size + cell_division_size) * y) * char_size);
          glVertex2f(currentX + ((cell_size + cell_division_size) * x + cell_size) * char_size / aspect_ratio,
                     currentY - ((cell_size + cell_division_size) * y) * char_size);
          glVertex2f(currentX + ((cell_size + cell_division_size) * x + cell_size) * char_size / aspect_ratio,
                     currentY - ((cell_size + cell_division_size) * y + cell_size) * char_size);
          glVertex2f(currentX + ((cell_size + cell_division_size) * x) * char_size / aspect_ratio,
                     currentY - ((cell_size + cell_division_size) * y + cell_size) * char_size);
        }
      }
    }

    currentX += (total_size * char_size) / aspect_ratio;
  }
  glEnd();
}
