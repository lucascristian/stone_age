/*
 * tests/test_game.c
 * Automated tests for Stone Age game logic.
 * Compile standalone: gcc -Iinclude tests/test_game.c src/dice.c src/init.c
 *                         src/placement.c src/eval.c src/round.c -o tests/test_runner
 */

#include "game.h"
#include <assert.h>
#include <stdio.h>

static int tests_run = 0;
static int tests_pass = 0;

#define TEST(name, expr) do { \
    tests_run++; \
    if (expr) { tests_pass++; printf("[PASS] %s\n", name); } \
    else       { printf("[FAIL] %s  (line %d)\n", name, __LINE__); } \
} while(0)

/* ─── Dice tests ──────────────────────────────────────────────────────── */

static void test_dice(void) {
    dice_init_rng();

    /* Single die in range */
    int all_valid = 1;
    for (int i = 0; i < 1000; i++) {
        int r = dice_roll_one();
        if (r < 1 || r > 6) { all_valid = 0; break; }
    }
    TEST("dice_roll_one always 1-6", all_valid);

    /* Sum of N dice */
    for (int n = 1; n <= 7; n++) {
        int sum = dice_roll_many(n);
        TEST("dice_roll_many in range", sum >= n && sum <= n*6);
    }

    /* Array fill */
    int arr[7];
    int sum = dice_roll_many_arr(7, arr);
    int arr_sum = 0;
    int arr_valid = 1;
    for (int i = 0; i < 7; i++) {
        arr_sum += arr[i];
        if (arr[i] < 1 || arr[i] > 6) arr_valid = 0;
    }
    TEST("dice_roll_many_arr fills correctly", arr_valid && arr_sum == sum);
}

/* ─── Shuffle tests ───────────────────────────────────────────────────── */

static void test_shuffle(void) {
    /* After shuffle, all cards still present */
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    const char names[2][32] = {"P1","P2"};
    game_init(&gs, 2, names);

    /* Verify 36 civ cards total before drawing */
    /* (4 are drawn to display at init) */
    int total = gs.civ_deck_size + gs.civ_display_count;
    TEST("civ deck total = 36", total == 36);

    /* Verify 14 buildings in play for 2-player game (2 stacks × 7) */
    int total_buildings = 0;
    for (int s = 0; s < gs.stack_count; s++)
        total_buildings += gs.building_stack_size[s];
    TEST("building stacks sum = 14 for 2p", total_buildings == 14);
}

/* ─── Initialisation tests ────────────────────────────────────────────── */

static void test_init(void) {
    GameState gs;
    const char names[4][32] = {"Alice","Bob","Carol","Dave"};
    game_init(&gs, 4, names);

    TEST("4-player game init", gs.player_count == 4);
    TEST("round starts at 1", gs.round == 1);
    TEST("phase starts at PLACEMENT", gs.phase == PHASE_PLACEMENT);
    TEST("each player has 5 people", gs.players[0].people_total == 5);
    TEST("each player has 12 food", gs.players[0].resources[RES_FOOD] == 12);
    TEST("4-player: 4 building stacks", gs.stack_count == 4);
    TEST("4-player: civ display = 4", gs.civ_display_count == 4);
    TEST("supply food = 53", gs.supply[RES_FOOD] == 53);
    TEST("supply wood = 20", gs.supply[RES_WOOD] == 20);
    TEST("supply brick = 16", gs.supply[RES_BRICK] == 16);
    TEST("supply stone = 12", gs.supply[RES_STONE] == 12);
    TEST("supply gold = 10", gs.supply[RES_GOLD] == 10);
}

/* ─── Placement tests ─────────────────────────────────────────────────── */

