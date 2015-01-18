#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include <list>
#include <stdexcept>
#include <vector>

#include "gl_text.hh"
#include "level.hh"
#include "util.hh"

using namespace std;



void render_level_state(const level_state& l, int window_w, int window_h) {
  glBegin(GL_QUADS);

  for (int y = 0; y < l.h; y++) {
    for (int x = 0; x < l.w; x++) {
      bool draw_center = false;
      int param = l.at(x, y).param;
      switch (l.at(x, y).type) {
        case Empty:
          glColor4f(0.0, 0.0, (float)param / 256, 1.0);
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
        case GreenBomb:
          glColor4f(0.0, 1.0, 0.0, 1.0);
          draw_center = true;
          break;
        case YellowBomb:
          glColor4f(0.8, 0.8, 0.0, 1.0);
          draw_center = true;
          break;
        case RedBomb:
          glColor4f(param ? ((float)param / 256) : 1.0, 0.0, 0.0, 1.0);
          draw_center = true;
          break;
        case YellowBombTrigger:
          glColor4f(0.8, 0.8, 0.0, 1.0);
          break;
        case Explosion:
          glColor4f((float)param / 256, (float)param / 512, 0.0, 1.0);
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

      glVertex3f(to_window(x, l.w), -to_window(y, l.h), 1);
      glVertex3f(to_window(x + 1, l.w), -to_window(y, l.h), 1);
      glVertex3f(to_window(x + 1, l.w), -to_window(y + 1, l.h), 1);
      glVertex3f(to_window(x, l.w), -to_window(y + 1, l.h), 1);

      if (draw_center) {
        glColor4f(1.0, 1.0, 1.0, 1.0);
        glVertex3f(to_window(4 * x + 1, 4 * l.w), -to_window(4 * y + 1, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 3, 4 * l.w), -to_window(4 * y + 1, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 3, 4 * l.w), -to_window(4 * y + 3, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 1, 4 * l.w), -to_window(4 * y + 3, 4 * l.h), 1);
      }
    }
  }

  glEnd();

  glBegin(GL_TRIANGLES);
  glColor4f(1.0, 1.0, 1.0, 1.0);
  for (int y = 0; y < l.h; y++) {
    for (int x = 0; x < l.w; x++) {
      if (l.at(x, y).is_left_portal()) {
        glVertex3f(to_window(4 * x, 4 * l.w), -to_window(4 * y + 2, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 1, 4 * l.w), -to_window(4 * y + 1, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 1, 4 * l.w), -to_window(4 * y + 3, 4 * l.h), 1);
      }
      if (l.at(x, y).is_right_portal()) {
        glVertex3f(to_window(4 * x + 4, 4 * l.w), -to_window(4 * y + 2, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 3, 4 * l.w), -to_window(4 * y + 1, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 3, 4 * l.w), -to_window(4 * y + 3, 4 * l.h), 1);
      }
      if (l.at(x, y).is_up_portal()) {
        glVertex3f(to_window(4 * x + 2, 4 * l.w), -to_window(4 * y, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 1, 4 * l.w), -to_window(4 * y + 1, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 3, 4 * l.w), -to_window(4 * y + 1, 4 * l.h), 1);
      }
      if (l.at(x, y).is_down_portal()) {
        glVertex3f(to_window(4 * x + 2, 4 * l.w), -to_window(4 * y + 4, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 1, 4 * l.w), -to_window(4 * y + 3, 4 * l.h), 1);
        glVertex3f(to_window(4 * x + 3, 4 * l.w), -to_window(4 * y + 3, 4 * l.h), 1);
      }
    }
  }
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

  if (l.updates_per_second < 20.0f)
    draw_text(-0.99, 0.97, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
        "SLOW");
}

void render_stripe_animation(int window_w, int window_h, int stripe_width,
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


level_state game;
bool paused = true;
bool player_did_lose = false;
bool should_reload_state = false;
enum player_impulse current_impulse = None;

static void glfw_key_cb(GLFWwindow* window, int key, int scancode,
    int action, int mods) {

  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    if (key == GLFW_KEY_ESCAPE) {
      if (mods & GLFW_MOD_SHIFT)
        should_reload_state = true;
      else
        glfwSetWindowShouldClose(window, 1);

    } else if (key == GLFW_KEY_ENTER) {
      paused = !paused;
      player_did_lose = false;

    } else if (key == GLFW_KEY_TAB) {
      if (game.updates_per_second == 20.0f)
        game.updates_per_second = 2.0f;
      else
        game.updates_per_second = 20.0f;

    } else {
      if (key == GLFW_KEY_LEFT)
        current_impulse = Left;
      if (key == GLFW_KEY_RIGHT)
        current_impulse = Right;
      if (key == GLFW_KEY_UP)
        current_impulse = Up;
      if (key == GLFW_KEY_DOWN)
        current_impulse = Down;
      if (key == GLFW_KEY_SPACE)
        current_impulse = DropBomb;
    }
  }

  if (action == GLFW_RELEASE) {
    // note: we don't check for paused here to avoid bad state if the player
    // pauses while holding a direction key
    if (((key == GLFW_KEY_LEFT) && (current_impulse == Left)) ||
        ((key == GLFW_KEY_RIGHT) && (current_impulse == Right)) ||
        ((key == GLFW_KEY_UP) && (current_impulse == Up)) ||
        ((key == GLFW_KEY_DOWN) && (current_impulse == Down)) ||
        ((key == GLFW_KEY_SPACE) && (current_impulse == DropBomb)))
      current_impulse = None;
  }
}

static void glfw_resize_cb(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

static void glfw_error_cb(int error, const char* description) {
  fprintf(stderr, "[GLFW %d] %s\n", error, description);
}

int main(int argc, char* argv[]) {

  const char* level_filename = (argc > 1) ? argv[1] : NULL;
  int level_index = (argc > 2) ? atoi(argv[2]) : 0;
  vector<level_state> initial_state = load_level_index(level_filename);
  if ((level_index >= initial_state.size()) || (level_index < 0)) {
    fprintf(stderr, "invalid level index\n");
    return 1;
  }
  game = initial_state[level_index];
  bool level_is_valid = game.validate();

  if (!glfwInit()) {
    fprintf(stderr, "failed to initialize GLFW\n");
    exit(1);
  }
  glfwSetErrorCallback(glfw_error_cb);

  GLFWwindow* window = glfwCreateWindow(game.w * 24, game.h * 24,
      "Move Blocks and Eat Stuff", NULL, NULL);
  if (!window) {
    glfwTerminate();
    fprintf(stderr, "failed to create window\n");
    exit(1);
  }

  glfwSetFramebufferSizeCallback(window, glfw_resize_cb);
  glfwSetKeyCallback(window, glfw_key_cb);

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
      draw_text(0, 0.7, 1, 1, 1, 1, (float)window_w / window_h, 0.03, true,
          "MOVE BLOCKS AND EAT STUFF");
      draw_text(0, 0.3, 1, 0, 0, 1, (float)window_w / window_h, 0.02, true,
          "LEVEL IS CORRUPT");
      draw_text(0, 0.0, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
          "ESC: EXIT");

    } else {

      if (!game.player_is_alive()) {
        paused = true;
        player_did_lose = true;
        game = initial_state[level_index];
      } else if (game.player_did_win) {
        paused = true;

      } else if (should_reload_state) {
        paused = true;
        game = initial_state[level_index];
        should_reload_state = false;

      } else {
        uint64_t usec_per_update = 1000000.0 / game.updates_per_second;
        uint64_t now_time = now();
        uint64_t update_diff = now_time - last_update_time;
        if (update_diff >= usec_per_update) {
          if (!paused)
            game.exec_frame(current_impulse);
          last_update_time = now_time;
        }

        render_level_state(game, window_w, window_h);
        if (paused)
          render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f,
              0.6f, 0.0f, 0.0f, 0.0f, 0.1f);
      }

      if (paused) {
        draw_text(0, 0.7, 1, 1, 1, 1, (float)window_w / window_h, 0.03, true,
            "MOVE BLOCKS AND EAT STUFF");

        if (game.player_did_win) {
          if (level_index < initial_state.size() - 1) {
            level_index++;
            if (level_index < initial_state.size()) {
              game = initial_state[level_index];
            }

          } else
            draw_text(0, 0.3, 1, 1, 1, 1, (float)window_w / window_h, 0.02, true,
                "YOU WIN");
        } else {
          if (player_did_lose)
            draw_text(0, 0.3, 1, 0, 0, 1, (float)window_w / window_h, 0.02, true,
                "LEVEL %d - PRESS ENTER TO TRY AGAIN", level_index);
          else
            draw_text(0, 0.3, 1, 1, 1, 1, (float)window_w / window_h, 0.02, true,
                "LEVEL %d - PRESS ENTER TO PLAY", level_index);

          draw_text(0, 0, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
              "UP/DOWN/LEFT/RIGHT: MOVE");
          draw_text(0, -0.1, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
              "SPACE: DROP BOMB");
          draw_text(0, -0.2, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
              "TAB: TOGGLE SPEED");
          draw_text(0, -0.3, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
              "ENTER: PAUSE");
          draw_text(0, -0.4, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
              "SHIFT+ESC: RESTART LEVEL");
          draw_text(0, -0.5, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
              "ESC: EXIT");
        }
      }
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
