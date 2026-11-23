/*
 * agents.c — Heuristic AI agents for Stone Age
 *
 * Design principles:
 * 1. Buildings are ALWAYS the best return when affordable — they give
 *    immediate guaranteed points.  All scoring is calibrated so that
 *    any affordable building outscores raw resource gathering.
 * 2. Resource gathering is done to ACCUMULATE what is needed for the
 *    best available building, not just to maximize expected yield.
 * 3. Long-term investments (Hut/Tool/Field) are scored on a declining
 *    curve so they are only prioritised early in the game.
 * 4. The placement loop caps workers per resource location so the agent
 *    is forced to diversify — preventing "all workers in one spot".
 * 5. Hunt is the universal fallback — truly unlimited per the rulebook.
 */

#include "agents.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* ─── tiny helpers ─────────────────────────────────────────────────── */
static int total_tool_value(const Player *p) {
    int v = 0;
    for (int i = 0; i < p->tool_count; i++) v += p->tools[i].value;
    for (int i = 0; i < p->onetime_tool_count; i++) v += p->onetime_tools[i];
    return v;
}

static int total_resources(const Player *p) {
    int t = 0;
    for (int r = RES_WOOD; r < RES_COUNT; r++) t += p->resources[r];
    return t;
}

static float expected_yield(int workers, int divisor, int tools) {
    return ((3.5f * workers) + tools) / (float)divisor;
}

static int res_pts(ResourceType r) {
    switch(r){
        case RES_WOOD:  return VAL_WOOD;
        case RES_BRICK: return VAL_BRICK;
        case RES_STONE: return VAL_STONE;
        case RES_GOLD:  return VAL_GOLD;
        default:        return 1;
    }
}

/* ─── building value ───────────────────────────────────────────────── */
float agent_building_value(const GameState *gs, int stack, AgentType type) {
    if (!gs->building_top_valid[stack]) return -999.0f;
    const Building *b = &gs->building_top[stack];
    float pts;
    if (b->type == BUILD_SPECIFIC) {
        pts = (float)b->points;
    } else if (b->type == BUILD_SELECTED) {
        /* 4 resources from 2 kinds — optimistic estimate using stone+gold */
        pts = 2 * VAL_STONE + 2 * VAL_GOLD;
    } else {
        /* Flex: assume we pay 4 best resources we have */
        pts = 4 * 4.5f;
    }
    /* Agents weight buildings differently */
    if (type == AGENT_GREEDY_RESOURCES) return pts * 2.0f;
    if (type == AGENT_GREEDY_FOOD)      return pts * 0.9f;
    return pts * 1.5f;
}

/* ─── expected resource value score ───────────────────────────────── */
float agent_expected_resources(int workers, int divisor) {
    return (3.5f * workers) / (float)divisor;
}

/* ─── building target bonus ────────────────────────────────────────── */
/*
 * Returns extra score for gathering resource `res` to help afford buildings.
 * Handles specific, selected, and flex buildings.
 */
