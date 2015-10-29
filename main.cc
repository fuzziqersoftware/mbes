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



const vector<pair<cell_state, const char*>> editor_available_cells({
  make_pair(cell_state(Empty, 1),          "EMPTY"),
  make_pair(cell_state(Circuit),           "CIRCUIT"),
  make_pair(cell_state(Rock),              "ROCK"),
  make_pair(cell_state(Exit),              "EXIT"),
  make_pair(cell_state(Player),            "PLAYER"),
  make_pair(cell_state(Item),              "ITEM"),
  make_pair(cell_state(Block),             "BLOCK"),
  make_pair(cell_state(RoundBlock),        "ROUND BLOCK"),
  make_pair(cell_state(RedBomb),           "RED BOMB"),
  make_pair(cell_state(YellowBomb),        "YELLOW BOMB"),
  make_pair(cell_state(GreenBomb),         "GREEN BOMB"),
  make_pair(cell_state(BlueBomb),          "BLUE BOMB"),
  make_pair(cell_state(GrayBomb),          "GRAY BOMB"),
  make_pair(cell_state(YellowBombTrigger), "YELLOW TRIGGER"),
  make_pair(cell_state(BombDude, Left),    "BOMB DUDE"),
  make_pair(cell_state(ItemDude, Left),    "ITEM DUDE"),
  make_pair(cell_state(RockGenerator),     "ROCK GENERATOR"),
  make_pair(cell_state(LeftPortal),        "LEFT PORTAL"),
  make_pair(cell_state(RightPortal),       "RIGHT PORTAL"),
  make_pair(cell_state(UpPortal),          "UP PORTAL"),
  make_pair(cell_state(DownPortal),        "DOWN PORTAL"),
  make_pair(cell_state(HorizontalPortal),  "HORIZONTAL PORTAL"),
  make_pair(cell_state(VerticalPortal),    "VERTICAL PORTAL"),
  make_pair(cell_state(Portal),            "PORTAL"),
});

static void editor_write_cell(level_state& l, uint32_t x, uint32_t y,
    const cell_state& cell) {
  l.at(x, y) = cell;
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
  }

  glVertex3f(to_window(x, l_w), -to_window(y, l_h), 1);
  glVertex3f(to_window(x + 1, l_w), -to_window(y, l_h), 1);
  glVertex3f(to_window(x + 1, l_w), -to_window(y + 1, l_h), 1);
  glVertex3f(to_window(x, l_w), -to_window(y + 1, l_h), 1);

  if (draw_center) {
    glColor4f(center_r, center_g, center_b, alpha);
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
        "%d %s REMAINING", items_remaining, (items_remaining == 1) ? "ITEM" : "ITEMS");
  else if (items_remaining < 0)
    draw_text(-0.99, -0.9, 0, 1, 0.5, 1, (float)window_w / window_h, 0.01, false,
        "%d EXTRA %s", -items_remaining, (items_remaining == -1) ? "ITEM" : "ITEMS");
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

  render_items_remaining(l.num_items_remaining, window_w, window_h);

  if (l.num_red_bombs > 0)
    draw_text(-0.99, -0.8, 0, 1, 0.5, 1, (float)window_w / window_h, 0.01, false,
        "%d RED %s", l.num_red_bombs, (l.num_red_bombs == 1) ? "BOMB" : "BOMBS");
  else if (l.num_red_bombs < 0)
    draw_text(-0.99, -0.8, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
        "%d RED %s IN DEBT", -l.num_red_bombs, (l.num_red_bombs == -1) ? "BOMB" : "BOMBS");
}



static void render_key_commands(float aspect_ratio, bool should_play_sounds) {
  draw_text(0, -0.5, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "SHIFT+I: INSTRUCTIONS");
  draw_text(0, -0.6, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      should_play_sounds ? "SHIFT+S: MUTE SOUND" : "SHIFT+S: UNMUTE SOUND");
  draw_text(0, -0.7, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "SHIFT+ARROW KEYS: CHANGE LEVEL");
  draw_text(0, -0.8, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "ESC: RESTART LEVEL / EXIT");
}

