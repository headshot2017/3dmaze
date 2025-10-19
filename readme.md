# 3dmaze

![image](screenshot.png)

This project aims to port the source code of the 3D Maze screensaver seen in Windows 9x over to multiple platforms:

| Platform       | Status      |
| -------------- | ----------- |
| Windows (SDL2) | Ported      |
| Linux (SDL2)   | Untested    |
| macOS (SDL2)   | Ported      |
| Nintendo DS    | Ported      |
| Sega Dreamcast | In progress |

Along with some additional objectives:

- [x] Make the maze playable

Contributions are welcome

## Third party libraries used

- stb_image
- freeglut

## Compiling

### For SDL2:

- Install the SDL2 and freeglut dependencies through a terminal with your package manager
- Build with cmake:

```
mkdir build-pc
cd build-pc
cmake ..
make
```

### For Nintendo DS:

- Install the [BlocksDS](https://blocksds.skylyrac.net/docs/setup/options/) SDK
- Build with `make -f Makefile.ds`
- Copy the "data" folder to the root of the SD card.
   - If you wish to build the game to use the NitroFS filesystem: `USE_NITRO=1 make -f Makefile.ds`

### For macOS:

- `brew install sdl2 freeglut glew xquartz`
- Build with `make`

```
open -a XQuartz
export DISPLAY=:0
./ssmaze
```
