#include <math.h>
#include <time.h>
#include "raylib.h"

// Sudoku tiles have 9 possible states.
#define TILE_STATES (9)

#define BOARD_WIDTH (9)
#define BOARD_SIZE (BOARD_WIDTH * BOARD_WIDTH)
#define BOARD_PADDING (16)

#define TILE_SIZE (128)
#define TILE_CENTER (TILE_SIZE / 2)
#define BOX_SIZE (TILE_SIZE / 3)

#define BOARD_TEXTURE_SIZE (BOARD_WIDTH * TILE_SIZE + BOARD_PADDING * 2)

#define SCREEN_WIDTH (800)
#define SCREEN_HEIGHT (800)

// The factor by which the board texture is scaled to fit the window.
// The board texture is constant, but the window is resizable.
static float screen_scale = SCREEN_WIDTH / (float) BOARD_TEXTURE_SIZE;

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

// Check if a specific bit in a tile is set.
bool is_set(int x, int y, int bit) {
    return *get_tile(x, y) & (1 << bit);
}

// The entropy of a tile is the number of superpositions it has.
// This is the number of bits set in the tile's integer value.
// This function doesnt take x and y because
// it makes iterating through the tiles slightly easier (see solve_board).
int get_tile_entropy(int i) {
    int x = i % BOARD_WIDTH;
    int y = i / BOARD_WIDTH;
    int entropy = 0;
    for (int i = 0; i < TILE_STATES; ++i) {
        entropy += is_set(x, y, i);
    }
    return entropy;
}

int get_board_entropy(void) {
    int entropy = 0;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        entropy += get_tile_entropy(i);
    }
    return entropy;
}

// Check if a tile's superpositions only contain one value.
bool is_collapsed(int x, int y) {
    return get_tile_entropy(x + y * BOARD_WIDTH) == 1;
}

// Get the value of a collapsed tile.
int get_collapsed_value(int x, int y) {
    int value = *get_tile(x, y);
    // This mask is the first 9 bits set to 1.
    // This is used because the integer value can be larger than 9 bits
    // and if it is, anything above the first 9 bits should be ignored.
    int mask = (1 << TILE_STATES) - 1;

    // log2 gets the index of the set bit
    // because a binary number with only one bit set
    // is some power of 2.
    return (int) log2(value & mask);
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

// Constrain a tile's peers by removing a superposition.
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

// Solve the board by recusively collapsing the tile with the least entropy.
// Entropy, in this case, is the number of superpositions a tile has.
void solve_board(void) {
    // If the board entropy is the same as the size of the board,
    // every tile has been collapsed to a single value, and the board is solved.
    if (get_board_entropy() == BOARD_SIZE) return;

    int sorted_tiles[BOARD_SIZE];
    for (int i = 0; i < BOARD_SIZE; ++i) sorted_tiles[i] = i;

    // Sort the tiles by entropy.
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = i + 1; j < BOARD_SIZE; ++j) {
            int a = get_tile_entropy(sorted_tiles[i]);
            int b = get_tile_entropy(sorted_tiles[j]);
            if (a <= b) continue;

            int temp = sorted_tiles[i];
            sorted_tiles[i] = sorted_tiles[j];
            sorted_tiles[j] = temp;
        }
    }

    // Find the first tile in the sorted array that is not collapsed.
    for (int i = 0; i < BOARD_SIZE; ++i) {
        int tile = sorted_tiles[i];
        int x = tile % BOARD_WIDTH;
        int y = tile / BOARD_WIDTH;
        if (is_collapsed(x, y)) continue;

        // Find a random value that is in the tile's superpositions.
        int value = GetRandomValue(0, TILE_STATES - 1);
        while (!is_set(x, y, value)) {
            value = (value + 1) % TILE_STATES;
        }

        // Collapse the tile to the random value.
        collapse_tile(x, y, value);
    }

    // Recursively solve the rest of the board.
    solve_board();
}

