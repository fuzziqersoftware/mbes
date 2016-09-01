#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef MACOSX
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <GLFW/glfw3.h>

#include <deque>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>

#include "audio.hh"
#include "gl_text.hh"
#include "level.hh"
#include "level_completion.hh"
#include "util.hh"

using namespace std;



struct editor_cell_definition {
  cell_state st;
  const char* name;
  const char* description;

  editor_cell_definition(cell_type ty, int param, const char* name,
      const char* description) : st(ty, param), name(name),
      description(description) { }
};

const vector<vector<editor_cell_definition>> editor_available_cells({
  {
    {Empty, 1,                "empty", "empty space. attenuates if left alone."},
    {Circuit, 0,              "circuit", "solid for everything except the player. does not fall. can be destroyed by explosions."},
    {Rock, 0,                 "rock", "obstacle. falls and rolls. objects roll off of it. can be destroyed by explosions."},
    {Exit, 0,                 "exit", "goal when all necessary items are collected. objects roll off of it. can be destroyed by explosions."},
    {Player, 0,               "player", "hey look! it\'s me!"},
    {Item, 0,                 "item", "the player needs some number of these to complete the level."},
    {Block, 0,                "block", "obstacle. does not fall. objects do not roll off of it. cannot be destroyed."},
    {RoundBlock, 0,           "round block", "obstacle. does not fall. objects roll off of it. can be destroyed by explosions."},
    {RedBomb, 0,              "red bomb", "the player can pick these up and drop them somewhere else. does not fall. explodes when something falls on it."},
    {YellowBomb, 0,           "yellow bomb", "the player can push these around and blow them up with the trigger. does not fall. explodes when something falls on it."},
    {GreenBomb, 0,            "green bomb", "the player can push these around and drop them on things. explodes when something falls on it or when it lands after falling."},
    {BlueBomb, 0,             "blue bomb", "behaves like a green bomb, but explodes into items."},
    {GrayBomb, 0,             "gray bomb", "behaves like a green bomb, but explodes into rocks."},
    {YellowBombTrigger, 0,    "yellow trigger", "explodes all yellow bombs at once. otherwise, behaves like a circuit."},
    {BombDude, Left,          "bomb dude", "automaton. follows the wall to its left. explodes when something falls on it."},
    {ItemDude, Left,          "item dude", "behaves like a bomb dude, but explodes into items."},
    {RockGenerator, 0,        "rock generator", "creates a rock every 16 frames if there\'s empty space beneath it. explodes when something falls on it."},
    {LeftPortal, 0,           "left portal", "the player can move through this only to the left. can be destroyed by explosions."},
    {RightPortal, 0,          "right portal", "the player can move through this only to the right. can be destroyed by explosions."},
    {UpPortal, 0,             "up portal", "the player can move through this only going up. can be destroyed by explosions."},
    {DownPortal, 0,           "down portal", "the player can move through this only going down. can be destroyed by explosions."},
    {HorizontalPortal, 0,     "horizontal portal", "the player can move through this only horizontally. can be destroyed by explosions."},
    {VerticalPortal, 0,       "vertical portal", "the player can move through this only vertically. can be destroyed by explosions."},
    {Portal, 0,               "portal", "the player can move through this in any direction. can be destroyed by explosions."},
    {Destroyer, 0,            "destroyer", "makes anything above it explode."},
    {Deleter, 0,              "deleter", "makes anything above it disappear."},
  }, {
    {LeftJumpPortal, 0,       "left jump portal", "the player can move through this only to the left. can be destroyed by explosions."},
    {RightJumpPortal, 0,      "right jump portal", "the player can move through this only to the right. can be destroyed by explosions."},
    {UpJumpPortal, 0,         "up jump portal", "the player can move through this only going up. can be destroyed by explosions."},
    {DownJumpPortal, 0,       "down jump portal", "the player can move through this only going down. can be destroyed by explosions."},
    {HorizontalJumpPortal, 0, "horizontal jump portal", "the player can move through this only horizontally. can be destroyed by explosions."},
    {VerticalJumpPortal, 0,   "vertical jump portal", "the player can move through this only vertically. can be destroyed by explosions."},
    {JumpPortal, 0,           "jump portal", "the player can move through this in any direction. can be destroyed by explosions."},
  }
});

static void editor_write_cell(level_state& l, uint32_t x, uint32_t y,
    const cell_state& cell) {
  l.at(x, y) = cell;
}