static float building_target_bonus(const GameState *gs, int player,
                                    ResourceType res, AgentType type) {
    const Player *p = &gs->players[player];
    float bonus = 0.0f;
    int total_res = 0;
    for (int r = RES_WOOD; r < RES_COUNT; r++) total_res += p->resources[r];

    for (int s = 0; s < gs->stack_count; s++) {
        if (!gs->building_top_valid[s]) continue;
        const Building *b = &gs->building_top[s];
        float bmul = (type==AGENT_GREEDY_RESOURCES) ? 2.0f :
                     (type==AGENT_GREEDY_FOOD)      ? 1.2f : 1.5f;

        if (b->type == BUILD_SPECIFIC) {
            /* For specific: if we're 1 resource away, high bonus */
            int need = b->required[res];
            if (need == 0) continue;
            int missing = need - p->resources[res];
            if (missing <= 0) continue;
            int can_rest = 1;
            for (int r = RES_WOOD; r < RES_COUNT; r++) {
                if (r==(int)res) continue;
                if (p->resources[r] < b->required[r]) {can_rest=0;break;}
            }
            if (can_rest) bonus += (float)b->points * bmul / (float)missing;

        } else if (b->type == BUILD_SELECTED) {
            /* For selected: need 4 total from 2 types - bonus for any resource */
            /* Find best pair we're close to completing */
            int best_pair = 0;
            for (int r2 = RES_WOOD; r2 < RES_COUNT; r2++) {
                if (r2 == (int)res) continue;
                int combined = p->resources[res] + p->resources[r2];
                if (combined > best_pair) best_pair = combined;
            }
            int missing_sel = 4 - (p->resources[res] + (best_pair - p->resources[res]));
            if (missing_sel > 0 && missing_sel <= 2)
                bonus += 18.0f * bmul / (float)(missing_sel + 1);

        } else {
            /* Flex: any resource helps - bonus proportional to how close we are */
            int needed_flex = 4 - total_res;
            if (needed_flex > 0 && needed_flex <= 3)
                bonus += 15.0f * bmul / (float)(needed_flex + 1);
        }
    }
    return fminf(bonus, 20.0f); /* cap bonus to prevent runaway */
}

/* ─── diversity bonus ──────────────────────────────────────────────── */
/*
 * Returns extra score for gathering a resource the agent has very little of.
 * This ensures agents diversify their resource portfolio and can afford
 * selected buildings (which need 4 resources from 2 different kinds).
 */
static float diversity_bonus(const Player *p, ResourceType res, AgentType type) {
    if (type == AGENT_GREEDY_FOOD) {
        /* Food agent: needs non-food resources for cards/buildings */
        int nonf_total = 0;
        for (int r = RES_WOOD; r < RES_COUNT; r++) nonf_total += p->resources[r];
        int this_type = p->resources[res];
        /* If this type is 0 and total non-food is < 4, strongly boost */
        if (this_type == 0 && nonf_total < 4) return 8.0f;
        if (this_type <= 1 && nonf_total < 2) return 5.0f;
    }
    /* All agents: if this resource is the least-held non-food resource,
       give a small boost to encourage diversification for selected buildings */
    int min_nonf = 999;
    for (int r = RES_WOOD; r < RES_COUNT; r++) {
        if (p->resources[r] < min_nonf) min_nonf = p->resources[r];
    }
    if (p->resources[res] == min_nonf && min_nonf < 2) return 3.0f;
    return 0.0f;
}

/* ─── per-location score ───────────────────────────────────────────── */
/*
 * Returns the VALUE of sending `workers` people to `loc`.
 * Calibration goal: an affordable 10-pt building must always score
 * above any resource gathering, and Hut/Tool/Field must decline
 * enough that they don't dominate past round 5.
 */
