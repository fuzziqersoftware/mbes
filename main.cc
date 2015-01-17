#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include <list>
#include <stdexcept>
#include <vector>

#include "gl_text.h"

using namespace std;



unsigned long long now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}



enum player_impulse {
  None = 0,
  Up,
  Down,
  Left,
  Right,
  DropBomb,
};

enum block_fall_action {
  Resting = 0,
  Falling,
};

enum cell_type {
  Empty = 0,
  Circuit,
  Rock,
  Exit,
  Player,
  Item,
  Block,
  RoundBlock,
  GreenBomb,
  YellowBomb,
  YellowBombTrigger,
  RedBomb,
  Explosion,
  ItemDude,
  BombDude,
  LeftPortal,
  RightPortal,
  UpPortal,
  DownPortal,
  HorizontalPortal,
  VerticalPortal,
  Portal,
};

enum explosion_type {
  NormalExplosion = 0,
  ItemExplosion,
};

struct cell_state {
  cell_type type;
  int param;
  bool moved;

  cell_state() : type(Empty), param(1), moved(false) { }
  cell_state(cell_type type, int param = 0, bool moved = false) : type(type),
      param(param), moved(moved) { }

  bool is_round() const {
    return (this->type == Rock) || (this->type == Item) ||
           (this->type == RoundBlock);
  }
  bool should_fall() const {
    return (this->type == Rock) || (this->type == Item) ||
           (this->type == GreenBomb);
  }
  bool destroyable() const {
    return (this->type != Block);
  }
  bool is_bomb() const {
    return (this->type == GreenBomb) || (this->type == RedBomb) ||
           (this->type == YellowBomb) || (this->type == ItemDude) ||
           (this->type == BombDude);
  }
  bool is_volatile() const {
    return (this->type == Player) || this->is_dude() || this->is_bomb();
  }
  bool is_edible() const {
    return (this->type == Empty) || (this->type == Circuit) ||
           (this->type == Item) || (this->type == RedBomb) ||
           (this->type == YellowBombTrigger);
  }
  bool is_pushable_horizontal() const {
    return (this->type == Rock) || (this->type == GreenBomb) ||
           (this->type == YellowBomb);
  }
  bool is_pushable_vertical() const {
    return (this->type == YellowBomb);
  }
  bool is_dude() const {
    return (this->type == ItemDude) || (this->type == BombDude);
  }
  explosion_type explosion_type() const {
    return (this->type == ItemDude) ? ItemExplosion : NormalExplosion;
  }
  bool is_left_portal() const {
    return (this->type == LeftPortal) || (this->type == HorizontalPortal) ||
           (this->type == Portal);
  }
  bool is_right_portal() const {
    return (this->type == RightPortal) || (this->type == HorizontalPortal) ||
           (this->type == Portal);
  }
  bool is_up_portal() const {
    return (this->type == UpPortal) || (this->type == VerticalPortal) ||
           (this->type == Portal);
  }
  bool is_down_portal() const {
    return (this->type == DownPortal) || (this->type == VerticalPortal) ||
           (this->type == Portal);
  }
};

struct explosion_info {
  int x;
  int y;
  int size;
  int frames; // how many more frames before it occurs
  explosion_type type;

  explosion_info(int x, int y, int size = 1, int frames = 0,
      explosion_type type = NormalExplosion) : x(x), y(y), size(size),
      frames(frames), type(type) { }
};

struct level_state {
  int w;
  int h;
  int player_x;
  int player_y;
  int num_items_remaining;
  int num_red_bombs;
  bool player_will_drop_bomb;
  bool player_did_win;
  float updates_per_second;
  vector<cell_state> cells;
  list<explosion_info> pending_explosions;

  level_state() : level_state(60, 24) { }

  level_state(int w, int h, int player_x = 1, int player_y = 1) : w(w), h(h),
      player_x(player_x), player_y(player_y), num_items_remaining(0),
      num_red_bombs(0), player_will_drop_bomb(false), player_did_win(false),
      updates_per_second(20.0f), cells(w * h) {
    for (int x = 0; x < this->w; x++) {
      this->at(x, 0) = cell_state(Block);
      this->at(x, this->h - 1) = cell_state(Block);
    }
    for (int y = 0; y < this->h; y++) {
      this->at(0, y) = cell_state(Block);
      this->at(this->w - 1, y) = cell_state(Block);
    }
    this->at(this->player_x, this->player_y) = cell_state(Player);
  }

