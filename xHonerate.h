// HEADER FOR DEMO PROGRAM
//   Here be Constants, Global Variables, Structs, and Function References
//   Oh also a copy/paste of the general midi specification

/* Constants */
const unsigned int DRUM_CHANNEL = 9;

#define GRASS     ' '
#define EMPTY     '.'
#define WATER     '~'
#define MOUNTAIN  '^'
#define PLAYER    '*'

/* Structs */
struct padSetting {
  int channel;
  int program;
};

/* Funcion References */
void playNote(fluid_synth_t* synth, int channel, int key, int velocity);

// game
int is_move_okay(int y, int x);
void draw_map(void);
void draw_new_map(void);

/*---------------------------\
| GENERAL MIDI SPECIFICATION |
\----------------------------/
Piano
1 Acoustic Grand Piano
2 Bright Acoustic Piano
3 Electric Grand Piano
4 Honky-tonk Piano
5 Electric Piano 1
6 Electric Piano 2
7 Harpsichord
8 Clavinet

Chromatic Percussion
9 Celesta
10 Glockenspiel
11 Music Box
12 Vibraphone
13 Marimba
14 Xylophone
15 Tubular Bells
16 Dulcimer

Organ
17 Drawbar Organ
18 Percussive Organ
19 Rock Organ
20 Church Organ
21 Reed Organ
22 Accordion
23 Harmonica
24 Tango Accordion

Guitar
25 Acoustic Guitar (nylon)
26 Acoustic Guitar (steel)
27 Electric Guitar (jazz)
28 Electric Guitar (clean)
29 Electric Guitar (muted)
30 Overdriven Guitar
31 Distortion Guitar
32 Guitar Harmonics

Bass
33 Acoustic Bass
34 Electric Bass (finger)
35 Electric Bass (pick)
36 Fretless Bass
37 Slap Bass 1
38 Slap Bass 2
39 Synth Bass 1
40 Synth Bass 2

Strings
41 Violin
42 Viola
43 Cello
44 Contrabass
45 Tremolo Strings
46 Pizzicato Strings
47 Orchestral Harp
48 Timpani

Ensemble
49 String Ensemble 1
50 String Ensemble 2
51 Synth Strings 1
52 Synth Strings 2
53 Choir Aahs
54 Voice Oohs
55 Synth Choir
56 Orchestra Hit

Brass
57 Trumpet
58 Trombone
59 Tuba
60 Muted Trumpet
61 French Horn
62 Brass Section
63 Synth Brass 1
64 Synth Brass 2

Reed
65 Soprano Sax
66 Alto Sax
67 Tenor Sax
68 Baritone Sax
69 Oboe
70 English Horn
71 Bassoon
72 Clarinet

Pipe
73 Piccolo
74 Flute
75 Recorder
76 Pan Flute
77 Blown bottle
78 Shakuhachi
79 Whistle
80 Ocarina

Synth Lead
81 Lead 1 (square)
82 Lead 2 (sawtooth)
83 Lead 3 (calliope)
84 Lead 4 (chiff)
85 Lead 5 (charang)
86 Lead 6 (voice)
87 Lead 7 (fifths)
88 Lead 8 (bass + lead)

Synth Pad
89 Pad 1 (new age)
90 Pad 2 (warm)
91 Pad 3 (polysynth)
92 Pad 4 (choir)
93 Pad 5 (bowed)
94 Pad 6 (metallic)
95 Pad 7 (halo)
96 Pad 8 (sweep)

Synth Effects
97 FX 1 (rain)
98 FX 2 (soundtrack)
99 FX 3 (crystal)
100 FX 4 (atmosphere)
101 FX 5 (brightness)
102 FX 6 (goblins)
103 FX 7 (echoes)
104 FX 8 (sci-fi)

Ethnic
105 Sitar
106 Banjo
107 Shamisen
108 Koto
109 Kalimba
110 Bagpipe
111 Fiddle
112 Shanai

Percussive
113 Tinkle Bell
114 Agogo
115 Steel Drums
116 Woodblock
117 Taiko Drum
118 Melodic Tom
119 Synth Drum
120 Reverse Cymbal

Sound effects
121 Guitar Fret Noise
122 Breath Noise
123 Seashore
124 Bird Tweet
125 Telephone Ring
126 Helicopter
127 Applause
128 Gunshot


Percussion

GM Standard Drum Map
In GM standard MIDI files, channel 10 is reserved for percussion instruments only. Notes recorded on channel 10 always produce percussion sounds when transmitted to a keyboard or synth module which uses the GM standard. Each of the 128 different possible note numbers correlate to a unique percussive instrument, but the sound's pitch is not relative to the note number.

If a MIDI file is programmed to the General MIDI protocol, then the results are predictable, but the sound fidelity may vary depending on the quality of the GM synthesizer:

35 Bass Drum 2
36 Bass Drum 1
37 Side Stick/Rimshot
38 Snare Drum 1
39 Hand Clap
40 Snare Drum 2
41 Low Tom 2
42 Closed Hi-hat
43 Low Tom 1
44 Pedal Hi-hat
45 Mid Tom 2
46 Open Hi-hat
47 Mid Tom 1
48 High Tom 2
49 Crash Cymbal 1
50 High Tom 1
51 Ride Cymbal 1
52 Chinese Cymbal
53 Ride Bell
54 Tambourine
55 Splash Cymbal
56 Cowbell
57 Crash Cymbal 2
58 Vibra Slap
59 Ride Cymbal 2
60 High Bongo
61 Low Bongo
62 Mute High Conga
63 Open High Conga
64 Low Conga
65 High Timbale
66 Low Timbale
67 High Agogô
68 Low Agogô
69 Cabasa
70 Maracas
71 Short Whistle
72 Long Whistle
73 Short Güiro
74 Long Güiro
75 Claves
76 High Wood Block
77 Low Wood Block
78 Mute Cuíca
79 Open Cuíca
80 Mute Triangle
81 Open Triangle


Controller events
In MIDI, adjustable parameters for each of the 16 possible MIDI channels may be set with the Control Change message, which has a Control Number parameter and a Control Value parameter. GM also specifies which operations should be performed by multiple Control Numbers:[1]

1 Modulation wheel
7 Volume
10 Pan
11 Expression
64 Sustain pedal
100 Registered Parameter Number LSB
101 Registered Parameter Number MSB
121 All controllers off
123 All notes off
-----------------------------------*/