#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include <list>
#include <stdexcept>
#include <vector>

using namespace std;

enum level_completion_state {
  // these are saved in a file, so don't change the values of existing entries
  NotAttempted = 0,
  Attempted = 1,
  Completed = 2,
};

struct level_completion_v1 {
  level_completion_state state;
  uint64_t frames;
  uint64_t extra_items;
  uint64_t extra_bombs;
  uint64_t cleared_space;
  uint64_t attenuated_space;
};

struct level_completion {
  level_completion_state state;
  uint64_t frames;
  uint64_t extra_items;
  uint64_t extra_bombs;
  uint64_t cleared_space;
  uint64_t attenuated_space;
  uint64_t entropy;

  level_completion();
  level_completion(const level_completion_v1& v1);
};

vector<level_completion> load_level_completion_state_v1(const string& filename);
vector<level_completion> load_level_completion_state(const string& filename);
void save_level_completion_state(const string& filename, const vector<level_completion>& lc);
