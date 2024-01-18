#pragma once
#include <cstdint>

struct Input_events {
    enum CODES {
        ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        ESC
    };

    enum STATE {
        DOWN = 1,
        UP = 1 << 1,
        HELD = 1 << 2
    };

    uint8_t event[255];
};

bool init_application(int argc, char **argv, void **app, int *width, int *height, char **window_title);
bool update_application(void *app, double frame_time);
void handle_input(void *app, Input_events &input_events);
bool render_application(void *app, uint32_t *pixels, int width, int height);

uint8_t *read_file(char *filename, uint64_t *file_size);
void message_box(char *title, char *msg);
void clear_console();