#include <math.h>
#include "raylib.h"

#define BOARD_WIDTH 9
#define BOARD_SIZE (BOARD_WIDTH * BOARD_WIDTH)

#define TILE_SIZE 128
#define TILE_CENTER (TILE_SIZE / 2)
#define BOX_SIZE (TILE_SIZE / 3)
#define BOARD_PADDING 16

#define BOARD_TEXTURE_WIDTH (BOARD_WIDTH * TILE_SIZE + BOARD_PADDING * 2)

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

static float screen_scale = SCREEN_WIDTH / (float) BOARD_TEXTURE_WIDTH;

// The tiles are each stored as an integer,
// with the first 9 bits representing its superpositions.
// If a bit is set, the tile is allowed to be that number.
static int tiles[BOARD_SIZE] = { 0 };

// A place to store the previous state of the tiles.
// TODO: A real undo system that can go back multiple steps.
static int last_tiles[BOARD_SIZE] = { 0 };

// Reset all tiles to be a superposition of 1-9.
void reset_tiles(void) {
    // 0x1FF sets the first 9 bits to 1.
    // This indicates that the tile can be any number.
    for (int i = 0; i < BOARD_SIZE; ++i) tiles[i] = 0x1FF;
}

// Undo the last change to the tiles.
void undo_tiles(void) {
    // Copy the last state of the tiles back to the current state.
    for (int i = 0; i < BOARD_SIZE; ++i) tiles[i] = last_tiles[i];
}

// Get a pointer to a tile at a given position.
int *get_tile(int x, int y) {
    // The tiles are stored in a 1D array.
    // Multiplying by BOARD_WIDTH gets the row,
    // and adding the column gets the 1D index of the tile.
    return &tiles[y * BOARD_WIDTH + x];
}

// Check if a tile's superpositions only contain one value.
bool is_collapsed(int x, int y) {
    int value = *get_tile(x, y);
    return (value & (value - 1)) == 0;
}

bool is_set(int x, int y, int bit) {
    return *get_tile(x, y) & (1 << bit);
}

// Get the value of a collapsed tile.
int get_collapsed_value(int x, int y) {
    // Using log2 gets the index of the set bit
    // because a binary number with only one bit set
    // is some power of 2.
    return (int) log2(*get_tile(x, y));
}

// Forwards declaration for recursion.
void constrain_peers(int x, int y, int value);

// Remove a superposition from a tile
// by unsetting the bit at the index of the value.
void constrain_tile(int x, int y, int value) {
    *get_tile(x, y) &= ~(1 << value);

    // If the tile was just collapsed because of this constraint,
    // propagate the constraint to its peers.
    if (is_collapsed(x, y)) constrain_peers(x, y, get_collapsed_value(x, y));
}

// Constrain the superpositions of a tile's peers by removing a possible value.
void constrain_peers(int x, int y, int value) {
    // Constrain the row and column that the tile belongs to.
    for (int i = 0; i < BOARD_WIDTH; ++i) {
        // Skip the tile that set the constraint.
        if (i == x || i == y) continue;

        // Skip tiles that are already collapsed.
        if (!is_collapsed(i, y)) constrain_tile(i, y, value);
        if (!is_collapsed(x, i)) constrain_tile(x, i, value);
    }

    // Successively dividing and multiplying by 3,
    // effectively rounds down to the nearest multiple of 3.
    // This gives us the index of the top-left tile in the box.
    int box_x = x / 3 * 3;
    int box_y = y / 3 * 3;

    // Constrain the 3x3 box that the tile belongs to.
    for (int i = box_y; i < box_y + 3; ++i) {
        for (int j = box_x; j < box_x + 3; ++j) {
            // Skip the tile that set the constraint.
            if (i == y && j == x) continue;

            // Skip tiles that are already collapsed.
            if (!is_collapsed(j, i)) constrain_tile(j, i, value);
            
        }
    }
}

// Collapse a tile to a single value.
void collapse_tile(int x, int y, int value) {
    // Store the last board state for undo.
    for (int i = 0; i < BOARD_SIZE; ++i) last_tiles[i] = tiles[i];

    // Set the tile to only contain the collapsed value.
    *get_tile(x, y) = 1 << value;

    // Constrain the superpositions of the tile's in the same
    // row, column, and 3x3 box to not contain the collapsed value.
    constrain_peers(x, y, value);
}

