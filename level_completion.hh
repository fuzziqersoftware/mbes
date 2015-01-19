#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include <list>
#include <stdexcept>
#include <vector>

using namespace std;

enum level_completion {
  // these are saved in a file, so don't change the values of existing entries
  NotAttempted = 0,
  Attempted = 1,
  Completed = 2,
};

vector<level_completion> load_level_completion_state(const char* filename);
void save_level_completion_state(const char* filename, const vector<level_completion>& lc);
