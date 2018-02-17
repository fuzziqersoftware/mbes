#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include <deque>
#include <list>
#include <stdexcept>
#include <vector>


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

struct level_completion_v2 {
  level_completion_state state;
  uint64_t frames;
  uint64_t extra_items;
  uint64_t extra_bombs;
  uint64_t cleared_space;
  uint64_t attenuated_space;
  uint64_t entropy;
};

struct level_completion {
  level_completion_state state;
  uint64_t frames;
  uint64_t extra_items;
  uint64_t extra_bombs;
  uint64_t cleared_space;
  uint64_t attenuated_space;
  uint64_t entropy;
  uint64_t rewind_count;

  level_completion();
  level_completion(const level_completion_v1& v1);
  level_completion(const level_completion_v2& v2);
};

std::vector<level_completion> load_level_completion_state_v1(
    const std::string& filename);
std::vector<level_completion> load_level_completion_state(
    const std::string& filename);
void save_level_completion_state(const std::string& filename,
    const std::vector<level_completion>& lc);

std::deque<struct player_actions> load_recording(const std::string& filename);
void save_recording(const std::string& filename,
    const std::deque<struct player_actions>& recording);