// Draw a tile at a given board position.
void draw_tile(int x, int y) {
    int tile_x = x * TILE_SIZE + BOARD_PADDING;
    int tile_y = y * TILE_SIZE + BOARD_PADDING;

    // Draw the tile's border.
    DrawRectangleLines(tile_x, tile_y, TILE_SIZE, TILE_SIZE, BLACK);

    // If the tile is collapsed, draw the collapsed value at the center.
    if (is_collapsed(x, y)) {
        int tile_x_center = tile_x + TILE_CENTER;
        int tile_y_center = tile_y + TILE_CENTER;
        const char *text = TextFormat("%d", get_collapsed_value(x, y) + 1);
        int text_width = MeasureText(text, 48);
        DrawText(text, tile_x_center - text_width / 2, tile_y_center - 24, 48, BLACK);
        return;
    }

    // If the tile is not collapsed, draw the remaining superpositions.
    for (int bit = 0; bit < 9; ++bit) {
        // Do not draw the superposition if it is not set.
        if (!is_set(x, y, bit)) continue;

        int subtile_x = tile_x + bit / 3 * BOX_SIZE;
        int subtile_y = tile_y + bit % 3 * BOX_SIZE;

        Rectangle subtile_rect = {
            subtile_x, subtile_y,
            BOX_SIZE, BOX_SIZE
        };

        // Get the mouse position, scaled according to the screen scale.
        Vector2 mouse_pos = GetMousePosition();
        mouse_pos.x /= screen_scale;
        mouse_pos.y /= screen_scale;

        // Highlight the subtile if the mouse is hovering over it.
        bool is_hovered = CheckCollisionPointRec(mouse_pos, subtile_rect);
        if (is_hovered) DrawRectangleRec(subtile_rect, LIGHTGRAY);

        // Draw the number of the superposition at the center of the subtile.
        const char *text = TextFormat(" %d", bit + 1);
        DrawText(text, subtile_x, subtile_y + 8, 32, GRAY);

        // If the user clicks on a subtile,
        // collapse the tile to the value of the subtile.
        if (is_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            collapse_tile(x, y, bit);
        }
    }
}

// Draw the entire board.
void draw_board(void) {
    // Draw all tiles on the board.
    for (int iter = 0; iter < BOARD_SIZE; ++iter) {
        draw_tile(iter % BOARD_WIDTH, iter / BOARD_WIDTH);
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

void update(void) {
}

int main(void) {
    int width = SCREEN_WIDTH;
    int height = SCREEN_HEIGHT;
    const char *title = "Sudoku WFC";
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, title);
    SetTargetFPS(60);

    reset_tiles();

    screen_scale = width / (float) BOARD_TEXTURE_WIDTH;
    RenderTexture2D board_texture = LoadRenderTexture(BOARD_TEXTURE_WIDTH, BOARD_TEXTURE_WIDTH);

    Rectangle source = { 0, 0, BOARD_TEXTURE_WIDTH, -BOARD_TEXTURE_WIDTH };
    Rectangle dest = { screen_scale, screen_scale, width, height };

    Camera2D board_camera = { 0 };
    board_camera.zoom = 1.f;

    Camera2D screen_camera = { 0 };
    screen_camera.zoom = 1.f;

    while (!WindowShouldClose()) {
        // Update the title to contain the time it took to show the last frame.
        float frame_ms = GetFrameTime() * 1000.f;
        SetWindowTitle(TextFormat("Sudoku WFC - %.2f ms/frame", frame_ms));

        if (IsWindowResized()) {
            width = GetScreenWidth();
            height = GetScreenHeight();
            int min_size = width < height ? width : height;
            SetWindowSize(min_size, min_size);
            width = GetScreenWidth();
            height = GetScreenHeight();
            screen_scale = width / (float) BOARD_TEXTURE_WIDTH;
            dest = (Rectangle) { screen_scale, screen_scale, width, height };
        }

        BeginTextureMode(board_texture);
            BeginMode2D(board_camera);
                ClearBackground(RAYWHITE);
                draw_board();
            EndMode2D();
        EndTextureMode();


        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode2D(screen_camera);
                DrawTexturePro(board_texture.texture, source, dest, (Vector2) { 0, 0 }, 0, WHITE);
            EndMode2D();
            if (IsKeyPressed(KEY_Z)) undo_tiles();
            if (IsKeyPressed(KEY_R)) reset_tiles();
        EndDrawing();
    }

    UnloadRenderTexture(board_texture);
    CloseWindow();

    return 0;
}