// Draw a tile at a given board position.
void draw_tile(int x, int y) {
    int tile_x = x * TILE_SIZE + BOARD_PADDING;
    int tile_y = y * TILE_SIZE + BOARD_PADDING;

    // Draw the tile's border.
    DrawRectangleLines(tile_x, tile_y, TILE_SIZE, TILE_SIZE, BLACK);

    // If the tile is collapsed, draw the collapsed value at the center.
    if (is_collapsed(x, y)) {
        int x_center = tile_x + TILE_CENTER;
        int y_center = tile_y + TILE_CENTER;
        const char *text = TextFormat("%d", get_collapsed_value(x, y) + 1);
        int text_width = MeasureText(text, 48);
        DrawText(text, x_center - text_width / 2, y_center - 24, 48, BLACK);
        return;
    }

    // If the tile is not collapsed, draw the remaining superpositions.
    for (int bit = 0; bit < TILE_STATES; ++bit) {
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
        // NOTE: I would like if this were handled in the main loop instead,
        // but I already had the tile positions available here.
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
            BOARD_PADDING, BOARD_PADDING + i * TILE_SIZE,
            BOARD_WIDTH * TILE_SIZE, 6
        };
        DrawRectangleLinesEx(rect, 6, BLACK);

        rect = (Rectangle) {
            BOARD_PADDING + i * TILE_SIZE, BOARD_PADDING,
            6, BOARD_WIDTH * TILE_SIZE
        };
        DrawRectangleLinesEx(rect, 6, BLACK);
    }
}

int main(void) {
    SetRandomSeed(time(0));

    int width = SCREEN_WIDTH;
    int height = SCREEN_HEIGHT;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, "Sudoku WFC");
    SetTargetFPS(60);

    // Initialize the tiles to potentially be any number.
    reset_tiles();

    // Create a render texture to draw the board to.
    int board_size = BOARD_TEXTURE_SIZE;
    RenderTexture2D board = LoadRenderTexture(board_size, board_size);

    // The source rectangle is the size of the board texture.
    // The height component is negative because OpenGL's coordinate system
    // uses the bottom-left corner as the origin instead of the top-left.
    Rectangle source = { 0, 0, BOARD_TEXTURE_SIZE, -BOARD_TEXTURE_SIZE };

    // The destination rectangle is the entire size of the window.
    Rectangle destination = { 0, 0, width, height };

    // Camera2D objects to handle zooming and panning.
    // In this case, they don't do anything special
    // besides being the target of the render textures.
    Camera2D board_camera = { .zoom = 1.f };
    Camera2D screen_camera = { .zoom = 1.f };

    static const Vector2 origin = { 0, 0 };

    while (!WindowShouldClose()) {
        // Update the title to contain the time it took to show the last frame.
        float frame_ms = GetFrameTime() * 1000.f;
        SetWindowTitle(TextFormat("Sudoku WFC - %.2f ms/frame", frame_ms));

        // If the window is resized, adjust the render texture's size,
        // and update the destination rectangle to fit the new window size.
        if (IsWindowResized()) {
            // Get the new window size.
            width = GetScreenWidth();
            height = GetScreenHeight();

            // Ideally I would just center the board in the window,
            // and scale it to fit the smallest window dimension,
            // but for now I'll just force the window to be square.

            // Get the smallest window dimension.
            int min_size = width < height ? width : height;

            // Set the window size to be square
            // with the window's smallest dimension.
            SetWindowSize(min_size, min_size);

            // Get the new window size again,
            // because It was just changed to be square.
            width = GetScreenWidth();
            height = GetScreenHeight();

            // Update the screen scale and destination rectangle.
            screen_scale = width / (float) BOARD_TEXTURE_SIZE;
            destination = (Rectangle) { 0, 0, width, height };
        }

        // Start drawing to the render texture.
        BeginTextureMode(board);
            // Draw the board to the render texture.
            BeginMode2D(board_camera);
                ClearBackground(RAYWHITE);
                draw_board();
            EndMode2D();
        EndTextureMode();

        // Start drawing to the window.
        BeginDrawing();
            ClearBackground(RAYWHITE);
            // Draw the render texture to the window.
            BeginMode2D(screen_camera);
                DrawTexturePro(
                    board.texture,
                    source, destination,
                    origin, 0,
                    WHITE
                );
            EndMode2D();
        EndDrawing();

        // Handle user input.
        if (IsKeyPressed(KEY_Z)) undo_tiles();
        if (IsKeyPressed(KEY_R)) reset_tiles();
        if (IsKeyPressed(KEY_S)) {
            if (get_board_entropy() == BOARD_SIZE) reset_tiles();
            solve_board();
        }
    }

    // Release the render texture and close the window.
    UnloadRenderTexture(board);
    CloseWindow();

    return 0;
}
