#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include <list>
#include <stdexcept>
#include <vector>

using namespace std;



enum player_impulse {
  None = 0,
  Up,
  Down,
  Left,
  Right,
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
  BlueBomb,
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
  GrayBomb,
  RockGenerator,
  Destroyer,
  Deleter,
  LeftJumpPortal,
  RightJumpPortal,
  UpJumpPortal,
  DownJumpPortal,
  HorizontalJumpPortal,
  VerticalJumpPortal,
  JumpPortal,
  PullStone,
};

enum explosion_type {
  NormalExplosion = 0,
  ItemExplosion,
  RockExplosion,
};

enum events_mask {
  NoEvents         = 0x0000,
  ObjectFalling    = 0x0001,
  ObjectLanded     = 0x0002,
  ItemCollected    = 0x0004,
  CircuitEaten     = 0x0008,
  RedBombCollected = 0x0010,
  RedBombDropped   = 0x0020,
  ObjectPushed     = 0x0040,
  Exploded         = 0x0080,
  ItemExploded     = 0x0100,
  PlayerWon        = 0x0200,
};

struct cell_state {
  cell_type type;
  int32_t param;
  bool moved;

  void read(FILE*);
  void write(FILE*) const;

  cell_state();
  cell_state(cell_type type, int param = 0, bool moved = false);

  bool is_round() const;
  bool should_fall() const;
  bool destroyable() const;
  bool is_bomb() const;
  bool is_volatile() const;
  bool is_edible() const;
  bool is_pushable_horizontal() const;
  bool is_pushable_vertical() const;
  bool is_pullable() const;
  bool is_dude() const;
  explosion_type get_explosion_type() const;
  bool is_left_portal() const;
  bool is_right_portal() const;
  bool is_up_portal() const;
  bool is_down_portal() const;
  bool is_jump_portal() const;
};

struct explosion_info {
  int32_t x;
  int32_t y;
  int32_t size;
  int32_t frames; // how many more frames before it occurs
  explosion_type type;

  explosion_info(int x, int y, int size = 1, int frames = 0,
      explosion_type type = NormalExplosion);

  void read(FILE*);
  void write(FILE*) const;
};

struct level_state {
  uint32_t w;
  uint32_t h;
  int32_t player_x;
  int32_t player_y;
  int32_t num_items_remaining;
  int32_t num_red_bombs;
  uint64_t frames_executed;
  uint64_t player_lose_frame;
  double player_lose_buffer;
  vector<cell_state> cells;
  list<explosion_info> pending_explosions;

  float updates_per_second;
  bool player_will_drop_bomb;
  bool player_did_win;

  level_state(uint32_t w = 60, uint32_t h = 24, int32_t player_x = 1,
      int32_t player_y = 1);

  void read(FILE*);
  void write(FILE*) const;

  cell_state& at(int x, int y);
  const cell_state& at(int x, int y) const;
  void move_cell(int x, int y, player_impulse dir);
  bool player_is_alive() const;
  double player_is_losing() const;
  bool validate() const;
  int count_items() const;
  int count_cells_of_type(cell_type c) const;
  int count_attenuated_space() const;
  int compute_entropy() const;
  void compute_player_coordinates();

  void player_drop_bomb();
  uint64_t exec_frame(enum player_impulse impulse);
};

vector<level_state> import_supaplex_levels(const char* filename);
vector<level_state> load_levels(const char* filename);
void save_levels(const vector<level_state>& levels, const char* filename);