  cell_state& at(int x, int y) {
    if (x < 0 || x >= this->w)
      throw out_of_range("x");
    if (y < 0 || y >= this->h)
      throw out_of_range("y");
    return this->cells[y * this->w + x];
  }

  const cell_state& at(int x, int y) const {
    if (x < 0 || x >= this->w)
      throw out_of_range("x");
    if (y < 0 || y >= this->h)
      throw out_of_range("y");
    return this->cells[y * this->w + x];
  }

  void move_cell(int x, int y, player_impulse dir) {
    this->at(x, y).moved = true;
    if (dir == Left)
      this->at(x - 1, y) = this->at(x, y);
    else if (dir == Right)
      this->at(x + 1, y) = this->at(x, y);
    else if (dir == Up)
      this->at(x, y - 1) = this->at(x, y);
    else if (dir == Down)
      this->at(x, y + 1) = this->at(x, y);
    else
      throw runtime_error("invalid direction");
    this->at(x, y) = cell_state(Empty);
  }

  bool player_is_alive() const {
    return (this->at(this->player_x, this->player_y).type == Player);
  }
};

void exec_frame(level_state& l, enum player_impulse impulse) {
  for (int y = l.h - 1; y >= 0; y--) {
    for (int x = 0; x < l.w; x++) {
      // rule #0: explosions disappear, items appear
      if (l.at(x, y).type == Explosion) {
        l.at(x, y).param -= 16;
        if (l.at(x, y).param <= 0) {
          l.at(x, y) = cell_state(Empty);
        }
      }

      // rule #1: red bombs attenuate, then explode
      if (l.at(x, y).type == RedBomb && l.at(x, y).param) {
        l.at(x, y).param += 16;
        if (l.at(x, y).param >= 256)
          l.pending_explosions.emplace_back(x, y);
      }

      // rule #1: empty space attenuates
      if (l.at(x, y).type == Empty) {
        int param = l.at(x, y).param;
        if ((param && param < 256) ||
            (l.at(x - 1, y).type == Empty && l.at(x - 1, y).param) ||
            (l.at(x + 1, y).type == Empty && l.at(x + 1, y).param) ||
            (l.at(x, y - 1).type == Empty && l.at(x, y - 1).param) ||
            (l.at(x, y + 1).type == Empty && l.at(x, y + 1).param))
          l.at(x, y).param++;
      }

      // rule #2: rocks, items and green bombs fall
      if (l.at(x, y).should_fall() && !l.at(x, y).moved) {
        if (l.at(x, y + 1).type == Empty) {
          l.at(x, y + 1) = cell_state(l.at(x, y).type, Falling, true);
          l.at(x, y) = cell_state(Empty);
        } else if (l.at(x, y + 1).is_volatile() && (l.at(x, y).param == Falling)) {
          l.pending_explosions.emplace_back(x, y + 1, 1, 0, l.at(x, y + 1).explosion_type());
        } else if (l.at(x, y).type == GreenBomb && (l.at(x, y).param == Falling)) {
          l.pending_explosions.emplace_back(x, y, 1, 2);
        } else {
          l.at(x, y).param = Resting;
        }
      }

      // rule #3: round, fallable objects roll off other round objects
      if (l.at(x, y).should_fall() && l.at(x, y).is_round() &&
          l.at(x, y + 1).is_round() && !l.at(x, y).moved) {
        if (l.at(x - 1, y).type == Empty && l.at(x - 1, y + 1).type == Empty) {
          l.at(x - 1, y) = cell_state(l.at(x, y).type, Resting, true);
          l.at(x, y) = cell_state(Empty);
        } else if (l.at(x + 1, y).type == Empty && l.at(x + 1, y + 1).type == Empty) {
          l.at(x + 1, y) = cell_state(l.at(x, y).type, Resting, true);
          l.at(x, y) = cell_state(Empty);
        }
      }

      // rule #4: dudes move along their left wall
      if (l.at(x, y).is_dude() && !l.at(x, y).moved) {
        bool should_check_backturn = l.at(x, y).param > 0;

        l.at(x, y).param = abs(l.at(x, y).param);
        if (abs(l.at(x, y).param) == Left) {
          if (should_check_backturn && (l.at(x, y + 1).type == Empty))
            l.at(x, y).param = -Down;
          else if (l.at(x - 1, y).type == Empty)
            l.move_cell(x, y, Left);
          else
            l.at(x, y).param = Up;

        } else if (abs(l.at(x, y).param) == Up) {
          if (should_check_backturn && (l.at(x - 1, y).type == Empty))
            l.at(x, y).param = -Left;
          else if (l.at(x, y - 1).type == Empty)
            l.move_cell(x, y, Up);
          else
            l.at(x, y).param = Right;

        } else if (abs(l.at(x, y).param) == Right) {
          if (should_check_backturn && (l.at(x, y - 1).type == Empty))
            l.at(x, y).param = -Up;
          else if (l.at(x + 1, y).type == Empty)
            l.move_cell(x, y, Right);
          else
            l.at(x, y).param = Down;

        } else if (abs(l.at(x, y).param) == Down) {
          if (should_check_backturn && (l.at(x + 1, y).type == Empty))
            l.at(x, y).param = -Right;
          else if (l.at(x, y + 1).type == Empty)
            l.move_cell(x, y, Down);
          else
            l.at(x, y).param = Left;
        }
      }
    }
  }

  // process pending explosions
  list<explosion_info> new_explosions;
  for (auto it = l.pending_explosions.begin(); it != l.pending_explosions.end();) {
    if (it->frames == 0) {
      for (int yy = -it->size; yy <= it->size; yy++) {
        for (int xx = -it->size; xx <= it->size; xx++) {
          if (l.at(it->x + xx, it->y + yy).destroyable()) {
            if ((xx || yy) && l.at(it->x + xx, it->y + yy).is_bomb())
              new_explosions.emplace_back(it->x + xx, it->y + yy, 1, 5,
                  l.at(it->x + xx, it->y + yy).explosion_type());
            if (it->type == ItemExplosion)
              l.at(it->x + xx, it->y + yy) = cell_state(Item);
            else
              l.at(it->x + xx, it->y + yy) = cell_state(Explosion, 255);
          }
        }
      }
      it = l.pending_explosions.erase(it);
    } else {
      it->frames--;
      it++;
    }
  }
  for (const auto& it : new_explosions)
    l.pending_explosions.push_back(it);

  // rule #2: players can have impulses
  if (impulse == DropBomb)
    l.player_will_drop_bomb = true;

  cell_state* player_target_cell = NULL;
  if (impulse == Up)
    player_target_cell = &l.at(l.player_x, l.player_y - 1);
  else if (impulse == Down)
    player_target_cell = &l.at(l.player_x, l.player_y + 1);
  else if (impulse == Left)
    player_target_cell = &l.at(l.player_x - 1, l.player_y);
  else if (impulse == Right)
    player_target_cell = &l.at(l.player_x + 1, l.player_y);

  if (player_target_cell) {
    if (player_target_cell->type == Item)
      l.num_items_remaining--;
    if (player_target_cell->type == RedBomb)
      l.num_red_bombs++;
    if ((player_target_cell->type == Exit) && (l.num_red_bombs >= 0) &&
        (l.num_items_remaining <= 0))
      l.player_did_win = true;

    // if the player is moving into a portal, put them on the other side of it
    cell_state* portal_target_cell = NULL;
    if ((impulse == Left) && player_target_cell->is_left_portal())
      portal_target_cell = &l.at(l.player_x - 2, l.player_y);
    else if ((impulse == Right) && player_target_cell->is_right_portal())
      portal_target_cell = &l.at(l.player_x + 2, l.player_y);
    else if ((impulse == Up) && player_target_cell->is_up_portal())
      portal_target_cell = &l.at(l.player_x, l.player_y - 2);
    else if ((impulse == Down) && player_target_cell->is_down_portal())
      portal_target_cell = &l.at(l.player_x, l.player_y + 2);
    if (portal_target_cell && portal_target_cell->type == Empty) {
      *portal_target_cell = l.at(l.player_x, l.player_y);
      l.at(l.player_x, l.player_y) = cell_state(Empty);
      if (impulse == Left)
        l.player_x -= 2;
      else if (impulse == Right)
        l.player_x += 2;
      else if (impulse == Up)
        l.player_y -= 2;
      else if (impulse == Down)
        l.player_y += 2;
    } else {

      // if the player is pushing something, move it out of the way first
      cell_state* push_target_cell = NULL;
      if ((impulse == Left) && player_target_cell->is_pushable_horizontal())
        push_target_cell = &l.at(l.player_x - 2, l.player_y);
      else if ((impulse == Right) && player_target_cell->is_pushable_horizontal())
        push_target_cell = &l.at(l.player_x + 2, l.player_y);
      else if ((impulse == Up) && player_target_cell->is_pushable_vertical())
        push_target_cell = &l.at(l.player_x, l.player_y - 2);
      else if ((impulse == Down) && player_target_cell->is_pushable_vertical())
        push_target_cell = &l.at(l.player_x, l.player_y + 2);
      if (push_target_cell && push_target_cell->type == Empty) {
        *push_target_cell = *player_target_cell;
        *player_target_cell = cell_state(Empty);
      }

      if (player_target_cell->is_edible()) {
        if (player_target_cell->type == YellowBombTrigger)
          for (int yy = 0; yy < l.h; yy++)
            for (int xx = 0; xx < l.w; xx++)
              if (l.at(xx, yy).type == YellowBomb)
                l.pending_explosions.emplace_back(xx, yy);

        *player_target_cell = l.at(l.player_x, l.player_y);
        if (l.player_will_drop_bomb) {
          l.num_red_bombs--;
          l.at(l.player_x, l.player_y) = cell_state(RedBomb, 1);
        } else {
          l.at(l.player_x, l.player_y) = cell_state(Empty);
        }
        l.player_will_drop_bomb = false;

        if (impulse == Up)
          l.player_y--;
        else if (impulse == Down)
          l.player_y++;
        else if (impulse == Left)
          l.player_x--;
        else if (impulse == Right)
          l.player_x++;
      }
    }
  }

  // finally, clear all the moved flags for the next frame
  for (int y = 0; y < l.h; y++)
    for (int x = 0; x < l.w; x++)
      l.at(x, y).moved = false;
}

