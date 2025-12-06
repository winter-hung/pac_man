# Pac-Man Game

A simple Pac-Man clone built with C++ and SDL2.

## Overview

This is a 2D Pac-Man game implementation that features:

- Classic Pac-Man gameplay with ghosts and dots
- SDL2 graphics rendering
- Tile-based map system
- Basic collision detection
- Ghost AI with random movement

## Files

- `pacman.cpp` - Original implementation of the game
  - Pac-Man mouth animation
  - Centered ghost movement
  - Improved collision detection
  - Better tile-based movement system
- `run.sh` - Compilation script

## Controls

- **Arrow Keys**: Move Pac-Man (Up, Down, Left, Right)
- **ESC**: Quit the game

## How to Play

1. Eat all dots to win
2. Avoid ghosts
3. Navigate through the maze using the arrow keys

## Building the Game

The project requires SDL2 library to be installed. Use the provided run script to compile:

```bash
./run.sh
```

This will compile the improved version (`pacman_fix.cpp`) using g++ with SDL2 libraries.

## Requirements

- C++17 compiler (g++)
- SDL2 library
- macOS (paths in run.sh are configured for Homebrew SDL2 installation)

## Game Features

- Simple 2D graphics with colored circles and rectangles
- Real-time gameplay with delta time for consistent speed
- Multiple ghosts with different colors
- Score tracking based on dots eaten
- Win/lose conditions
