#ifndef AGENTS_H
#define AGENTS_H

#include "types.h"
#include "game.h"

/* ─── Agent types ─────────────────────────────────────────────────────────
   AGENT_GREEDY_RESOURCES : greedy agent that prioritizes resource gathering
                            and building acquisition for immediate points
   AGENT_GREEDY_FOOD      : greedy agent that prioritizes food production
                            and population growth for immediate survival
   AGENT_BALANCED         : balanced agent that weighs short and long term
                            value of every action before deciding
   ──────────────────────────────────────────────────────────────────────── */

typedef enum {
    AGENT_GREEDY_RESOURCES = 0,
    AGENT_GREEDY_FOOD,
    AGENT_BALANCED
} AgentType;

/* ─── Action descriptor ───────────────────────────────────────────────────
   Used internally by agents to score and rank possible placements
   ──────────────────────────────────────────────────────────────────────── */

typedef struct {
    LocationType loc;
    int          worker_count;
    float        score;        /* agent's estimated value of this action   */
} ActionCandidate;

/* ─── Public API ──────────────────────────────────────────────────────────
   agent_place_workers  : called during Phase 1 to place all workers
   agent_eval_actions   : called during Phase 2 to resolve all workers
   agent_choose_payment : called when agent needs to pay for card/building
   ──────────────────────────────────────────────────────────────────────── */

void agent_place_workers(GameState *gs, int player, AgentType type);
void agent_eval_actions(GameState *gs, int player, AgentType type);
void agent_choose_payment(const Player *p, int total_cost,
                          int out_payment[RES_COUNT]);

/* ─── Internal helpers (also useful for analysis) ─────────────────────── */
float agent_score_location(const GameState *gs, int player,
                           LocationType loc, int workers, AgentType type);
float agent_expected_resources(int workers, int divisor);
float agent_building_value(const GameState *gs, int stack, AgentType type);
float agent_civcard_value(const GameState *gs, int slot,
                          int player, AgentType type);

#endif /* AGENTS_H */
