#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ─── Constants ─────────────────────────────────────────────────────────── */
#define MAX_PLAYERS          4
#define MIN_PLAYERS          2
#define MAX_PEOPLE           10   /* per player (5 start + 5 hut gains)     */
#define START_PEOPLE         5
#define START_FOOD           12
#define MAX_TOOLS            3    /* tool slots on player board              */
#define MAX_BUILDINGS        28   /* total building tiles                    */
#define BUILDINGS_PER_STACK  7
#define MAX_BUILDING_STACKS  4
#define MAX_CIV_CARDS        36
#define CIV_DISPLAY_SIZE     4
#define MAX_RESOURCES_EACH   30   /* generous upper bound for supply         */

/* Resource locations – capacity 7 workers each */
#define MAX_WORKERS_RESOURCE 7
#define MAX_WORKERS_TOOL     1
#define MAX_WORKERS_HUT      2
#define MAX_WORKERS_FIELD    1

/* Dice divisors for each resource */
#define DIV_FOOD   2
#define DIV_WOOD   3
#define DIV_BRICK  4
#define DIV_STONE  5
#define DIV_GOLD   6

/* Resource values for building scoring */
#define VAL_FOOD   1   /* not used in buildings but kept for completeness  */
#define VAL_WOOD   3
#define VAL_BRICK  4
#define VAL_STONE  5
#define VAL_GOLD   6

/* ─── Enumerations ───────────────────────────────────────────────────────── */
typedef enum {
    RES_FOOD = 0,
    RES_WOOD,
    RES_BRICK,
    RES_STONE,
    RES_GOLD,
    RES_COUNT
} ResourceType;

typedef enum {
    LOC_HUNT = 0,
    LOC_FOREST,
    LOC_CLAY_PIT,
    LOC_QUARRY,
    LOC_RIVER,
    LOC_TOOL_MAKER,
    LOC_HUT,
    LOC_FIELD,
    LOC_CIV_CARD_0,
    LOC_CIV_CARD_1,
    LOC_CIV_CARD_2,
    LOC_CIV_CARD_3,
    LOC_BUILDING_0,
    LOC_BUILDING_1,
    LOC_BUILDING_2,
    LOC_BUILDING_3,
    LOC_COUNT
} LocationType;

typedef enum {
    BUILD_SPECIFIC = 0,   /* fixed resources, fixed amounts, fixed points  */
    BUILD_SELECTED,       /* player chooses resources, fixed amount (4)    */
    BUILD_FLEX            /* player chooses resources AND amount (1-7)     */
} BuildingType;

typedef enum {
    CIV_TOP_FOOD = 0,
    CIV_TOP_RESOURCE,
    CIV_TOP_DICE_RESOURCE,
    CIV_TOP_POINTS,
    CIV_TOP_TOOL,
    CIV_TOP_FOOD_PROD,
    CIV_TOP_ANY_CARD,
    CIV_TOP_ONETIME_TOOL,
    CIV_TOP_TWO_RESOURCES,
    CIV_TOP_DICE_CHOICE     /* the "roll N dice, each player picks" card   */
} CivCardTopType;

typedef enum {
    CIV_BOT_GREEN = 0,      /* culture symbol – multiply different symbols */
    CIV_BOT_FARMER,         /* sand: farmers × food_track                  */
    CIV_BOT_TOOL_MAKER,     /* sand: tool_makers × total_tools             */
    CIV_BOT_HUT_BUILDER,    /* sand: hut_builders × buildings              */
    CIV_BOT_SHAMAN          /* sand: shamen × people                       */
} CivCardBotType;

/* Culture symbols (green background civ cards) */
typedef enum {
    CULTURE_HEALING = 0,
    CULTURE_ART,
    CULTURE_WRITING,
    CULTURE_POTTERY,
    CULTURE_TIME,
    CULTURE_TRANSPORT,
    CULTURE_MUSIC,
    CULTURE_WEAVING,
    CULTURE_COUNT
} CultureSymbol;

