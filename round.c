#include "game.h"

/* ══════════════════════════════════════════════════════════════════════════
   FEEDING PHASE
   ══════════════════════════════════════════════════════════════════════════ */

int feed_players(GameState *gs) {
    int starved_mask = 0;
    for (int i = 0; i < gs->player_count; i++) {
        Player *p = &gs->players[i];
        int need  = p->people_total;

        /* Collect food from food track */
        int prod = p->food_track;
        int supply_take = (gs->supply[RES_FOOD] >= prod) ? prod : gs->supply[RES_FOOD];
        gs->supply[RES_FOOD]    -= supply_take;
        p->resources[RES_FOOD]  += supply_take;

        game_log(gs, "%s: food track=%d, produced=%d, needs=%d food.",
                 p->name, p->food_track, supply_take, need);

        if (p->resources[RES_FOOD] >= need) {
            p->resources[RES_FOOD] -= need;
            gs->supply[RES_FOOD]   += need;
            game_log(gs, "%s feeds all %d people. Remaining food: %d.",
                     p->name, need, p->resources[RES_FOOD]);
        } else {
            int deficit = need - p->resources[RES_FOOD];
            int food_used = p->resources[RES_FOOD];
            p->resources[RES_FOOD] = 0;

            int total_res = 0;
            for (int r = RES_WOOD; r < RES_COUNT; r++)
                total_res += p->resources[r];

            if (total_res >= deficit) {
                int remaining = deficit;
                for (int r = RES_WOOD; r < RES_COUNT && remaining > 0; r++) {
                    int use = (p->resources[r] < remaining) ?
                              p->resources[r] : remaining;
                    p->resources[r] -= use;
                    remaining -= use;
                }
                game_log(gs, "%s is short %d food - pays with resources.", p->name, deficit);
            } else {
                p->score -= 10;
                if (p->score < 0) p->score = 0;
                game_log(gs, "*** %s cannot feed people! Loses 10 points. ***", p->name);
                starved_mask |= (1 << i);
            }
            gs->supply[RES_FOOD] += food_used;
        }
    }
    return starved_mask;
}

/* ══════════════════════════════════════════════════════════════════════════
   ROUND END
   ══════════════════════════════════════════════════════════════════════════ */

void round_refill_civ(GameState *gs) {
    CivCard temp[CIV_DISPLAY_SIZE];
    int tc = 0;
    for (int i = 0; i < gs->civ_display_count; i++)
        temp[tc++] = gs->civ_display[i];

    int need = CIV_DISPLAY_SIZE - tc;
    int new_count = (need < gs->civ_deck_size) ? need : gs->civ_deck_size;
    int idx = 0;
    for (int i = 0; i < new_count; i++)
        gs->civ_display[idx++] = gs->civ_deck[--gs->civ_deck_size];
    for (int i = 0; i < tc; i++)
        gs->civ_display[idx++] = temp[i];
    gs->civ_display_count = idx;
}

void round_end(GameState *gs) {
    for (int i = 0; i < gs->player_count; i++) {
        Player *p = &gs->players[i];
        memset(p->workers_at, 0, sizeof(p->workers_at));
        p->people_available = p->people_total;
        tools_reset(p);
    }
    memset(gs->workers_at, 0, sizeof(gs->workers_at));

    gs->start_player   = (gs->start_player + 1) % gs->player_count;
    gs->current_player = gs->start_player;

    round_refill_civ(gs);

    gs->round++;
    gs->phase = PHASE_PLACEMENT;
    game_log(gs, "--- Round %d begins. Start player: %s ---",
             gs->round, gs->players[gs->start_player].name);
}

/* ══════════════════════════════════════════════════════════════════════════
   GAME OVER CHECK
   ══════════════════════════════════════════════════════════════════════════ */

