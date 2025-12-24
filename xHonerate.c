// xHonorate core
#include <ncurses.h>
#include <fluidsynth.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "xHonerate.h"

struct RuinTile {
  int y;
  int x;
  bool active;
};

static const int MAX_MOON_RUINS = 180;
static const int MAX_WHISPER_RUINS = 120;
static RuinTile moon_ruins[MAX_MOON_RUINS];
static RuinTile whisper_ruins[MAX_WHISPER_RUINS];
static int moon_ruin_count = 0;
static int whisper_ruin_count = 0;
static int shrine_y_global = 0;
static int shrine_x_global = 0;
static int corridor_x_global = 0;
static int block1_y_global = 0;
static int block2_y_global = 0;

static void fluidsynth_log_filter(int level, const char* message, void* data) {
  if (level == FLUID_WARN) {
    if (strstr(message, "SDL3 not initialized") != NULL) return;
    if (strstr(message, "Sample 'SineWave'") != NULL) return;
  }
  fputs(message, stderr);
}

static bool is_ruin_at(RuinTile* ruins, int count, int y, int x) {
  for (int i = 0; i < count; i++) {
    if (ruins[i].active && ruins[i].y == y && ruins[i].x == x) return true;
  }
  return false;
}

static bool clear_adjacent_ruin(RuinTile* ruins, int count, int y, int x) {
  for (int i = 0; i < count; i++) {
    if (!ruins[i].active) continue;
    int dy = ruins[i].y - y;
    int dx = ruins[i].x - x;
    if ((dy == 0 && abs(dx) == 1) || (dx == 0 && abs(dy) == 1)) {
      ruins[i].active = false;
      return true;
    }
  }
  return false;
}

static void playNoteHold(fluid_synth_t* synth, int channel, int note, int velocity, int ms) {
  fluid_synth_noteon(synth, channel, note, velocity);
  napms(ms);
  fluid_synth_noteoff(synth, channel, note);
}

static void add_ruin(RuinTile* ruins, int* count, int max_count, int y, int x) {
  if (*count >= max_count) return;
  for (int i = 0; i < *count; i++) {
    if (ruins[i].y == y && ruins[i].x == x) {
      ruins[i].active = true;
      return;
    }
  }
  ruins[*count] = {y, x, true};
  (*count)++;
}

static bool is_on_whisper_corridor(int map_rows, int y, int x) {
  if (x == corridor_x_global || x == corridor_x_global + 1) {
    if (y >= 2 && y < map_rows - 2) return true;
  }
  if ((y == shrine_y_global || y == shrine_y_global + 1) &&
      x >= (corridor_x_global < shrine_x_global ? corridor_x_global : shrine_x_global) &&
      x <= (corridor_x_global > shrine_x_global ? corridor_x_global : shrine_x_global)) {
    return true;
  }
  return false;
}