/* Game phase */
typedef enum {
    PHASE_PLACEMENT = 0,
    PHASE_ACTIONS,
    PHASE_FEEDING,
    PHASE_ROUND_END
} GamePhase;

/* ─── Structs ─────────────────────────────────────────────────────────────  */

typedef struct {
    BuildingType type;
    int          points;                  /* for SPECIFIC type              */
    int          required[RES_COUNT];     /* for SPECIFIC type              */
    int          amount;                  /* for SELECTED: always 4         */
    /* FLEX: 1-7 of any mix – points computed from resource values          */
} Building;

typedef struct {
    CivCardTopType  top_type;
    int             top_value;            /* food amount / resource type /
                                             points / tool value / etc.     */
    int             top_resource;         /* for CIV_TOP_RESOURCE           */

    CivCardBotType  bot_type;
    CultureSymbol   culture;              /* if bot_type == CIV_BOT_GREEN   */

    int             onetime_tool_value;   /* for CIV_TOP_ONETIME_TOOL       */
    int             cost;                 /* resources needed to buy card   */
} CivCard;

typedef struct {
    int value;          /* 1, 2, 3, or 4   */
    int used;           /* 1 = used this round, turned 90° */
} Tool;

typedef struct {
    char        name[32];
    int         color;                    /* 0-3 for SDL colour index        */

    /* Economy */
    int         resources[RES_COUNT];     /* food, wood, brick, stone, gold  */
    int         food_track;               /* position on food track (0-10)   */
    int         score;                    /* position on scoring track       */

    /* People */
    int         people_total;            /* current total people            */
    int         people_available;        /* not yet placed this round       */
    int         workers_at[LOC_COUNT];   /* workers placed per location     */

    /* Tools */
    Tool        tools[MAX_TOOLS];
    int         tool_count;              /* how many tool slots filled      */

    /* One-time tool cards (from civ cards) */
    int         onetime_tools[3];
    int         onetime_tool_count;

    /* Two-resource card available */
    int         has_two_res_card;

    /* Acquired civ cards */
    CivCard     civ_cards[MAX_CIV_CARDS];
    int         civ_card_count;

    /* Acquired buildings */
    Building    buildings[MAX_BUILDINGS];
    int         building_count;

    /* Tracking for scoring */
    int         civ_culture_count[CULTURE_COUNT]; /* green bg cards        */
    int         civ_farmer_count;
    int         civ_tool_maker_count;
    int         civ_hut_builder_count;
    int         civ_shaman_count;
} Player;

typedef struct {
    /* Supply on board */
    int     supply[RES_COUNT];             /* wood/brick/stone/gold in area  */

    /* Workers at each location (across all players) */
    int     workers_at[LOC_COUNT];         /* total workers at location      */
    int     owner_at[LOC_COUNT];           /* for single-slot locations      */

    /* Civilization card display (up to 4 face-up) */
    CivCard civ_display[CIV_DISPLAY_SIZE];
    int     civ_display_count;
    CivCard civ_deck[MAX_CIV_CARDS];
    int     civ_deck_size;

    /* Building stacks */
    Building building_stacks[MAX_BUILDING_STACKS][BUILDINGS_PER_STACK];
    int      building_stack_size[MAX_BUILDING_STACKS];
    int      stack_count;                  /* stacks in play (= player count, min 2) */
    /* top face-up building on each stack */
    Building building_top[MAX_BUILDING_STACKS];
    int      building_top_valid[MAX_BUILDING_STACKS];

    /* Players */
    Player   players[MAX_PLAYERS];
    int      player_count;
    int      current_player;              /* index 0..player_count-1        */
    int      start_player;

    /* Round / phase */
    int      round;
    GamePhase phase;

    /* Log */
    char     log[4096];
    int      log_cursor;

    /* Game over */
    int      game_over;
    int      winner;                      /* -1 = not decided yet           */
} GameState;

#endif /* TYPES_H */
