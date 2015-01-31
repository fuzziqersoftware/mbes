#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>
#include <GLFW/glfw3.h>

#include <list>
#include <stdexcept>
#include <vector>

#include "audio.hh"
#include "gl_text.hh"
#include "level.hh"
#include "level_completion.hh"
#include "util.hh"

using namespace std;



static void render_stripe_animation(int window_w, int window_h, int stripe_width,
    float br, float bg, float bb, float ba, float sr, float sg, float sb,
    float sa) {
  glBegin(GL_QUADS);
  glColor4f(br, bg, bb, ba);
  glVertex3f(-1.0f, -1.0f, 1.0f);
  glVertex3f(1.0f, -1.0f, 1.0f);
  glVertex3f(1.0f, 1.0f, 1.0f);
  glVertex3f(-1.0f, 1.0f, 1.0f);

  glColor4f(sr, sg, sb, sa);
  int xpos;
  for (xpos = -2 * stripe_width +
        (float)(now() % 3000000) / 3000000 * 2 * stripe_width;
       xpos < window_w + window_h;
       xpos += (2 * stripe_width)) {
    glVertex2f(to_window(xpos, window_w), 1);
    glVertex2f(to_window(xpos + stripe_width, window_w), 1);
    glVertex2f(to_window(xpos - window_h + stripe_width, window_w), -1);
    glVertex2f(to_window(xpos - window_h, window_w), -1);
  }
  glEnd();
}



static void render_cell_quads(const cell_state& cell, int x, int y, int l_w,
    int l_h) {
  bool draw_center = false;
  switch (cell.type) {
    case Empty:
      glColor4f(0.0, 0.0, (float)cell.param / 256, 1.0);
      break;
    case Circuit:
      glColor4f(0.0, 0.8, 0.0, 1.0);
      break;
    case Rock:
      glColor4f(0.6, 0.6, 0.6, 1.0);
      break;
    case Exit:
      glColor4f(0.5, 1.0, 0.0, 1.0);
      break;
    case Player:
      glColor4f(1.0, 0.0, 0.0, 1.0);
      break;
    case Item:
      glColor4f(1.0, 0.0, 1.0, 1.0);
      break;
    case Block:
      glColor4f(1.0, 1.0, 1.0, 1.0);
      break;
    case RoundBlock:
      glColor4f(0.9, 0.9, 1.0, 1.0);
      break;
    case BlueBomb:
      glColor4f(0.0, 0.5, 1.0, 1.0);
      draw_center = true;
      break;
    case GreenBomb:
      glColor4f(0.0, 1.0, 0.0, 1.0);
      draw_center = true;
      break;
    case YellowBomb:
      glColor4f(0.8, 0.8, 0.0, 1.0);
      draw_center = true;
      break;
    case RedBomb:
      glColor4f(cell.param ? ((float)cell.param / 256) : 1.0, 0.0, 0.0, 1.0);
      draw_center = true;
      break;
    case YellowBombTrigger:
      glColor4f(0.8, 0.8, 0.0, 1.0);
      break;
    case Explosion:
      glColor4f((float)cell.param / 256, (float)cell.param / 512, 0.0, 1.0);
      break;
    case ItemDude:
      glColor4f(0.0, 0.5, 1.0, 1.0);
      break;
    case BombDude:
      glColor4f(1.0, 0.5, 0.0, 1.0);
      break;
    case LeftPortal:
    case RightPortal:
    case UpPortal:
    case DownPortal:
    case HorizontalPortal:
    case VerticalPortal:
    case Portal:
      glColor4f(0.7, 0.0, 0.0, 1.0);
      break;
  }

  glVertex3f(to_window(x, l_w), -to_window(y, l_h), 1);
  glVertex3f(to_window(x + 1, l_w), -to_window(y, l_h), 1);
  glVertex3f(to_window(x + 1, l_w), -to_window(y + 1, l_h), 1);
  glVertex3f(to_window(x, l_w), -to_window(y + 1, l_h), 1);

  if (draw_center) {
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glVertex3f(to_window(4 * x + 1, 4 * l_w), -to_window(4 * y + 1, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 3, 4 * l_w), -to_window(4 * y + 1, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 3, 4 * l_w), -to_window(4 * y + 3, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 1, 4 * l_w), -to_window(4 * y + 3, 4 * l_h), 1);
  }
}

