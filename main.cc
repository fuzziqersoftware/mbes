#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "gl_text.h"

using namespace std;



unsigned long long now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}



enum direction {
  None = 0,
  Up = 1,
  Down = 2,
  Left = 3,
  Right = 4,
};

enum cell_type {
  Empty = 0,
  Circuit = 1,
  Rock = 2,
  Exit = 3,
  Player = 4,
  Item = 5,
  Block = 6,
};

struct cell_state {
  cell_type type;
  int param;

  cell_state() : type(Empty), param(0) { }
  cell_state(cell_type type, int param) : type(type), param(param) { }
};

struct level_state {
  int w;
  int h;
  int player_x;
  int player_y;
  vector<cell_state> cells;

  level_state() : level_state(60, 24) { }

  level_state(int w, int h, int player_x = 1, int player_y = 1) : w(w), h(h),
      player_x(player_x), player_y(player_y), cells(w * h) {
    for (int x = 0; x < this->w; x++) {
      this->at(x, 0) = cell_state(Block, 0);
      this->at(x, this->h - 1) = cell_state(Block, 0);
    }
    for (int y = 0; y < this->h; y++) {
      this->at(0, y) = cell_state(Block, 0);
      this->at(this->w - 1, y) = cell_state(Block, 0);
    }
    this->at(this->player_x, this->player_y) = cell_state(Player, 0);
  }

  cell_state& at(int x, int y) {
    if (x < 0 || x >= this->w)
      throw out_of_range("x");
    if (y < 0 || y >= this->h)
      throw out_of_range("y");
    return this->cells[y * this->w + x];
  }
};

void exec_frame(struct level_state& l, enum direction player_impulse) {
  for (int y = l.h - 1; y >= 0; y--) {
    for (int x = 0; x < l.w; x++) {
      // rule #1: rocks and items fall
      if ((l.at(x, y).type == Rock || l.at(x, y).type == Item) && (l.at(x, y + 1).type == Empty)) {
        l.at(x, y + 1) = l.at(x, y);
        l.at(x, y) = cell_state(Empty, 0);
      }
    }
  }

  // rule #2: players can have impulses
  struct cell_state* player_target_cell = NULL;
  if (player_impulse == Up)
    player_target_cell = &l.at(l.player_x, l.player_y - 1);
  else if (player_impulse == Down)
    player_target_cell = &l.at(l.player_x, l.player_y + 1);
  else if (player_impulse == Left)
    player_target_cell = &l.at(l.player_x - 1, l.player_y);
  else if (player_impulse == Right)
    player_target_cell = &l.at(l.player_x + 1, l.player_y);
  if (player_target_cell) {
    if (player_target_cell->type == Empty || player_target_cell->type == Circuit ||
        player_target_cell->type == Item) {
      *player_target_cell = l.at(l.player_x, l.player_y);
      l.at(l.player_x, l.player_y) = cell_state(Empty, 0);
      if (player_impulse == Up)
        l.player_y--;
      else if (player_impulse == Down)
        l.player_y++;
      else if (player_impulse == Left)
        l.player_x--;
      else if (player_impulse == Right)
        l.player_x++;
    }
  }
}

void render_level_state_terminal(struct level_state& l) {
  for (int y = 0; y < l.h; y++) {
    for (int x = 0; x < l.w; x++) {
      switch (l.at(x, y).type) {
        case Empty:
          fputc(' ', stdout);
          break;
        case Circuit:
          fputc('*', stdout);
          break;
        case Rock:
          fputc('o', stdout);
          break;
        case Exit:
          fputc('E', stdout);
          break;
        case Player:
          fputc('U', stdout);
          break;
        case Item:
          fputc('+', stdout);
          break;
        case Block:
          fputc('8', stdout);
          break;
      }
    }
    fputc('\n', stdout);
  }
}

// TODO use projection matrix to make this unnecessary
float to_window(float x, float w) {
  return ((x / w) * 2) - 1;
}