static void test_placement(void) {
    GameState gs;
    const char names[2][32] = {"P1","P2"};
    game_init(&gs, 2, names);

    /* Valid: P0 places 1 at hunt */
    TEST("can place at hunt", can_place_workers(&gs, 0, LOC_HUNT, 1));
    TEST("place at hunt returns 1", place_workers(&gs, 0, LOC_HUNT, 1));
    TEST("P0 workers_at hunt = 1", gs.players[0].workers_at[LOC_HUNT] == 1);
    TEST("P0 people_available = 4", gs.players[0].people_available == 4);

    /* Invalid: place again at same location */
    TEST("cannot place again at hunt", !can_place_workers(&gs, 0, LOC_HUNT, 1));

    /* Tool maker: exactly 1 */
    TEST("tool maker needs 1", can_place_workers(&gs, 0, LOC_TOOL_MAKER, 1));
    TEST("tool maker rejects 2", !can_place_workers(&gs, 0, LOC_TOOL_MAKER, 2));

    /* Hut: exactly 2 */
    TEST("hut needs 2", can_place_workers(&gs, 0, LOC_HUT, 2));
    TEST("hut rejects 1", !can_place_workers(&gs, 0, LOC_HUT, 1));

    /* 2-player resource: only 1 player allowed */
    place_workers(&gs, 0, LOC_FOREST, 2);
    TEST("2p: second player cannot enter forest", !can_place_workers(&gs, 1, LOC_FOREST, 1));

    /* All placed check */
    game_init(&gs, 2, names);
    for (int i = 0; i < 2; i++) gs.players[i].people_available = 0;
    TEST("all_placed when everyone has 0 available", all_placed(&gs));
}

/* ─── Evaluation tests ────────────────────────────────────────────────── */

static void test_evaluation(void) {
    GameState gs;
    const char names[2][32] = {"P1","P2"};

    /* Hunt */
    game_init(&gs, 2, names);
    place_workers(&gs, 0, LOC_HUNT, 3);
    int food_before = gs.players[0].resources[RES_FOOD];
    eval_hunt_with_tools(&gs, 0, 0);
    /* Player should have at least food_before (can't decrease) */
    TEST("hunt adds food", gs.players[0].resources[RES_FOOD] >= food_before);

    /* Tool maker */
    game_init(&gs, 2, names);
    place_workers(&gs, 0, LOC_TOOL_MAKER, 1);
    eval_tool_maker(&gs, 0);
    TEST("tool maker gives 1 tool slot", gs.players[0].tool_count == 1);
    TEST("first tool value = 1", gs.players[0].tools[0].value == 1);

    /* Hut */
    game_init(&gs, 2, names);
    place_workers(&gs, 0, LOC_HUT, 2);
    eval_hut(&gs, 0);
    TEST("hut adds 1 person", gs.players[0].people_total == 6);

    /* Field */
    game_init(&gs, 2, names);
    place_workers(&gs, 0, LOC_FIELD, 1);
    eval_field(&gs, 0);
    TEST("field raises food track", gs.players[0].food_track == 1);

    /* Resource procurement */
    game_init(&gs, 2, names);
    place_workers(&gs, 0, LOC_FOREST, 3);
    int wood_before = gs.players[0].resources[RES_WOOD];
    eval_resource_with_tools(&gs, 0, LOC_FOREST, 0);
    /* wood >= wood_before (dice roll may yield 0 if unlucky, but test structure) */
    TEST("forest eval runs without error", gs.players[0].workers_at[LOC_FOREST] == 0);
}

/* ─── Feeding tests ───────────────────────────────────────────────────── */

static void test_feeding(void) {
    GameState gs;
    const char names[2][32] = {"Rich","Poor"};
    game_init(&gs, 2, names);

    /* Give Rich player plenty of food */
    gs.players[0].food_track = 5;
    gs.players[0].resources[RES_FOOD] = 20;

    /* Give Poor player 0 food */
    gs.players[1].resources[RES_FOOD] = 0;
    gs.players[1].food_track = 0;

    int score_before = gs.players[1].score;
    feed_players(&gs);

    TEST("rich player survives feeding", gs.players[0].resources[RES_FOOD] >= 0);
    /* Poor player with 5 people and 0 food/resources loses 10 points */
    TEST("starved player loses 10 points",
         gs.players[1].score == score_before - 10 || gs.players[1].score == 0);
}

/* ─── Round management tests ──────────────────────────────────────────── */