static void render_cell_tris(const cell_state& cell, int x, int y, int l_w,
    int l_h) {
  if (cell.is_left_portal()) {
    glVertex3f(to_window(4 * x, 4 * l_w), -to_window(4 * y + 2, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 1, 4 * l_w), -to_window(4 * y + 1, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 1, 4 * l_w), -to_window(4 * y + 3, 4 * l_h), 1);
  }
  if (cell.is_right_portal()) {
    glVertex3f(to_window(4 * x + 4, 4 * l_w), -to_window(4 * y + 2, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 3, 4 * l_w), -to_window(4 * y + 1, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 3, 4 * l_w), -to_window(4 * y + 3, 4 * l_h), 1);
  }
  if (cell.is_up_portal()) {
    glVertex3f(to_window(4 * x + 2, 4 * l_w), -to_window(4 * y, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 1, 4 * l_w), -to_window(4 * y + 1, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 3, 4 * l_w), -to_window(4 * y + 1, 4 * l_h), 1);
  }
  if (cell.is_down_portal()) {
    glVertex3f(to_window(4 * x + 2, 4 * l_w), -to_window(4 * y + 4, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 1, 4 * l_w), -to_window(4 * y + 3, 4 * l_h), 1);
    glVertex3f(to_window(4 * x + 3, 4 * l_w), -to_window(4 * y + 3, 4 * l_h), 1);
  }
}

static void render_level_state(const level_state& l, int window_w, int window_h) {
  glBegin(GL_QUADS);
  for (int y = 0; y < l.h; y++)
    for (int x = 0; x < l.w; x++)
      render_cell_quads(l.at(x, y), x, y, l.w, l.h);
  glEnd();

  glBegin(GL_TRIANGLES);
  glColor4f(1.0, 1.0, 1.0, 1.0);
  for (int y = 0; y < l.h; y++)
    for (int x = 0; x < l.w; x++)
      render_cell_tris(l.at(x, y), x, y, l.w, l.h);
  glEnd();

  if (l.num_items_remaining > 1)
    draw_text(-0.99, -0.9, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
        "%d ITEMS REMAINING", l.num_items_remaining);
  else if (l.num_items_remaining == 1)
    draw_text(-0.99, -0.9, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
        "1 ITEM REMAINING");

  if (l.num_red_bombs > 1)
    draw_text(-0.99, -0.8, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
        "%d RED BOMBS", l.num_red_bombs);
  else if (l.num_red_bombs == 1)
    draw_text(-0.99, -0.8, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
        "1 RED BOMB");
  else if (l.num_red_bombs == -1)
    draw_text(-0.99, -0.8, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
        "1 RED BOMB IN DEBT");
  else if (l.num_red_bombs < -1)
    draw_text(-0.99, -0.8, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
        "%d RED BOMBS IN DEBT", -l.num_red_bombs);

  if (l.updates_per_second != 20.0f) {
    render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.1f);
    //draw_text(-0.99, 0.97, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
    //    "SLOW");
  }
}



static void render_paused_screen(int window_w, int window_h,
    const vector<level_completion>& completion, int level_index,
    bool player_did_win, bool player_did_lose) {

  size_t num_completed = 0;
  for (const auto& it : completion)
    if (it == Completed)
      num_completed++;

  render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f,
      0.0f, 0.0f, 0.1f);

  float aspect_ratio = (float)window_w / window_h;
  draw_text(0, 0.9, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
      "FUZZIQER SOFTWARE");
  draw_text(0, 0.7, 1, 1, 1, 1, (float)window_w / window_h, 0.03, true,
      "MOVE BLOCKS AND EAT STUFF");

  if (player_did_win)
    draw_text(0, 0.3, 1, 1, 1, 1, aspect_ratio, 0.02, true,
        "YOU WIN");
  else {
    if (player_did_lose)
      draw_text(0, 0.3, 1, 0, 0, 1, aspect_ratio, 0.02, true,
          "LEVEL %d - PRESS ENTER TO TRY AGAIN", level_index);
    else
      draw_text(0, 0.3, 1, 1, 1, 1, aspect_ratio, 0.02, true,
          "LEVEL %d - PRESS ENTER TO PLAY", level_index);

    if (completion[level_index] == Completed)
      draw_text(0, 0.1, 0.5, 1, 0.5, 1, aspect_ratio, 0.01, true,
          "YOU HAVE ALREADY COMPLETED THIS LEVEL (%lu/%lu)", num_completed, completion.size());
    else
      draw_text(0, 0.1, 1, 1, 1, 1, aspect_ratio, 0.01, true,
          "YOU HAVE COMPLETED %lu OF %lu LEVELS", num_completed, completion.size());

    draw_text(0, -0.1, 1, 1, 1, 1, aspect_ratio, 0.01, true,
        "UP/DOWN/LEFT/RIGHT: MOVE");
    draw_text(0, -0.2, 1, 1, 1, 1, aspect_ratio, 0.01, true,
        "SPACE: DROP BOMB");
    draw_text(0, -0.3, 1, 1, 1, 1, aspect_ratio, 0.01, true,
        "TAB: TOGGLE SPEED");
    draw_text(0, -0.4, 1, 1, 1, 1, aspect_ratio, 0.01, true,
        "ENTER: PAUSE");
    draw_text(0, -0.6, 1, 1, 1, 1, aspect_ratio, 0.01, true,
        "SHIFT+LEFT/RIGHT: CHANGE LEVEL");
    draw_text(0, -0.7, 1, 1, 1, 1, aspect_ratio, 0.01, true,
        "ESC: RESTART LEVEL / EXIT");
  }
}