// TODO use projection matrix to make this unnecessary
float to_window(float x, float w) {
  return ((x / w) * 2) - 1;
}

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
          glColor4f(0.5, 0.0, 0.0, 1.0);
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

level_state generate_test_level() {
  level_state l(60, 24);
  for (int y = 0; y < l.h; y++) {
    for (int x = 0; x < l.w; x++) {
      if (x < 30) {
        if (((x + y) & 1) && (l.at(x, y).type == Empty)) {
          l.at(x, y) = cell_state(Circuit);
        }
      } else {
        if (y < 12) {
          if (((x + y) & 1) && (l.at(x, y).type == Empty)) {
            l.at(x, y) = cell_state(Item);
          }
        } else {
          if (l.at(x, y).type == Empty) {
            l.at(x, y) = cell_state(Circuit);
          }
        }
      }
    }
  }
  return l;
}

struct supaplex_level {
  uint8_t cells[24][60];
  uint32_t unknown;
  uint8_t initial_gravity;
  uint8_t spfix_version;
  char title[23];
  uint8_t initial_rock_freeze;
  uint8_t num_items_needed;
  uint8_t num_gravity_ports;
  struct {
    uint16_t location;
    uint8_t enable_gravity;
    uint8_t enable_rock_freeze;
    uint8_t enable_enemy_freeze;
    uint8_t unused;
  } custom_ports[10];
  uint32_t unused;
};