static void render_level_stats(const level_completion& lc, float aspect_ratio) {
  draw_text(0, -0.2, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "BEST TIME: %llu", lc.frames);
  draw_text(0, -0.3, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "BEST OVER-ACHIEVEMENT: %llu %s", lc.extra_items, (lc.extra_items == 1) ? "ITEM" : "ITEMS");
  draw_text(0, -0.4, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "MOST EXTRA BOMBS: %llu %s", lc.extra_bombs, (lc.extra_bombs == 1) ? "BOMB" : "BOMBS");
  draw_text(0, -0.5, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "MOST CLEARED SPACE: %llu %s", lc.cleared_space, (lc.cleared_space == 1) ? "CELL" : "CELLS");
  draw_text(0, -0.6, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "LEAST ATTENUATED SPACE: %llu %s", lc.attenuated_space, (lc.attenuated_space == 1) ? "CELL" : "CELLS");
}

static void render_paused_screen(int window_w, int window_h,
    const vector<level_completion>& completion, int level_index,
    bool player_did_win, bool player_did_lose, bool should_play_sounds) {

  size_t num_completed = 0;
  for (const auto& it : completion)
    if (it.state == Completed)
      num_completed++;

  render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f,
      0.0f, 0.0f, 0.1f);

  float aspect_ratio = (float)window_w / window_h;
  draw_text(0, 0.9, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
      "FUZZIQER SOFTWARE");
  draw_text(0, 0.7, 1, 1, 1, 1, (float)window_w / window_h, 0.03, true,
      "MOVE BLOCKS AND EAT STUFF");

  if (player_did_win) {
    draw_text(0, 0.3, 1, 1, 1, 1, aspect_ratio, 0.02, true,
        "YOU WIN");
    draw_text(0, 0.1, 1, 1, 1, 1, aspect_ratio, 0.01, true,
        "YOU HAVE COMPLETED ALL %llu LEVELS!", completion.size());
    // TODO render overall stats here

  } else {
    if (player_did_lose)
      draw_text(0, 0.3, 1, 0, 0, 1, aspect_ratio, 0.02, true,
          "LEVEL %d - PRESS ENTER TO TRY AGAIN", level_index);
    else
      draw_text(0, 0.3, 1, 1, 1, 1, aspect_ratio, 0.02, true,
          "LEVEL %d - PRESS ENTER TO PLAY", level_index);

    if (num_completed == completion.size()) {
      draw_text(0, 0.1, 0.5, 1, 0.5, 1, aspect_ratio, 0.01, true,
          "YOU HAVE COMPLETED ALL %llu LEVELS!", completion.size());
      render_level_stats(completion[level_index], aspect_ratio);

    } else if (completion[level_index].state == Completed) {
      draw_text(0, 0.1, 0.5, 1, 0.5, 1, aspect_ratio, 0.01, true,
          "YOU HAVE ALREADY COMPLETED THIS LEVEL (%lu/%lu)", num_completed, completion.size());
      render_level_stats(completion[level_index], aspect_ratio);

    } else {
      draw_text(0, 0.1, 1, 1, 1, 1, aspect_ratio, 0.01, true,
          "YOU HAVE COMPLETED %lu OF %lu LEVELS", num_completed, completion.size());
      render_key_commands(aspect_ratio, should_play_sounds);
    }
  }
}

