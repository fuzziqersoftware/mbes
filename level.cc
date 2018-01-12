#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include <list>
#include <stdexcept>
#include <vector>

#include "level.hh"
#include "util.hh"

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
         (this->type == GreenBomb) || (this->type == BlueBomb) ||
         (this->type == GrayBomb);
}

bool cell_state::destroyable() const {
  return (this->type != Block) && (this->type != Destroyer) &&
         (this->type != Deleter);
}

bool cell_state::is_bomb() const {
  return (this->type == GreenBomb) || (this->type == RedBomb) ||
         (this->type == YellowBomb) || (this->type == BlueBomb) ||
         (this->type == GrayBomb) || (this->type == ItemDude) ||
         (this->type == BombDude) || (this->type == RockGenerator);
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
         (this->type == BlueBomb) || (this->type == YellowBomb) ||
         (this->type == GrayBomb);
}

bool cell_state::is_pushable_vertical() const {
  return (this->type == YellowBomb);
}

bool cell_state::is_pullable() const {
  return (this->type == PullStone);
}

bool cell_state::is_dude() const {
  return (this->type == ItemDude) || (this->type == BombDude);
}

explosion_type cell_state::get_explosion_type() const {
  if (this->type == ItemDude || this->type == BlueBomb)
    return ItemExplosion;
  if (this->type == GrayBomb)
    return RockExplosion;
  return NormalExplosion;
}

bool cell_state::is_left_portal() const {
  return (this->type == LeftPortal) || (this->type == HorizontalPortal) ||
         (this->type == Portal) || (this->type == LeftJumpPortal) ||
         (this->type == HorizontalJumpPortal) || (this->type == JumpPortal);
}

bool cell_state::is_right_portal() const {
  return (this->type == RightPortal) || (this->type == HorizontalPortal) ||
         (this->type == Portal) || (this->type == RightJumpPortal) ||
         (this->type == HorizontalJumpPortal) || (this->type == JumpPortal);
}

bool cell_state::is_up_portal() const {
  return (this->type == UpPortal) || (this->type == VerticalPortal) ||
         (this->type == Portal) || (this->type == UpJumpPortal) ||
         (this->type == VerticalJumpPortal) || (this->type == JumpPortal);
}

bool cell_state::is_down_portal() const {
  return (this->type == DownPortal) || (this->type == VerticalPortal) ||
         (this->type == Portal) || (this->type == DownJumpPortal) ||
         (this->type == VerticalJumpPortal) || (this->type == JumpPortal);
}

bool cell_state::is_jump_portal() const {
  return (this->type == LeftJumpPortal) || (this->type == RightJumpPortal) ||
         (this->type == UpJumpPortal) || (this->type == DownJumpPortal) ||
         (this->type == VerticalJumpPortal) || (this->type == HorizontalJumpPortal) ||
         (this->type == JumpPortal);
}

void cell_state::read(FILE* f) {
  fread_or_throw(f, &this->type, sizeof(this->type));
  fread_or_throw(f, &this->param, sizeof(this->param));
  // note: we don't read `moved` since it's only transient data
  this->moved = false;
}

void cell_state::write(FILE* f) const {
  fwrite_or_throw(f, &this->type, sizeof(this->type));
  fwrite_or_throw(f, &this->param, sizeof(this->param));
  // note: we don't write `moved` since it's only transient data
}



explosion_info::explosion_info(int x, int y, int size, int frames,
    explosion_type type) : x(x), y(y), size(size), frames(frames), type(type) {
}

void explosion_info::read(FILE* f) {
  fread_or_throw(f, this, sizeof(*this));
}

void explosion_info::write(FILE* f) const {
  fwrite_or_throw(f, this, sizeof(*this));
}