int check_game_over(GameState *gs) {
    int need = CIV_DISPLAY_SIZE - gs->civ_display_count;
    if (need > 0 && gs->civ_deck_size < need) {
        game_log(gs, "GAME OVER: Not enough civilization cards to refill display.");
        return 1;
    }
    for (int s = 0; s < gs->stack_count; s++) {
        if (gs->building_stack_size[s] == 0) {
            game_log(gs, "GAME OVER: Building stack %d exhausted.", s + 1);
            return 1;
        }
    }
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════════
   FINAL SCORING
   ══════════════════════════════════════════════════════════════════════════ */

void score_final(GameState *gs) {
    game_log(gs, "=== FINAL SCORING ===");

    for (int i = 0; i < gs->player_count; i++) {
        Player *p = &gs->players[i];

        /* Green civ cards */
        int unique = 0, extras = 0;
        for (int c = 0; c < CULTURE_COUNT; c++) {
            if (p->civ_culture_count[c] > 0) {
                unique++;
                extras += p->civ_culture_count[c] - 1;
            }
        }
        int green_pts = unique * unique + extras;
        p->score += green_pts;
        game_log(gs, "%s: green cards %d unique -> %d pts (+%d extras).",
                 p->name, unique, green_pts, extras);

        /* Farmers x food_track */
        int farmer_pts = p->civ_farmer_count * p->food_track;
        p->score += farmer_pts;
        game_log(gs, "%s: %d farmers x %d food = %d pts.",
                 p->name, p->civ_farmer_count, p->food_track, farmer_pts);

        /* Tool-makers x total tool value */
        int total_tool_val = 0;
        for (int t = 0; t < p->tool_count; t++) total_tool_val += p->tools[t].value;
        int tool_pts = p->civ_tool_maker_count * total_tool_val;
        p->score += tool_pts;
        game_log(gs, "%s: %d tool-makers x %d = %d pts.",
                 p->name, p->civ_tool_maker_count, total_tool_val, tool_pts);

        /* Hut-builders x buildings */
        int hut_pts = p->civ_hut_builder_count * p->building_count;
        p->score += hut_pts;
        game_log(gs, "%s: %d hut-builders x %d buildings = %d pts.",
                 p->name, p->civ_hut_builder_count, p->building_count, hut_pts);

        /* Shamen x people */
        int shaman_pts = p->civ_shaman_count * p->people_total;
        p->score += shaman_pts;
        game_log(gs, "%s: %d shamen x %d people = %d pts.",
                 p->name, p->civ_shaman_count, p->people_total, shaman_pts);

        /* Remaining resources = 1 pt each */
        int res_pts = 0;
        for (int r = RES_WOOD; r < RES_COUNT; r++) res_pts += p->resources[r];
        p->score += res_pts;
        game_log(gs, "%s: %d remaining resources = %d pts.", p->name, res_pts, res_pts);

        game_log(gs, "%s TOTAL: %d points.", p->name, p->score);
    }

    /* Find winner */
    int best = -1;
    for (int i = 0; i < gs->player_count; i++) {
        if (best < 0 || gs->players[i].score > gs->players[best].score)
            best = i;
    }

    /* Tiebreaker */
    for (int i = 0; i < gs->player_count; i++) {
        if (i == best) continue;
        if (gs->players[i].score == gs->players[best].score) {
            int tb_i    = gs->players[i].food_track;
            int tb_best = gs->players[best].food_track;
            for (int t = 0; t < gs->players[i].tool_count; t++)
                tb_i += gs->players[i].tools[t].value;
            for (int t = 0; t < gs->players[best].tool_count; t++)
                tb_best += gs->players[best].tools[t].value;
            tb_i    += gs->players[i].people_total;
            tb_best += gs->players[best].people_total;
            if (tb_i > tb_best) best = i;
        }
    }

    gs->winner    = best;
    gs->game_over = 1;
    game_log(gs, "WINNER: %s with %d points!",
             gs->players[best].name, gs->players[best].score);
}

/* ══════════════════════════════════════════════════════════════════════════
   SAVE / LOAD
   ══════════════════════════════════════════════════════════════════════════ */

int game_save(const GameState *gs, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return 0;
    fwrite(gs, sizeof(GameState), 1, f);
    fclose(f);
    return 1;
}

int game_load(GameState *gs, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;
    fread(gs, sizeof(GameState), 1, f);
    fclose(f);
    return 1;
}
