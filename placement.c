#include "game.h"

/* ══════════════════════════════════════════════════════════════════════════
   CAPACITY HELPERS
   ══════════════════════════════════════════════════════════════════════════ */

static int loc_capacity(const GameState *gs, LocationType loc) {
    if (loc == LOC_HUNT)          return 999; /* unlimited per player        */
    if (loc == LOC_TOOL_MAKER)    return MAX_WORKERS_TOOL;
    if (loc == LOC_HUT)           return MAX_WORKERS_HUT;
    if (loc == LOC_FIELD)         return MAX_WORKERS_FIELD;
    if (loc >= LOC_FOREST && loc <= LOC_RIVER) return MAX_WORKERS_RESOURCE;
    if (loc >= LOC_CIV_CARD_0 && loc <= LOC_CIV_CARD_3) return 1;
    if (loc >= LOC_BUILDING_0 && loc <= LOC_BUILDING_3)  return 1;
    return 0;
}

/* For 2/3 players only 1 player may use each resource location per round.
   For 4 players any number of players may share (up to 7 total). */
static int loc_player_already_there(const GameState *gs, int player,
                                    LocationType loc) {
    return gs->players[player].workers_at[loc] > 0;
}

/* ══════════════════════════════════════════════════════════════════════════
   VALIDATION
   ══════════════════════════════════════════════════════════════════════════ */

int validate_loc_choice(const GameState *gs, int player,
                        LocationType loc, int count) {
    if (count <= 0) return 0;
    const Player *p = &gs->players[player];
    if (p->people_available < count) return 0;

    int cap = loc_capacity(gs, loc);

    /* Placing again in same location is not allowed */
    if (loc_player_already_there(gs, player, loc)) return 0;

    /* Total workers already at location */
    int already = gs->workers_at[loc];

    /* Special rules */
    if (loc == LOC_TOOL_MAKER) {
        if (count != 1)  return 0;
        if (already >= 1) return 0;
    }
    if (loc == LOC_HUT) {
        if (count != 2)  return 0;
        if (already >= 2) return 0;
    }
    if (loc == LOC_FIELD) {
        if (count != 1)  return 0;
        if (already >= 1) return 0;
    }
    /* 2-3 players: only 1 player per resource location */
    if (gs->player_count <= 3 &&
        loc >= LOC_FOREST && loc <= LOC_RIVER) {
        if (already > 0) return 0;
    }
    /* Resource locations: shared cap of 7 */
    if (loc >= LOC_FOREST && loc <= LOC_RIVER) {
        if (already + count > cap) return 0;
    }
    /* Civ card slots */
    if (loc >= LOC_CIV_CARD_0 && loc <= LOC_CIV_CARD_3) {
        if (count != 1) return 0;
        int slot = loc - LOC_CIV_CARD_0;
        if (slot >= gs->civ_display_count) return 0;
        if (already >= 1) return 0;
    }
    /* Building slots */
    if (loc >= LOC_BUILDING_0 && loc <= LOC_BUILDING_3) {
        if (count != 1) return 0;
        int stack = loc - LOC_BUILDING_0;
        if (stack >= gs->stack_count) return 0;
        if (!gs->building_top_valid[stack]) return 0;
        if (already >= 1) return 0;
    }
    return 1;
}

int can_place_workers(const GameState *gs, int player,
                      LocationType loc, int count) {
    return validate_loc_choice(gs, player, loc, count);
}

/* ══════════════════════════════════════════════════════════════════════════
   PLACEMENT
   ══════════════════════════════════════════════════════════════════════════ */

int place_workers(GameState *gs, int player, LocationType loc, int count) {
    if (!validate_loc_choice(gs, player, loc, count)) return 0;
    Player *p = &gs->players[player];
    p->workers_at[loc]    += count;
    p->people_available   -= count;
    gs->workers_at[loc]   += count;

    const char *loc_names[] = {
        "Hunt","Forest","Clay Pit","Quarry","River",
        "Tool Maker","Hut","Field",
        "Civ Card 1","Civ Card 2","Civ Card 3","Civ Card 4",
        "Building 1","Building 2","Building 3","Building 4"
    };
    game_log(gs, "%s places %d worker(s) at %s.",
             p->name, count, loc_names[loc]);
    return 1;
}

/* ══════════════════════════════════════════════════════════════════════════
   PHASE CHECK
   ══════════════════════════════════════════════════════════════════════════ */

int all_placed(const GameState *gs) {
    for (int i = 0; i < gs->player_count; i++) {
        if (gs->players[i].people_available > 0) return 0;
    }
    return 1;
}

/* ══════════════════════════════════════════════════════════════════════════
   RESOURCE PAYMENT VALIDATION
   ══════════════════════════════════════════════════════════════════════════ */

int validate_resource_payment(const Player *p, const int cost[RES_COUNT]) {
    for (int r = 1; r < RES_COUNT; r++) {   /* skip food (r=0)              */
        if (p->resources[r] < cost[r]) return 0;
    }
    return 1;
}