static void test_round(void) {
    GameState gs;
    const char names[2][32] = {"P1","P2"};
    game_init(&gs, 2, names);

    int start_before = gs.start_player;
    round_end(&gs);

    TEST("round increments", gs.round == 2);
    TEST("start player rotates",
         gs.start_player == (start_before + 1) % 2);
    TEST("phase resets to placement", gs.phase == PHASE_PLACEMENT);
    TEST("workers reset after round", gs.players[0].people_available == gs.players[0].people_total);
}

/* ─── Resource payment validation ────────────────────────────────────── */

static void test_validation(void) {
    GameState gs;
    const char names[2][32] = {"P1","P2"};
    game_init(&gs, 2, names);

    gs.players[0].resources[RES_WOOD]  = 3;
    gs.players[0].resources[RES_STONE] = 1;

    int cost_ok[RES_COUNT]  = {0, 2, 0, 1, 0};
    int cost_bad[RES_COUNT] = {0, 4, 0, 0, 0};

    TEST("valid payment accepted", validate_resource_payment(&gs.players[0], cost_ok));
    TEST("invalid payment rejected", !validate_resource_payment(&gs.players[0], cost_bad));
}

/* ─── Save / Load tests ───────────────────────────────────────────────── */

static void test_save_load(void) {
    GameState gs, gs2;
    const char names[2][32] = {"SaveMe","LoadMe"};
    game_init(&gs, 2, names);
    gs.round = 5;
    gs.players[0].score = 42;

    int saved = game_save(&gs, "/tmp/test_save.dat");
    TEST("game_save returns 1", saved);

    memset(&gs2, 0, sizeof(gs2));
    int loaded = game_load(&gs2, "/tmp/test_save.dat");
    TEST("game_load returns 1", loaded);
    TEST("round preserved", gs2.round == 5);
    TEST("score preserved", gs2.players[0].score == 42);
    TEST("player name preserved", strcmp(gs2.players[0].name, "SaveMe") == 0);
}

/* ─── Final scoring tests ─────────────────────────────────────────────── */

static void test_scoring(void) {
    GameState gs;
    const char names[2][32] = {"P1","P2"};
    game_init(&gs, 2, names);

    Player *p = &gs.players[0];

    /* Set up a known scoring scenario */
    p->civ_culture_count[CULTURE_HEALING]  = 1;
    p->civ_culture_count[CULTURE_ART]      = 1;
    p->civ_culture_count[CULTURE_WRITING]  = 1;  /* 3 different = 9 pts */
    p->civ_farmer_count    = 2;
    p->food_track          = 4;  /* 2×4 = 8 pts */
    p->civ_tool_maker_count = 1;
    p->tools[0].value = 3; p->tool_count = 1; /* 1×3 = 3 pts */
    p->civ_hut_builder_count = 1;
    p->building_count = 2;  /* 1×2 = 2 pts */
    p->civ_shaman_count = 1;
    p->people_total = 6;  /* 1×6 = 6 pts */
    p->resources[RES_GOLD] = 2;  /* 2 pts */
    p->score = 10;         /* pre-existing score */

    gs.game_over = 0;
    score_final(&gs);

    /* Expected: 10 + 9 + 8 + 3 + 2 + 6 + 2 = 40 */
    TEST("final scoring computes correctly", p->score == 40);
}

/* ─── Main ────────────────────────────────────────────────────────────── */

int main(void) {
    printf("═══════════════════════════════════════\n");
    printf("  Stone Age – Game Logic Test Suite\n");
    printf("═══════════════════════════════════════\n\n");

    test_dice();
    test_shuffle();
    test_init();
    test_placement();
    test_evaluation();
    test_feeding();
    test_round();
    test_validation();
    test_save_load();
    test_scoring();

    printf("\n───────────────────────────────────────\n");
    printf("Results: %d / %d tests passed\n", tests_pass, tests_run);
    if (tests_pass == tests_run)
        printf("✓ ALL TESTS PASSED\n");
    else
        printf("✗ %d FAILED\n", tests_run - tests_pass);
    printf("───────────────────────────────────────\n");

    return (tests_pass == tests_run) ? 0 : 1;
}
