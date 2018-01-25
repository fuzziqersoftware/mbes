#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include <deque>
#include <list>
#include <stdexcept>
#include <utility>
#include <vector>



enum player_impulse {
  None = 0,
  Up = 1,
  Down = 2,
  Left = 3,
  Right = 4,
};

struct player_actions {
  enum player_impulse impulse;
  bool drop_bomb;
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
  WhiteBomb,
};

enum explosion_type {
  NormalExplosion = 0,
  ItemExplosion,
  RockExplosion,
  BlockExplosion,
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
  cell_state(cell_type type, int32_t param = 0, bool moved = false);

  bool is_round() const;
  bool should_fall() const;
  bool destroyable() const;
  bool is_bomb() const;
  bool is_volatile() const;
  bool is_edible() const;
  bool is_pushable(player_impulse dir) const;
  bool is_pullable() const;
  bool is_dude() const;
  explosion_type get_explosion_type() const;
  bool is_portal(player_impulse dir) const;
  bool is_jump_portal() const;
};

struct explosion_info {
  uint64_t frame; // explosion occurs on this frame
  int32_t x;
  int32_t y;
  int32_t size;
  explosion_type type;

  explosion_info(uint64_t frame, int32_t x, int32_t y, int32_t size = 1,
      explosion_type type = NormalExplosion);

  bool operator==(const explosion_info& other) const;

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
  std::vector<cell_state> cells;
  std::list<explosion_info> pending_explosions;

  float updates_per_second;
  bool player_will_drop_bomb;
  bool player_did_win;

  struct undo_log_entry {
    enum class entry_type {
      FrameMarker = 0,
      Cell,
      CreateExplosion,
      ExecuteExplosion,
      GetItem,
      GetRedBomb,
      DropRedBomb,
    };
    entry_type type;

    union {
      struct {
        int32_t x;
        int32_t y;
        cell_state old_state;
      } cell;

      uint64_t frame;

      explosion_info explosion;
    };

    undo_log_entry(entry_type type);
    undo_log_entry(uint64_t frame);
    undo_log_entry(int32_t x, int32_t y, const cell_state& old_state);
    undo_log_entry(entry_type type, const explosion_info& explosion);
  };
  std::deque<undo_log_entry> undo_log;

  level_state(uint32_t w = 60, uint32_t h = 24, int32_t player_x = 1,
      int32_t player_y = 1);

  void read(FILE*);
  void write(FILE*) const;

  cell_state& at(int32_t x, int32_t y);
  cell_state& at(const std::pair<int32_t, int32_t>& pos);
  const cell_state& at(int32_t x, int32_t y) const;
  const cell_state& at(const std::pair<int32_t, int32_t>& pos) const;

  void write_cell_to_undo_log(int32_t x, int32_t y);
  void write_cell_to_undo_log(const std::pair<int32_t, int32_t>& pos);

  void create_explosion(uint64_t frame, int32_t x, int32_t y, int32_t size = 1,
      explosion_type type = NormalExplosion);

  bool player_is_alive() const;
  double player_is_losing() const;
  bool validate() const;
  size_t count_items() const;
  size_t count_cells_of_type(cell_type c) const;
  size_t count_attenuated_space() const;
  size_t compute_entropy() const;
  void compute_player_coordinates();

  uint64_t exec_frame(const struct player_actions& actions);
  void rewind_frames(size_t count);
  void rewind_frames_until(uint64_t target_frame);
};

std::vector<level_state> load_levels(const char* filename);
void save_levels(const std::vector<level_state>& levels, const char* filename);