float agent_score_location(const GameState *gs, int player,
                           LocationType loc, int workers, AgentType type) {
    const Player *p = &gs->players[player];
    int tv  = total_tool_value(p);
    int rnd = gs->round;
    float decay = fmaxf(0.1f, (12.0f - rnd) / 12.0f); /* 1.0 → 0.1 over 12 rounds */
    float val   = 0.0f;

    switch (loc) {

    /* ── Food / Hunt ──────────────────────────────────────────────── */
    case LOC_HUNT: {
        float food    = expected_yield(workers, DIV_FOOD, tv);
        int   have    = p->resources[RES_FOOD];
        int   need    = p->people_total - p->food_track;
        if (need < 0) need = 0;
        /* For food agent: urgency is lower when food_track covers needs well.
           We cap urgency to ensure food agent STILL gathers some non-food
           resources for civ cards and buildings. */
        float urgency;
        if (type == AGENT_GREEDY_FOOD) {
            /* Softer urgency: max 2.0 even when starving */
            urgency = (have < need)   ? 2.0f :
                      (have < need+2) ? 1.2f : 0.5f;
            val = food * urgency * 1.8f;
        } else {
            urgency = (have < need)   ? 3.0f :
                      (have < need*2) ? 1.5f : 0.6f;
            val = food * urgency;
            if (type == AGENT_GREEDY_RESOURCES) val *= 0.4f;
        }
        break;
    }

    /* ── Resource locations ───────────────────────────────────────── */
    case LOC_FOREST:   val = expected_yield(workers,DIV_WOOD, tv)*VAL_WOOD;
                       if(type==AGENT_GREEDY_RESOURCES) val*=1.3f;
                       /* Food agent: boost resources when starved of non-food */
                       if(type==AGENT_GREEDY_FOOD){
                           int nonf=0; for(int r=RES_WOOD;r<RES_COUNT;r++) nonf+=p->resources[r];
                           if(nonf<3) val*=1.8f;
                       }
                       val += building_target_bonus(gs,player,RES_WOOD,type) + diversity_bonus(p,RES_WOOD,type); break;
    case LOC_CLAY_PIT: val = expected_yield(workers,DIV_BRICK,tv)*VAL_BRICK;
                       if(type==AGENT_GREEDY_RESOURCES) val*=1.3f;
                       if(type==AGENT_GREEDY_FOOD){int nonf=0;for(int r=RES_WOOD;r<RES_COUNT;r++)nonf+=p->resources[r];if(nonf<3)val*=1.8f;}
                       val += building_target_bonus(gs,player,RES_BRICK,type) + diversity_bonus(p,RES_BRICK,type); break;
    case LOC_QUARRY:   val = expected_yield(workers,DIV_STONE,tv)*VAL_STONE;
                       if(type==AGENT_GREEDY_RESOURCES) val*=1.3f;
                       if(type==AGENT_GREEDY_FOOD){int nonf=0;for(int r=RES_WOOD;r<RES_COUNT;r++)nonf+=p->resources[r];if(nonf<3)val*=1.8f;}
                       val += building_target_bonus(gs,player,RES_STONE,type) + diversity_bonus(p,RES_STONE,type); break;
    case LOC_RIVER:    val = expected_yield(workers,DIV_GOLD, tv)*VAL_GOLD;
                       if(type==AGENT_GREEDY_RESOURCES) val*=1.4f;
                       if(type==AGENT_GREEDY_FOOD){int nonf=0;for(int r=RES_WOOD;r<RES_COUNT;r++)nonf+=p->resources[r];if(nonf<3)val*=1.8f;}
                       val += building_target_bonus(gs,player,RES_GOLD,type) + diversity_bonus(p,RES_GOLD,type); break;

    /* ── Tool Maker ───────────────────────────────────────────────── */
    case LOC_TOOL_MAKER: {
        /* Each tool point adds ~0.5 resources per round on average */
        int cur = total_tool_value(p);
        float gain = (cur < 4) ? 8.0f : (cur < 8) ? 5.0f : 2.0f;
        val = gain * decay;
        if (type == AGENT_BALANCED)         val *= 1.4f;
        if (type == AGENT_GREEDY_FOOD)      val *= 0.4f;
        if (type == AGENT_GREEDY_RESOURCES) val *= 0.6f;
        break;
    }

    /* ── Hut ──────────────────────────────────────────────────────── */
    case LOC_HUT: {
        /* An extra person = ~3 more actions over the game at ~4 pts each */
        val = 12.0f * decay;
        if (type == AGENT_GREEDY_FOOD)      val *= 1.6f;
        if (type == AGENT_BALANCED)         val *= 1.3f;
        if (type == AGENT_GREEDY_RESOURCES) val *= 0.5f;
        break;
    }

    /* ── Field ────────────────────────────────────────────────────── */
    case LOC_FIELD: {
        /* +1 food production every round: saves ~0.5 food avg per round */
        val = 6.0f * decay + p->civ_farmer_count * 3.0f;
        if (type == AGENT_GREEDY_FOOD)      val *= 2.0f;
        if (type == AGENT_BALANCED)         val *= 1.2f;
        if (type == AGENT_GREEDY_RESOURCES) val *= 0.3f;
        break;
    }

    /* ── Civilisation cards ───────────────────────────────────────── */
    case LOC_CIV_CARD_0: case LOC_CIV_CARD_1:
    case LOC_CIV_CARD_2: case LOC_CIV_CARD_3: {
        int slot = loc - LOC_CIV_CARD_0;
        if (slot >= gs->civ_display_count) { val = -999.0f; break; }
        const CivCard *c = &gs->civ_display[slot];
        /* immediate top effect */
        float top = 0.0f;
        switch(c->top_type) {
            case CIV_TOP_FOOD:           top = (float)c->top_value * 0.5f; break;
            case CIV_TOP_RESOURCE:       top = (float)res_pts(c->top_resource); break;
            case CIV_TOP_POINTS:         top = (float)c->top_value; break;
            case CIV_TOP_TOOL:
            case CIV_TOP_ONETIME_TOOL:   top = 5.0f; break;
            case CIV_TOP_FOOD_PROD:      top = 6.0f; break;
            case CIV_TOP_TWO_RESOURCES:  top = 8.0f; break;
            default:                      top = 2.0f; break;
        }
        /* end-game scoring estimate */
        float bot = 0.0f;
        switch(c->bot_type) {
            case CIV_BOT_GREEN: {
                int uniq = 0;
                for(int i=0;i<CULTURE_COUNT;i++) if(p->civ_culture_count[i]>0) uniq++;
                bot = (float)(2*uniq+1); break;
            }
            case CIV_BOT_FARMER:     bot = (float)p->food_track * 1.5f; break;
            case CIV_BOT_TOOL_MAKER: bot = (float)total_tool_value(p) * 1.0f; break;
            case CIV_BOT_HUT_BUILDER:bot = (float)p->building_count * 2.5f; break;
            case CIV_BOT_SHAMAN:     bot = (float)p->people_total * 1.5f; break;
        }
        val = top + bot - (float)c->cost;
        if (type == AGENT_GREEDY_FOOD && c->bot_type == CIV_BOT_FARMER)      val *= 1.6f;
        if (type == AGENT_GREEDY_RESOURCES && c->bot_type == CIV_BOT_HUT_BUILDER) val *= 1.5f;
        if (type == AGENT_BALANCED)                                             val *= 1.1f;
        break;
    }

    /* ── Buildings ────────────────────────────────────────────────── */
    case LOC_BUILDING_0: case LOC_BUILDING_1:
    case LOC_BUILDING_2: case LOC_BUILDING_3: {
        int stack = loc - LOC_BUILDING_0;
        if (!gs->building_top_valid[stack]) { val = -999.0f; break; }
        const Building *b = &gs->building_top[stack];
        /* Affordability check */
        if (b->type == BUILD_SPECIFIC) {
            if (!validate_resource_payment(p, b->required))
                { val = -999.0f; break; }
        } else if (b->type == BUILD_SELECTED) {
            /* Need 4 resources from exactly 2 kinds.
               Check if any pair of resource types has combined >= 4. */
            int can_afford = 0;
            for (int r1=RES_WOOD; r1<RES_COUNT && !can_afford; r1++) {
                if (p->resources[r1] == 0) continue;
                for (int r2=r1+1; r2<RES_COUNT && !can_afford; r2++) {
                    if (p->resources[r2] == 0) continue;
                    if (p->resources[r1] + p->resources[r2] >= 4)
                        can_afford = 1;
                }
            }
            if (!can_afford) { val=-999.0f; break; }
        } else { /* FLEX */
            if (total_resources(p) < 1) { val = -999.0f; break; }
        }
        val = agent_building_value(gs, stack, type);
        break;
    }

    default: val = 0.0f; break;
    }
    return val;
}