static void init_ruins(int map_rows, int cols, int relic_y, int relic_x) {
  int attempts;
  moon_ruin_count = 0;
  whisper_ruin_count = 0;
  corridor_x_global = (cols / 4) + (rand() % (cols / 2));
  {
    int top = 2;
    int bottom = (map_rows / 3);
    if (bottom <= top) bottom = top + 1;
    shrine_y_global = top + (rand() % (bottom - top + 1));
  }
  shrine_x_global = (cols / 4) + (rand() % (cols / 2));
  {
    int min1 = map_rows / 3;
    int max1 = map_rows / 2;
    if (max1 <= min1) max1 = min1 + 1;
    block1_y_global = min1 + (rand() % (max1 - min1 + 1));
    int min2 = map_rows / 2;
    int max2 = map_rows - 3;
    if (max2 <= min2) max2 = min2 + 1;
    block2_y_global = min2 + (rand() % (max2 - min2 + 1));
  }
  if (block1_y_global == shrine_y_global) block1_y_global++;
  if (block2_y_global == shrine_y_global) block2_y_global--;
  if (block1_y_global < 2) block1_y_global = 2;
  if (block2_y_global < 2) block2_y_global = 2;

  // Moonlit Ruins: clustered around the center.
  attempts = 0;
  while (moon_ruin_count < 120 && attempts < 4000) {
    int y = 2 + rand() % (map_rows - 4);
    int x = 2 + rand() % (cols - 4);
    if (abs(y - relic_y) <= 2 && abs(x - relic_x) <= 2) { attempts++; continue; }
    if (y == shrine_y_global && x == shrine_x_global) { attempts++; continue; }
    if (is_ruin_at(moon_ruins, moon_ruin_count, y, x)) { attempts++; continue; }
    moon_ruins[moon_ruin_count++] = {y, x, true};
    attempts++;
  }

  // Whispering Woods: heavier density, avoid shrine area.
  attempts = 0;
  while (whisper_ruin_count < 80 && attempts < 5000) {
    int y = 2 + rand() % (map_rows - 4);
    int x = 2 + rand() % (cols - 4);
    if (abs(y - shrine_y_global) <= 2 && abs(x - shrine_x_global) <= 2) { attempts++; continue; }
    if (is_on_whisper_corridor(map_rows, y, x)) { attempts++; continue; }
    if (is_ruin_at(whisper_ruins, whisper_ruin_count, y, x)) { attempts++; continue; }
    whisper_ruins[whisper_ruin_count++] = {y, x, true};
    attempts++;
  }

  // Mandatory blockers on the main path in Whispering Woods.
  add_ruin(whisper_ruins, &whisper_ruin_count, MAX_WHISPER_RUINS, block1_y_global, corridor_x_global);
  add_ruin(whisper_ruins, &whisper_ruin_count, MAX_WHISPER_RUINS, block2_y_global, corridor_x_global);
}