static const char* plural(uint64_t i) {
  return (i == 1) ? "" : "s";
}



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
    int l_h, float alpha = 1.0f) {

  float x1 = to_window(x, l_w);
  float x2 = to_window(x + 1, l_w);
  float y1 = to_window(y, l_h);
  float y2 = to_window(y + 1, l_h);

  if (cell.type == Destroyer) {
    glColor4f(1.0, 0.5, 0.0, alpha);
    glVertex3f(x1, -y1, 1);
    glVertex3f(x2, -y1, 1);
    glColor4f(0.5, 0.0, 0.0, alpha);
    glVertex3f(x2, -y2, 1);
    glVertex3f(x1, -y2, 1);
    return;
  }

  if (cell.type == Deleter) {
    glColor4f(1.0, 0.0, 0.0, alpha);
    glVertex3f(x1, -y1, 1);
    glVertex3f(x2, -y1, 1);
    glColor4f(1.0, 1.0, 1.0, alpha);
    glVertex3f(x2, -y2, 1);
    glVertex3f(x1, -y2, 1);
    return;
  }

  bool draw_center = false;
  float center_r = 1.0, center_g = 1.0, center_b = 1.0;
  switch (cell.type) {
    case Empty:
      glColor4f(0.0, 0.0, (float)cell.param / 256, alpha);
      break;
    case Circuit:
      glColor4f(0.0, 0.8, 0.0, alpha);
      break;
    case Rock:
      glColor4f(0.6, 0.6, 0.6, alpha);
      break;
    case Exit:
      glColor4f(0.5, 1.0, 0.0, alpha);
      break;
    case Player:
      glColor4f(1.0, 0.0, 0.0, alpha);
      break;
    case Item:
      glColor4f(0.8, 0.0, 0.8, alpha);
      break;
    case Block:
      glColor4f(1.0, 1.0, 1.0, alpha);
      break;
    case RoundBlock:
      glColor4f(0.9, 0.9, 1.0, alpha);
      break;
    case BlueBomb:
      glColor4f(0.0, 0.5, 1.0, alpha);
      draw_center = true;
      break;
    case GreenBomb:
      glColor4f(0.0, 1.0, 0.0, alpha);
      draw_center = true;
      break;
    case YellowBomb:
      glColor4f(0.8, 0.8, 0.0, alpha);
      draw_center = true;
      break;
    case RedBomb:
      glColor4f(cell.param ? ((float)cell.param / 256) : 1.0, 0.0, 0.0, alpha);
      draw_center = true;
      break;
    case GrayBomb:
      glColor4f(0.6, 0.6, 0.6, alpha);
      draw_center = true;
      break;
    case RockGenerator:
      glColor4f(0.6, 0.6, 0.6, alpha);
      draw_center = true;
      center_r = 0.0;
      center_g = 0.0;
      center_b = 0.0;
      break;
    case YellowBombTrigger:
      glColor4f(0.8, 0.8, 0.0, alpha);
      break;
    case Explosion:
      glColor4f((float)cell.param / 256, (float)cell.param / 512, 0.0, alpha);
      break;
    case ItemDude:
      glColor4f(0.0, 0.5, 1.0, alpha);
      break;
    case BombDude:
      glColor4f(1.0, 0.5, 0.0, alpha);
      break;
    case LeftPortal:
    case RightPortal:
    case UpPortal:
    case DownPortal:
    case HorizontalPortal:
    case VerticalPortal:
    case Portal:
      glColor4f(0.7, 0.0, 0.0, alpha);
      break;
    case LeftJumpPortal:
    case RightJumpPortal:
    case UpJumpPortal:
    case DownJumpPortal:
    case HorizontalJumpPortal:
    case VerticalJumpPortal:
    case JumpPortal:
      glColor4f(0.0, 0.7, 0.7, alpha);
      break;
    case Destroyer:
    case Deleter:
      // unreachable because we checked for these above, but this silences a
      // compiler warning
      break;
  }

  glVertex3f(x1, -y1, 1);
  glVertex3f(x2, -y1, 1);
  glVertex3f(x2, -y2, 1);
  glVertex3f(x1, -y2, 1);

  if (draw_center) {
    glColor4f(center_r, center_g, center_b, alpha);
    x1 = to_window(4 * x + 1, 4 * l_w);
    x2 = to_window(4 * x + 3, 4 * l_w);
    y1 = to_window(4 * y + 1, 4 * l_h);
    y2 = to_window(4 * y + 3, 4 * l_h);
    glVertex3f(x1, -y1, 1);
    glVertex3f(x2, -y1, 1);
    glVertex3f(x2, -y2, 1);
    glVertex3f(x1, -y2, 1);
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

static void render_cell(const cell_state& cell, int x, int y, int l_w, int l_h,
    float alpha = 1.0) {
  glBegin(GL_QUADS);
  render_cell_quads(cell, x, y, l_w, l_h, alpha);
  glEnd();

  glBegin(GL_TRIANGLES);
  glColor4f(1.0, 1.0, 1.0, alpha);
  render_cell_tris(cell, x, y, l_w, l_h);
  glEnd();
}

static void render_items_remaining(int items_remaining, int window_w, int window_h) {
  if (items_remaining > 0)
    draw_text(-0.99, -0.9, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
        "%d item%s remaining", items_remaining, plural(items_remaining));
  else if (items_remaining < 0)
    draw_text(-0.99, -0.9, 0, 1, 0.5, 1, (float)window_w / window_h, 0.01, false,
        "%d extra item%s", -items_remaining, plural(-items_remaining));
}

static void render_level_state(const level_state& l, int window_w, int window_h,
    bool show_stats) {
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

  if (show_stats) {
    uint64_t empty_cells = l.count_cells_of_type(Empty);
    uint64_t attenuated_space = l.count_attenuated_space();
    uint64_t entropy = l.compute_entropy();
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
    draw_text(-0.99, -0.4, 1, 1, 1, 1, (float)window_w / window_h, 0.01, false,
        "%d frame%s", l.frames_executed, plural(l.frames_executed));
    draw_text(-0.99, -0.5, 1, 1, 1, 1, (float)window_w / window_h, 0.01, false,
        "%d empty cell%s", empty_cells, plural(empty_cells));
    draw_text(-0.99, -0.6, 1, 1, 1, 1, (float)window_w / window_h, 0.01, false,
        "%d attenuated cell%s", attenuated_space, plural(attenuated_space));
    draw_text(-0.99, -0.7, 1, 1, 1, 1, (float)window_w / window_h, 0.01, false,
        "%d bit%s of entropy", entropy, plural(entropy));
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  render_items_remaining(l.num_items_remaining, window_w, window_h);

  if (l.num_red_bombs > 0)
    draw_text(-0.99, -0.8, 0, 1, 0.5, 1, (float)window_w / window_h, 0.01, false,
        "%d red bomb%s", l.num_red_bombs, plural(l.num_red_bombs));
  else if (l.num_red_bombs < 0)
    draw_text(-0.99, -0.8, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
        "%d red bomb%s in debt", -l.num_red_bombs, plural(-l.num_red_bombs));
}



static void render_key_commands(float aspect_ratio, bool should_play_sounds,
    bool show_stats) {
  draw_text(0, -0.4, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "shift+i: how to play");
  draw_text(0, -0.5, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "shift+s: %smute sound", should_play_sounds ? "" : "un");
  draw_text(0, -0.6, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "shift+arrow keys: change level");
  draw_text(0, -0.7, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "x: %s stats", show_stats ? "hide" : "show");
  draw_text(0, -0.8, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "esc: restart level / exit");
}

static void render_level_stats(const level_completion& lc, float aspect_ratio) {
  draw_text(0, -0.2, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "Best time: %llu", lc.frames);
  draw_text(0, -0.3, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "Most extra items: %llu", lc.extra_items);
  draw_text(0, -0.4, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "Most extra bombs: %llu", lc.extra_bombs);
  draw_text(0, -0.5, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "Most cleared cells: %llu", lc.cleared_space);
  draw_text(0, -0.6, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "Fewest attenuated cells: %llu", lc.attenuated_space);
  draw_text(0, -0.7, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "Least entropy: %llu", lc.entropy);
}

static void render_paused_screen(int window_w, int window_h,
    const vector<level_completion>& completion, int level_index,
    bool player_did_win, bool player_did_lose, uint64_t frames_executed,
    bool should_play_sounds, bool show_stats) {

  size_t num_completed = 0;
  for (const auto& it : completion)
    if (it.state == Completed)
      num_completed++;

  render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f,
      0.0f, 0.0f, 0.1f);

  float aspect_ratio = (float)window_w / window_h;
  draw_text(0, 0.9, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
      "Fuzziqer Software");
  draw_text(0, 0.7, 1, 1, 1, 1, (float)window_w / window_h, 0.03, true,
      "MOVE BLOCKS AND EAT STUFF");

  if (player_did_win) {
    draw_text(0, 0.3, 1, 1, 1, 1, aspect_ratio, 0.02, true,
        "You win!");
    draw_text(0, 0.1, 1, 1, 1, 1, aspect_ratio, 0.01, true,
        "You have completed all %llu levels!", completion.size());
    // TODO render overall stats here

  } else {
    if (player_did_lose)
      draw_text(0, 0.3, 1, 0, 0, 1, aspect_ratio, 0.02, true,
          "Press enter to try level %d again", level_index);
    else if (frames_executed)
      draw_text(0, 0.3, 1, 1, 1, 1, aspect_ratio, 0.02, true,
          "Press enter to continue level %d", level_index);
    else
      draw_text(0, 0.3, 1, 1, 1, 1, aspect_ratio, 0.02, true,
          "Press enter to begin level %d", level_index);

    if (num_completed == completion.size()) {
      draw_text(0, 0.1, 0.5, 1, 0.5, 1, aspect_ratio, 0.01, true,
          "You have completed all %llu levels!", completion.size());
      render_level_stats(completion[level_index], aspect_ratio);

    } else if (completion[level_index].state == Completed) {
      draw_text(0, 0.1, 0.5, 1, 0.5, 1, aspect_ratio, 0.01, true,
          "You have already completed this level (%lu/%lu)", num_completed, completion.size());
      render_level_stats(completion[level_index], aspect_ratio);

    } else {
      draw_text(0, 0.1, 1, 0.5, 0.5, 1, aspect_ratio, 0.01, true,
          "You haven\'t completed this level yet (%lu/%lu)", num_completed, completion.size());
      render_key_commands(aspect_ratio, should_play_sounds, show_stats);
    }
  }
}

static void render_instructions_page(int window_w, int window_h, int page_num) {

  float aspect_ratio = (float)window_w / window_h;

  draw_text(0, 0.9, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "How to play (page %d/3)",
      page_num + 1);
  draw_text(0, 0.8, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "left/right arrow keys: change page / esc: exit to main menu");

  if (page_num == 0) {
    draw_text(0,  0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "You are the red box. Lucky you!");
    draw_text(0,  0.5,   1,   1,   1, 1, aspect_ratio, 0.01, true, "Your goal in each level is to collect the items (purple), then go to the exit (light green).");
    draw_text(0,  0.4,   1,   1,   1, 1, aspect_ratio, 0.01, true, "On some levels there may be extra items that you don\'t need to collect.");

    draw_text(0,  0.2,   1,   1,   1, 1, aspect_ratio, 0.01, true, "Watch out for falling items, rocks and bombs.");
    draw_text(0,  0.1,   1,   1,   1, 1, aspect_ratio, 0.01, true, "If anything falls on you or explodes near you, you lose!");
    draw_text(0,  0.0,   1,   1,   1, 1, aspect_ratio, 0.01, true, "You can push rocks and bombs out of the way if there\'s empty space on the other side.");
    draw_text(0, -0.1,   1,   1,   1, 1, aspect_ratio, 0.01, true, "If stacked, rocks and items will roll off of each other to form a neat pile.");

    draw_text(0, -0.3,   1,   1,   1, 1, aspect_ratio, 0.01, true, "Keys when playing:");
    draw_text(0, -0.4,   1,   1,   1, 1, aspect_ratio, 0.01, true, "arrow keys: move around, push objects");
    draw_text(0, -0.5,   1,   1,   1, 1, aspect_ratio, 0.01, true, "space: drop bomb (appears when you move away)");
    draw_text(0, -0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "tab: toggle speed (slow/fast)");
    draw_text(0, -0.7,   1,   1,   1, 1, aspect_ratio, 0.01, true, "enter: pause");
    draw_text(0, -0.8,   1,   1,   1, 1, aspect_ratio, 0.01, true, "esc: restart level / exit");

  } else if (page_num == 1) {
    draw_text(    0,  0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "Some objects you might encounter:");
    draw_text(-0.95,  0.5,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Rock. Gets in the way. Falls if there\'s nothing under it. You can push it around, or blow it up to get rid of it.");
    draw_text(-0.95,  0.5, 0.5, 0.5, 0.5, 1, aspect_ratio, 0.01, false, "\x80");
    draw_text(-0.95,  0.4,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Item. Your goal is to collect these. Careful - they fall and can be destroyed by explosions too.");
    draw_text(-0.95,  0.4, 0.8,   0, 0.8, 1, aspect_ratio, 0.01, false, "\x80");
    draw_text(-0.95,  0.3,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Circuit. It\'s solid for everything except you - eat this to get it out of the way.");
    draw_text(-0.95,  0.3,   0, 0.8,   0, 1, aspect_ratio, 0.01, false, "\x80");
    draw_text(-0.95,  0.2,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Block. It can\'t be moved or destroyed by anything.");
    draw_text(-0.95,  0.1,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Round block. Rocks and items will roll off of it. It can\'t be moved, but it can be destroyed by explosions.");
    draw_text(-0.95,  0.1, 0.9, 0.9,   1, 1, aspect_ratio, 0.01, false, "\x80");
    draw_text(-0.95,  0.0,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Red bomb. You can pick this up and drop it somewhere else to blow up objects in your way.");
    draw_text(-0.95,  0.0,   1,   0,   0, 1, aspect_ratio, 0.01, false, "\x81");
    draw_text(-0.95,  0.0,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x82");
    draw_text(-0.95, -0.1,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Yellow bomb. Push this around, then touch the yellow trigger to blow it up.");
    draw_text(-0.95, -0.1, 0.8, 0.8,   0, 1, aspect_ratio, 0.01, false, "\x81");
    draw_text(-0.95, -0.1,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x82");
    draw_text(-0.95, -0.2,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Yellow trigger. Blows up all yellow bombs at once.");
    draw_text(-0.95, -0.2, 0.8, 0.8,   0, 1, aspect_ratio, 0.01, false, "\x80");
    draw_text(-0.95, -0.3,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Green bomb. Explodes when something falls on it, or when it lands after falling.");
    draw_text(-0.95, -0.3,   0,   1,   0, 1, aspect_ratio, 0.01, false, "\x81");
    draw_text(-0.95, -0.3,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x82");
    draw_text(-0.95, -0.4,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Bomb dude. It moves by itself! Not harmful, but explodes if something falls on it.");
    draw_text(-0.95, -0.4,   1, 0.5,   0, 1, aspect_ratio, 0.01, false, "\x80");
    draw_text(-0.95, -0.5,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x80 Item dude. Like a bomb dude, but creates nine items instead of exploding.");
    draw_text(-0.95, -0.5,   0, 0.5,   1, 1, aspect_ratio, 0.01, false, "\x80");
    draw_text(-0.95, -0.6, 0.7,   0,   0, 1, aspect_ratio, 0.01, false, "\x80");
    draw_text(-0.95, -0.6,   1,   1,   1, 1, aspect_ratio, 0.01, false, "\x83 Portal. You can move through this in the direction of its arrow(s) if there\'s empty space on the other side.");
    draw_text(0, -0.8,   1,   1,   1, 1, aspect_ratio, 0.01, true, "There may be other types of objects not mentioned here. Strange things await...");

  } else if (page_num == 2) {
    draw_text(0,  0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "A few tips:");
    draw_text(0,  0.4,   1,   1,   1, 1, aspect_ratio, 0.01, true, "Some levels require you to move quickly in different directions. Try slow mode (press tab)");
    draw_text(0,  0.3,   1,   1,   1, 1, aspect_ratio, 0.01, true, "if it\'s too difficult to maneuver quickly enough.");
    draw_text(0,  0.1,   1,   1,   1, 1, aspect_ratio, 0.01, true, "Dudes don\'t hurt you, but they do tend to get in the way.");
    draw_text(0,  0.0,   1,   1,   1, 1, aspect_ratio, 0.01, true, "Be careful in confined spaces with dudes.");
    draw_text(0, -0.2,   1,   1,   1, 1, aspect_ratio, 0.01, true, "You can use red bombs even if you haven\'t picked any up, but if you\'re in debt (have used");
    draw_text(0, -0.3,   1,   1,   1, 1, aspect_ratio, 0.01, true, "more red bombs than you\'ve collected), then you can\'t finish the level.");
    draw_text(0, -0.5,   1,   1,   1, 1, aspect_ratio, 0.01, true, "After using a red bomb, you can pick it up again before it explodes. For example, you can");
    draw_text(0, -0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "use a red bomb to temporarily prevent objects from falling or rolling.");
    draw_text(0, -0.8,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 Blocks can\'t be destroyed by anything. If you need all the items from an item dude,");
    draw_text(0, -0.9,   1,   1,   1, 1, aspect_ratio, 0.01, true, "don\'t blow it up near a \x80 block - you\'ll lose an item that you need.");

  } else {
    draw_text(0, 0.7, 1, 0, 0, 1, aspect_ratio, 0.01, true,
        "INVALID PAGE NUMBER: %d", page_num);
  }
}



static void render_palette(const editor_cell_definition* selected_def, int l_w, int l_h, float alpha,
    bool can_save, int num_items_remaining) {

  for (size_t y = 0; y < editor_available_cells.size(); y++) {
    glBegin(GL_QUADS);
    for (size_t x = 0; x < editor_available_cells[y].size(); x++) {
      if (selected_def && (editor_available_cells[y][x].st.type == selected_def->st.type))
        glColor4f(1.0, 1.0, 1.0, alpha);
      else
        glColor4f(0.5, 0.5, 0.5, alpha * 0.5);
      glVertex3f(to_window(7 + 4 * x, 2 * l_w), -to_window(3 + 4 * y, 2 * l_h), 1);
      glVertex3f(to_window(3 + 4 * x, 2 * l_w), -to_window(3 + 4 * y, 2 * l_h), 1);
      glVertex3f(to_window(3 + 4 * x, 2 * l_w), -to_window(7 + 4 * y, 2 * l_h), 1);
      glVertex3f(to_window(7 + 4 * x, 2 * l_w), -to_window(7 + 4 * y, 2 * l_h), 1);
      render_cell_quads(editor_available_cells[y][x].st, 2 * x + 2, 2 * y + 2, l_w, l_h, alpha);
    }
    glEnd();

    glBegin(GL_TRIANGLES);
    glColor4f(1.0, 1.0, 1.0, alpha);
    for (size_t x = 0; x < editor_available_cells[y].size(); x++)
      render_cell_tris(editor_available_cells[y][x].st, 2 * x + 2, 2 * y + 2, l_w, l_h);
    glEnd();
  }

  if (selected_def) {
    draw_text(0, 0.5, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
        selected_def->name);
    draw_text(0, 0.4, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
        selected_def->description);
  }

  draw_text(0.0, 0.1, 1, 1, 1, alpha, (float)l_w / l_h, 0.02, true,
      "MOVE BLOCKS AND DESIGN LEVELS");

  draw_text(0, -0.1, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
      "space: open/close palette");
  draw_text(0, -0.2, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
      "left-click: draw cell (click+drag to draw multiple)");
  draw_text(0, -0.3, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
      "shift+up/down: change required item count");
  draw_text(0, -0.4, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
      "i: automatically compute required item count");
  if (can_save) {
    draw_text(0, -0.5, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
        "esc: exit editor; don't save level");
    draw_text(0, -0.6, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
        "enter: exit editor and save level");
  } else {
    draw_text(0, -0.5, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
        "esc/enter: exit editor and continue playing");
    draw_text(0, -0.6, 1, 0.5, 0.5, alpha, (float)l_w / l_h, 0.01, true,
        "editing current level state only; changes will be lost on restart");
  }

  render_items_remaining(num_items_remaining, l_w, l_h);
}



enum game_phase {
  Playing = 0,
  Paused,
  Instructions,
  Editing,
};

string levels_filename = "";
string default_levels_filename = "";
vector<level_state> initial_state;
level_state game;
game_phase phase = Paused;
bool show_stats = false;
bool player_did_lose = false;
bool should_reload_state = false;
bool should_play_sounds = true;
deque<enum player_impulse> recent_impulses;
enum player_impulse current_impulse = None;
int level_index = -1;
int should_change_to_level = -1;
int current_instructions_page = 0;

int mouse_x, mouse_y;
int editor_palette_intensity = 0;
int editor_highlight_x = 0, editor_highlight_y = 0;
const editor_cell_definition* editor_selected_cell_type = NULL;
bool editor_drawing = false;



static void glfw_key_cb(GLFWwindow* window, int key, int scancode,
    int action, int mods) {

  if (((action == GLFW_PRESS) || (action == GLFW_REPEAT))) {
    if (phase == Paused) {
      if ((key == GLFW_KEY_LEFT) && (mods & GLFW_MOD_SHIFT)) {
        should_change_to_level = (level_index == 0) ? (initial_state.size() - 1) : (level_index - 1);
        return;
      } else if ((key == GLFW_KEY_RIGHT) && (mods & GLFW_MOD_SHIFT)) {
        should_change_to_level = (level_index == initial_state.size() - 1) ? 0 : (level_index + 1);
        return;
      } else if ((key == GLFW_KEY_UP) && (mods & GLFW_MOD_SHIFT)) {
        should_change_to_level = (level_index < 10) ? 0 : (level_index - 10);
        return;
      } else if ((key == GLFW_KEY_DOWN) && (mods & GLFW_MOD_SHIFT)) {
        should_change_to_level = (level_index >= initial_state.size() - 10) ? (initial_state.size() - 1) : (level_index + 10);
        return;
      }
    } else if (phase == Instructions) {
      if (key == GLFW_KEY_LEFT) {
        if (current_instructions_page > 0)
          current_instructions_page--;
        return;
      } else if (key == GLFW_KEY_RIGHT) {
        if (current_instructions_page < 2)
          current_instructions_page++;
        return;
      }
    } else if (phase == Editing) {
      if ((key == GLFW_KEY_UP) && (mods & GLFW_MOD_SHIFT)) {
        game.num_items_remaining++;
        return;
      } else if ((key == GLFW_KEY_DOWN) && (mods & GLFW_MOD_SHIFT)) {
        game.num_items_remaining--;
        return;
      }
    }
  }

  if (action == GLFW_PRESS) {
    if ((key == GLFW_KEY_D) && (mods & GLFW_MOD_SHIFT)) {
      phase = Editing;
      editor_palette_intensity = 1024;

    } else if ((key == GLFW_KEY_I) && (mods & GLFW_MOD_SHIFT)) {
      phase = Instructions;
      current_instructions_page = 0;

    } else if ((key == GLFW_KEY_S) && (mods & GLFW_MOD_SHIFT)) {
      should_play_sounds = !should_play_sounds;

    } else if ((key == GLFW_KEY_I) && (phase == Editing))
      game.num_items_remaining = game.count_items();

    else if (key == GLFW_KEY_ESCAPE) {
      if (phase == Editing) {
        game.compute_player_coordinates();
        phase = Paused;
      } else if (phase == Instructions) {
        phase = Paused;
      } else if ((phase == Playing) || game.frames_executed)
        should_change_to_level = level_index;
      else
        glfwSetWindowShouldClose(window, 1);

    } else if (key == GLFW_KEY_ENTER) {
      if (phase == Editing) {
        game.compute_player_coordinates();
        if (!game.frames_executed) {
          initial_state[level_index] = game;
          // TODO: clear completion state for this level
          save_levels(initial_state, levels_filename.c_str());
        }
        phase = Paused;
      } else if (phase == Playing) {
        phase = Paused;
      } else {
        phase = Playing;
      }
      player_did_lose = false;

    } else if (key == GLFW_KEY_TAB) {
      if (mods & GLFW_MOD_SHIFT)
        game.updates_per_second = 200.0f;
      else if (game.updates_per_second == 20.0f)
        game.updates_per_second = 2.0f;
      else
        game.updates_per_second = 20.0f;

    } else if ((key == GLFW_KEY_LEFT) && ((phase == Playing) || (phase == Paused))) {
      current_impulse = Left;
      recent_impulses.emplace_back(Left);
      phase = Playing;
      player_did_lose = false;
    } else if ((key == GLFW_KEY_RIGHT) && ((phase == Playing) || (phase == Paused))) {
      current_impulse = Right;
      recent_impulses.emplace_back(Right);
      phase = Playing;
      player_did_lose = false;
    } else if ((key == GLFW_KEY_UP) && ((phase == Playing) || (phase == Paused))) {
      current_impulse = Up;
      recent_impulses.emplace_back(Up);
      phase = Playing;
      player_did_lose = false;
    } else if ((key == GLFW_KEY_DOWN) && ((phase == Playing) || (phase == Paused))) {
      current_impulse = Down;
      recent_impulses.emplace_back(Down);
      phase = Playing;
      player_did_lose = false;
    } else if ((key == GLFW_KEY_X) && ((phase == Playing) || (phase == Paused))) {
      show_stats = !show_stats;
    } else if ((key == GLFW_KEY_SPACE) && (phase != Instructions)) {
      if (phase == Editing) {
        if (editor_palette_intensity)
          editor_palette_intensity = 0;
        else
          editor_palette_intensity = 512;
      } else {
        game.player_drop_bomb();
        phase = Playing;
        player_did_lose = false;
      }
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

static void glfw_mouse_button_cb(GLFWwindow* window, int button, int action,
    int mods) {
  // non-editing phases only use the keyboard
  if (phase != Editing)
    return;

  if (action == GLFW_RELEASE) {
    editor_drawing = false;
    return;
  }

  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (editor_palette_intensity)
      editor_palette_intensity = 0;
    else
      editor_palette_intensity = 512;
  }

  if (!editor_palette_intensity && editor_selected_cell_type) {
    editor_drawing = true;
    editor_write_cell(game, editor_highlight_x, editor_highlight_y,
        editor_selected_cell_type->st);
  } else if (button == GLFW_MOUSE_BUTTON_LEFT)
    editor_palette_intensity = 0;
}

static void glfw_mouse_move_cb(GLFWwindow* window, double x, double y) {
  mouse_x = x;
  mouse_y = y;

  int window_w, window_h;
  glfwGetWindowSize(window, &window_w, &window_h);
  int cell_x = (mouse_x * game.w) / window_w;
  int cell_y = (mouse_y * game.h) / window_h;

  if ((cell_x < 0) || (cell_x > game.w) || (cell_y < 0) || (cell_y > game.h))
    return;

  editor_highlight_x = cell_x;
  editor_highlight_y = cell_y;

  if (editor_palette_intensity && (cell_x >= 1) && (cell_x <= 2 * editor_available_cells.size() + 1) &&
      (cell_y >= 1) && (cell_y <= 3))
    editor_palette_intensity = 512;

  if (editor_palette_intensity &&
      ((cell_y & 1) == 0) && (cell_y >= 2) &&
      (cell_y <= 2 * editor_available_cells.size() + 1) &&
      ((cell_x & 1) == 0) && (cell_x >= 2) &&
      (cell_x <= 2 * editor_available_cells[(cell_y - 2) / 2].size() + 1)) {
    editor_selected_cell_type = &editor_available_cells[(cell_y - 2) / 2][(cell_x - 2) / 2];

  } else if (editor_drawing && editor_selected_cell_type) {
    editor_write_cell(game, editor_highlight_x, editor_highlight_y,
        editor_selected_cell_type->st);
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

  if (!level_filename) {
#ifdef MACOSX
    CFURLRef app_url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFStringRef path = CFURLCopyFileSystemPath(app_url, kCFURLPOSIXPathStyle);
    const char *p = CFStringGetCStringPtr(path, CFStringGetSystemEncoding());

    // if we're in an app bundle, look in Resources/ for levels; else look in
    // the executable directory
    size_t p_len = strlen(p);
    if ((p_len >= 4) && !strcmp(p + p_len - 4, ".app")) {
      default_levels_filename = string(p) + "/Contents/Resources/levels.dat";
      levels_filename = string(p) + "/Contents/Resources/levels.mbl";
    } else {
      default_levels_filename = string(p) + "/levels.dat";
      levels_filename = string(p) + "/levels.mbl";
    }

    CFRelease(app_url);
    CFRelease(path);
#else
    // assume it's in the same working directory for now
    levels_filename = "levels.mbl";
#endif
  }

  try {
    initial_state = load_levels(levels_filename.c_str());
  } catch (const runtime_error& e) {
    fprintf(stderr, "can\'t load level index %s: %s\n", levels_filename.c_str(), e.what());
    try {
      initial_state = import_supaplex_levels(default_levels_filename.c_str());
    } catch (const runtime_error& e) {
      fprintf(stderr, "can\'t load level index %s: %s\n", default_levels_filename.c_str(), e.what());
      return 1;
    }
  }

  struct passwd *pw = getpwuid(getuid());
  string level_completion_filename = string(pw->pw_dir) + "/.mbes_progress";
  vector<level_completion> completion = load_level_completion_state(
      level_completion_filename);
  if (completion.empty()) {
    completion = load_level_completion_state_v1(
        string(pw->pw_dir) + "/.mbes_completion");
  }
  completion.resize(initial_state.size());

  if (level_index < 0) {
    // start at the first non-completed level
    for (level_index = 0; (completion[level_index].state == Completed) &&
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

  // auto-size the window based on the primary monitor size
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);
  int cell_size_w = (vidmode->width - 100) / game.w;
  int cell_size_h = (vidmode->height - 100) / game.h;
  int cell_size = (cell_size_w < cell_size_h) ? cell_size_w : cell_size_h;

  //glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
  GLFWwindow* window = glfwCreateWindow(game.w * cell_size, game.h * cell_size,
      "Move Blocks and Eat Stuff", NULL, NULL);
  if (!window) {
    glfwTerminate();
    fprintf(stderr, "failed to create window\n");
    return 2;
  }

  glfwSetFramebufferSizeCallback(window, glfw_resize_cb);
  glfwSetKeyCallback(window, glfw_key_cb);
  glfwSetMouseButtonCallback(window, glfw_mouse_button_cb);
  glfwSetCursorPosCallback(window, glfw_mouse_move_cb);
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
          "level is corrupt");
      draw_text(0, 0.0, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
          "esc: exit");

    } else if (phase == Instructions) {
      render_level_state(game, window_w, window_h, show_stats);
      render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f, 0.8f,
          0.0f, 0.0f, 0.0f, 0.1f);
      render_instructions_page(window_w, window_h, current_instructions_page);

    } else if (phase == Editing) {
      render_level_state(game, window_w, window_h, false);
      if (editor_selected_cell_type) {
        render_cell(editor_selected_cell_type->st, editor_highlight_x,
            editor_highlight_y, game.w, game.h);
      }
      if (editor_palette_intensity) {
        float alpha_factor = ((editor_palette_intensity > 256) ? 1.0f : ((float)editor_palette_intensity / 256));
        render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f,
            0.8f * alpha_factor, 0.0f, 0.0f, 0.0f, 0.1f * alpha_factor);
        render_palette(editor_selected_cell_type, game.w, game.h, alpha_factor,
            game.frames_executed == 0, game.num_items_remaining);
      }
      draw_text(-0.99, 0.97, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
            "editing level %d", level_index);

      last_update_time = now();

    } else {

      if (!game.player_is_alive() && (game.player_is_losing() == 1.0)) {
        phase = Paused;
        player_did_lose = true;
        game = initial_state[level_index];

      } else if (game.player_did_win) {
        phase = Paused;
        player_did_lose = false;

        // combine level stats
        level_completion& c = completion[level_index];
        c.state = Completed;
        if (game.frames_executed < c.frames)
          c.frames = game.frames_executed;
        if (-game.num_items_remaining > c.extra_items)
          c.extra_items = -game.num_items_remaining;
        if (game.num_red_bombs > c.extra_bombs)
          c.extra_bombs = game.num_red_bombs;
        uint64_t cleared_space = game.count_cells_of_type(Empty);
        if (cleared_space > c.cleared_space)
          c.cleared_space = cleared_space;
        uint64_t attenuated_space = game.count_attenuated_space();
        if (attenuated_space < c.attenuated_space)
          c.attenuated_space = attenuated_space;
        uint64_t entropy = game.compute_entropy();
        if (entropy < c.entropy)
          c.entropy = entropy;

        int next_level_index;
        if (level_index < initial_state.size() - 1) {
          next_level_index = level_index + 1;
        } else {
          // if you completed the last level, go to the first incomplete level
          for (next_level_index = 0; (completion[next_level_index].state == Completed) &&
              (next_level_index < initial_state.size()); next_level_index++);
        }

        if (next_level_index < initial_state.size()) {
          level_index = next_level_index;
          game = initial_state[level_index];
          level_is_valid = game.validate();
          if (completion[level_index].state != Completed)
            completion[level_index].state = Attempted;
        }
        save_level_completion_state(level_completion_filename.c_str(), completion);

      } else if ((should_change_to_level >= 0) && (should_change_to_level < initial_state.size())) {
        phase = Paused;
        level_index = should_change_to_level;
        game = initial_state[level_index];
        if (completion[level_index].state == NotAttempted) {
          completion[level_index].state = Attempted;
          save_level_completion_state(level_completion_filename.c_str(), completion);
        }
        should_change_to_level = -1;
        player_did_lose = false;

      } else {
        uint64_t usec_per_update = 1000000.0 / game.updates_per_second;
        uint64_t now_time = now();
        uint64_t update_diff = now_time - last_update_time;
        if (update_diff >= usec_per_update) {
          if (phase == Playing) {
            enum player_impulse effective_impulse = current_impulse;
            if (!recent_impulses.empty()) {
              effective_impulse = recent_impulses.front();
              recent_impulses.pop_front();
            }

            uint64_t events = game.exec_frame(effective_impulse);
            if (should_play_sounds) {
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
          }
          last_update_time = now_time;
        }
      }

      if (!game.player_did_win) {
        render_level_state(game, window_w, window_h, show_stats);
        if (game.updates_per_second != 20.0f)
          render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.1f);

        double lose_overlay_intensity = game.player_is_losing();
        if (lose_overlay_intensity != 0.0)
          render_stripe_animation(window_w, window_h, 100,
              1.0f, 0.0f, 0.0f, 0.6f * lose_overlay_intensity,
              1.0f, 0.0f, 0.0f, 0.1f * lose_overlay_intensity);
      }

      if (phase == Paused)
        render_paused_screen(window_w, window_h, completion, level_index,
            game.player_did_win, player_did_lose, game.frames_executed,
            should_play_sounds, show_stats);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