level_state load_level(const char* filename, int level_index) {

  if (!filename)
    return generate_test_level();

  supaplex_level spl;
  FILE* f = fopen(filename, "rb");
  if (!f)
    throw runtime_error("file not found");
  fseek(f, level_index * sizeof(supaplex_level), SEEK_SET);
  fread(&spl, 1, sizeof(spl), f);
  fclose(f);

  printf("note: level is called %23s\n", spl.title);

  level_state l;
  l.num_items_remaining = spl.num_items_needed;
  for (int y = 0; y < 24; y++) {
    for (int x = 0; x < 60; x++) {
      switch (spl.cells[y][x]) {
        case 0x00: // empty
          l.at(x, y) = cell_state(Empty, 1);
          break;
        case 0x01: // rock
          l.at(x, y) = cell_state(Rock);
          break;
        case 0x02: // circuit
        case 0x19: // zap circuit (TODO)
          l.at(x, y) = cell_state(Circuit);
          break;
        case 0x03: // player
          l.player_x = x;
          l.player_y = y;
          l.at(x, y) = cell_state(Player);
          break;
        case 0x04: // item
          l.at(x, y) = cell_state(Item);
          break;
        case 0x05: // small chip
        case 0x1A: // chip left (TODO)
        case 0x1B: // chip right (TODO)
        case 0x26: // chip top (TODO)
        case 0x27: // chip bottom (TODO)
          l.at(x, y) = cell_state(RoundBlock);
          break;
        case 0x06: // block
        case 0x1C: // small components (TODO)
        case 0x1D: // green dot (TODO)
        case 0x1E: // blue dot (TODO)
        case 0x1F: // red dot (TODO)
        case 0x20: // red/black striped block (TODO)
        case 0x21: // resistors mixed (TODO)
        case 0x22: // capacitor (TODO)
        case 0x23: // resistors horizontal (TODO)
        case 0x24: // resistors vertical identical (TODO)
        case 0x25: // resistors hirizontal identical (TODO)
          l.at(x, y) = cell_state(Block);
          break;
        case 0x07: // exit
          l.at(x, y) = cell_state(Exit);
          break;
        case 0x08: // green bomb
          l.at(x, y) = cell_state(GreenBomb);
          break;
        case 0x12: // yellow bomb
          l.at(x, y) = cell_state(YellowBomb);
          break;
        case 0x14: // red bomb
          l.at(x, y) = cell_state(RedBomb);
          break;
        case 0x13: // terminal
          l.at(x, y) = cell_state(YellowBombTrigger);
          break;
        case 0x11: // scissors
          l.at(x, y) = cell_state(BombDude, Up);
          break;
        case 0x18: // spark
          l.at(x, y) = cell_state(ItemDude, Up);
          break;
        case 0x09: // portal right (TODO)
        case 0x0F: // portal right special (TODO)
          l.at(x, y) = cell_state(RightPortal);
          break;
        case 0x0A: // portal down (TODO)
        case 0x0E: // portal down special (TODO)
          l.at(x, y) = cell_state(DownPortal);
          break;
        case 0x0B: // portal left (TODO)
        case 0x0D: // portal left special (TODO)
          l.at(x, y) = cell_state(LeftPortal);
          break;
        case 0x0C: // portal up (TODO)
        case 0x10: // portal up special (TODO)
          l.at(x, y) = cell_state(UpPortal);
          break;
        case 0x15: // portal vertical (TODO)
          l.at(x, y) = cell_state(VerticalPortal);
          break;
        case 0x16: // portal horizontal (TODO)
          l.at(x, y) = cell_state(HorizontalPortal);
          break;
        case 0x17: // portal 4-way (TODO)
          l.at(x, y) = cell_state(Portal);
          break;
        default:
          l.at(x, y) = cell_state(Empty, 255);
      }
      fprintf(stderr, "%02X", spl.cells[y][x]);
    }
    fprintf(stderr, "\n");
  }

  return l;
}



