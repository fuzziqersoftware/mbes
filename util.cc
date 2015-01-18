#include <sys/time.h>

unsigned long long now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

// TODO use projection matrix to make this unnecessary
float to_window(float x, float w) {
  return ((x / w) * 2) - 1;
}
