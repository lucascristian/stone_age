#ifndef GAME_H
#define GAME_H

#include "types.h"

/* ─── Dice ───────────────────────────────────────────────────────────────── */
void dice_init_rng(void);
int  dice_roll_one(void);                        /* 1-6                     */
int  dice_roll_many(int count);                  /* sum of `count` dice     */
int  dice_roll_many_arr(int count, int *out);    /* fill out[], return sum  */

/* ─── Deck / shuffle ─────────────────────────────────────────────────────── */
void shuffle_civ_deck(CivCard *deck, int size);
void shuffle_buildings(Building *tiles, int size);

/* ─── Initialisation ─────────────────────────────────────────────────────── */
void game_init_civ_deck(GameState *gs);
void game_init_buildings(GameState *gs);
void game_init_supply(GameState *gs);
void game_init_players(GameState *gs, int player_count, const char names[][32]);
void game_init(GameState *gs, int player_count, const char names[][32]);

/* ─── Worker placement ───────────────────────────────────────────────────── */
int  can_place_workers(const GameState *gs, int player, LocationType loc, int count);
int  place_workers(GameState *gs, int player, LocationType loc, int count);
int  all_placed(const GameState *gs);

/* ─── Worker evaluation (phase 2) ───────────────────────────────────────── */
int  eval_hunt(GameState *gs, int player);
int  eval_resource(GameState *gs, int player, LocationType loc);
int  eval_tool_maker(GameState *gs, int player);
int  eval_hut(GameState *gs, int player);
int  eval_field(GameState *gs, int player);
int  eval_civ_card(GameState *gs, int player, int slot);
int  eval_building(GameState *gs, int player, int stack_idx,
                   int res_choice[RES_COUNT]);   /* res_choice for SELECTED/FLEX */
void eval_player_actions(GameState *gs, int player);

/* ─── Tool helpers ───────────────────────────────────────────────────────── */
int  tools_available_total(const Player *p);     /* sum of unused tool vals  */
int  apply_tools(Player *p, int extra_needed);   /* use minimum tools needed */
void tools_reset(Player *p);                     /* un-use all tools         */

/* ─── Civilization card helpers ──────────────────────────────────────────── */
void apply_civ_top(GameState *gs, int player, CivCard *card);
void register_civ_bot(Player *p, const CivCard *card);

/* ─── Feeding phase ──────────────────────────────────────────────────────── */
int  feed_players(GameState *gs);                /* returns 0 ok, else mask  */

/* ─── Round management ───────────────────────────────────────────────────── */
void round_end(GameState *gs);
void round_refill_civ(GameState *gs);
int  check_game_over(GameState *gs);             /* 1 = game over            */

/* ─── Final scoring ──────────────────────────────────────────────────────── */
void score_final(GameState *gs);

/* ─── Input validation ───────────────────────────────────────────────────── */
int  validate_resource_payment(const Player *p, const int cost[RES_COUNT]);
int  validate_loc_choice(const GameState *gs, int player,
                         LocationType loc, int count);

/* ─── Logging ────────────────────────────────────────────────────────────── */
void game_log(GameState *gs, const char *fmt, ...);
void game_log_clear(GameState *gs);

/* ─── Save / Load ────────────────────────────────────────────────────────── */
int  game_save(const GameState *gs, const char *filename);
int  game_load(GameState *gs, const char *filename);

#endif /* GAME_H */

/* Additional declarations (tool-assisted eval, explicit civ payment) */
int  eval_hunt_with_tools(GameState *gs, int player, int tool_boost);
int  eval_resource_with_tools(GameState *gs, int player, LocationType loc, int tool_boost);
int  eval_civ_card_pay(GameState *gs, int player, int slot, const int payment[RES_COUNT]);
int  eval_hunt_with_tools(GameState *gs, int player, int tool_boost);
int  eval_resource_with_tools(GameState *gs, int player, LocationType loc, int tool_boost);
int  eval_civ_card_pay(GameState *gs, int player, int slot, const int payment[RES_COUNT]);
