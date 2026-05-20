#ifndef ASSETS_H
#define ASSETS_H

#define FONT_PATH        "puzzle_game/assets/fonts/SpaceMono-Regular.ttf"
#define FONT_BOLD_PATH   "puzzle_game/assets/fonts/SpaceMono-Bold.ttf"

#define BG_IMAGE_PATHS { \
    "puzzle_game/assets/images/space_bg.png", \
    "puzzle_game/assets/images/space_bg2.png", \
    "puzzle_game/assets/images/space_bg3.png", \
    "puzzle_game/assets/images/space_bg4.png" \
}
#define BG_IMAGE_COUNT 4

#define SFX_SUCCESS_PATH "puzzle_game/assets/sounds/success.wav"
#define SFX_FAIL_PATH    "puzzle_game/assets/sounds/fail.wav"
#define SFX_PICK_PATH    "puzzle_game/assets/sounds/pick.wav"

#define WINDOW_TITLE  "Interstellar Puzzle"
#define WINDOW_W      1024
#define WINDOW_H      680

#define MISSING_W     160
#define MISSING_H     160

#define MISSING_MIN_X  40
#define MISSING_MAX_X  (WINDOW_W - MISSING_W - 40)
#define MISSING_MIN_Y  40
#define MISSING_MAX_Y  (TRAY_Y - MISSING_H - 40)

#define TRAY_Y        490
#define TRAY_PIECE_W  MISSING_W
#define TRAY_PIECE_H  MISSING_H
#define TRAY_GAP      40

#define TIMER_BAR_X   40
#define TIMER_BAR_Y   640
#define TIMER_BAR_W   (WINDOW_W - 80)
#define TIMER_BAR_H   14
#define TIMER_SECONDS 60

#endif