void render_stripe_animation(int window_w, int window_h, int stripe_width) {
  glBegin(GL_QUADS);
  glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
  glVertex3f(-1.0f, -1.0f, 1.0f);
  glVertex3f(1.0f, -1.0f, 1.0f);
  glVertex3f(1.0f, 1.0f, 1.0f);
  glVertex3f(-1.0f, 1.0f, 1.0f);

  glColor4f(0.0f, 0.0f, 0.0f, 0.1f);
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
  level_state initial_state = load_level(level_filename, level_index);
  game = initial_state;

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

  while (!glfwWindowShouldClose(window)) {

    int window_w, window_h;
    glfwGetFramebufferSize(window, &window_w, &window_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!game.player_is_alive()) {
      paused = true;
      player_did_lose = true;
      game = initial_state;
    } else if (game.player_did_win) {
      paused = true;

    } else if (should_reload_state) {
      paused = true;
      game = initial_state;
      should_reload_state = false;

    } else {
      uint64_t usec_per_update = 1000000.0 / game.updates_per_second;
      uint64_t now_time = now();
      uint64_t update_diff = now_time - last_update_time;
      if (update_diff >= usec_per_update) {
        if (!paused)
          exec_frame(game, current_impulse);
        last_update_time = now_time;
      }

      render_level_state(game, window_w, window_h);
      if (paused)
        render_stripe_animation(window_w, window_h, 100);
    }

    if (paused) {
      draw_text(0, 0.7, 1, 1, 1, 1, (float)window_w / window_h, 0.03, true,
          "MOVE BLOCKS AND EAT STUFF");
      if (game.player_did_win)
        draw_text(0, 0.3, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
            "YOU WIN");
      else {
        if (player_did_lose)
          draw_text(0, 0.3, 1, 0, 0, 1, (float)window_w / window_h, 0.01, true,
              "YOU LOSE");
        else
          draw_text(0, 0.3, 1, 1, 1, 1, (float)window_w / window_h, 0.02, true,
              "PRESS ENTER TO PLAY");

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

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