level_state::level_state(uint32_t w, uint32_t h, int32_t player_x,
    int32_t player_y) : w(w), h(h), player_x(player_x), player_y(player_y),
    num_items_remaining(0), num_red_bombs(0), frames_executed(0),
    player_lose_frame(0), player_lose_buffer(1), cells(w * h),
    pending_explosions(), updates_per_second(20.0f),
    player_will_drop_bomb(false), player_did_win(false) {

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

void level_state::read(FILE* f) {
  fread_or_throw(f, &this->w, sizeof(this->w));
  fread_or_throw(f, &this->h, sizeof(this->h));
  fread_or_throw(f, &this->player_x, sizeof(this->player_x));
  fread_or_throw(f, &this->player_y, sizeof(this->player_y));
  fread_or_throw(f, &this->num_items_remaining, sizeof(this->num_items_remaining));
  fread_or_throw(f, &this->num_red_bombs, sizeof(this->num_red_bombs));
  fread_or_throw(f, &this->frames_executed, sizeof(this->frames_executed));

  this->cells.resize(this->w * this->h);
  for (uint64_t x = 0; x < this->w * this->h; x++)
    this->cells[x].read(f);

  uint64_t num_explosions;
  fread_or_throw(f, &num_explosions, sizeof(num_explosions));
  this->pending_explosions.clear();
  for (uint64_t x = 0; x < num_explosions; x++) {
    this->pending_explosions.emplace_back(0, 0);
    this->pending_explosions.back().read(f);
  }
}

void level_state::write(FILE* f) const {
  fwrite_or_throw(f, &this->w, sizeof(this->w));
  fwrite_or_throw(f, &this->h, sizeof(this->h));
  fwrite_or_throw(f, &this->player_x, sizeof(this->player_x));
  fwrite_or_throw(f, &this->player_y, sizeof(this->player_y));
  fwrite_or_throw(f, &this->num_items_remaining, sizeof(this->num_items_remaining));
  fwrite_or_throw(f, &this->num_red_bombs, sizeof(this->num_red_bombs));
  fwrite_or_throw(f, &this->frames_executed, sizeof(this->frames_executed));

  for (uint64_t x = 0; x < this->w * this->h; x++)
    this->cells[x].write(f);

  uint64_t num_explosions = this->pending_explosions.size();
  fwrite_or_throw(f, &num_explosions, sizeof(num_explosions));
  for (const explosion_info& e : this->pending_explosions)
    e.write(f);
}

cell_state& level_state::at(int x, int y) {
  while (x < 0)
    x += this->w;
  while (y < 0)
    y += this->h;
  return this->cells[(y % this->h) * this->w + (x % this->w)];
}

const cell_state& level_state::at(int x, int y) const {
  while (x < 0)
    x += this->w;
  while (y < 0)
    y += this->h;
  return this->cells[(y % this->h) * this->w + (x % this->w)];
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

double level_state::player_is_losing() const {
  if (this->player_is_alive())
    return 0.0;
  if (this->player_lose_frame > this->frames_executed)
    return 0.0;

  uint64_t lose_end_frame = this->player_lose_frame + this->player_lose_buffer * this->updates_per_second;
  if (this->frames_executed >= lose_end_frame)
    return 1.0;

  uint64_t total_lose_frames = lose_end_frame - this->player_lose_frame;
  return (double)(this->frames_executed - this->player_lose_frame) / total_lose_frames;
}

int level_state::count_items() const {
  int count = 0;
  for (int y = 0; y < this->h; y++) {
    for (int x = 0; x < this->w; x++) {
      if (this->at(x, y).type == Item)
        count++;
      else if ((this->at(x, y).type == ItemDude) || (this->at(x, y).type == BlueBomb))
        count += 9;
    }
  }
  return count;
}

int level_state::count_cells_of_type(cell_type c) const {
  int count = 0;
  for (int y = 0; y < this->h; y++)
    for (int x = 0; x < this->w; x++)
      if (this->at(x, y).type == c)
        count++;
  return count;
}

int level_state::count_attenuated_space() const {
  int count = 0;
  for (int y = 0; y < this->h; y++)
    for (int x = 0; x < this->w; x++)
      if ((this->at(x, y).type == Empty) && (this->at(x, y).param > 0))
        count++;
  return count;
}

int level_state::compute_entropy() const {
  // entropy is the number adjacent cell pairs that are different
  // only need to check right and down for each cell (up and left were already
  // checked by the time we get to this cell)
  int entropy = 0;
  for (int y = 0; y < this->h; y++)
    for (int x = 0; x < this->w; x++)
      entropy += (int)(this->at(x, y).type != this->at(x + 1, y).type) +
                 (int)(this->at(x, y).type != this->at(x, y + 1).type);
  return entropy;
}

bool level_state::validate() const {
  // check that a Player cell exists
  for (int y = 0; y < this->h; y++)
    for (int x = 0; x < this->w; x++)
      if (this->at(x, y).type == Player)
        return true;
  return false;
}



uint64_t level_state::exec_frame(const struct player_actions& actions) {

  uint64_t events_occurred = NoEvents;

  if (actions.drop_bomb) {
    this->player_will_drop_bomb = true;
  }

  for (int32_t y = this->h - 1; y >= 0; y--) {
    for (int32_t x = 0; x < this->w; x++) {
      // rule #0: explosions disappear
      if (this->at(x, y).type == Explosion) {
        this->at(x, y).param -= 16;
        if (this->at(x, y).param <= 0) {
          this->at(x, y) = cell_state(Empty);
        }
      }

      // rule #1: destroyers destroy anything on top of them, deleters remove
      // anything on top of them
      if (this->at(x, y).type == Destroyer &&
          this->at(x, y - 1).type != Empty && this->at(x, y - 1).type != Explosion) {
        this->pending_explosions.emplace_back(x, y - 1);
      }
      if (this->at(x, y).type == Deleter &&
          this->at(x, y - 1).type != Empty && this->at(x, y - 1).type != Explosion) {
        this->at(x, y - 1) = cell_state(Empty);
      }

      // rule #2: red bombs attenuate, then explode
      if (this->at(x, y).type == RedBomb && this->at(x, y).param) {
        this->at(x, y).param += 16;
        if (this->at(x, y).param >= 256)
          this->pending_explosions.emplace_back(x, y);
      }

      // rule #3: empty space attenuates
      if (this->at(x, y).type == Empty) {
        int param = this->at(x, y).param;
        if ((param && param < 256) ||
            (this->at(x - 1, y).type == Empty && this->at(x - 1, y).param) ||
            (this->at(x + 1, y).type == Empty && this->at(x + 1, y).param) ||
            (this->at(x, y - 1).type == Empty && this->at(x, y - 1).param) ||
            (this->at(x, y + 1).type == Empty && this->at(x, y + 1).param))
          this->at(x, y).param++;
      }

      // rule #4: rocks, items and certain bombs fall
      if (this->at(x, y).should_fall() && !this->at(x, y).moved) {
        if (this->at(x, y + 1).type == Empty) {
          events_occurred |= ObjectFalling;
          this->at(x, y + 1) = cell_state(this->at(x, y).type, Falling, true);
          this->at(x, y) = cell_state(Empty);
        } else if (this->at(x, y + 1).is_volatile() && (this->at(x, y).param == Falling)) {
          this->pending_explosions.emplace_back(x, y + 1, 1, 0, this->at(x, y + 1).get_explosion_type());
        } else if (this->at(x, y).is_bomb() && (this->at(x, y).param == Falling)) {
          this->pending_explosions.emplace_back(x, y, 1, 2, this->at(x, y).get_explosion_type());
          events_occurred |= ObjectLanded;
          this->at(x, y).param = Resting;
        } else {
          if (this->at(x, y).param == Falling)
            events_occurred |= ObjectLanded;
          this->at(x, y).param = Resting;
        }
      }

      // rule #5: round, fallable objects roll off other round objects
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

      // rule #6: dudes move along their left wall
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

      // rule #7: rock generators generate rocks periodically if there's space below them
      if (this->at(x, y).type == RockGenerator) {
        if (this->at(x, y + 1).type != Empty)
          this->at(x, y).param = 0;
        else {
          if (this->at(x, y).param >= 16) {
            this->at(x, y + 1) = cell_state(Rock);
            this->at(x, y).param = 0;
          } else
            this->at(x, y).param++;
        }
      }
    }
  }

  // process pending explosions
  list<explosion_info> new_explosions;
  for (auto it = this->pending_explosions.begin(); it != this->pending_explosions.end();) {
    if (it->frames == 0) {
      if (it->type == ItemExplosion)
        events_occurred |= ItemExploded;
      else
        events_occurred |= Exploded;

      for (int yy = -it->size; yy <= it->size; yy++) {
        for (int xx = -it->size; xx <= it->size; xx++) {
          if (this->at(it->x + xx, it->y + yy).destroyable()) {
            if ((xx || yy) && this->at(it->x + xx, it->y + yy).is_volatile()) {
              explosion_type new_type = this->at(it->x + xx, it->y + yy).get_explosion_type();
              if (it->type == ItemExplosion)
                new_type = ItemExplosion;
              if (it->type == RockExplosion)
                new_type = RockExplosion;
              new_explosions.emplace_back(it->x + xx, it->y + yy, 1, 5,
                  new_type);
            }
            if (it->type == ItemExplosion)
              this->at(it->x + xx, it->y + yy) = cell_state(Item);
            else if (it->type == RockExplosion)
              this->at(it->x + xx, it->y + yy) = cell_state(Rock);
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

  // if the player is losing or has lost, don't let them move
  if (this->player_is_alive()) {
    cell_state* player_target_cell = NULL;
    if (actions.impulse == Up)
      player_target_cell = &this->at(this->player_x, this->player_y - 1);
    else if (actions.impulse == Down)
      player_target_cell = &this->at(this->player_x, this->player_y + 1);
    else if (actions.impulse == Left)
      player_target_cell = &this->at(this->player_x - 1, this->player_y);
    else if (actions.impulse == Right)
      player_target_cell = &this->at(this->player_x + 1, this->player_y);

    if (player_target_cell) {
      if (player_target_cell->type == Item) {
        events_occurred |= ItemCollected;
        this->num_items_remaining--;
      }
      if (player_target_cell->type == RedBomb) {
        events_occurred |= RedBombCollected;
        this->num_red_bombs++;
      }
      if ((player_target_cell->type == Exit) && (this->num_red_bombs >= 0) &&
          (this->num_items_remaining <= 0)) {
        events_occurred |= PlayerWon;
        this->player_did_win = true;
      }

      // if the player is moving into a portal, put them on the other side of it
      // for jump portals, find a portal in the opposite direction along the
      // player's movement direction
      cell_state* portal_target_cell = NULL;
      int32_t new_player_x, new_player_y;
      if ((actions.impulse == Left) && player_target_cell->is_left_portal()) {
        if (player_target_cell->is_jump_portal()) {
          portal_target_cell = player_target_cell;
          for (int z = 2; z < this->w && portal_target_cell == player_target_cell; z++) {
            if (this->at(this->player_x - z, this->player_y).is_right_portal()) {
              portal_target_cell = &this->at(this->player_x - z - 1, this->player_y);
              new_player_x = this->player_x - z - 1;
              new_player_y = this->player_y;
            }
          }
        } else {
          portal_target_cell = &this->at(this->player_x - 2, this->player_y);
          new_player_x = this->player_x - 2;
          new_player_y = this->player_y;
        }

      } else if ((actions.impulse == Right) && player_target_cell->is_right_portal()) {
        if (player_target_cell->is_jump_portal()) {
          portal_target_cell = player_target_cell;
          for (int z = 2; z < this->w && portal_target_cell == player_target_cell; z++) {
            if (this->at(this->player_x + z, this->player_y).is_left_portal()) {
              portal_target_cell = &this->at(this->player_x + z + 1, this->player_y);
              new_player_x = this->player_x + z + 1;
              new_player_y = this->player_y;
            }
          }
        } else {
          portal_target_cell = &this->at(this->player_x + 2, this->player_y);
          new_player_x = this->player_x + 2;
          new_player_y = this->player_y;
        }

      } else if ((actions.impulse == Up) && player_target_cell->is_up_portal()) {
        if (player_target_cell->is_jump_portal()) {
          portal_target_cell = player_target_cell;
          for (int z = 2; z < this->h && portal_target_cell == player_target_cell; z++) {
            if (this->at(this->player_x, this->player_y - z).is_down_portal()) {
              portal_target_cell = &this->at(this->player_x, this->player_y - z - 1);
              new_player_x = this->player_x;
              new_player_y = this->player_y - z - 1;
            }
          }
        } else {
          portal_target_cell = &this->at(this->player_x, this->player_y - 2);
          new_player_x = this->player_x;
          new_player_y = this->player_y - 2;
        }

      } else if ((actions.impulse == Down) && player_target_cell->is_down_portal()) {
        if (player_target_cell->is_jump_portal()) {
          portal_target_cell = player_target_cell;
          for (int z = 2; z < this->h && portal_target_cell == player_target_cell; z++) {
            if (this->at(this->player_x, this->player_y + z).is_up_portal()) {
              portal_target_cell = &this->at(this->player_x, this->player_y + z + 1);
              new_player_x = this->player_x;
              new_player_y = this->player_y + z + 1;
            }
          }
        } else {
          portal_target_cell = &this->at(this->player_x, this->player_y + 2);
          new_player_x = this->player_x;
          new_player_y = this->player_y + 2;
        }
      }

      if (portal_target_cell && portal_target_cell->type == Empty) {
        *portal_target_cell = this->at(this->player_x, this->player_y);
        if (this->player_will_drop_bomb) {
          this->player_will_drop_bomb = false;
          this->num_red_bombs--;
          this->at(this->player_x, this->player_y) = cell_state(RedBomb, 1);
          events_occurred |= RedBombDropped;
        } else {
          this->at(this->player_x, this->player_y) = cell_state(Empty);
        }

        this->player_x = new_player_x;
        this->player_y = new_player_y;

      } else {

        // if the player is pushing something, move it out of the way first
        cell_state* push_target_cell = NULL;
        if ((actions.impulse == Left) && player_target_cell->is_pushable_horizontal())
          push_target_cell = &this->at(this->player_x - 2, this->player_y);
        else if ((actions.impulse == Right) && player_target_cell->is_pushable_horizontal())
          push_target_cell = &this->at(this->player_x + 2, this->player_y);
        else if ((actions.impulse == Up) && player_target_cell->is_pushable_vertical())
          push_target_cell = &this->at(this->player_x, this->player_y - 2);
        else if ((actions.impulse == Down) && player_target_cell->is_pushable_vertical())
          push_target_cell = &this->at(this->player_x, this->player_y + 2);
        if (push_target_cell && push_target_cell->type == Empty) {
          events_occurred |= ObjectPushed;
          *push_target_cell = *player_target_cell;
          *player_target_cell = cell_state(Empty);
        }

        // check if the cell is pullable - if so, pull it
        if (player_target_cell->is_pullable()) {
          events_occurred |= ObjectPushed;

          cell_state target_cell_contents = *player_target_cell;
          *player_target_cell = this->at(this->player_x, this->player_y);
          this->at(this->player_x, this->player_y) = target_cell_contents;

          if (actions.impulse == Up)
            this->player_y--;
          else if (actions.impulse == Down)
            this->player_y++;
          else if (actions.impulse == Left)
            this->player_x--;
          else if (actions.impulse == Right)
            this->player_x++;

        // check if the cell is edible - if so, eat it
        } else if (player_target_cell->is_edible()) {
          if (player_target_cell->type == YellowBombTrigger)
            for (int yy = 0; yy < this->h; yy++)
              for (int xx = 0; xx < this->w; xx++)
                if (this->at(xx, yy).type == YellowBomb)
                  this->pending_explosions.emplace_back(xx, yy);
          if (player_target_cell->type == Circuit)
            events_occurred |= CircuitEaten;

          *player_target_cell = this->at(this->player_x, this->player_y);
          if (this->player_will_drop_bomb) {
            this->player_will_drop_bomb = false;
            this->num_red_bombs--;
            this->at(this->player_x, this->player_y) = cell_state(RedBomb, 1);
            events_occurred |= RedBombDropped;
          } else {
            this->at(this->player_x, this->player_y) = cell_state(Empty);
          }

          if (actions.impulse == Up)
            this->player_y--;
          else if (actions.impulse == Down)
            this->player_y++;
          else if (actions.impulse == Left)
            this->player_x--;
          else if (actions.impulse == Right)
            this->player_x++;
        }
      }
    }
    while (this->player_x < 0)
      this->player_x += this->w;
    while (this->player_y < 0)
      this->player_y += this->h;
    this->player_x %= this->w;
    this->player_y %= this->h;

  // if the player has lost, update the loss frame if not already set
  } else if (!this->player_lose_frame) {
    this->player_lose_frame = this->frames_executed;
    this->updates_per_second = 20.0f;
  }

  // finally, clear all the moved flags for the next frame
  for (int y = 0; y < this->h; y++)
    for (int x = 0; x < this->w; x++)
      this->at(x, y).moved = false;

  this->frames_executed++;

  return events_occurred;
}

void level_state::compute_player_coordinates() {
  for (int y = 0; y < this->h; y++) {
    for (int x = 0; x < this->w; x++) {
      if (this->at(x, y).type == Player) {
        this->player_x = x;
        this->player_y = y;
        return;
      }
    }
  }
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
    case 0x09: // portal right
    case 0x0D: // portal right special (TODO)
      return cell_state(RightPortal);
    case 0x0A: // portal down
    case 0x0E: // portal down special (TODO)
      return cell_state(DownPortal);
    case 0x0B: // portal left
    case 0x0F: // portal left special (TODO)
      return cell_state(LeftPortal);
    case 0x0C: // portal up
    case 0x10: // portal up special (TODO)
      return cell_state(UpPortal);
    case 0x15: // portal vertical
      return cell_state(VerticalPortal);
    case 0x16: // portal horizontal
      return cell_state(HorizontalPortal);
    case 0x17: // portal 4-way
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

  if (l.num_items_remaining == 0)
    l.num_items_remaining = l.count_cells_of_type(Item);

  return l;
}

vector<level_state> import_supaplex_levels(const char* filename) {
  FILE* f = fopen(filename, "rb");
  if (!f)
    throw runtime_error("file not found");
  fseek(f, 0, SEEK_END);
  uint64_t num = ftell(f) / sizeof(supaplex_level);
  fseek(f, 0, SEEK_SET);

  vector<level_state> all_levels;
  for (uint64_t x = 0; x < num; x++) {
    supaplex_level spl;
    fread_or_throw(f, &spl, sizeof(spl));
    all_levels.push_back(import_supaplex_level(spl));
  }
  fclose(f);

  return all_levels;
}



vector<level_state> load_levels(const char* filename) {
  FILE* f = fopen(filename, "rb");
  if (!f)
    throw runtime_error("file not found");

  uint64_t file_version;
  fread_or_throw(f, &file_version, sizeof(file_version));
  if (file_version != 0)
    throw runtime_error("unsupported file version");

  uint64_t num_levels;
  fread_or_throw(f, &num_levels, sizeof(num_levels));

  vector<level_state> all_levels(num_levels);
  for (uint64_t x = 0; x < num_levels; x++) {
    all_levels[x].read(f);
  }
  fclose(f);

  return all_levels;
}

void save_levels(const vector<level_state>& levels, const char* filename) {
  FILE* f = fopen(filename, "wb");
  if (!f)
    throw runtime_error("can\'t open file");

  uint64_t file_version = 0;
  uint64_t num_levels = levels.size();
  fwrite_or_throw(f, &file_version, sizeof(file_version));
  fwrite_or_throw(f, &num_levels, sizeof(num_levels));

  for (const level_state& l : levels)
    l.write(f);
  fclose(f);
}
