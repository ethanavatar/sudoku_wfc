#include <math.h>
#include "raylib.h"

#define BOARD_WIDTH 9
#define BOARD_SIZE (BOARD_WIDTH * BOARD_WIDTH)

#define TILE_SIZE 128
#define BOARD_PADDING 16

#define SCREEN_WIDTH (BOARD_WIDTH * TILE_SIZE + BOARD_PADDING * 2)

static int last_tiles[BOARD_SIZE] = { 0 };

// The tiles are each stored as an integer,
// with the first 9 bits representing its superpositions.
// If a bit is set, the tile is allowed to be that number.
static int tiles[BOARD_SIZE] = { 0 };

// Reset all tiles to be a superposition of 1-9.
void reset_tiles(void) {
    // 0x1FF sets the first 9 bits to 1.
    // This indicates that the tile can be any number.
    for (int i = 0; i < BOARD_SIZE; ++i) tiles[i] = 0x1FF;
}

// Get a pointer to a tile at a given position.
int *get_tile(int x, int y) {
    return &tiles[y * BOARD_WIDTH + x];
}

// Check if a tile's superpositions only contain one value.
bool is_collapsed(int x, int y) {
    int value = *get_tile(x, y);
    return (value & (value - 1)) == 0;
}

// Forwards declaration for recursion.
void constrain_peers(int x, int y, int value);

// Remove a superposition from a tile
// by unsetting the bit at the index of the value.
void constrain_tile(int x, int y, int value) {
    *get_tile(x, y) &= ~(1 << value);

    // If the tile has *newly* been collapsed as a result of this,
    // propagate the constraint to its peers.
    if (is_collapsed(x, y)) {
        int collapsed_value = (int) log2(*get_tile(x, y));
        constrain_peers(x, y, collapsed_value);
    }
}

void constrain_peers(int x, int y, int value) {
    // Constrain the row that the tile belongs to.
    for (int i = 0; i < BOARD_WIDTH; ++i) {
        if (i == x) continue;
        if (is_collapsed(i, y)) continue;
        constrain_tile(i, y, value);
    }

    // Constrain the column that the tile belongs to.
    for (int i = 0; i < BOARD_WIDTH; ++i) {
        if (i == y) continue;
        if (is_collapsed(x, i)) continue;
        constrain_tile(x, i, value);
    }

    // Constrain the 3x3 box that the tile belongs to.
    int box_x = x / 3 * 3;
    int box_y = y / 3 * 3;
    for (int i = box_y; i < box_y + 3; ++i) {
        for (int j = box_x; j < box_x + 3; ++j) {
            if (i == y && j == x) continue;
            if (is_collapsed(j, i)) continue;
            constrain_tile(j, i, value);
        }
    }
}

// Collapse a tile to a single value.
void collapse_tile(int x, int y, int value) {
    for (int i = 0; i < BOARD_SIZE; ++i) last_tiles[i] = tiles[i];
    int index = y * BOARD_WIDTH + x;
    tiles[index] = 1 << value;
    constrain_peers(x, y, value);
}


void draw_tile(int x, int y) {
    int value = *get_tile(x, y);
    int tile_x = x * TILE_SIZE + BOARD_PADDING;
    int tile_y = y * TILE_SIZE + BOARD_PADDING;

    DrawRectangleLines(tile_x, tile_y, TILE_SIZE, TILE_SIZE, BLACK);

    if (is_collapsed(x, y)) {
        int collapsed_value = (int) log2(value) + 1;
        int tile_x_center = tile_x + TILE_SIZE / 2;
        int tile_y_center = tile_y + TILE_SIZE / 2;
        const char *text = TextFormat("%d", collapsed_value);
        int text_width = MeasureText(text, 48);
        DrawText(text, tile_x_center - text_width / 2, tile_y_center - 24, 48, BLACK);
        return;
    }

    Vector2 mouse_position = GetMousePosition();

    for (int iter = 0; iter < 9; ++iter) {
        int i = iter / 3;
        int j = iter % 3;

        int bit = i * 3 + j;
        int is_set = value & (1 << bit);

        if (!is_set) continue;

        int subtile_x = tile_x + j * TILE_SIZE / 3;
        int subtile_y = tile_y + i * TILE_SIZE / 3;

        Rectangle subtile_rect = {
            subtile_x,
            subtile_y,
            TILE_SIZE / 3,
            TILE_SIZE / 3
        };
        bool is_hovered = CheckCollisionPointRec(mouse_position, subtile_rect);
        if (is_hovered) {
            DrawRectangle(subtile_x, subtile_y, TILE_SIZE / 3, TILE_SIZE / 3, LIGHTGRAY);
        }

        // If the user clicks on a subtile, collapse the tile to the value of the subtile.
        if (is_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            collapse_tile(x, y, bit);
        }

        const char *text = TextFormat(" %d", bit + 1);
        DrawText(text, subtile_x, subtile_y + 8, 32, GRAY);
    }
}

void draw_board(void) {
    for (int y = 0; y < BOARD_WIDTH; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            draw_tile(x, y);
        }
    }

    // Draw thicker lines to separate the 3x3 boxes.
    for (int i = 0; i <= BOARD_WIDTH; i += 3) {
        Rectangle rect = {
            BOARD_PADDING,
            BOARD_PADDING + i * TILE_SIZE,
            BOARD_WIDTH * TILE_SIZE,
            6
        };
        DrawRectangleLinesEx(rect, 6, BLACK);

        rect = (Rectangle) {
            BOARD_PADDING + i * TILE_SIZE,
            BOARD_PADDING,
            6,
            BOARD_WIDTH * TILE_SIZE
        };
        DrawRectangleLinesEx(rect, 6, BLACK);
    }
}

int main(void) {
    const char *title = "Sudoku WFC";
    InitWindow(SCREEN_WIDTH, SCREEN_WIDTH, title);
    SetTargetFPS(60);

    reset_tiles();

    while (!WindowShouldClose()) {
        float frame_ms = GetFrameTime() * 1000.f;
        SetWindowTitle(TextFormat("Sudoku WFC - %.2f ms/frame", frame_ms));

        BeginDrawing();
            ClearBackground(RAYWHITE);
            draw_board();

            if (IsKeyPressed(KEY_Z)) {
                for (int i = 0; i < BOARD_SIZE; i++) tiles[i] = last_tiles[i];
            }

            if (IsKeyPressed(KEY_R)) {
                reset_tiles();
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
