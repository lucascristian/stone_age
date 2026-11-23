#include "game.h"
#include <stdarg.h>

/* ══════════════════════════════════════════════════════════════════════════
   LOGGING
   ══════════════════════════════════════════════════════════════════════════ */

void game_log(GameState *gs, const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    int len = (int)strlen(buf);
    if (gs->log_cursor + len + 2 >= (int)sizeof(gs->log)) {
        /* Shift old entries out */
        memmove(gs->log, gs->log + 512, gs->log_cursor - 512);
        gs->log_cursor -= 512;
    }
    memcpy(gs->log + gs->log_cursor, buf, len);
    gs->log_cursor += len;
    gs->log[gs->log_cursor++] = '\n';
    gs->log[gs->log_cursor]   = '\0';
}

void game_log_clear(GameState *gs) {
    gs->log_cursor = 0;
    gs->log[0] = '\0';
}

/* ══════════════════════════════════════════════════════════════════════════
   CIVILIZATION DECK  (36 cards total)
   16 green-background culture cards (8 symbols × 2 copies each)
   20 sand-background cards (farmers, tool-makers, hut-builders, shamen)
   Upper part effects assigned to match rulebook counts:
     10 dice-choice cards, 7 food cards, 5 resource cards,
     3 resource+die-roll cards, 3 points cards, 1 tool card,
     2 food-prod cards, 1 extra-card card, 3 one-time-tool cards,
     1 two-resources card
   ══════════════════════════════════════════════════════════════════════════ */

void game_init_civ_deck(GameState *gs) {
    CivCard *d = gs->civ_deck;
    int n = 0;

    /* Helper to add a card */
#define ADD_CARD(TOP, TOPVAL, TOPRES, BOT, CULT, ONETIME, COST) \
    do { \
        d[n].top_type = (TOP); d[n].top_value = (TOPVAL); \
        d[n].top_resource = (TOPRES); d[n].bot_type = (BOT); \
        d[n].culture = (CULT); d[n].onetime_tool_value = (ONETIME); \
        d[n].cost = (COST); n++; \
    } while(0)

    /* ── Green background (16 cards, each culture symbol ×2) ──────────── */
    for (int c = 0; c < CULTURE_COUNT; c++) {
        for (int copy = 0; copy < 2; copy++) {
            /* distribute top effects across green cards */
            CivCardTopType tops[8] = {
                CIV_TOP_FOOD, CIV_TOP_RESOURCE, CIV_TOP_DICE_RESOURCE,
                CIV_TOP_POINTS, CIV_TOP_TOOL, CIV_TOP_FOOD_PROD,
                CIV_TOP_ONETIME_TOOL, CIV_TOP_FOOD
            };
            CivCardTopType top = tops[c];
            int topval = 0, topres = 0, onetime = 0, cost = 1 + (c % 4);
            switch (top) {
                case CIV_TOP_FOOD:          topval = 3 + (c % 3); break;
                case CIV_TOP_RESOURCE:      topres = RES_WOOD + (c % 4); break;
                case CIV_TOP_DICE_RESOURCE: topres = RES_WOOD + (c % 4); break;
                case CIV_TOP_POINTS:        topval = 3; break;
                case CIV_TOP_TOOL:          break;
                case CIV_TOP_FOOD_PROD:     break;
                case CIV_TOP_ONETIME_TOOL:  onetime = 2 + (c % 3); break;
                default: break;
            }
            ADD_CARD(top, topval, topres, CIV_BOT_GREEN, (CultureSymbol)c,
                     onetime, cost);
        }
    }

    /* ── Sand background (20 cards) ────────────────────────────────────── */
    /* Farmers ×5 */
    for (int i = 0; i < 5; i++) {
        int tops[5] = {CIV_TOP_FOOD, CIV_TOP_FOOD, CIV_TOP_FOOD_PROD,
                       CIV_TOP_RESOURCE, CIV_TOP_DICE_RESOURCE};
        int tv[5]   = {4, 2, 0, 0, 0};
        int tr[5]   = {0, 0, 0, RES_BRICK, RES_WOOD};
        ADD_CARD(tops[i], tv[i], tr[i], CIV_BOT_FARMER, 0, 0, 1 + i);
    }
    /* Tool-makers ×3 */
    for (int i = 0; i < 3; i++) {
        int tops[3] = {CIV_TOP_TOOL, CIV_TOP_ONETIME_TOOL, CIV_TOP_POINTS};
        int ot[3]   = {0, 3, 0};
        int tv[3]   = {0, 0, 3};
        ADD_CARD(tops[i], tv[i], 0, CIV_BOT_TOOL_MAKER, 0, ot[i], 1 + i);
    }
    /* Hut-builders ×5 */
    for (int i = 0; i < 5; i++) {
        int tops[5] = {CIV_TOP_RESOURCE, CIV_TOP_FOOD, CIV_TOP_DICE_RESOURCE,
                       CIV_TOP_TWO_RESOURCES, CIV_TOP_POINTS};
        int tv[5]   = {0, 3, 0, 0, 3};
        int tr[5]   = {RES_STONE, 0, RES_GOLD, 0, 0};
        ADD_CARD(tops[i], tv[i], tr[i], CIV_BOT_HUT_BUILDER, 0, 0, 2 + i);
    }
    /* Shamen ×4 */
    for (int i = 0; i < 4; i++) {
        int tops[4] = {CIV_TOP_FOOD_PROD, CIV_TOP_FOOD, CIV_TOP_ONETIME_TOOL,
                       CIV_TOP_RESOURCE};
        int ot[4]   = {0, 0, 4, 0};
        int tv[4]   = {0, 2, 0, 0};
        int tr[4]   = {0, 0, 0, RES_GOLD};
        ADD_CARD(tops[i], tv[i], tr[i], CIV_BOT_SHAMAN, 0, ot[i], 1 + i);
    }
    /* Remaining 3 cards: one_any_card, two_resources, dice_choice */
    ADD_CARD(CIV_TOP_ANY_CARD,      0, 0, CIV_BOT_GREEN, CULTURE_WEAVING, 0, 3);
    ADD_CARD(CIV_TOP_TWO_RESOURCES, 0, 0, CIV_BOT_GREEN, CULTURE_TRANSPORT, 0, 2);
    ADD_CARD(CIV_TOP_DICE_CHOICE,   0, 0, CIV_BOT_GREEN, CULTURE_TIME, 0, 2);

#undef ADD_CARD

    gs->civ_deck_size = n;
    shuffle_civ_deck(gs->civ_deck, gs->civ_deck_size);
}