enum game_phase {
  Playing = 0,
  Paused,
  Instructions,
};

level_state game;
game_phase phase = Paused;
bool player_did_lose = false;
bool should_reload_state = false;
enum player_impulse current_impulse = None;
int level_index = -1;
int should_change_to_level = -1;
bool debug_mode = false;

static void glfw_key_cb(GLFWwindow* window, int key, int scancode,
    int action, int mods) {

  if (((action == GLFW_PRESS) || (action == GLFW_REPEAT)) && (mods & GLFW_MOD_SHIFT)) {
    if (key == GLFW_KEY_LEFT) {
      should_change_to_level = level_index - 1;
      return;
    } else if (key == GLFW_KEY_RIGHT) {
      should_change_to_level = level_index + 1;
      return;
    }
  }

  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_ESCAPE) {
      if (phase == Playing)
        should_change_to_level = level_index;
      else
        glfwSetWindowShouldClose(window, 1);

    } else if (key == GLFW_KEY_ENTER) {
      if (mods & GLFW_MOD_SHIFT)
        debug_mode = !debug_mode;

      else {
        if (phase == Playing)
          phase = Paused;
        else
          phase = Playing;
        player_did_lose = false;
      }

    } else if (key == GLFW_KEY_TAB) {
      if (mods & GLFW_MOD_SHIFT)
        game.updates_per_second = 200.0f;
      else if (game.updates_per_second == 20.0f)
        game.updates_per_second = 2.0f;
      else
        game.updates_per_second = 20.0f;

    } else if (key == GLFW_KEY_LEFT) {
      current_impulse = Left;
      phase = Playing;
      player_did_lose = false;
    } else if (key == GLFW_KEY_RIGHT) {
      current_impulse = Right;
      phase = Playing;
      player_did_lose = false;
    } else if (key == GLFW_KEY_UP) {
      current_impulse = Up;
      phase = Playing;
      player_did_lose = false;
    } else if (key == GLFW_KEY_DOWN) {
      current_impulse = Down;
      phase = Playing;
      player_did_lose = false;
    } else if (key == GLFW_KEY_SPACE) {
      game.player_drop_bomb();
      phase = Playing;
      player_did_lose = false;
    }
  }

  if (action == GLFW_RELEASE) {
    // note: we don't check for paused here to avoid bad state if the player
    // pauses while holding a direction key
    if (((key == GLFW_KEY_LEFT) && (current_impulse == Left)) ||
        ((key == GLFW_KEY_RIGHT) && (current_impulse == Right)) ||
        ((key == GLFW_KEY_UP) && (current_impulse == Up)) ||
        ((key == GLFW_KEY_DOWN) && (current_impulse == Down)))
      current_impulse = None;
  }
}

static void glfw_focus_cb(GLFWwindow* window, int focused) {
  if ((focused == GL_FALSE) && (phase == Playing)) {
    phase = Paused;
  }
}