/*-------------------\
|------- MAIN -------|
\-------------------*/
int main(int argc, char **argv) {
    // synth variables
    fluid_settings_t* _settings;
    fluid_synth_t* _synth;
    fluid_audio_driver_t* _adriver;
    int _sfont_id;

    // game variables
    int y, x;
    int region = 0;
    const int region_count = 3;
    bool gate_open = false;
    bool has_relic = false;
    bool relic_placed = false;
    bool has_key = false;
    bool moon_ruins_cleared = false;
    bool whisper_ruins_cleared = false;
    bool in_forest = false;
    int win_flash = 0;
    bool win_riff_played = false;

    // input and note variables
    int _inputChar;
#ifdef _WIN32
    SHORT _prevUp = 0, _prevDown = 0, _prevLeft = 0, _prevRight = 0;
    SHORT _prevW = 0, _prevA = 0, _prevS = 0, _prevD = 0, _prevQ = 0, _prevE = 0;
#endif
    int _channel = 0;
    int _note = 48;
    int _velocity = 42;
    int _program = 15;

    // Create the synth and apply settings.
    fluid_set_log_function(FLUID_WARN, fluidsynth_log_filter, NULL);
    _settings = new_fluid_settings();
    // Match common Windows mixer defaults to avoid format errors.
    fluid_settings_setnum(_settings, "synth.sample-rate", 44100.0);
    // Increase gain for better audibility on Windows mixers.
    fluid_settings_setnum(_settings, "synth.gain", 0.8);
    // Give the synth more headroom to avoid ringbuffer warnings.
    fluid_settings_setint(_settings, "synth.polyphony", 256);
    // Larger buffers reduce crackling on some Windows devices.
    fluid_settings_setint(_settings, "audio.periods", 8);
    fluid_settings_setint(_settings, "audio.period-size", 256);
    _synth = new_fluid_synth(_settings);
    // Try multiple Windows-friendly audio drivers in order.
    {
      const char* _drivers[] = {"waveout", "dsound", "wasapi", "portaudio", "file"};
      _adriver = NULL;
      for (int i = 0; i < 5 && !_adriver; i++) {
        fluid_settings_setstr(_settings, "audio.driver", _drivers[i]);
        _adriver = new_fluid_audio_driver(_settings, _synth);
      }
    }
    _sfont_id = fluid_synth_sfload(_synth, "smiles.sf2", 1);

    // Channel 1 program
    fluid_synth_program_select(_synth, _channel, _sfont_id, 0, _program);

    // init ncurses for keyboard input
    initscr();

    // grant ability to read arrow keays
    keypad(stdscr, TRUE);
    // non-blocking input so we can drain repeats and keep movement smooth
    nodelay(stdscr, TRUE);

    // added for game
    cbreak();
    noecho();

    clear();

    /* start player at lower-left */
    int map_bottom = LINES - 2;
    y = map_bottom;
    x = 0;
    in_forest = false;

    srand((unsigned int)time(NULL));
    {
      int map_rows = LINES - 1;
      int relic_y = map_rows / 2;
      int relic_x = COLS / 2;
      init_ruins(map_rows, COLS, relic_y, relic_x);
    }

    /* initialize the quest map */
    draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
    draw_hud(region, gate_open, has_relic, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared);
    refresh();

    /*-------------------\
    |----- MAIN LOOP ----|
    \-------------------*/
    while (true) {
      /* by default, you get a blinking cursor - use it to indicate player */
      mvaddch(y, x, PLAYER);
      move(y, x);

      int map_rows = LINES - 1;
      int forest_top = 3;
      int forest_bottom = map_rows - 5;
      int forest_left = COLS / 2;
      int forest_right = COLS - 4;
      draw_hud(region, gate_open, has_relic, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared);
      refresh();

#ifdef _WIN32
      // On Windows, use edge-triggered key state to avoid repeats while held.
      _inputChar = ERR;
      bool _interact = false;
      SHORT _cur = GetAsyncKeyState('Q');
      if ((_cur & 0x8000) && !(_prevQ & 0x8000)) break;
      _prevQ = _cur;
      _cur = GetAsyncKeyState('E');
      if ((_cur & 0x8000) && !(_prevE & 0x8000)) _interact = true;
      _prevE = _cur;

      _cur = GetAsyncKeyState(VK_UP);
      if ((_cur & 0x8000) && !(_prevUp & 0x8000)) _inputChar = KEY_UP;
      _prevUp = _cur;
      if (_inputChar == ERR) {
        _cur = GetAsyncKeyState(VK_DOWN);
        if ((_cur & 0x8000) && !(_prevDown & 0x8000)) _inputChar = KEY_DOWN;
        _prevDown = _cur;
      }
      if (_inputChar == ERR) {
        _cur = GetAsyncKeyState(VK_LEFT);
        if ((_cur & 0x8000) && !(_prevLeft & 0x8000)) _inputChar = KEY_LEFT;
        _prevLeft = _cur;
      }
      if (_inputChar == ERR) {
        _cur = GetAsyncKeyState(VK_RIGHT);
        if ((_cur & 0x8000) && !(_prevRight & 0x8000)) _inputChar = KEY_RIGHT;
        _prevRight = _cur;
      }
      if (_inputChar == ERR) {
        _cur = GetAsyncKeyState('W');
        if ((_cur & 0x8000) && !(_prevW & 0x8000)) _inputChar = 'W';
        _prevW = _cur;
      }
      if (_inputChar == ERR) {
        _cur = GetAsyncKeyState('S');
        if ((_cur & 0x8000) && !(_prevS & 0x8000)) _inputChar = 'S';
        _prevS = _cur;
      }
      if (_inputChar == ERR) {
        _cur = GetAsyncKeyState('A');
        if ((_cur & 0x8000) && !(_prevA & 0x8000)) _inputChar = 'A';
        _prevA = _cur;
      }
      if (_inputChar == ERR) {
        _cur = GetAsyncKeyState('D');
        if ((_cur & 0x8000) && !(_prevD & 0x8000)) _inputChar = 'D';
        _prevD = _cur;
      }
#else
      // read one key per frame
      _inputChar = getch();
      bool _interact = false;
      if (_inputChar == 'q') break;
      if (_inputChar == 'e' || _inputChar == 'E') _interact = true;
#endif

      bool _moved = false;
      /* test inputted key and determine direction */
      switch (_inputChar) {
        case KEY_UP:
        case 'w':
        case 'W':
            _note = 60;
            if ((y > 0) && is_move_okay(y - 1, x, gate_open)) {
              mvaddch(y, x, EMPTY);
              y = y - 1;
              _moved = true;
            } else if(y == 0) {
              region = (region + 1) % region_count;
              clear();
              in_forest = false;
              draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
              draw_hud(region, gate_open, has_relic, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared);
              y = map_bottom;
              _moved = true;
            }
            break;
        case KEY_DOWN:
        case 's':
        case 'S':
            _note = 48;
            if ((y < map_bottom) && is_move_okay(y + 1, x, gate_open)) {
              mvaddch(y, x, EMPTY);
              y = y + 1;
              _moved = true;
            } else if(y == map_bottom) {
              region = (region + region_count - 1) % region_count;
              clear();
              in_forest = false;
              draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
              draw_hud(region, gate_open, has_relic, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared);
              y = 0;
              _moved = true;
            }
            break;
        case KEY_LEFT:
        case 'a':
        case 'A':
            _note = 55;
            if ((x > 0) && is_move_okay(y, x - 1, gate_open)) {
              mvaddch(y, x, EMPTY);
              x = x - 1;
              _moved = true;
            }
            break;
        case KEY_RIGHT:
        case 'd':
        case 'D':
            _note = 51;
            if ((x < COLS - 1) && is_move_okay(y, x + 1, gate_open)) {
              mvaddch(y, x, EMPTY);
              x = x + 1;
              _moved = true;
            }
            break;
      }

      int key_y = forest_top + 3;
      int key_x = forest_left + 4;
      int relic_y = map_rows / 2;
      int relic_x = COLS / 2;
      int gate_top = relic_y - 1;
      int gate_bottom = relic_y + 1;
      int gate_left = relic_x - 2;
      int gate_right = relic_x + 2;
      int shrine_y = shrine_y_global;
      int shrine_x = shrine_x_global;

      bool current_in_forest = (region == 0 &&
                                y >= forest_top && y <= forest_bottom &&
                                x >= forest_left && x <= forest_right);
      if (current_in_forest != in_forest) {
        in_forest = current_in_forest;
        draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
      }

      if (_interact) {
        if (region == 1 && has_key) {
          if (clear_adjacent_ruin(moon_ruins, moon_ruin_count, y, x)) {
            draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
          }
        }
        if (region == 2 && has_key) {
          if (clear_adjacent_ruin(whisper_ruins, whisper_ruin_count, y, x)) {
            draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
          }
        }
        if (region == 1 && !gate_open && has_key) {
          if (y >= gate_top && y <= gate_bottom &&
              (x == gate_left - 1 || x == gate_right + 1)) {
            gate_open = true;
            draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
          }
          if (x >= gate_left && x <= gate_right &&
              (y == gate_top - 1 || y == gate_bottom + 1)) {
            gate_open = true;
            draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
          }
        }
        if (region == 2 && has_relic && !relic_placed) {
          int dy = y - shrine_y;
          int dx = x - shrine_x;
          if ((dy == 0 && abs(dx) == 1) || (dx == 0 && abs(dy) == 1)) {
            has_relic = false;
            relic_placed = true;
            win_flash = 30;
            draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
          }
        }
      }

      if (region == 0 && in_forest && !has_key && y == key_y && x == key_x) {
        has_key = true;
        draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
      }

      if (region == 1 && gate_open && !has_relic && y == relic_y && x == relic_x) {
        has_relic = true;
        draw_region(region, gate_open, relic_placed, has_key, moon_ruins_cleared, whisper_ruins_cleared, in_forest);
      }

      if (_moved) {
        if (region == 0) _program = 24;      // Acoustic Guitar (nylon)
        else if (region == 1) _program = 89; // Pad 2 (warm)
        else _program = 70;                 // Bassoon
        fluid_synth_program_select(_synth, _channel, _sfont_id, 0, _program);
        playNote(_synth, _channel, _note, _velocity);
      }
      if (region == 2 && relic_placed && win_flash > 0) {
        char glow = (win_flash % 2 == 0) ? '*' : '+';
        mvaddch(shrine_y - 1, shrine_x, glow);
        mvaddch(shrine_y + 1, shrine_x, glow);
        mvaddch(shrine_y, shrine_x - 1, glow);
        mvaddch(shrine_y, shrine_x + 1, glow);
        mvaddch(shrine_y - 1, shrine_x - 1, glow);
        mvaddch(shrine_y - 1, shrine_x + 1, glow);
        mvaddch(shrine_y + 1, shrine_x - 1, glow);
        mvaddch(shrine_y + 1, shrine_x + 1, glow);
        win_flash--;
      }
      if (region == 2 && relic_placed && !win_riff_played) {
        int riff_notes[] = {60, 64, 67, 72, 74, 72, 67, 64, 60};
        int riff_count = sizeof(riff_notes) / sizeof(riff_notes[0]);
        fluid_synth_program_select(_synth, _channel, _sfont_id, 0, 51);
        for (int i = 0; i < riff_count; i++) {
          playNoteHold(_synth, _channel, riff_notes[i], 120, 140);
        }
        win_riff_played = true;
      }
      napms(30);
    }

    /* Clean up synth */
    delete_fluid_audio_driver(_adriver);
    delete_fluid_synth(_synth);
    delete_fluid_settings(_settings);
    /* Clean up curses */
    endwin();

    /* Say Goodbye */
    std::cout << std::endl <<  "Thanks for playing!" << std::endl;
    /* End program successfully */
    return 0;
}