/* ─── payment helpers ──────────────────────────────────────────────── */
void agent_choose_payment(const Player *p, int total_cost,
                          int out[RES_COUNT]) {
    memset(out, 0, RES_COUNT * sizeof(int));
    int rem = total_cost;
    /* Pay cheapest first (wood→brick→stone→gold) */
    for (int r = RES_WOOD; r < RES_COUNT && rem > 0; r++) {
        int use = (p->resources[r] < rem) ? p->resources[r] : rem;
        out[r] = use; rem -= use;
    }
}

static void pay_selected(const Player *p, int out[RES_COUNT]) {
    memset(out, 0, RES_COUNT * sizeof(int));
    /* Find the pair of resource types that together has the most resources.
       Prefer expensive resources (stone+gold) for higher score. */
    int best1=-1, best2=-1, best_total=-1;
    for (int r1=RES_WOOD; r1<RES_COUNT; r1++) {
        if (p->resources[r1] == 0) continue;
        for (int r2=r1+1; r2<RES_COUNT; r2++) {
            if (p->resources[r2] == 0) continue;
            int combined = p->resources[r1] + p->resources[r2];
            /* Prefer higher combined and more expensive resources */
            int score = combined * 10 + r1 + r2;
            if (combined >= 4 && score > best_total) {
                best_total = score;
                best1 = r1; best2 = r2;
            }
        }
    }
    if (best1 < 0) return; /* no valid pair */
    int a=p->resources[best1], b=p->resources[best2];
    /* Split 4 between best1 and best2, maximizing expensive types */
    if (best2 > best1) {
        /* best2 is more expensive - use as much as possible */
        if      (b>=3 && a>=1) { out[best2]=3; out[best1]=1; }
        else if (b>=2 && a>=2) { out[best2]=2; out[best1]=2; }
        else if (b>=1 && a>=3) { out[best2]=1; out[best1]=3; }
        else                    { out[best2]=b; out[best1]=4-b; }
    } else {
        if      (a>=3 && b>=1) { out[best1]=3; out[best2]=1; }
        else if (a>=2 && b>=2) { out[best1]=2; out[best2]=2; }
        else if (a>=1 && b>=3) { out[best1]=1; out[best2]=3; }
        else                    { out[best1]=a; out[best2]=4-a; }
    }
}

