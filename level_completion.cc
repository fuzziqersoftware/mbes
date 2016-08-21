#include <stdint.h>
#include <stdio.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "level_completion.hh"

using namespace std;

level_completion::level_completion() : state(NotAttempted),
    frames(0xFFFFFFFFFFFFFFFF), extra_items(0), extra_bombs(0),
    cleared_space(0), attenuated_space(0xFFFFFFFFFFFFFFFF) { }

level_completion::level_completion(const level_completion_v1& v1) :
    state(v1.state), frames(v1.frames), extra_items(v1.extra_items),
    extra_bombs(v1.extra_bombs), cleared_space(v1.cleared_space),
    attenuated_space(v1.attenuated_space), entropy(0xFFFFFFFFFFFFFFFF) { }

vector<level_completion> load_level_completion_state_v1(const string& filename) {
  FILE* f = fopen(filename.c_str(), "rb");
  if (!f)
    return vector<level_completion>();

  fseek(f, 0, SEEK_END);
  uint64_t num = ftell(f) / sizeof(level_completion_v1);
  fseek(f, 0, SEEK_SET);

  vector<level_completion_v1> lc(num);
  fread(lc.data(), lc.size(), sizeof(level_completion_v1), f);
  fclose(f);

  vector<level_completion> ret;
  for (const auto& lcv1 : lc)
    ret.emplace_back(lcv1);

  return ret;
}

vector<level_completion> load_level_completion_state(const string& filename) {
  FILE* f = fopen(filename.c_str(), "rb");
  if (!f)
    return vector<level_completion>();

  uint64_t version = 0;
  fread(&version, sizeof(version), 1, f);
  if (version != 2)
    return vector<level_completion>();

  fseek(f, 0, SEEK_END);
  uint64_t num = (ftell(f) - sizeof(version)) / sizeof(level_completion);
  fseek(f, sizeof(version), SEEK_SET);

  vector<level_completion> lc(num);
  fread(lc.data(), lc.size(), sizeof(level_completion), f);
  fclose(f);

  return lc;
}

void save_level_completion_state(const string& filename, const vector<level_completion>& lc) {
  FILE* f = fopen(filename.c_str(), "wb");
  if (!f)
    throw runtime_error("can\'t open file");

  uint64_t version = 2;
  fwrite(&version, sizeof(version), 1, f);

  fwrite(lc.data(), lc.size(), sizeof(level_completion), f);

  fclose(f);
}
