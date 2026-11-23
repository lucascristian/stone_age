#include "game.h"

/* ══════════════════════════════════════════════════════════════════════════
   TOOL HELPERS
   ══════════════════════════════════════════════════════════════════════════ */

int tools_available_total(const Player *p) {
    int total = 0;
    for (int i = 0; i < p->tool_count; i++)
        if (!p->tools[i].used) total += p->tools[i].value;
    for (int i = 0; i < p->onetime_tool_count; i++)
        total += p->onetime_tools[i];
    return total;
}

/* Use tools to add exactly `extra` to a roll.
   Returns actual extra added (may be less if not enough tools). */
int apply_tools(Player *p, int extra_needed) {
    int added = 0;
    /* Regular tools first */
    for (int i = 0; i < p->tool_count && added < extra_needed; i++) {
        if (!p->tools[i].used) {
            p->tools[i].used = 1;
            added += p->tools[i].value;
        }
    }
    /* One-time tools */
    for (int i = p->onetime_tool_count - 1; i >= 0 && added < extra_needed; i--) {
        added += p->onetime_tools[i];
        /* Remove one-time tool */
        p->onetime_tool_count--;
    }
    return added;
}

void tools_reset(Player *p) {
    for (int i = 0; i < p->tool_count; i++) p->tools[i].used = 0;
}

/* ══════════════════════════════════════════════════════════════════════════
   HUNT
   ══════════════════════════════════════════════════════════════════════════ */

int eval_hunt(GameState *gs, int player) {
    Player *p = &gs->players[player];
    int workers = p->workers_at[LOC_HUNT];
    if (workers == 0) return 0;

    int roll = dice_roll_many(workers);
    int food = roll / DIV_FOOD;

    /* Players may add tools - we ask UI to handle this, here we just take
       what was pre-set in p's tool state (UI calls apply_tools before here) */
    game_log(gs, "%s hunts with %d worker(s): rolled %d -> %d food.",
             p->name, workers, roll, food);

    /* Take food from supply (supply tracks food for the board; each player
       stores their own food in resources[RES_FOOD]) */
    int take = (gs->supply[RES_FOOD] >= food) ? food : gs->supply[RES_FOOD];
    gs->supply[RES_FOOD] -= take;
    p->resources[RES_FOOD] += take;

    /* Reset workers */
    gs->workers_at[LOC_HUNT] -= workers;
    p->workers_at[LOC_HUNT]   = 0;
    return take;
}

/* Hunt with explicit tool boost (called from UI after player chose tools) */
int eval_hunt_with_tools(GameState *gs, int player, int tool_boost) {
    Player *p = &gs->players[player];
    int workers = p->workers_at[LOC_HUNT];
    if (workers == 0) return 0;

    int roll  = dice_roll_many(workers);
    int total = roll + tool_boost;
    int food  = total / DIV_FOOD;

    game_log(gs, "%s hunts: %d dice -> %d + %d tools = %d total -> %d food.",
             p->name, workers, roll, tool_boost, total, food);

    int take = (gs->supply[RES_FOOD] >= food) ? food : gs->supply[RES_FOOD];
    gs->supply[RES_FOOD] -= take;
    p->resources[RES_FOOD] += take;

    gs->workers_at[LOC_HUNT] -= workers;
    p->workers_at[LOC_HUNT]   = 0;
    return take;
}

/* ══════════════════════════════════════════════════════════════════════════
   RESOURCE PROCUREMENT (Forest/Clay/Quarry/River)
   ══════════════════════════════════════════════════════════════════════════ */

static int loc_to_resource(LocationType loc) {
    switch (loc) {
        case LOC_FOREST:   return RES_WOOD;
        case LOC_CLAY_PIT: return RES_BRICK;
        case LOC_QUARRY:   return RES_STONE;
        case LOC_RIVER:    return RES_GOLD;
        default:           return -1;
    }
}

static int resource_divisor(ResourceType res) {
    switch (res) {
        case RES_WOOD:  return DIV_WOOD;
        case RES_BRICK: return DIV_BRICK;
        case RES_STONE: return DIV_STONE;
        case RES_GOLD:  return DIV_GOLD;
        default:        return 1;
    }
}

int eval_resource(GameState *gs, int player, LocationType loc) {
    Player *p = &gs->players[player];
    int workers = p->workers_at[loc];
    if (workers == 0) return 0;

    int res = loc_to_resource(loc);
    int div = resource_divisor(res);

    int roll  = dice_roll_many(workers);
    int count = roll / div;

    game_log(gs, "%s gathers at %s: %d dice -> %d / %d = %d resource(s).",
             p->name,
             loc==LOC_FOREST?"Forest":loc==LOC_CLAY_PIT?"Clay Pit":
             loc==LOC_QUARRY?"Quarry":"River",
             workers, roll, div, count);

    /* Resources are unlimited per rulebook - replace with substitute if 0 */
    p->resources[res] += count;

    gs->workers_at[loc] -= workers;
    p->workers_at[loc]   = 0;
    return count;
}