void render_level_state_opengl(struct level_state& l) {
  glBegin(GL_QUADS);

  for (int y = 0; y < l.h; y++) {
    for (int x = 0; x < l.w; x++) {
      switch (l.at(x, y).type) {
        case Empty:
          glColor4f(0.0, 0.0, (float)(l.at(x, y).param) / 256, 1.0);
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
      }
      glVertex3f(to_window(x, l.w), -to_window(y, l.h), 1);
      glVertex3f(to_window(x + 1, l.w), -to_window(y, l.h), 1);
      glVertex3f(to_window(x + 1, l.w), -to_window(y + 1, l.h), 1);
      glVertex3f(to_window(x, l.w), -to_window(y + 1, l.h), 1);
    }
  }

  glEnd();
}

struct level_state generate_test_level() {
  struct level_state l(60, 24);
  for (int y = 0; y < l.h; y++) {
    for (int x = 0; x < l.w; x++) {
      if (x < 30) {
        if (((x + y) & 1) && (l.at(x, y).type == Empty)) {
          l.at(x, y) = cell_state(Circuit, 0);
        }
      } else {
        if (y < 12) {
          if (((x + y) & 1) && (l.at(x, y).type == Empty)) {
            l.at(x, y) = cell_state(Item, 0);
          }
        } else {
          if (l.at(x, y).type == Empty) {
            l.at(x, y) = cell_state(Circuit, 0);
          }
        }
      }
    }
  }
  return l;
}



void render_stripe_animation(int window_w, int window_h, int stripe_width) {
  glBegin(GL_QUADS);
  glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
  glVertex3f(-1.0f, -1.0f, 1.0f);
  glVertex3f(1.0f, -1.0f, 1.0f);
  glVertex3f(1.0f, 1.0f, 1.0f);
  glVertex3f(-1.0f, 1.0f, 1.0f);

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


struct level_state game;
int paused = 1;
enum direction player_direction = None;

static void glfw_key_cb(GLFWwindow* window, int key, int scancode,
    int action, int mods) {

  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    if (key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(window, 1);
    if (key == GLFW_KEY_ENTER)
      paused = !paused;
    if (!paused) {
      if (key == GLFW_KEY_LEFT)
        player_direction = Left;
      if (key == GLFW_KEY_RIGHT)
        player_direction = Right;
      if (key == GLFW_KEY_UP)
        player_direction = Up;
      if (key == GLFW_KEY_DOWN)
        player_direction = Down;
    }
  }

  if (action == GLFW_RELEASE) {
    // note: we don't check for paused here to avoid bad state if the player
    // pauses while holding a direction key
    if (((key == GLFW_KEY_LEFT) && (player_direction == Left)) ||
        ((key == GLFW_KEY_RIGHT) && (player_direction == Right)) ||
        ((key == GLFW_KEY_UP) && (player_direction == Up)) ||
        ((key == GLFW_KEY_DOWN) && (player_direction == Down)))
      player_direction = None;
  }
}

static void glfw_resize_cb(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

static void glfw_error_cb(int error, const char* description) {
  fprintf(stderr, "[GLFW %d] %s\n", error, description);
}

int main(int argc, char* argv[]) {

  game = generate_test_level();

  if (!glfwInit()) {
    fprintf(stderr, "failed to initialize GLFW\n");
    exit(1);
  }
  glfwSetErrorCallback(glfw_error_cb);

  GLFWwindow* window = glfwCreateWindow(60 * 24, 24 * 24,
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
  float updates_per_second = 20;
  uint64_t usec_per_update = 1000000.0 / updates_per_second;

  while (!glfwWindowShouldClose(window)) {

    uint64_t now_time = now();
    uint64_t update_diff = now_time - last_update_time;
    if (update_diff >= usec_per_update) {
      if (!paused)
        exec_frame(game, player_direction);
      last_update_time = now_time;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_level_state_opengl(game);

    if (paused) {
      int window_w, window_h;
      glfwGetFramebufferSize(window, &window_w, &window_h);
      render_stripe_animation(window_w, window_h, 100);
      glBegin(GL_QUADS);
      draw_text(0, 0.7, 1, 1, 1, 1, (float)window_w / window_h, 0.03, true,
          "MOVE BLOCKS AND EAT STUFF");
      draw_text(0, 0.3, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
          "PRESS ENTER TO BEGIN");
      glEnd();
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
