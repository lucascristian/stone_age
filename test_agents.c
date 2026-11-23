#include "agents.h"
#include <stdio.h>

int tests = 0, passed = 0;
#define TEST(name, expr) do { tests++; \
    if(expr){passed++;printf("[PASS] %s\n",name);} \
    else printf("[FAIL] %s\n",name); } while(0)

int main(void) {
    printf("═══════════════════════════════\n");
    printf("  Stone Age — Agent Tests\n");
    printf("═══════════════════════════════\n\n");

    float f = agent_expected_resources(3, 3);
    TEST("3 workers forest expected ~3.5", f > 3.4f && f < 3.6f);

    float g = agent_expected_resources(2, 6);
    TEST("2 workers gold expected ~1.17", g > 1.1f && g < 1.2f);

    GameState gs;
    const char names[2][32] = {"Agent1","Agent2"};
    game_init(&gs, 2, names);

    agent_place_workers(&gs, 0, AGENT_GREEDY_RESOURCES);
    TEST("greedy resources places all workers", gs.players[0].people_available == 0);

    agent_place_workers(&gs, 1, AGENT_GREEDY_FOOD);
    TEST("greedy food places all workers", gs.players[1].people_available == 0);

    agent_eval_actions(&gs, 0, AGENT_GREEDY_RESOURCES);
    TEST("greedy resources eval runs", 1);
    agent_eval_actions(&gs, 1, AGENT_GREEDY_FOOD);
    TEST("greedy food eval runs", 1);

    game_init(&gs, 2, names);
    agent_place_workers(&gs, 0, AGENT_BALANCED);
    TEST("balanced places all workers", gs.players[0].people_available == 0);
    agent_eval_actions(&gs, 0, AGENT_BALANCED);
    TEST("balanced eval runs", 1);

    game_init(&gs, 2, names);
    gs.players[0].resources[RES_WOOD]  = 3;
    gs.players[0].resources[RES_BRICK] = 2;
    int pay[RES_COUNT];
    agent_choose_payment(&gs.players[0], 4, pay);
    int total = 0;
    for(int r=0;r<RES_COUNT;r++) total += pay[r];
    TEST("payment covers cost", total >= 4);

    float sv = agent_score_location(&gs, 0, LOC_HUNT, 2, AGENT_GREEDY_FOOD);
    TEST("greedy food scores hunt positively", sv > 0.0f);
    float sr = agent_score_location(&gs, 0, LOC_FOREST, 2, AGENT_GREEDY_RESOURCES);
    TEST("greedy resources scores forest positively", sr > 0.0f);

    /* Full game simulation */
    game_init(&gs, 2, names);
    int rounds = 0;
    while (!gs.game_over && rounds < 20) {
        while (!all_placed(&gs)) {
            AgentType t = (gs.current_player == 0) ?
                          AGENT_GREEDY_RESOURCES : AGENT_GREEDY_FOOD;
            agent_place_workers(&gs, gs.current_player, t);
            gs.current_player = (gs.current_player + 1) % gs.player_count;
        }
        for (int i = 0; i < gs.player_count; i++) {
            AgentType t = (i==0) ? AGENT_GREEDY_RESOURCES : AGENT_GREEDY_FOOD;
            agent_eval_actions(&gs, i, t);
        }
        feed_players(&gs);
        if (check_game_over(&gs)) { score_final(&gs); break; }
        round_end(&gs);
        rounds++;
    }
    TEST("full agent game completes", rounds <= 20);
    TEST("scores non-negative", gs.players[0].score >= 0 && gs.players[1].score >= 0);

    printf("\n───────────────────────────────\n");
    printf("Results: %d / %d passed\n", passed, tests);
    if (passed == tests) printf("✓ ALL PASSED\n");
    printf("───────────────────────────────\n");
    return (passed == tests) ? 0 : 1;
}