int eval_resource_with_tools(GameState *gs, int player,
                              LocationType loc, int tool_boost) {
    Player *p = &gs->players[player];
    int workers = p->workers_at[loc];
    if (workers == 0) return 0;

    int res   = loc_to_resource(loc);
    int div   = resource_divisor(res);
    int roll  = dice_roll_many(workers);
    int total = roll + tool_boost;
    int count = total / div;

    const char *loc_name = loc==LOC_FOREST?"Forest":loc==LOC_CLAY_PIT?"Clay Pit":
                           loc==LOC_QUARRY?"Quarry":"River";
    game_log(gs, "%s at %s: %d dice -> %d + %d tools = %d -> %d %s.",
             p->name, loc_name, workers, roll, tool_boost, total, count,
             res==RES_WOOD?"wood":res==RES_BRICK?"brick":
             res==RES_STONE?"stone":"gold");

    p->resources[res] += count;
    gs->workers_at[loc] -= workers;
    p->workers_at[loc]   = 0;
    return count;
}

/* ══════════════════════════════════════════════════════════════════════════
   TOOL MAKER
   ══════════════════════════════════════════════════════════════════════════ */

int eval_tool_maker(GameState *gs, int player) {
    Player *p = &gs->players[player];
    if (p->workers_at[LOC_TOOL_MAKER] == 0) return 0;

    /* Tool progression: 1st-3rd -> value 1, 4th-6th -> flip to 2,
       7th-9th -> replace with 3, 10th-12th -> flip to 4 */
    int total_tools = p->tool_count * 0; /* count existing slots */
    for (int i = 0; i < p->tool_count; i++) total_tools++;
    int total_acq = 0; /* total times player has visited tool maker         */
    for (int i = 0; i < p->tool_count; i++)
        total_acq++;

    if (p->tool_count < MAX_TOOLS) {
        /* Add a new tool slot with value 1 */
        p->tools[p->tool_count].value = 1;
        p->tools[p->tool_count].used  = 0;
        p->tool_count++;
        game_log(gs, "%s gets a new tool (value 1). Total tools: %d.",
                 p->name, p->tool_count);
    } else {
        /* Upgrade existing tools */
        for (int i = 0; i < p->tool_count; i++) {
            if (p->tools[i].value < 4) {
                p->tools[i].value++;
                game_log(gs, "%s upgrades a tool to value %d.",
                         p->name, p->tools[i].value);
                break;
            }
        }
    }

    gs->workers_at[LOC_TOOL_MAKER] -= 1;
    p->workers_at[LOC_TOOL_MAKER]   = 0;
    return 1;
}

/* ══════════════════════════════════════════════════════════════════════════
   HUT
   ══════════════════════════════════════════════════════════════════════════ */

int eval_hut(GameState *gs, int player) {
    Player *p = &gs->players[player];
    if (p->workers_at[LOC_HUT] == 0) return 0;

    if(p->people_total < MAX_PEOPLE) {
        p->people_total++;
    }
    game_log(gs, "%s builds a hut - now has %d people.", p->name, p->people_total);

    gs->workers_at[LOC_HUT] -= 2;
    p->workers_at[LOC_HUT]   = 0;
    return 1;
}

/* ══════════════════════════════════════════════════════════════════════════
   FIELD
   ══════════════════════════════════════════════════════════════════════════ */

int eval_field(GameState *gs, int player) {
    Player *p = &gs->players[player];
    if (p->workers_at[LOC_FIELD] == 0) return 0;

    if (p->food_track < 10) {
        p->food_track++;
        game_log(gs, "%s farms - food production now %d.", p->name, p->food_track);
    } else {
        game_log(gs, "%s farms - food track already at max 10.", p->name);
    }

    gs->workers_at[LOC_FIELD] -= 1;
    p->workers_at[LOC_FIELD]   = 0;
    return 1;
}

/* ══════════════════════════════════════════════════════════════════════════
   CIVILIZATION CARD
   ══════════════════════════════════════════════════════════════════════════ */

