# xHonerate
Terminal adventure game

Featuring:
Keyboard Piano


Use Arrow keys or WASD to move, leaving a trail of dots.
Press q to [q]uit.

Enjoy walking around :)

Packages used:
ncurses - used for stream of keyboard input in real time
fluidsynth - software synthesizer

Run to Compile - 
g++ -w -o xHonerate xHonerate.c `pkg-config fluidsynth --libs` -lcurses


## Mac Quickstart Setup
Install Homebrew 
```
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```
Install fluidsynth via Homebrew
```
brew install fluidsynth pkg-config
```

## Running the program
Compile the code
```
source compile.sh
```

Run xHonerate
```
./xHonerate
```
## Linux Quickstart Setup
Install fluidsynth
```
sudo apt-get install fluidsynth
sudo apt-get install libfluidsynth-dev
```

## Windows
- Download and install MSYS2
    - Get the installer from https://www.msys2.org/
    - Run it and accept defaults (e.g., C:\msys64)
- Update MSYS2 core packages
    - Open MSYS2 UCRT64 from the Start menu
    - Run:
```
pacman -Syu
```

- If it prompts to close and reopen the shell, do that, then run:

```
pacman -Syu
```
- Install build tools + deps
```
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-fluidsynth mingw-w64-ucrt-x86_64-ncurses
```
```
mingw-w64-ucrt-x86_64-pkgconf
```
- Run to Compile:
```
g++ -w -o xHonerate.exe xHonerate.c `pkg-config --libs --cflags ncursesw fluidsynth` -lncursesw
```
- Run to Play:
```
./xHonerate
```