/*---------------------------\
|- FUNCTION IMPLEMENTATIONS -|
\---------------------------*/
// Play a note on the synth.  This will only do a "stab" not a held note currently.
void playNote(fluid_synth_t* synth, int channel, int note, int velocity) {
  /* Play a note */
  fluid_synth_noteon(synth, channel, note, velocity);
  /* Stop the note */
  fluid_synth_noteoff(synth, channel, note);
}

int is_move_okay(int y, int x, bool gate_open)
{
    int testch;

    /* return true if the space is okay to move into */
    testch = mvinch(y, x) & A_CHARTEXT;
    if (testch == GATE && gate_open) return 1;
    if (testch == WATER || testch == TREE || testch == RUIN ||
        testch == SHRINE || testch == SHRINE_FILLED || testch == GATE) {
      return 0;
    }
    return ((testch == GRASS) || (testch == EMPTY) ||
            (testch == RELIC) || (testch == KEYITEM));
}

void draw_region(int region, bool gate_open, bool relic_placed, bool has_key, bool moon_ruins_cleared, bool whisper_ruins_cleared, bool in_forest)
{
    int y, x;
    int map_rows = LINES - 1;
    int forest_top = 3;
    int forest_bottom = map_rows - 5;
    int forest_left = COLS / 2;
    int forest_right = COLS - 4;
    int key_y = forest_top + 3;
    int key_x = forest_left + 4;
    int relic_y = map_rows / 2;
    int relic_x = COLS / 2;
    int shrine_y = shrine_y_global;
    int shrine_x = shrine_x_global;

    if (region == 0) {
      char base = in_forest ? EMPTY : GRASS;
      int path_x = forest_left + 2;
      int path_top = forest_bottom - 2;
      for (y = 0; y < map_rows; y++) {
        mvhline(y, 0, base, COLS);
      }
      for (y = 2; y < map_rows - 3; y++) {
        mvhline(y, 2, WATER, COLS / 4);
      }
      for (y = forest_top; y <= forest_bottom; y++) {
        mvhline(y, forest_left, in_forest ? GRASS : TREE, forest_right - forest_left + 1);
      }
      if (!in_forest) {
        for (y = path_top; y <= forest_bottom; y++) {
          mvaddch(y, path_x, EMPTY);
          mvaddch(y, path_x + 1, EMPTY);
        }
      }
      if (in_forest && !has_key) {
        mvaddch(key_y, key_x, KEYITEM);
      }
    } else if (region == 1) {
      for (y = 0; y < map_rows; y++) {
        mvhline(y, 0, EMPTY, COLS);
      }
      for (int i = 0; i < moon_ruin_count; i++) {
        if (moon_ruins[i].active) mvaddch(moon_ruins[i].y, moon_ruins[i].x, RUIN);
      }
      for (y = 2; y < map_rows - 2; y += 3) {
        mvaddch(y, 2, TREE);
        mvaddch(y, COLS - 3, TREE);
      }
      if (!gate_open) {
        mvhline(relic_y - 1, relic_x - 2, GATE, 5);
        mvhline(relic_y + 1, relic_x - 2, GATE, 5);
        mvvline(relic_y - 1, relic_x - 2, GATE, 3);
        mvvline(relic_y - 1, relic_x + 2, GATE, 3);
      }
      mvaddch(relic_y, relic_x, RELIC);
    } else if (region == 2) {
      if (relic_placed) {
        for (y = 0; y < map_rows; y++) {
          mvhline(y, 0, GRASS, COLS);
        }
      } else {
        for (y = 0; y < map_rows; y++) {
          mvhline(y, 0, EMPTY, COLS);
        }
        for (y = 1; y < map_rows - 1; y += 2) {
          for (x = 2; x < COLS - 2; x += 3) {
            if ((x + y) % 2 == 0) mvaddch(y, x, TREE);
          }
        }
        for (y = 2; y < map_rows - 2; y += 3) {
          for (x = 3; x < COLS - 3; x += 5) {
            if ((x + y) % 4 == 1) mvaddch(y, x, TREE);
          }
        }
        for (int i = 0; i < whisper_ruin_count; i++) {
          if (whisper_ruins[i].active) mvaddch(whisper_ruins[i].y, whisper_ruins[i].x, RUIN);
        }
        int corridor_x = corridor_x_global;
        for (y = 2; y < map_rows - 2; y++) {
          mvaddch(y, corridor_x, EMPTY);
          if (corridor_x + 1 < COLS) mvaddch(y, corridor_x + 1, EMPTY);
        }
        {
          int sx = corridor_x_global < shrine_x_global ? corridor_x_global : shrine_x_global;
          int ex = corridor_x_global > shrine_x_global ? corridor_x_global : shrine_x_global;
          for (x = sx; x <= ex; x++) {
            mvaddch(shrine_y, x, EMPTY);
            if (shrine_y + 1 < map_rows) mvaddch(shrine_y + 1, x, EMPTY);
          }
        }
        if (is_ruin_at(whisper_ruins, whisper_ruin_count, block1_y_global, corridor_x)) {
          mvaddch(block1_y_global, corridor_x, RUIN);
        }
        if (is_ruin_at(whisper_ruins, whisper_ruin_count, block2_y_global, corridor_x)) {
          mvaddch(block2_y_global, corridor_x, RUIN);
        }
      }
      mvaddch(shrine_y, shrine_x, relic_placed ? SHRINE_FILLED : SHRINE);
    }
}

