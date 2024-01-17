#pragma once
#include <cstdint>

bool init_application(int argc, char **argv, void **app, int *width, int *height, char **window_title);
bool update_application(void *app);
void handle_input(void *app);
bool render_application(void *app, uint32_t *pixels, int width, int height);

uint8_t *read_file(char *filename, uint64_t *file_size);
void message_box(char *title, char *msg);