static void render_instructions_page(int window_w, int window_h, int page_num) {

  float aspect_ratio = (float)window_w / window_h;

  draw_text(0, 0.9, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "INSTRUCTIONS (PAGE %d/3)",
      page_num + 1);
  draw_text(0, 0.8, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "LEFT/RIGHT ARROW KEYS: CHANGE PAGE; ESC: EXIT INSTRUCTIONS");

  if (page_num == 0) {
    draw_text(0,  0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "YOU ARE THE \x80 RED BOX. LUCKY YOU!");
    draw_text(0,  0.6,   1,   0,   0, 1, aspect_ratio, 0.01, true, "            \x80                    ");
    draw_text(0,  0.5,   1,   1,   1, 1, aspect_ratio, 0.01, true, "YOUR GOAL IN EACH LEVEL IS TO COLLECT THE \x80 ITEMS, THEN GO TO THE \x80 EXIT.");
    draw_text(0,  0.5, 0.8,   0, 0.8, 1, aspect_ratio, 0.01, true, "                                          \x80                              ");
    draw_text(0,  0.5, 0.5,   1,   0, 1, aspect_ratio, 0.01, true, "                                                                  \x80      ");

    draw_text(0,  0.4,   1,   1,   1, 1, aspect_ratio, 0.01, true, "ON SOME LEVELS THERE MAY BE EXTRA ITEMS THAT YOU DON\'T NEED TO COLLECT.");

    draw_text(0,  0.2,   1,   1,   1, 1, aspect_ratio, 0.01, true, "WATCH OUT FOR FALLING \x80 ITEMS, \x80 ROCKS AND \x80 BOMBS.");
    draw_text(0,  0.2, 0.8,   0, 0.8, 1, aspect_ratio, 0.01, true, "                      \x80                            ");
    draw_text(0,  0.2, 0.6, 0.6, 0.6, 1, aspect_ratio, 0.01, true, "                               \x80                   ");
    draw_text(0,  0.2,   0,   1,   0, 1, aspect_ratio, 0.01, true, "                                           \x81       ");
    draw_text(0,  0.2,   1,   1,   1, 1, aspect_ratio, 0.01, true, "                                           \x82       ");
    draw_text(0,  0.1,   1,   1,   1, 1, aspect_ratio, 0.01, true, "YOU LOSE IF ANYTHING FALLS ON YOU OR EXPLODES NEAR YOU.");
    draw_text(0,  0.0,   1,   1,   1, 1, aspect_ratio, 0.01, true, "YOU CAN PUSH ROCKS AND BOMBS OUT OF THE WAY IF THERE'S EMPTY SPACE ON THE OTHER SIDE.");
    draw_text(0, -0.1,   1,   1,   1, 1, aspect_ratio, 0.01, true, "IF STACKED, ROCKS AND ITEMS WILL ROLL OFF EACH OTHER TO FORM A NEAT PILE.");

    draw_text(0, -0.3,   1,   1,   1, 1, aspect_ratio, 0.01, true, "KEYS WHEN PLAYING:");
    draw_text(0, -0.4,   1,   1,   1, 1, aspect_ratio, 0.01, true, "ARROW KEYS: MOVE AROUND, PUSH OBJECTS             ");
    draw_text(0, -0.5,   1,   1,   1, 1, aspect_ratio, 0.01, true, "     SPACE: DROP BOMB (APPEARS WHEN YOU MOVE AWAY)");
    draw_text(0, -0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "       TAB: TOGGLE SPEED (SLOW/FAST)              ");
    draw_text(0, -0.7,   1,   1,   1, 1, aspect_ratio, 0.01, true, "     ENTER: PAUSE                                 ");
    draw_text(0, -0.8,   1,   1,   1, 1, aspect_ratio, 0.01, true, "       ESC: RESTART LEVEL / EXIT                  ");

  } else if (page_num == 1) {
    draw_text(0,  0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "SOME OBJECTS YOU MIGHT ENCOUNTER:");
    draw_text(0,  0.4,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 CIRCUIT. IT\'S SOLID FOR EVERYTHING EXCEPT YOU - EAT THESE TO GET THEM OUT OF THE WAY.");
    draw_text(0,  0.4,   0, 0.8,   0, 1, aspect_ratio, 0.01, true, "\x80                                                                                      ");
    draw_text(0,  0.3,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 BLOCK. IT CAN\'T BE MOVED OR DESTROYED BY ANYTHING.                                   ");
    draw_text(0,  0.2,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 ROUND BLOCK. ROCKS AND ITEMS WILL ROLL OFF OF IT. CAN BE DESTROYED BY EXPLOSIONS.    ");
    draw_text(0,  0.2, 0.9, 0.9,   1, 1, aspect_ratio, 0.01, true, "\x80                                                                                      ");
    draw_text(0,  0.1,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 RED BOMB. YOU CAN PICK THESE UP AND DROP THEM LATER.                                 ");
    draw_text(0,  0.1,   1,   0,   0, 1, aspect_ratio, 0.01, true, "\x81                                                                                      ");
    draw_text(0,  0.1,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x82                                                                                      ");
    draw_text(0,  0.0,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 YELLOW BOMB. PUSH THESE AROUND, THEN TOUCH THE YELLOW TRIGGER TO SET THEM OFF.       ");
    draw_text(0,  0.0, 0.8, 0.8,   0, 1, aspect_ratio, 0.01, true, "\x81                                                                                      ");
    draw_text(0,  0.0,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x82                                                                                      ");
    draw_text(0, -0.1,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 YELLOW TRIGGER. SETS OFF ALL YELLOW BOMBS AT ONCE.                                   ");
    draw_text(0, -0.1, 0.8, 0.8,   0, 1, aspect_ratio, 0.01, true, "\x80                                                                                      ");
    draw_text(0, -0.2,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 GREEN BOMB. EXPLODES WHEN IT LANDS AFTER FALLING, OR IF SOMETHING IS DROPPED ON IT.  ");
    draw_text(0, -0.2,   0,   1,   0, 1, aspect_ratio, 0.01, true, "\x81                                                                                      ");
    draw_text(0, -0.2,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x82                                                                                      ");
    draw_text(0, -0.3,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 BOMB DUDE. IT MOVES BY ITSELF! NOT HARMFUL, BUT EXPLODES IF SOMETHING FALLS ON IT.   ");
    draw_text(0, -0.3,   1, 0.5,   0, 1, aspect_ratio, 0.01, true, "\x80                                                                                      ");
    draw_text(0, -0.4,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 ITEM DUDE. LIKE A BOMB DUDE, BUT CREATES NINE ITEMS WHEN YOU KILL IT!                ");
    draw_text(0, -0.4,   0, 0.5,   1, 1, aspect_ratio, 0.01, true, "\x80                                                                                      ");
    draw_text(0, -0.5,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 PORTAL. YOU CAN ONLY MOVE THROUGH THIS IN THE DIRECTION OF ITS ARROW(S).             ");
    draw_text(0, -0.5, 0.7,   0,   0, 1, aspect_ratio, 0.01, true, "\x80                                                                                      ");
    draw_text(0, -0.5,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x83                                                                                      ");
    draw_text(0, -0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "  THERE ALSO MUST BE EMPTY SPACE ON THE OTHER SIDE.                                    ");
    draw_text(0, -0.8,   1,   1,   1, 1, aspect_ratio, 0.01, true, "THERE MAY BE OTHER TYPES OF OBJECTS NOT MENTIONED HERE. STRANGE THINGS AWAIT...");

  } else if (page_num == 2) {
    draw_text(0,  0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "A FEW TIPS:");
    draw_text(0,  0.4,   1,   1,   1, 1, aspect_ratio, 0.01, true, "SOME LEVELS REQUIRE YOU TO MOVE QUICKLY IN DIFFERENT DIRECTIONS. TRY SLOW MODE (PRESS TAB)");
    draw_text(0,  0.3,   1,   1,   1, 1, aspect_ratio, 0.01, true, "IF IT\'S TOO DIFFICULT TO MANEUVER QUICKLY ENOUGH.");
    draw_text(0,  0.1,   1,   1,   1, 1, aspect_ratio, 0.01, true, "DUDES DON\'T HURT YOU, BUT THEY DO TEND TO GET IN THE WAY.");
    draw_text(0,  0.0,   1,   1,   1, 1, aspect_ratio, 0.01, true, "BE CAREFUL IN CONFINED SPACES WITH DUDES.");
    draw_text(0, -0.2,   1,   1,   1, 1, aspect_ratio, 0.01, true, "YOU CAN USE RED BOMBS EVEN IF YOU HAVEN\'T PICKED ANY UP, BUT IF YOU'RE IN DEBT (HAVE USED");
    draw_text(0, -0.3,   1,   1,   1, 1, aspect_ratio, 0.01, true, "MORE RED BOMBS THAN YOU\'VE COLLECTED), THEN YOU CAN\'T FINISH THE LEVEL.");
    draw_text(0, -0.5,   1,   1,   1, 1, aspect_ratio, 0.01, true, "AFTER USING A RED BOMB, YOU CAN PICK IT UP AGAIN BEFORE IT EXPLODES. FOR EXAMPLE, YOU CAN");
    draw_text(0, -0.6,   1,   1,   1, 1, aspect_ratio, 0.01, true, "USE A RED BOMB TO TEMPORARILY PREVENT OBJECTS FROM FALLING OR ROLLING.");
    draw_text(0, -0.8,   1,   1,   1, 1, aspect_ratio, 0.01, true, "\x80 BLOCKS CAN\'T BE DESTROYED BY ANYTHING. IF YOU NEED ALL THE ITEMS FROM AN ITEM DUDE,");
    draw_text(0, -0.9,   1,   1,   1, 1, aspect_ratio, 0.01, true, "DON\'T BLOW IT UP NEAR A BLOCK - YOU'LL LOSE AN ITEM THAT YOU NEED.");

  } else {
    draw_text(0, 0.7, 1, 0, 0, 1, aspect_ratio, 0.01, true,
        "INVALID PAGE NUMBER: %d", page_num);
  }
}



static void render_palette(int sel_type, int l_w, int l_h, float alpha,
    bool can_save, int num_items_remaining) {
  glBegin(GL_QUADS);

  glColor4f(1.0, 1.0, 1.0, alpha * 0.5);
  glVertex3f(to_window(3, 2 * l_w), -to_window(3, 2 * l_h), 1);
  glVertex3f(to_window(4 * editor_available_cells.size() + 3, 2 * l_w), -to_window(3, 2 * l_h), 1);
  glVertex3f(to_window(4 * editor_available_cells.size() + 3, 2 * l_w), -to_window(7, 2 * l_h), 1);
  glVertex3f(to_window(3, 2 * l_w), -to_window(7, 2 * l_h), 1);

  glColor4f(1.0, 1.0, 1.0, alpha);
  glVertex3f(to_window(4 * sel_type + 3, 2 * l_w), -to_window(3, 2 * l_h), 1);
  glVertex3f(to_window(4 * sel_type + 7, 2 * l_w), -to_window(3, 2 * l_h), 1);
  glVertex3f(to_window(4 * sel_type + 7, 2 * l_w), -to_window(7, 2 * l_h), 1);
  glVertex3f(to_window(4 * sel_type + 3, 2 * l_w), -to_window(7, 2 * l_h), 1);

  for (size_t x = 0; x < editor_available_cells.size(); x++)
    render_cell_quads(editor_available_cells[x].first, 2 * x + 2, 2, l_w, l_h, alpha);
  glEnd();

  glBegin(GL_TRIANGLES);
  glColor4f(1.0, 1.0, 1.0, alpha);
  for (size_t x = 0; x < editor_available_cells.size(); x++)
    render_cell_tris(editor_available_cells[x].first, 2 * x + 2, 2, l_w, l_h);
  glEnd();

  draw_text(0, 0.6, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
      editor_available_cells[sel_type].second);

  draw_text(0.0, 0.2, 1, 1, 1, alpha, (float)l_w / l_h, 0.02, true,
      "MOVE BLOCKS AND DESIGN LEVELS");

  draw_text(0, 0.0, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
      "SPACE / RIGHT-CLICK: OPEN/CLOSE PALETTE");
  draw_text(0, -0.1, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
      "LEFT-CLICK: DRAW CELL (DRAG TO DRAW MULTIPLE)");
  draw_text(0, -0.2, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
      "SHIFT+UP/DOWN: CHANGE REQUIRED ITEM COUNT");
  draw_text(0, -0.3, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
      "I: AUTOMATICALLY COMPUTE REQUIRED ITEM COUNT");
  if (can_save) {
    draw_text(0, -0.4, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
        "ESC: EXIT EDITOR; DON\'T SAVE CHANGES");
    draw_text(0, -0.5, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
        "ENTER: EXIT EDITOR AND SAVE CHANGES");
  } else {
    draw_text(0, -0.4, 1, 1, 1, alpha, (float)l_w / l_h, 0.01, true,
        "ESC / ENTER: EXIT EDITOR; DON\'T SAVE CHANGES");
    draw_text(0, -0.5, 1, 0, 0, alpha, (float)l_w / l_h, 0.01, true,
        "CAN\'T SAVE A PARTIALLY-PLAYED LEVEL");
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
bool player_did_lose = false;
bool should_reload_state = false;
bool should_play_sounds = true;
enum player_impulse current_impulse = None;
int level_index = -1;
int should_change_to_level = -1;
int current_instructions_page = 0;

int mouse_x, mouse_y;
int editor_palette_intensity = 0;
int editor_highlight_x = 0, editor_highlight_y = 0;
int editor_selected_cell_type;
bool editor_drawing = false;



static void glfw_key_cb(GLFWwindow* window, int key, int scancode,
    int action, int mods) {

  if (((action == GLFW_PRESS) || (action == GLFW_REPEAT))) {
    if (phase == Paused) {
      if ((key == GLFW_KEY_LEFT) && (mods & GLFW_MOD_SHIFT)) {
        should_change_to_level = level_index - 1;
        return;
      } else if ((key == GLFW_KEY_RIGHT) && (mods & GLFW_MOD_SHIFT)) {
        should_change_to_level = level_index + 1;
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
      } else if (phase == Playing)
        phase = Paused;
      else
        phase = Playing;
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
      phase = Playing;
      player_did_lose = false;
    } else if ((key == GLFW_KEY_RIGHT) && ((phase == Playing) || (phase == Paused))) {
      current_impulse = Right;
      phase = Playing;
      player_did_lose = false;
    } else if ((key == GLFW_KEY_UP) && ((phase == Playing) || (phase == Paused))) {
      current_impulse = Up;
      phase = Playing;
      player_did_lose = false;
    } else if ((key == GLFW_KEY_DOWN) && ((phase == Playing) || (phase == Paused))) {
      current_impulse = Down;
      phase = Playing;
      player_did_lose = false;
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

  if (!editor_palette_intensity) {
    editor_drawing = true;
    editor_write_cell(game, editor_highlight_x, editor_highlight_y,
        editor_available_cells[editor_selected_cell_type].first);
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

  if (editor_palette_intensity && ((cell_x & 1) == 0) && (cell_x >= 2) &&
      (cell_x <= 2 * editor_available_cells.size() + 1) && (cell_y == 2)) {
    editor_selected_cell_type = (cell_x - 2) / 2;

  } else if (editor_drawing) {
    editor_write_cell(game, editor_highlight_x, editor_highlight_y,
        editor_available_cells[editor_selected_cell_type].first);
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
  string level_completion_filename = string(pw->pw_dir) + "/.mbes_completion";
  vector<level_completion> completion = load_level_completion_state(
      level_completion_filename.c_str());
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
          "LEVEL IS CORRUPT");
      draw_text(0, 0.0, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
          "ESC: EXIT");

    } else if (phase == Instructions) {
      render_level_state(game, window_w, window_h);
      render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f, 0.8f,
          0.0f, 0.0f, 0.0f, 0.1f);
      render_instructions_page(window_w, window_h, current_instructions_page);

    } else if (phase == Editing) {
      render_level_state(game, window_w, window_h);
      render_cell(editor_available_cells[editor_selected_cell_type].first,
          editor_highlight_x, editor_highlight_y, game.w, game.h);
      if (editor_palette_intensity) {
        float alpha_factor = ((editor_palette_intensity > 256) ? 1.0f : ((float)editor_palette_intensity / 256));
        render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f,
            0.8f * alpha_factor, 0.0f, 0.0f, 0.0f, 0.1f * alpha_factor);
        render_palette(editor_selected_cell_type, game.w, game.h, alpha_factor,
            game.frames_executed == 0, game.num_items_remaining);
      }
      draw_text(-0.99, 0.97, 1, 0, 0, 1, (float)window_w / window_h, 0.01, false,
            "EDITING LEVEL %d", level_index);

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
            uint64_t events = game.exec_frame(current_impulse);
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
        render_level_state(game, window_w, window_h);
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
            game.player_did_win, player_did_lose, should_play_sounds);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
