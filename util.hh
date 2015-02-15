#include <stdio.h>

unsigned long long now();
float to_window(float x, float w);

void fread_or_throw(FILE* f, void* data, size_t size);
void fwrite_or_throw(FILE* f, const void* data, size_t size);
