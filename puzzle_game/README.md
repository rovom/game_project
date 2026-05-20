# 🌌 Interstellar Puzzle — SDL2 C Game

A drag-and-drop puzzle game built in C with SDL2.  
The player must identify the correct missing piece of a space image and drag it into the gap.

---

## 📁 Project Structure

```
puzzle_game/
├── main.c          ← Entry point & game loop
├── game.c          ← All game logic, rendering, event handling
├── game.h          ← Public interface (structs, function declarations)
├── assets.h        ← All asset paths and layout constants
├── Makefile        ← Build system
├── README.md       ← This file
└── assets/
    ├── fonts/
    │   ├── SpaceMono-Regular.ttf
    │   └── SpaceMono-Bold.ttf
    ├── images/     ← (auto-generated procedurally at runtime)
    └── sounds/     ← optional WAV/MP3 files
```

---

## 🔧 Dependencies

| Library        | Package (Ubuntu/Debian)    |
|----------------|---------------------------|
| SDL2           | `libsdl2-dev`             |
| SDL2_image     | `libsdl2-image-dev`       |
| SDL2_ttf       | `libsdl2-ttf-dev`         |
| SDL2_mixer     | `libsdl2-mixer-dev` *(optional)* |

Install all at once:
```bash
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

---

## 🏗️ Build

```bash
# Default build (no sound)
make

# Or manually:
gcc main.c game.c -lSDL2 -lSDL2_image -lSDL2_ttf -lm -o puzzle

# With SDL2_mixer sound support:
gcc main.c game.c -DUSE_MIXER -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lm -o puzzle
```

---

## 🎮 Controls

| Input                        | Action                         |
|------------------------------|-------------------------------|
| **Left click + drag**        | Pick up and move a piece       |
| **Release mouse button**     | Drop piece (snaps if close)    |
| **R**                        | Restart / reshuffle            |
| **ESC** (during play)        | Show quit screen               |
| **ESC** (on result screen)   | Quit game                      |

---

## 🎯 Gameplay

1. A space image is displayed with a **missing piece** (shown as a glowing cyan dashed rectangle).
2. Three candidate pieces appear in the **bottom tray** — only one fits correctly.
3. **Drag** the piece you think is correct into the missing slot.
4. If it overlaps the target zone by ≥ 55%, it snaps:
   - ✅ **Correct** → "PUZZLE SOLVED!" message + green glow
   - ❌ **Wrong** → "WRONG PIECE" message + red screen
5. A **shrinking colour bar** at the bottom counts down 60 seconds.  
   Run out of time → "TIME OUT" message.

---

## 🖼️ Assets

All visual assets are **procedurally generated at runtime** using SDL2's renderer:

- **Space background**: deep-navy gradient, two nebula blobs, a ringed gas giant, and a black hole with accretion disk
- **Puzzle hole**: the missing region is cut from the black-hole area
- **Correct piece**: exact crop of the background at the missing location
- **Wrong pieces**: crops from different areas with a colour tint overlay

No external image files are required.

---

## 🔊 Optional Sound

Place WAV files in `assets/sounds/`:
```
assets/sounds/success.wav
assets/sounds/fail.wav
assets/sounds/pick.wav
```
Then rebuild with `-DUSE_MIXER` flag (see above).

---

## 🧠 Architecture

| File       | Responsibility                                                  |
|------------|----------------------------------------------------------------|
| `assets.h` | All magic numbers, file paths, window/layout constants         |
| `game.h`   | `GameContext` struct, `Piece` struct, function signatures       |
| `game.c`   | `game_init`, `game_destroy`, `game_update`, `game_render`, `game_handle_event`, procedural texture generators |
| `main.c`   | SDL event loop, calls game_* functions, handles quit           |

---

## 📌 Key SDL2 Concepts Used

- `SDL_CreateTexture` + `SDL_TEXTUREACCESS_TARGET` — off-screen rendering for procedural assets
- `SDL_RenderCopy` with source `SDL_Rect` — cropping pieces from the background
- `SDL_BLENDMODE_BLEND` — alpha transparency for overlays, shadows, glow effects
- `SDL_PollEvent` — non-blocking event loop
- `TTF_RenderText_Blended` — anti-aliased text rendering
- Double buffering via `SDL_RenderPresent`