void draw_hud(int region, bool gate_open, bool has_relic, bool relic_placed, bool has_key, bool moon_ruins_cleared, bool whisper_ruins_cleared)
{
    int hud_row = LINES - 1;
    mvhline(hud_row, 0, GRASS, COLS);
    if (relic_placed) {
      mvprintw(hud_row, 1, "The woods bloom with light. Your quest is complete. Press q to quit.");
    } else if (region == 0) {
      if (has_key) {
        mvprintw(hud_row, 1, "Meadow Road - key found. Head north to the moonlit ruins.");
      } else {
        mvprintw(hud_row, 1, "Meadow Road - enter the forest to find the key.");
      }
    } else if (region == 1) {
      if (!has_key) {
        mvprintw(hud_row, 1, "Moonlit Ruins - you need the key from the meadow.");
      } else if (!gate_open) {
        mvprintw(hud_row, 1, "Moonlit Ruins - use the key at the ruins/gate (press E).");
      } else if (!has_relic) {
        mvprintw(hud_row, 1, "Moonlit Ruins - claim the relic at the heart.");
      } else {
        mvprintw(hud_row, 1, "Moonlit Ruins - go north to the whispering woods.");
      }
    } else if (region == 2) {
      if (!has_relic) {
        mvprintw(hud_row, 1, "Whispering Woods - bring the relic from the south.");
      } else {
        mvprintw(hud_row, 1, "Whispering Woods - place the relic at the shrine (press E).");
      }
    }
}