static void glfw_resize_cb(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

static void glfw_error_cb(int error, const char* description) {
  fprintf(stderr, "[GLFW %d] %s\n", error, description);
}

int main(int argc, char* argv[]) {

  srand(time(NULL) ^ getpid());

  const char* level_filename = (argc > 1) ? argv[1] : NULL;
  level_index = (argc > 2) ? atoi(argv[2]) : -1;

  char filename_buffer[MAXPATHLEN];
  if (!level_filename) {
    CFURLRef app_url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFStringRef path = CFURLCopyFileSystemPath(app_url, kCFURLPOSIXPathStyle);
    const char *p = CFStringGetCStringPtr(path, CFStringGetSystemEncoding());

    // if we're in an app bundle, look in Resources/ for levels; else look in
    // the executable directory
    size_t p_len = strlen(p);
    if ((p_len >= 4) && !strcmp(p + p_len - 4, ".app"))
      sprintf(filename_buffer, "%s/Contents/Resources/levels.dat", p);
    else
      sprintf(filename_buffer, "%s/levels.dat", p);
    level_filename = filename_buffer;

    CFRelease(app_url);
    CFRelease(path);
  }

  vector<level_state> initial_state;
  try {
    initial_state = load_level_index(level_filename);
  } catch (const runtime_error& e) {
    fprintf(stderr, "can\'t load level index: %s\n", e.what());
    return 1;
  }

  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  sprintf(filename_buffer, "%s/.mbes_completion", homedir);
  vector<level_completion> completion = load_level_completion_state(filename_buffer);
  completion.resize(initial_state.size());

  if (level_index < 0) {
    // start at the first non-completed level
    for (level_index = 0; (completion[level_index] == Completed) &&
        (level_index < initial_state.size()); level_index++);
  }

  if (level_index >= initial_state.size()) {
    level_index = 0;
  }

  game = initial_state[level_index];
  bool level_is_valid = game.validate();

  init_al();
  sine_wave get_item_sound(880, 0.1);
  sine_wave drop_bomb_sound(440, 0.1);
  sine_wave circuit_eaten_sound(1760, 0.05);
  split_noise explosion_sound(10, 1.5, 1.0, true);
  split_noise landing_sound(10, 0.02, 1.0, false);
  split_noise push_sound(10, 0.05, 1.0, false);
  // TODO more sounds

  if (!glfwInit()) {
    fprintf(stderr, "failed to initialize GLFW\n");
    return 2;
  }
  glfwSetErrorCallback(glfw_error_cb);

  GLFWwindow* window = glfwCreateWindow(game.w * 24, game.h * 24,
      "Move Blocks and Eat Stuff", NULL, NULL);
  if (!window) {
    glfwTerminate();
    fprintf(stderr, "failed to create window\n");
    return 2;
  }

  glfwSetFramebufferSizeCallback(window, glfw_resize_cb);
  glfwSetKeyCallback(window, glfw_key_cb);
  glfwSetWindowFocusCallback(window, glfw_focus_cb);

  glfwMakeContextCurrent(window);

  // 2D drawing mode
  glDisable(GL_LIGHTING);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // raster operations config
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_LINE_SMOOTH);
  glLineWidth(3);
  glPointSize(12);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  uint64_t last_update_time = now();

  while (!glfwWindowShouldClose(window)) {

    int window_w, window_h;
    glfwGetFramebufferSize(window, &window_w, &window_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!level_is_valid) {
      render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f, 0.6f,
          1.0, 0.0, 0.0, 0.3);
      draw_text(0, 0.3, 1, 0, 0, 1, (float)window_w / window_h, 0.02, true,
          "LEVEL IS CORRUPT");
      draw_text(0, 0.0, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
          "ESC: EXIT");

    } else {

      if (!game.player_is_alive()) {
        phase = Paused;
        player_did_lose = true;
        game = initial_state[level_index];

      } else if (game.player_did_win) {
        phase = Paused;
        player_did_lose = false;
        completion[level_index] = Completed;
        if (level_index < initial_state.size() - 1) {
          level_index++;
          game = initial_state[level_index];
          level_is_valid = game.validate();
          if (completion[level_index] != Completed)
            completion[level_index] = Attempted;
        }
        save_level_completion_state(filename_buffer, completion);

      } else if ((should_change_to_level >= 0) && (should_change_to_level < initial_state.size())) {
        phase = Paused;
        level_index = should_change_to_level;
        game = initial_state[level_index];
        if (completion[level_index] != Completed)
          completion[level_index] = Attempted;
        save_level_completion_state(filename_buffer, completion);
        should_change_to_level = -1;
        player_did_lose = false;

      } else {
        uint64_t usec_per_update = 1000000.0 / game.updates_per_second;
        uint64_t now_time = now();
        uint64_t update_diff = now_time - last_update_time;
        if (update_diff >= usec_per_update) {
          if (phase == Playing) {
            uint64_t events = game.exec_frame(current_impulse);
            if (events & (RedBombCollected | ItemCollected))
              get_item_sound.play();
            if (events & RedBombDropped)
              drop_bomb_sound.play();
            if (events & CircuitEaten)
              circuit_eaten_sound.play();
            if (events & (Exploded | ItemExploded))
              explosion_sound.play();
            if (events & ObjectLanded)
              landing_sound.play();
            if (events & ObjectPushed)
              push_sound.play();
          }
          last_update_time = now_time;
        }
      }

      if (!game.player_did_win)
        render_level_state(game, window_w, window_h);

      if (debug_mode) {
        draw_text(-0.99, 0.97, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
            "AT %d, %d - CELL INDEX %d - FILE OFFSET %X", game.player_x, game.player_y,
            game.player_y * 60 + game.player_x, 1536 * level_index + game.player_y * 60 + game.player_x);
      }

      if (phase == Paused)
        render_paused_screen(window_w, window_h, completion, level_index,
            game.player_did_win, player_did_lose);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
