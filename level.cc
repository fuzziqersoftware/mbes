#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include <list>
#include <stdexcept>
#include <vector>

#include "level.hh"

using namespace std;

cell_state::cell_state() : type(Empty), param(1), moved(false) {
}

cell_state::cell_state(cell_type type, int param, bool moved) : type(type),
    param(param), moved(moved) {
}

bool cell_state::is_round() const {
  return (this->type == Rock) || (this->type == Item) ||
         (this->type == RoundBlock);
}

bool cell_state::should_fall() const {
  return (this->type == Rock) || (this->type == Item) ||
         (this->type == GreenBomb);
}

bool cell_state::destroyable() const {
  return (this->type != Block);
}

bool cell_state::is_bomb() const {
  return (this->type == GreenBomb) || (this->type == RedBomb) ||
         (this->type == YellowBomb) || (this->type == ItemDude) ||
         (this->type == BombDude);
}

bool cell_state::is_volatile() const {
  return (this->type == Player) || this->is_dude() || this->is_bomb();
}

bool cell_state::is_edible() const {
  return (this->type == Empty) || (this->type == Circuit) ||
         (this->type == Item) || (this->type == RedBomb) ||
         (this->type == YellowBombTrigger);
}

bool cell_state::is_pushable_horizontal() const {
  return (this->type == Rock) || (this->type == GreenBomb) ||
         (this->type == YellowBomb);
}

bool cell_state::is_pushable_vertical() const {
  return (this->type == YellowBomb);
}

bool cell_state::is_dude() const {
  return (this->type == ItemDude) || (this->type == BombDude);
}

explosion_type cell_state::explosion_type() const {
  return (this->type == ItemDude) ? ItemExplosion : NormalExplosion;
}

bool cell_state::is_left_portal() const {
  return (this->type == LeftPortal) || (this->type == HorizontalPortal) ||
         (this->type == Portal);
}

bool cell_state::is_right_portal() const {
  return (this->type == RightPortal) || (this->type == HorizontalPortal) ||
         (this->type == Portal);
}

bool cell_state::is_up_portal() const {
  return (this->type == UpPortal) || (this->type == VerticalPortal) ||
         (this->type == Portal);
}

bool cell_state::is_down_portal() const {
  return (this->type == DownPortal) || (this->type == VerticalPortal) ||
         (this->type == Portal);
}



explosion_info::explosion_info(int x, int y, int size, int frames,
    explosion_type type) : x(x), y(y), size(size), frames(frames), type(type) {
}



level_state::level_state(int w, int h, int player_x, int player_y) : w(w), h(h),
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
  if ((this->player_x >= 0) && (this->player_y >= 0))
    this->at(this->player_x, this->player_y) = cell_state(Player);
}

cell_state& level_state::at(int x, int y) {
  if (x < 0 || x >= this->w)
    throw out_of_range("x");
  if (y < 0 || y >= this->h)
    throw out_of_range("y");
  return this->cells[y * this->w + x];
}

const cell_state& level_state::at(int x, int y) const {
  if (x < 0 || x >= this->w)
    throw out_of_range("x");
  if (y < 0 || y >= this->h)
    throw out_of_range("y");
  return this->cells[y * this->w + x];
}