/* ─── PHASE 1 — WORKER PLACEMENT ──────────────────────────────────── */
void agent_place_workers(GameState *gs, int player, AgentType type) {
    Player *p = &gs->players[player];

    /*
     * Worker caps per resource location:
     *   2-player: cap 2  (2×2=4 ≤ 7)
     *   3-player: cap 2  (3×2=6 ≤ 7)
     *   4-player: cap 1  (4×1=4 ≤ 7)
     * This forces the agent to use multiple resource locations,
     * accumulating the VARIETY of resources needed for buildings.
     */
    int res_cap = (gs->player_count <= 3) ? 2 : 1;

    while (p->people_available > 0) {
        float  best_score   = -9999.0f;
        LocationType best_loc = LOC_COUNT;
        int    best_w       = 0;

        for (int loc = 0; loc < LOC_COUNT; loc++) {
            int min_w = 1, max_w = p->people_available;

            /* Location-specific worker count rules */
            if (loc == LOC_HUT)        { min_w = max_w = 2; }
            else if (loc == LOC_TOOL_MAKER || loc == LOC_FIELD) { min_w = max_w = 1; }
            else if (loc >= LOC_CIV_CARD_0 && loc <= LOC_CIV_CARD_3) { min_w = max_w = 1; }
            else if (loc >= LOC_BUILDING_0 && loc <= LOC_BUILDING_3) { min_w = max_w = 1; }
            else if (loc >= LOC_FOREST && loc <= LOC_RIVER) {
                if (max_w > res_cap) max_w = res_cap;
            }
            /* LOC_HUNT: truly unlimited, keep max_w = people_available */

            for (int w = min_w; w <= max_w; w++) {
                if (!can_place_workers(gs, player, (LocationType)loc, w)) continue;
                float s = agent_score_location(gs, player, (LocationType)loc, w, type);
                if (s > best_score) {
                    best_score = s;
                    best_loc   = (LocationType)loc;
                    best_w     = w;
                }
            }
        }

        if (best_loc == LOC_COUNT) {
            /* Truly stuck — force remaining workers to Hunt */
            int rem = p->people_available;
            p->workers_at[LOC_HUNT] += rem;
            gs->workers_at[LOC_HUNT] += rem;
            p->people_available = 0;
            game_log(gs, "%s: no valid placement, %d worker(s) forced to Hunt.",
                     p->name, rem);
            break;
        }

        place_workers(gs, player, best_loc, best_w);
    }
}