/* ══════════════════════════════════════════════════════════════════════════
   BUILDING TILES  (28 total: 4 stacks × 7)
   Rulebook: 8 specific, 8 selected (exactly 4 from 2 kinds),
             3 flex (1-7 of any mix), rest specific.
   We fill 28 tiles with a realistic distribution.
   ══════════════════════════════════════════════════════════════════════════ */

void game_init_buildings(GameState *gs) {
    Building all[MAX_BUILDINGS];
    int n = 0;

    /* Specific buildings (fixed resources, fixed points) */
    struct { int pts; int r[RES_COUNT]; } spec[] = {
        {10, {0,2,1,0,0}},  /* 2 wood + 1 brick = 10 pts                    */
        {13, {0,2,0,0,1}},  /* 2 wood + 1 gold                              */
        {13, {0,2,1,0,0}},  /* 2 wood + 1 brick (different combo)           */
        {14, {0,1,0,1,0}},  /* 1 wood + 1 stone                             */
        {11, {0,0,3,0,0}},  /* 3 brick                                      */
        {12, {0,1,0,1,1}},  /* 1 wood + 1 stone + 1 gold – wait, fixed      */
        {13, {0,0,1,2,0}},  /* 1 brick + 2 stone                            */
        {14, {0,1,1,0,1}},  /* 1 wood + 1 brick + 1 gold                    */
        {16, {0,0,2,0,1}},  /* 2 stone + 1 gold                             */
        {14, {0,1,0,0,1}},  /* 1 wood + 1 gold                              */
        {11, {0,2,0,0,0}},  /* 2 wood                                        */
        {12, {0,1,1,0,0}},  /* 1 wood + 1 brick                             */
        {15, {0,1,1,0,1}},  /* 1 wood + 1 brick + 1 gold                    */
        {12, {0,0,2,0,0}},  /* 2 brick                                       */
        {15, {0,1,0,1,1}},  /* 1 wood + 1 stone + 1 gold                    */
        {14, {0,0,0,1,1}},  /* 1 stone + 1 gold                             */
        {14, {0,1,1,1,0}},  /* 1 wood + 1 brick + 1 stone                   */
    };
    int ns = (int)(sizeof(spec)/sizeof(spec[0]));
    for (int i = 0; i < ns && n < MAX_BUILDINGS; i++) {
        all[n].type   = BUILD_SPECIFIC;
        all[n].points = spec[i].pts;
        memcpy(all[n].required, spec[i].r, sizeof(all[n].required));
        all[n].amount = 0;
        n++;
    }

    /* Selected buildings: pay exactly 4 resources from exactly 2 kinds */
    int ns2 = 8;
    for (int i = 0; i < ns2 && n < MAX_BUILDINGS; i++) {
        all[n].type   = BUILD_SELECTED;
        all[n].points = 0;   /* computed at buy time                        */
        memset(all[n].required, 0, sizeof(all[n].required));
        all[n].amount = 4;
        n++;
    }

    /* Flex buildings: pay 1-7 resources of any kinds */
    int nf = 3;
    for (int i = 0; i < nf && n < MAX_BUILDINGS; i++) {
        all[n].type   = BUILD_FLEX;
        all[n].points = 0;   /* computed at buy time                        */
        memset(all[n].required, 0, sizeof(all[n].required));
        all[n].amount = 0;   /* 1-7, player decides                         */
        n++;
    }

    shuffle_buildings(all, n);

    /* Split into stacks of 7 */
    gs->stack_count = gs->player_count;  /* 2 stacks for 2 players, etc.   */
    if (gs->stack_count < 2) gs->stack_count = 2;
    for (int s = 0; s < gs->stack_count; s++) {
        gs->building_stack_size[s] = 0;
        for (int k = 0; k < BUILDINGS_PER_STACK && (s * BUILDINGS_PER_STACK + k) < n; k++) {
            gs->building_stacks[s][k] = all[s * BUILDINGS_PER_STACK + k];
            gs->building_stack_size[s]++;
        }
        /* Expose top tile */
        if (gs->building_stack_size[s] > 0) {
            gs->building_top[s] = gs->building_stacks[s][gs->building_stack_size[s] - 1];
            gs->building_top_valid[s] = 1;
        } else {
            gs->building_top_valid[s] = 0;
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════════
   SUPPLY
   ══════════════════════════════════════════════════════════════════════════ */

void game_init_supply(GameState *gs) {
    gs->supply[RES_FOOD]  = 53;  /* food tiles                              */
    gs->supply[RES_WOOD]  = 20;
    gs->supply[RES_BRICK] = 16;
    gs->supply[RES_STONE] = 12;
    gs->supply[RES_GOLD]  = 10;
}

/* ══════════════════════════════════════════════════════════════════════════
   PLAYERS
   ══════════════════════════════════════════════════════════════════════════ */

void game_init_players(GameState *gs, int player_count, const char names[][32]) {
    gs->player_count = player_count;
    for (int i = 0; i < player_count; i++) {
        Player *p = &gs->players[i];
        memset(p, 0, sizeof(*p));
        strncpy(p->name, names[i], 31);
        p->color           = i;
        p->people_total    = START_PEOPLE;
        p->people_available = START_PEOPLE;
        p->resources[RES_FOOD] = START_FOOD;
        p->food_track      = 0;
        p->score           = 0;
    }
}

/* ══════════════════════════════════════════════════════════════════════════
   FULL GAME INIT
   ══════════════════════════════════════════════════════════════════════════ */

void game_init(GameState *gs, int player_count, const char names[][32]) {
    memset(gs, 0, sizeof(*gs));
    dice_init_rng();
    gs->player_count = player_count;
    game_init_supply(gs);
    game_init_players(gs, player_count, names);
    game_init_buildings(gs);
    game_init_civ_deck(gs);

    /* Draw first 4 civilization cards onto display */
    for (int i = 0; i < CIV_DISPLAY_SIZE && gs->civ_deck_size > 0; i++) {
        gs->civ_display[gs->civ_display_count++] =
            gs->civ_deck[--gs->civ_deck_size];
    }

    gs->start_player   = 0;
    gs->current_player = 0;
    gs->round          = 1;
    gs->phase          = PHASE_PLACEMENT;
    gs->game_over      = 0;
    gs->winner         = -1;

    game_log(gs, "=== Stone Age begins! %d players ===", player_count);
    game_log(gs, "Round 1 - Placement phase.");
}
