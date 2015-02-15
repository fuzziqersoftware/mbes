#include <stdio.h>
#include <sys/time.h>

#include <stdexcept>

using namespace std;

unsigned long long now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

// TODO use projection matrix to make this unnecessary
float to_window(float x, float w) {
  return ((x / w) * 2) - 1;
}

void fread_or_throw(FILE* f, void* data, size_t size) {
  if (fread(data, 1, size, f) != size)
    throw runtime_error("can\'t read file");
}

void fwrite_or_throw(FILE* f, const void* data, size_t size) {
  if (fwrite(data, 1, size, f) != size)
    throw runtime_error("can\'t write file");
}
