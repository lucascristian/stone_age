#ifndef UI_H
#define UI_H

#include "types.h"
#include <SDL.h>
#include <SDL_ttf.h>

#define WINDOW_W   1400
#define WINDOW_H   860
#define PANEL_W    360
#define BOARD_W    (WINDOW_W - PANEL_W)

#define C_BG        ((SDL_Color){18,  12,   6, 255})
#define C_PANEL     ((SDL_Color){24,  16,   8, 255})
#define C_BOARD     ((SDL_Color){38,  26,  12, 255})
#define C_LOC_RES   ((SDL_Color){30,  52,  14, 255})
#define C_LOC_SPE   ((SDL_Color){52,  36,  10, 255})
#define C_LOC_CIV   ((SDL_Color){14,  36,  58, 255})
#define C_LOC_BLD   ((SDL_Color){48,  22,  42, 255})
#define C_GOLD      ((SDL_Color){210,170,  40, 255})
#define C_TEXT      ((SDL_Color){220,200, 155, 255})
#define C_MUTED     ((SDL_Color){120,100,  65, 255})
#define C_GREEN     ((SDL_Color){ 80,200,  80, 255})
#define C_RED       ((SDL_Color){220,  70,  70, 255})
#define C_LOG_BG    ((SDL_Color){ 14,   9,   4, 255})
#define C_LOG_TEXT  ((SDL_Color){160, 140, 100, 255})
#define C_ACT_BG    ((SDL_Color){ 22,  15,   6, 255})
#define C_BTN       ((SDL_Color){ 55,  38,  14, 255})
#define C_BTN_HOV   ((SDL_Color){ 85,  60,  20, 255})
#define C_BTN_SEL   ((SDL_Color){110,  80,  24, 255})
#define C_BTN_DIS   ((SDL_Color){ 32,  26,  14, 255})
#define C_OUTLINE   ((SDL_Color){160, 128,  36, 255})
#define C_DIM       ((SDL_Color){ 70,  55,  25, 255})
#define C_DIVIDER   ((SDL_Color){ 60,  45,  18, 255})
#define C_CPU       ((SDL_Color){ 40,  80,  40, 255})
#define C_P0  ((SDL_Color){220,  60,  50, 255})
#define C_P1  ((SDL_Color){ 60, 110, 210, 255})
#define C_P2  ((SDL_Color){ 60, 180,  60, 255})
#define C_P3  ((SDL_Color){210, 180,  50, 255})
#define C_FOOD  ((SDL_Color){ 55, 170,  50, 255})
#define C_WOOD  ((SDL_Color){ 95,  60,  18, 255})
#define C_BRICK ((SDL_Color){185,  72,  32, 255})
#define C_STONE ((SDL_Color){140, 140, 148, 255})
#define C_RGOLD ((SDL_Color){210, 175,  28, 255})

/* Player type: 0=Human, 1=Greedy Resources, 2=Greedy Food, 3=Balanced */
#define PLAYER_HUMAN    0
#define PLAYER_CPU_RES  1
#define PLAYER_CPU_FOOD 2
#define PLAYER_CPU_BAL  3

typedef enum { SCREEN_MENU=0, SCREEN_SETUP, SCREEN_GAME, SCREEN_GAMEOVER } Screen;
typedef enum {
    ACTION_NONE=0,
    ACTION_EVAL_SELECT_TOOLS,
    ACTION_EVAL_CIV_RESOURCE,
    ACTION_EVAL_BUILD_RESOURCE,
} UIActionState;

typedef struct {
    SDL_Rect rect;
    char     label[64];
    int      enabled, hovered, id;
} Button;

#define MAX_BUTTONS 80

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font_lg, *font_md, *font_sm, *font_xs;
    Screen        screen;
    UIActionState action_state;
    LocationType  sel_location;
    int           sel_count;
    int           sel_stack, sel_slot;
    int           temp_res[RES_COUNT];
    int           temp_extra_tool;
    Button        buttons[MAX_BUTTONS];
    int           button_count;
    int           setup_player_count;
    char          setup_names[MAX_PLAYERS][32];
    int           setup_player_types[MAX_PLAYERS]; /* PLAYER_* constants */
    int           player_types[MAX_PLAYERS];        /* active game types */
    int           setup_name_focus;
    int           mouse_x, mouse_y, mouse_clicked;
    char          overlay_msg[128];
    int           overlay_timer;
    int           eval_player;
    int           eval_done[MAX_PLAYERS];
    int           cpu_thinking;  /* delay frames for CPU "thinking" */
} UIState;

int  ui_init(UIState *ui);
void ui_destroy(UIState *ui);
void ui_handle_events(UIState *ui, GameState *gs, int *quit);
void ui_render(UIState *ui, const GameState *gs);
void ui_render_menu(UIState *ui);
void ui_render_setup(UIState *ui);
void ui_render_game(UIState *ui, const GameState *gs);
void ui_render_gameover(UIState *ui, const GameState *gs);
void ui_draw_rect(UIState *ui, SDL_Rect r, SDL_Color c);
void ui_draw_rect_outline(UIState *ui, SDL_Rect r, SDL_Color c, int t);
void ui_fill_rounded(UIState *ui, SDL_Rect r, SDL_Color c, int rad);
void ui_draw_text(UIState *ui, const char *txt, int x, int y, SDL_Color c, TTF_Font *f);
void ui_draw_text_centered(UIState *ui, const char *txt, SDL_Rect b, SDL_Color c, TTF_Font *f);
void ui_draw_resource_icon(UIState *ui, ResourceType res, int x, int y, int sz);
void ui_draw_worker(UIState *ui, int color, int x, int y, int r);
Button *ui_add_button(UIState *ui, int x, int y, int w, int h, const char *lbl, int id, int en);
void    ui_clear_buttons(UIState *ui);
int     ui_button_hit(UIState *ui, int id);
void    ui_draw_buttons(UIState *ui);
void    ui_show_overlay(UIState *ui, const char *msg);

#endif
