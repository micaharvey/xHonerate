/*  Keyboard Piano

    ncurses - used for stream of keyboard input in real time
    fluidsynth - software synthesizer

    compile - g++ -w -o xHonerate xHonerate.c `pkg-config fluidsynth --libs` -lcurses
*/
#include <ncurses.h>
#include <fluidsynth.h>
#include <stdlib.h>
#include <iostream>
#include "xHonerate.h"

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

    // input and note variables
    int _inputChar;
    int _channel = 0;
    int _note = 48;
    int _velocity = 111;
    int _program = 13;

    // Create the synth and apply settings.
    _settings = new_fluid_settings();
    _synth = new_fluid_synth(_settings);
    _adriver = new_fluid_audio_driver(_settings, _synth);
    _sfont_id = fluid_synth_sfload(_synth, "smiles.sf2", 1);

    // Channel 1 program
    fluid_synth_program_select(_synth, _channel, _sfont_id, 0, _program);

    // init ncurses for keyboard input
    initscr();

    // grant ability to read arrow keays
    keypad(stdscr, TRUE);

    // added for game
    cbreak();
    noecho();

    clear();

    /* initialize the quest map */
    draw_map();

    /* start player at lower-left */
    y = LINES - 1;
    x = 0;

    /*-------------------\
    |----- MAIN LOOP ----|
    \-------------------*/
    while (true) {
      /* by default, you get a blinking cursor - use it to indicate player */
      mvaddch(y, x, PLAYER);
      move(y, x);

      refresh();

      // wait for keyboard input
      _inputChar = getch();

      // [Q]UIT on 'q' press
      if (_inputChar == 'q') break;

      /* test inputted key and determine direction */
      switch (_inputChar) {
        case KEY_UP:
        case 'w':
        case 'W':
            _note = 60;
            if ((y > 0) && is_move_okay(y - 1, x)) {
              mvaddch(y, x, EMPTY);
              y = y - 1;
            }
            break;
        case KEY_DOWN:
        case 's':
        case 'S':
            _note = 48;
            if ((y < LINES - 1) && is_move_okay(y + 1, x)) {
              mvaddch(y, x, EMPTY);
              y = y + 1;
            }
            break;
        case KEY_LEFT:
        case 'a':
        case 'A':
            _note = 55;
            if ((x > 0) && is_move_okay(y, x - 1)) {
              mvaddch(y, x, EMPTY);
              x = x - 1;
            }
            break;
        case KEY_RIGHT:
        case 'd':
        case 'D':
            _note = 51;
            if ((x < COLS - 1) && is_move_okay(y, x + 1)) {
              mvaddch(y, x, EMPTY);
              x = x + 1;
            }
            break;
        }

        playNote(_synth, _channel, _note, _velocity);
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


/*--------------------------\
|-FUNCTION IMPLEMENTATIONS -|
\--------------------------*/
// Play a note on the synth.  This will only do a "stab" not a held note currently.
void playNote(fluid_synth_t* synth, int channel, int note, int velocity) {
  /* Play a note */
  fluid_synth_noteon(synth, channel, note, velocity);
  /* Stop the note */
  fluid_synth_noteoff(synth, channel, note);
}

int is_move_okay(int y, int x)
{
    int testch;

    /* return true if the space is okay to move into */

    testch = mvinch(y, x);
    return ((testch == GRASS) || (testch == EMPTY));
}

void draw_map(void)
{
    int y, x;

    /* draw the quest map */

    /* background */

    for (y = 0; y < LINES; y++) {
      mvhline(y, 0, GRASS, COLS);
    }

    /* mountains, and mountain path */

    for (x = COLS / 2; x < COLS * 3 / 4; x++) {
      mvvline(0, x, MOUNTAIN, LINES);
    }

    mvhline(LINES / 4, 0, GRASS, COLS);

    /* lake */

    for (y = 1; y < LINES / 2; y++) {
      mvhline(y, 1, WATER, COLS / 3);
    }
}
