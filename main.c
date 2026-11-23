#include "game.h"
#include "ui.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    UIState   ui;
    GameState gs;

    memset(&gs, 0, sizeof(gs));

    if (!ui_init(&ui)) {
        fprintf(stderr, "Failed to initialise UI.\n");
        return 1;
    }

    int quit = 0;
    while (!quit) {
        ui_handle_events(&ui, &gs, &quit);
        ui_render(&ui, &gs);
        SDL_Delay(16); /* ~60 fps */
    }

    ui_destroy(&ui);
    return 0;
}
