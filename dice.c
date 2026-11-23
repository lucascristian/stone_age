#include "game.h"
#include <stdarg.h>

/* ══════════════════════════════════════════════════════════════════════════
   DICE
   ══════════════════════════════════════════════════════════════════════════ */

void dice_init_rng(void) {
    srand((unsigned)time(NULL));
}

int dice_roll_one(void) {
    return (rand() % 6) + 1;
}

int dice_roll_many(int count) {
    int total = 0;
    for (int i = 0; i < count; i++) total += dice_roll_one();
    return total;
}

int dice_roll_many_arr(int count, int *out) {
    int total = 0;
    for (int i = 0; i < count; i++) {
        out[i] = dice_roll_one();
        total += out[i];
    }
    return total;
}

/* ══════════════════════════════════════════════════════════════════════════
   SHUFFLE  (Fisher-Yates)
   ══════════════════════════════════════════════════════════════════════════ */

void shuffle_civ_deck(CivCard *deck, int size) {
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        CivCard tmp = deck[i]; deck[i] = deck[j]; deck[j] = tmp;
    }
}

void shuffle_buildings(Building *tiles, int size) {
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Building tmp = tiles[i]; tiles[i] = tiles[j]; tiles[j] = tmp;
    }
}