void apply_civ_top(GameState *gs, int player, CivCard *card) {
    Player *p = &gs->players[player];
    switch (card->top_type) {
        case CIV_TOP_FOOD:
            p->resources[RES_FOOD] += card->top_value;
            game_log(gs, "  -> Gains %d food.", card->top_value);
            break;
        case CIV_TOP_RESOURCE:
            p->resources[card->top_resource]++;
            game_log(gs, "  -> Gains 1 resource (type %d).", card->top_resource);
            break;
        case CIV_TOP_DICE_RESOURCE: {
            int roll = dice_roll_many(2);
            int div  = resource_divisor(card->top_resource);
            int gain = roll / div;
            p->resources[card->top_resource] += gain;
            game_log(gs, "  -> Dice roll %d / %d = %d resource(s).", roll, div, gain);
            break;
        }
        case CIV_TOP_POINTS:
            p->score += card->top_value;
            game_log(gs, "  -> Gains %d points.", card->top_value);
            break;
        case CIV_TOP_TOOL:
            eval_tool_maker(gs, player);
            break;
        case CIV_TOP_FOOD_PROD:
            if (p->food_track < 10) p->food_track++;
            game_log(gs, "  -> Food production +1 (now %d).", p->food_track);
            break;
        case CIV_TOP_ANY_CARD:
            if (gs->civ_deck_size > 0) {
                CivCard extra = gs->civ_deck[--gs->civ_deck_size];
                p->civ_cards[p->civ_card_count++] = extra;
                register_civ_bot(p, &extra);
                game_log(gs, "  -> Takes extra civ card from deck.");
            }
            break;
        case CIV_TOP_ONETIME_TOOL:
            if (p->onetime_tool_count < 3) {
                p->onetime_tools[p->onetime_tool_count++] = card->onetime_tool_value;
                game_log(gs, "  -> Gets one-time tool value %d.", card->onetime_tool_value);
            }
            break;
        case CIV_TOP_TWO_RESOURCES:
            p->has_two_res_card = 1;
            game_log(gs, "  -> Gets 2-resource card (use anytime).");
            break;
        case CIV_TOP_DICE_CHOICE:
            /* Simplified: current player picks best die result */
            {
                int roll = dice_roll_one();
                if (roll <= 2) p->resources[RES_WOOD]++;
                else if (roll <= 3) p->resources[RES_BRICK]++;
                else if (roll <= 4) p->resources[RES_STONE]++;
                else p->resources[RES_GOLD]++;
                game_log(gs, "  -> Dice-choice card: rolled %d, gains resource.", roll);
            }
            break;
        default: break;
    }
}

void register_civ_bot(Player *p, const CivCard *card) {
    switch (card->bot_type) {
        case CIV_BOT_GREEN:
            p->civ_culture_count[card->culture]++;
            break;
        case CIV_BOT_FARMER:
            p->civ_farmer_count++;
            break;
        case CIV_BOT_TOOL_MAKER:
            p->civ_tool_maker_count++;
            break;
        case CIV_BOT_HUT_BUILDER:
            p->civ_hut_builder_count++;
            break;
        case CIV_BOT_SHAMAN:
            p->civ_shaman_count++;
            break;
    }
}

int eval_civ_card(GameState *gs, int player, int slot) {
    Player *p = &gs->players[player];
    LocationType loc = (LocationType)(LOC_CIV_CARD_0 + slot);
    if (p->workers_at[loc] == 0) return 0;
    if (slot >= gs->civ_display_count) return 0;

    CivCard *card = &gs->civ_display[slot];
    int cost = card->cost;

    /* Check payment: player must pay `cost` resources (not food) */
    int total_res = 0;
    for (int r = RES_WOOD; r < RES_COUNT; r++) total_res += p->resources[r];
    if (total_res < cost) {
        game_log(gs, "%s cannot afford civ card (need %d resources).", p->name, cost);
        gs->workers_at[loc] -= 1;
        p->workers_at[loc]   = 0;
        return 0;
    }

    /* Deduct resources (prefer cheapest: wood first) */
    int remaining = cost;
    for (int r = RES_WOOD; r < RES_COUNT && remaining > 0; r++) {
        int take = (p->resources[r] < remaining) ? p->resources[r] : remaining;
        p->resources[r] -= take;
        remaining -= take;
    }

    game_log(gs, "%s acquires a civilization card (cost %d).", p->name, cost);

    /* Apply top-part immediately */
    apply_civ_top(gs, player, card);

    /* Store card and register bottom */
    p->civ_cards[p->civ_card_count++] = *card;
    register_civ_bot(p, card);

    /* Remove from display (slide left) */
    for (int i = slot; i < gs->civ_display_count - 1; i++)
        gs->civ_display[i] = gs->civ_display[i + 1];
    gs->civ_display_count--;

    gs->workers_at[loc] -= 1;
    p->workers_at[loc]   = 0;
    return 1;
}