/* ─── PHASE 2 — ACTION EVALUATION ─────────────────────────────────── */
void agent_eval_actions(GameState *gs, int player, AgentType type) {
    Player *p = &gs->players[player];

    for (int loc = 0; loc < LOC_COUNT; loc++) {
        if (p->workers_at[loc] == 0) continue;

        switch (loc) {

        case LOC_HUNT: {
            int boost = (type == AGENT_GREEDY_FOOD) ? total_tool_value(p) :
                        (type == AGENT_BALANCED)    ? total_tool_value(p)/2 : 0;
            eval_hunt_with_tools(gs, player, boost);
            break;
        }

        case LOC_FOREST: case LOC_CLAY_PIT: case LOC_QUARRY: case LOC_RIVER: {
            int boost = (type == AGENT_GREEDY_RESOURCES) ? total_tool_value(p) :
                        (type == AGENT_BALANCED)         ? total_tool_value(p)/2 : 0;
            eval_resource_with_tools(gs, player, (LocationType)loc, boost);
            break;
        }

        case LOC_TOOL_MAKER: eval_tool_maker(gs, player); break;
        case LOC_HUT:        eval_hut(gs, player);        break;
        case LOC_FIELD:      eval_field(gs, player);      break;

        case LOC_CIV_CARD_0: case LOC_CIV_CARD_1:
        case LOC_CIV_CARD_2: case LOC_CIV_CARD_3: {
            int slot = loc - LOC_CIV_CARD_0;
            if (slot < gs->civ_display_count) {
                int pay[RES_COUNT];
                agent_choose_payment(p, gs->civ_display[slot].cost, pay);
                eval_civ_card_pay(gs, player, slot, pay);
            }
            break;
        }

        case LOC_BUILDING_0: case LOC_BUILDING_1:
        case LOC_BUILDING_2: case LOC_BUILDING_3: {
            int stack = loc - LOC_BUILDING_0;
            if (!gs->building_top_valid[stack]) break;
            Building *b = &gs->building_top[stack];
            int pay[RES_COUNT];
            memset(pay, 0, sizeof(pay));

            if (b->type == BUILD_SPECIFIC) {
                memcpy(pay, b->required, sizeof(pay));
                eval_building(gs, player, stack, pay);

            } else if (b->type == BUILD_SELECTED) {
                pay_selected(p, pay);
                int tot=0, kinds=0;
                for(int r=RES_WOOD;r<RES_COUNT;r++){tot+=pay[r];if(pay[r]>0)kinds++;}
                if (tot == 4 && kinds == 2)
                    eval_building(gs, player, stack, pay);

            } else { /* FLEX */
                int rem = total_resources(p);
                if (rem > 7) rem = 7;
                /* Pay most expensive first (maximize points) */
                for (int r=RES_GOLD; r>=RES_WOOD && rem>0; r--) {
                    int use = p->resources[r] < rem ? p->resources[r] : rem;
                    pay[r] = use; rem -= use;
                }
                if (rem == 0) eval_building(gs, player, stack, pay);
            }
            break;
        }

        default: break;
        }
    }
}
