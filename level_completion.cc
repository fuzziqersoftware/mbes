#include <stdint.h>
#include <stdio.h>

#include <stdexcept>
#include <vector>

#include "level_completion.hh"

using namespace std;

level_completion::level_completion() : state(NotAttempted),
    frames(0xFFFFFFFFFFFFFFFF), extra_items(0), extra_bombs(0),
    cleared_space(0), attenuated_space(0xFFFFFFFFFFFFFFFF) { }

vector<level_completion> load_level_completion_state(const char* filename) {
  FILE* f = fopen(filename, "rb");
  if (!f)
    return vector<level_completion>();
  fseek(f, 0, SEEK_END);
  uint64_t num = ftell(f) / sizeof(level_completion);
  fseek(f, 0, SEEK_SET);

  vector<level_completion> lc(num);
  fread(lc.data(), lc.size(), sizeof(level_completion), f);
  fclose(f);

  return lc;
}

void save_level_completion_state(const char* filename, const vector<level_completion>& lc) {
  FILE* f = fopen(filename, "wb");
  if (!f)
    throw runtime_error("can\'t open file");

  fwrite(lc.data(), lc.size(), sizeof(level_completion), f);
  fclose(f);
}