void level_state::move_cell(int x, int y, player_impulse dir) {
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

bool level_state::player_is_alive() const {
  return (this->at(this->player_x, this->player_y).type == Player);
}

bool level_state::validate() const {
  // check that a Player cell exists
  for (int y = 0; y < this->h; y++)
    for (int x = 0; x < this->w; x++)
      if (this->at(x, y).type == Player)
        return true;
  return false;
}



void level_state::exec_frame(enum player_impulse impulse) {
  for (int y = this->h - 1; y >= 0; y--) {
    for (int x = 0; x < this->w; x++) {
      // rule #0: explosions disappear, items appear
      if (this->at(x, y).type == Explosion) {
        this->at(x, y).param -= 16;
        if (this->at(x, y).param <= 0) {
          this->at(x, y) = cell_state(Empty);
        }
      }

      // rule #1: red bombs attenuate, then explode
      if (this->at(x, y).type == RedBomb && this->at(x, y).param) {
        this->at(x, y).param += 16;
        if (this->at(x, y).param >= 256)
          this->pending_explosions.emplace_back(x, y);
      }

      // rule #1: empty space attenuates
      if (this->at(x, y).type == Empty) {
        int param = this->at(x, y).param;
        if ((param && param < 256) ||
            (this->at(x - 1, y).type == Empty && this->at(x - 1, y).param) ||
            (this->at(x + 1, y).type == Empty && this->at(x + 1, y).param) ||
            (this->at(x, y - 1).type == Empty && this->at(x, y - 1).param) ||
            (this->at(x, y + 1).type == Empty && this->at(x, y + 1).param))
          this->at(x, y).param++;
      }

      // rule #2: rocks, items and green bombs fall
      if (this->at(x, y).should_fall() && !this->at(x, y).moved) {
        if (this->at(x, y + 1).type == Empty) {
          this->at(x, y + 1) = cell_state(this->at(x, y).type, Falling, true);
          this->at(x, y) = cell_state(Empty);
        } else if (this->at(x, y + 1).is_volatile() && (this->at(x, y).param == Falling)) {
          this->pending_explosions.emplace_back(x, y + 1, 1, 0, this->at(x, y + 1).explosion_type());
        } else if (this->at(x, y).type == GreenBomb && (this->at(x, y).param == Falling)) {
          this->pending_explosions.emplace_back(x, y, 1, 2);
        } else {
          this->at(x, y).param = Resting;
        }
      }

      // rule #3: round, fallable objects roll off other round objects
      if (this->at(x, y).should_fall() && this->at(x, y).is_round() &&
          this->at(x, y + 1).is_round() && !this->at(x, y).moved) {
        if (this->at(x - 1, y).type == Empty && this->at(x - 1, y + 1).type == Empty) {
          this->at(x - 1, y) = cell_state(this->at(x, y).type, Resting, true);
          this->at(x, y) = cell_state(Empty);
        } else if (this->at(x + 1, y).type == Empty && this->at(x + 1, y + 1).type == Empty) {
          this->at(x + 1, y) = cell_state(this->at(x, y).type, Resting, true);
          this->at(x, y) = cell_state(Empty);
        }
      }

      // rule #4: dudes move along their left wall
      if (this->at(x, y).is_dude() && !this->at(x, y).moved) {
        bool should_check_backturn = this->at(x, y).param > 0;

        this->at(x, y).param = abs(this->at(x, y).param);
        if (abs(this->at(x, y).param) == Left) {
          if (should_check_backturn && (this->at(x, y + 1).type == Empty))
            this->at(x, y).param = -Down;
          else if (this->at(x - 1, y).type == Empty)
            this->move_cell(x, y, Left);
          else
            this->at(x, y).param = Up;

        } else if (abs(this->at(x, y).param) == Up) {
          if (should_check_backturn && (this->at(x - 1, y).type == Empty))
            this->at(x, y).param = -Left;
          else if (this->at(x, y - 1).type == Empty)
            this->move_cell(x, y, Up);
          else
            this->at(x, y).param = Right;

        } else if (abs(this->at(x, y).param) == Right) {
          if (should_check_backturn && (this->at(x, y - 1).type == Empty))
            this->at(x, y).param = -Up;
          else if (this->at(x + 1, y).type == Empty)
            this->move_cell(x, y, Right);
          else
            this->at(x, y).param = Down;

        } else if (abs(this->at(x, y).param) == Down) {
          if (should_check_backturn && (this->at(x + 1, y).type == Empty))
            this->at(x, y).param = -Right;
          else if (this->at(x, y + 1).type == Empty)
            this->move_cell(x, y, Down);
          else
            this->at(x, y).param = Left;
        }
      }
    }
  }

  // process pending explosions
  list<explosion_info> new_explosions;
  for (auto it = this->pending_explosions.begin(); it != this->pending_explosions.end();) {
    if (it->frames == 0) {
      for (int yy = -it->size; yy <= it->size; yy++) {
        for (int xx = -it->size; xx <= it->size; xx++) {
          if (this->at(it->x + xx, it->y + yy).destroyable()) {
            if ((xx || yy) && this->at(it->x + xx, it->y + yy).is_volatile()) {
              explosion_type new_type = this->at(it->x + xx, it->y + yy).explosion_type();
              if (it->type == ItemExplosion)
                new_type = ItemExplosion;
              new_explosions.emplace_back(it->x + xx, it->y + yy, 1, 5,
                  new_type);
            }
            if (it->type == ItemExplosion)
              this->at(it->x + xx, it->y + yy) = cell_state(Item);
            else
              this->at(it->x + xx, it->y + yy) = cell_state(Explosion, 255);
          }
        }
      }
      it = this->pending_explosions.erase(it);
    } else {
      it->frames--;
      it++;
    }
  }
  for (const auto& it : new_explosions)
    this->pending_explosions.push_back(it);

  // rule #2: players can have impulses
  if (impulse == DropBomb)
    this->player_will_drop_bomb = true;

  cell_state* player_target_cell = NULL;
  if (impulse == Up)
    player_target_cell = &this->at(this->player_x, this->player_y - 1);
  else if (impulse == Down)
    player_target_cell = &this->at(this->player_x, this->player_y + 1);
  else if (impulse == Left)
    player_target_cell = &this->at(this->player_x - 1, this->player_y);
  else if (impulse == Right)
    player_target_cell = &this->at(this->player_x + 1, this->player_y);

  if (player_target_cell) {
    if (player_target_cell->type == Item)
      this->num_items_remaining--;
    if (player_target_cell->type == RedBomb)
      this->num_red_bombs++;
    if ((player_target_cell->type == Exit) && (this->num_red_bombs >= 0) &&
        (this->num_items_remaining <= 0))
      this->player_did_win = true;

    // if the player is moving into a portal, put them on the other side of it
    cell_state* portal_target_cell = NULL;
    if ((impulse == Left) && player_target_cell->is_left_portal())
      portal_target_cell = &this->at(this->player_x - 2, this->player_y);
    else if ((impulse == Right) && player_target_cell->is_right_portal())
      portal_target_cell = &this->at(this->player_x + 2, this->player_y);
    else if ((impulse == Up) && player_target_cell->is_up_portal())
      portal_target_cell = &this->at(this->player_x, this->player_y - 2);
    else if ((impulse == Down) && player_target_cell->is_down_portal())
      portal_target_cell = &this->at(this->player_x, this->player_y + 2);

    if (portal_target_cell && portal_target_cell->type == Empty) {
      *portal_target_cell = this->at(this->player_x, this->player_y);
      if (this->player_will_drop_bomb) {
        this->num_red_bombs--;
        this->at(this->player_x, this->player_y) = cell_state(RedBomb, 1);
      } else {
        this->at(this->player_x, this->player_y) = cell_state(Empty);
      }
      this->player_will_drop_bomb = false;

      if (impulse == Left)
        this->player_x -= 2;
      else if (impulse == Right)
        this->player_x += 2;
      else if (impulse == Up)
        this->player_y -= 2;
      else if (impulse == Down)
        this->player_y += 2;
    } else {

      // if the player is pushing something, move it out of the way first
      cell_state* push_target_cell = NULL;
      if ((impulse == Left) && player_target_cell->is_pushable_horizontal())
        push_target_cell = &this->at(this->player_x - 2, this->player_y);
      else if ((impulse == Right) && player_target_cell->is_pushable_horizontal())
        push_target_cell = &this->at(this->player_x + 2, this->player_y);
      else if ((impulse == Up) && player_target_cell->is_pushable_vertical())
        push_target_cell = &this->at(this->player_x, this->player_y - 2);
      else if ((impulse == Down) && player_target_cell->is_pushable_vertical())
        push_target_cell = &this->at(this->player_x, this->player_y + 2);
      if (push_target_cell && push_target_cell->type == Empty) {
        *push_target_cell = *player_target_cell;
        *player_target_cell = cell_state(Empty);
      }

      if (player_target_cell->is_edible()) {
        if (player_target_cell->type == YellowBombTrigger)
          for (int yy = 0; yy < this->h; yy++)
            for (int xx = 0; xx < this->w; xx++)
              if (this->at(xx, yy).type == YellowBomb)
                this->pending_explosions.emplace_back(xx, yy);

        *player_target_cell = this->at(this->player_x, this->player_y);
        if (this->player_will_drop_bomb) {
          this->num_red_bombs--;
          this->at(this->player_x, this->player_y) = cell_state(RedBomb, 1);
        } else {
          this->at(this->player_x, this->player_y) = cell_state(Empty);
        }
        this->player_will_drop_bomb = false;

        if (impulse == Up)
          this->player_y--;
        else if (impulse == Down)
          this->player_y++;
        else if (impulse == Left)
          this->player_x--;
        else if (impulse == Right)
          this->player_x++;
      }
    }
  }

  // finally, clear all the moved flags for the next frame
  for (int y = 0; y < this->h; y++)
    for (int x = 0; x < this->w; x++)
      this->at(x, y).moved = false;
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

static cell_state state_for_supaplex_cell(uint8_t contents) {
  switch (contents) {
    case 0x00: // empty
      return cell_state(Empty, 1);
    case 0x01: // rock
      return cell_state(Rock);
    case 0x02: // circuit
    case 0x19: // zap circuit (TODO)
      return cell_state(Circuit);
    case 0x03: // player
      return cell_state(Player);
    case 0x04: // item
      return cell_state(Item);
    case 0x05: // small chip
    case 0x1A: // chip left (TODO)
    case 0x1B: // chip right (TODO)
    case 0x26: // chip top (TODO)
    case 0x27: // chip bottom (TODO)
      return cell_state(RoundBlock);
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
      return cell_state(Block);
    case 0x07: // exit
      return cell_state(Exit);
    case 0x08: // green bomb
      return cell_state(GreenBomb);
    case 0x12: // yellow bomb
      return cell_state(YellowBomb);
    case 0x14: // red bomb
      return cell_state(RedBomb);
    case 0x13: // terminal
      return cell_state(YellowBombTrigger);
    case 0x11: // scissors
      return cell_state(BombDude, Up);
    case 0x18: // spark
      return cell_state(ItemDude, Up);
    case 0x09: // portal right (TODO)
    case 0x0D: // portal right special (TODO)
      return cell_state(RightPortal);
    case 0x0A: // portal down (TODO)
    case 0x0E: // portal down special (TODO)
      return cell_state(DownPortal);
    case 0x0B: // portal left (TODO)
    case 0x0F: // portal left special (TODO)
      return cell_state(LeftPortal);
    case 0x0C: // portal up (TODO)
    case 0x10: // portal up special (TODO)
      return cell_state(UpPortal);
    case 0x15: // portal vertical (TODO)
      return cell_state(VerticalPortal);
    case 0x16: // portal horizontal (TODO)
      return cell_state(HorizontalPortal);
    case 0x17: // portal 4-way (TODO)
      return cell_state(Portal);
    default:
      return cell_state(Empty, 255);
  }
}

static level_state import_supaplex_level(const supaplex_level& spl) {
  bool need_top_cover = false, need_bottom_cover = false,
       need_left_cover = false, need_right_cover = false;
  for (int y = 0; y < 24; y++) {
    if (state_for_supaplex_cell(spl.cells[y][0]).type != Block)
      need_left_cover = true;
    if (state_for_supaplex_cell(spl.cells[y][59]).type != Block)
      need_right_cover = true;
  }
  for (int x = 0; x < 60; x++) {
    if (state_for_supaplex_cell(spl.cells[0][x]).type != Block)
      need_top_cover = true;
    if (state_for_supaplex_cell(spl.cells[23][x]).type != Block)
      need_bottom_cover = true;
  }

  int level_w = 60 + need_left_cover + need_right_cover;
  int level_h = 24 + need_top_cover + need_bottom_cover;
  level_state l(level_w, level_h, -1, -1);
  l.num_items_remaining = spl.num_items_needed;
  for (int y = 0; y < 24; y++) {
    for (int x = 0; x < 60; x++) {
      int target_x = x + need_left_cover;
      int target_y = y + need_top_cover;
      l.at(target_x, target_y) = state_for_supaplex_cell(spl.cells[y][x]);
      if ((l.at(target_x, target_y).type == Player) &&
          (l.player_x < 0) && (l.player_y < 0)) {
        l.player_x = target_x;
        l.player_y = target_y;
      }
    }
  }

  return l;
}

vector<level_state> load_level_index(const char* filename) {
  FILE* f = fopen(filename, "rb");
  if (!f)
    throw runtime_error("file not found");
  fseek(f, 0, SEEK_END);
  uint64_t num = ftell(f) / sizeof(supaplex_level);
  fseek(f, 0, SEEK_SET);

  vector<level_state> all_levels;
  for (uint64_t x = 0; x < num; x++) {
    supaplex_level spl;
    fread(&spl, 1, sizeof(spl), f);
    all_levels.push_back(import_supaplex_level(spl));
  }
  fclose(f);

  return all_levels;
}