/* eval_civ_card with explicit resource payment chosen by player */
int eval_civ_card_pay(GameState *gs, int player, int slot,
                      const int payment[RES_COUNT]) {
    Player *p = &gs->players[player];
    LocationType loc = (LocationType)(LOC_CIV_CARD_0 + slot);
    if (p->workers_at[loc] == 0) return 0;
    if (slot >= gs->civ_display_count) return 0;

    CivCard *card = &gs->civ_display[slot];

    /* Validate payment totals */
    int total_pay = 0;
    for (int r = RES_WOOD; r < RES_COUNT; r++) {
        if (p->resources[r] < payment[r]) {
            game_log(gs, "%s: insufficient resources.", p->name);
            return 0;
        }
        total_pay += payment[r];
    }
    if (total_pay < card->cost) {
        game_log(gs, "%s: payment too low for civ card.", p->name);
        return 0;
    }

    for (int r = RES_WOOD; r < RES_COUNT; r++)
        p->resources[r] -= payment[r];

    game_log(gs, "%s acquires civ card (paid %d resources).", p->name, total_pay);
    apply_civ_top(gs, player, card);
    p->civ_cards[p->civ_card_count++] = *card;
    register_civ_bot(p, card);

    for (int i = slot; i < gs->civ_display_count - 1; i++)
        gs->civ_display[i] = gs->civ_display[i + 1];
    gs->civ_display_count--;

    gs->workers_at[loc] -= 1;
    p->workers_at[loc]   = 0;
    return 1;
}

/* ══════════════════════════════════════════════════════════════════════════
   BUILDING
   ══════════════════════════════════════════════════════════════════════════ */

static int resource_value(ResourceType r) {
    switch (r) {
        case RES_WOOD:  return VAL_WOOD;
        case RES_BRICK: return VAL_BRICK;
        case RES_STONE: return VAL_STONE;
        case RES_GOLD:  return VAL_GOLD;
        default:        return 1;
    }
}

int eval_building(GameState *gs, int player, int stack_idx,
                  int res_choice[RES_COUNT]) {
    Player *p = &gs->players[player];
    LocationType loc = (LocationType)(LOC_BUILDING_0 + stack_idx);
    if (p->workers_at[loc] == 0) return 0;
    if (!gs->building_top_valid[stack_idx]) return 0;

    Building *b = &gs->building_top[stack_idx];
    int pts = 0;

    if (b->type == BUILD_SPECIFIC) {
        if (!validate_resource_payment(p, b->required)) {
            game_log(gs, "%s cannot afford building.", p->name);
            gs->workers_at[loc] -= 1; p->workers_at[loc] = 0;
            return 0;
        }
        for (int r = 0; r < RES_COUNT; r++)
            p->resources[r] -= b->required[r];
        pts = b->points;

    } else if (b->type == BUILD_SELECTED) {
        /* res_choice must be exactly 4 resources from exactly 2 kinds */
        int kinds = 0, total = 0;
        for (int r = RES_WOOD; r < RES_COUNT; r++) {
            if (res_choice[r] > 0) kinds++;
            total += res_choice[r];
            if (p->resources[r] < res_choice[r]) {
                game_log(gs, "%s: not enough resources.", p->name); return 0;
            }
        }
        if (total != 4 || kinds != 2) {
            game_log(gs, "%s: selected building needs exactly 4 from 2 kinds.", p->name);
            return 0;
        }
        for (int r = RES_WOOD; r < RES_COUNT; r++) {
            pts += res_choice[r] * resource_value(r);
            p->resources[r] -= res_choice[r];
        }

    } else { /* BUILD_FLEX */
        int total = 0;
        for (int r = RES_WOOD; r < RES_COUNT; r++) {
            if (p->resources[r] < res_choice[r]) {
                game_log(gs, "%s: not enough resources.", p->name); return 0;
            }
            total += res_choice[r];
        }
        if (total < 1 || total > 7) {
            game_log(gs, "%s: flex building requires 1-7 resources.", p->name);
            return 0;
        }
        for (int r = RES_WOOD; r < RES_COUNT; r++) {
            pts += res_choice[r] * resource_value(r);
            p->resources[r] -= res_choice[r];
        }
    }

    /* Award points */
    p->score += pts;
    game_log(gs, "%s builds! Scores %d points (total: %d).", p->name, pts, p->score);

    /* Store building */
    p->buildings[p->building_count++] = *b;

    /* Pop stack */
    gs->building_stack_size[stack_idx]--;
    if (gs->building_stack_size[stack_idx] > 0) {
        gs->building_top[stack_idx] =
            gs->building_stacks[stack_idx][gs->building_stack_size[stack_idx] - 1];
        gs->building_top_valid[stack_idx] = 1;
    } else {
        gs->building_top_valid[stack_idx] = 0;
    }

    gs->workers_at[loc] -= 1;
    p->workers_at[loc]   = 0;
    return pts;
}
