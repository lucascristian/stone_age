#include "ui.h"
#include "game.h"
#include "agents.h"
#include <stdio.h>
#include <string.h>

static const char *res_names[]={"Food","Wood","Brick","Stone","Gold"};

/* Button IDs */
#define BTN_LOC(l)   (100+(l))
#define BTN_CNT(c)   (200+(c))
#define BTN_CONFIRM  300
#define BTN_DONE     301
#define BTN_SKIP     302
#define BTN_NOTOOL   310
#define BTN_TOOL(t)  (311+(t))
#define BTN_RMINUS(r)(400+(r))
#define BTN_RPLUS(r) (450+(r))
#define BTN_FEED     500
#define BTN_SAVE     501
#define BTN_LOAD     502

#define AX   8
#define AY   322
#define AW   (BOARD_W-16)

/* ── Map UI player type to AgentType ─────────────────────────────── */
static AgentType ui_to_agent(int player_type){
    switch(player_type){
        case PLAYER_CPU_RES:  return AGENT_GREEDY_RESOURCES;
        case PLAYER_CPU_FOOD: return AGENT_GREEDY_FOOD;
        case PLAYER_CPU_BAL:  return AGENT_BALANCED;
        default:              return AGENT_BALANCED;
    }
}

/* ── Is current player a CPU? ────────────────────────────────────── */
static int is_cpu(UIState *ui, int player){
    return ui->player_types[player] != PLAYER_HUMAN;
}

/* ── Placement buttons ──────────────────────────────────────────── */
static void build_placement(UIState *ui, const GameState *gs){
    ui_clear_buttons(ui);
    int bw=85, bh=32, gap=5;
    ui_draw_text(ui,"Place workers at:", AX, AY-14, C_GOLD, ui->font_xs);
    const char *lnames[]={"Hunt","Forest","Clay","Quarry","River","Tool","Hut","Field"};
    LocationType locs[]={LOC_HUNT,LOC_FOREST,LOC_CLAY_PIT,LOC_QUARRY,LOC_RIVER,
                         LOC_TOOL_MAKER,LOC_HUT,LOC_FIELD};
    int bx=AX;
    for(int i=0;i<8;i++){
        int can=can_place_workers(gs,gs->current_player,locs[i],1)||
                can_place_workers(gs,gs->current_player,locs[i],2);
        ui_add_button(ui,bx,AY,bw,bh,lnames[i],BTN_LOC(locs[i]),can);
        bx+=bw+gap;
    }
    bx=AX;
    int row2y=AY+bh+gap;
    const char *civn[]={"Civ 1","Civ 2","Civ 3","Civ 4"};
    for(int i=0;i<CIV_DISPLAY_SIZE;i++){
        int en=(i<gs->civ_display_count)&&
               can_place_workers(gs,gs->current_player,(LocationType)(LOC_CIV_CARD_0+i),1);
        ui_add_button(ui,bx,row2y,bw,bh,civn[i],BTN_LOC(LOC_CIV_CARD_0+i),en);
        bx+=bw+gap;
    }
    const char *bldn[]={"Bld 1","Bld 2","Bld 3","Bld 4"};
    for(int i=0;i<gs->stack_count;i++){
        int en=gs->building_top_valid[i]&&
               can_place_workers(gs,gs->current_player,(LocationType)(LOC_BUILDING_0+i),1);
        ui_add_button(ui,bx,row2y,bw,bh,bldn[i],BTN_LOC(LOC_BUILDING_0+i),en);
        bx+=bw+gap;
    }
    if(ui->sel_location>=0&&ui->sel_location<LOC_COUNT){
        int row3y=row2y+bh+gap;
        ui_draw_text(ui,"How many workers:", AX, row3y-13, C_GOLD, ui->font_xs);
        for(int c=1;c<=7;c++){
            char lbl[4];snprintf(lbl,4,"%d",c);
            int en=can_place_workers(gs,gs->current_player,ui->sel_location,c);
            ui_add_button(ui,AX+(c-1)*46,row3y,40,30,lbl,BTN_CNT(c),en);
        }
        int can_confirm=(ui->sel_count>0)&&
                        can_place_workers(gs,gs->current_player,ui->sel_location,ui->sel_count);
        ui_add_button(ui,AX,row3y+38,110,32,"Confirm",BTN_CONFIRM,can_confirm);
    }
    ui_add_button(ui,BOARD_W-188,AY,82,28,"Save",BTN_SAVE,1);
    ui_add_button(ui,BOARD_W-98, AY,82,28,"Load",BTN_LOAD,1);
}

/* ── Action buttons ─────────────────────────────────────────────── */
static void build_actions(UIState *ui, const GameState *gs){
    ui_clear_buttons(ui);
    const Player *p=&gs->players[ui->eval_player];
    int bw=120, bh=32, gap=5, bx=AX, by=AY;
    ui_draw_text(ui,"Resolve your workers:", AX, AY-14, C_GOLD, ui->font_xs);
    const char *albl[]={"Hunt","Forest","Clay Pit","Quarry","River",
        "Tool Maker","Hut","Field",
        "Civ Card 1","Civ Card 2","Civ Card 3","Civ Card 4",
        "Building 1","Building 2","Building 3","Building 4"};
    for(int loc=0;loc<LOC_COUNT;loc++){
        if(p->workers_at[loc]>0){
            ui_add_button(ui,bx,by,bw,bh,albl[loc],BTN_LOC(loc),1);
            bx+=bw+gap;
            if(bx+bw>BOARD_W-8){bx=AX;by+=bh+gap;}
        }
    }
    ui_add_button(ui,BOARD_W-180,AY,170,32,"Done (next player)",BTN_DONE,1);
}

static void build_tools(UIState *ui, const GameState *gs){
    const Player *p=&gs->players[ui->eval_player];
    int by=AY+42;
    ui_draw_text(ui,"Add tools to roll?", AX, by-13, C_GOLD, ui->font_xs);
    ui_add_button(ui,AX,by,88,30,"No Tools",BTN_NOTOOL,1);
    int bx=AX+96;
    for(int t=0;t<p->tool_count;t++){
        if(!p->tools[t].used){
            char lbl[16];snprintf(lbl,sizeof(lbl),"+%d Tool",p->tools[t].value);
            ui_add_button(ui,bx,by,88,30,lbl,BTN_TOOL(t),1);
            bx+=96;
        }
    }
    ui_add_button(ui,AX,by+38,100,30,"Confirm",BTN_CONFIRM,1);
}

static void build_payment(UIState *ui, const GameState *gs, int required){
    const Player *p=&gs->players[ui->eval_player];
    int by=AY+42;
    int current=0;
    for(int r=0;r<RES_COUNT;r++)current+=ui->temp_res[r];
    ui_draw_text(ui,"Choose resources to pay:", AX, by-13, C_GOLD, ui->font_xs);
    int rx=AX;
    for(int r=RES_WOOD;r<RES_COUNT;r++){
        char minus[10],plus[10],val[12];
        snprintf(minus,sizeof(minus),"-%s",res_names[r]);
        snprintf(plus,sizeof(plus),"+%s",res_names[r]);
        snprintf(val,sizeof(val),"%d/%d",ui->temp_res[r],p->resources[r]);
        ui_draw_text(ui,res_names[r],rx,by,C_TEXT,ui->font_xs);
        ui_add_button(ui,rx,by+14,52,24,minus,BTN_RMINUS(r),ui->temp_res[r]>0);
        ui_draw_text(ui,val,rx+4,by+42,C_TEXT,ui->font_xs);
        ui_add_button(ui,rx+56,by+14,52,24,plus,BTN_RPLUS(r),p->resources[r]>ui->temp_res[r]);
        rx+=116;
    }
    char tot[40];snprintf(tot,sizeof(tot),"Paying %d / need %d",current,required);
    ui_draw_text(ui,tot,AX,by+68,(current>=required)?C_GREEN:C_RED,ui->font_sm);
    ui_add_button(ui,AX,by+88,120,32,"Confirm Pay",BTN_CONFIRM,current>=required);
    ui_add_button(ui,AX+130,by+88,80,32,"Cancel",BTN_SKIP,1);
}

/* ══ CPU PLACEMENT ═══════════════════════════════════════════════════ */
static void cpu_do_placement(UIState *ui, GameState *gs){
    AgentType at=ui_to_agent(ui->player_types[gs->current_player]);
    agent_place_workers(gs, gs->current_player, at);
    game_log(gs,"[CPU] %s finished placing.",gs->players[gs->current_player].name);
}

/* ══ CPU ACTIONS ══════════════════════════════════════════════════════ */
static void cpu_do_actions(UIState *ui, GameState *gs, int player){
    AgentType at=ui_to_agent(ui->player_types[player]);
    agent_eval_actions(gs, player, at);
    game_log(gs,"[CPU] %s finished actions.",gs->players[player].name);
}

/* ══ HANDLE PLACEMENT ════════════════════════════════════════════════ */
static void handle_placement(UIState *ui, GameState *gs){
    /* If current player is CPU, do it automatically */
    if(is_cpu(ui, gs->current_player)){
        ui_clear_buttons(ui);
        ui_draw_text(ui,"CPU is placing workers...", AX, AY, C_CPU, ui->font_sm);
        /* Small delay so player can see what CPU does */
        if(ui->cpu_thinking>0){ui->cpu_thinking--;return;}
        cpu_do_placement(ui, gs);
        ui->cpu_thinking=20; /* ~0.3s delay between CPU turns */
        if(all_placed(gs)){
            gs->phase=PHASE_ACTIONS;
            gs->current_player=gs->start_player;
            ui->eval_player=gs->start_player;
            memset(ui->eval_done,0,sizeof(ui->eval_done));
            game_log(gs,"Actions phase begins.");
        } else {
            gs->current_player=(gs->current_player+1)%gs->player_count;
        }
        return;
    }

    build_placement(ui,gs);
    for(int loc=0;loc<LOC_COUNT;loc++){
        if(ui_button_hit(ui,BTN_LOC(loc))){
            ui->sel_location=(LocationType)loc;
            ui->sel_count=0;
            if(loc==LOC_HUT)ui->sel_count=2;
            else if(loc==LOC_TOOL_MAKER||loc==LOC_FIELD||
                (loc>=LOC_CIV_CARD_0&&loc<=LOC_CIV_CARD_3)||
                (loc>=LOC_BUILDING_0&&loc<=LOC_BUILDING_3))
                ui->sel_count=1;
        }
    }
    for(int c=1;c<=7;c++)if(ui_button_hit(ui,BTN_CNT(c)))ui->sel_count=c;
    if(ui_button_hit(ui,BTN_CONFIRM)){
        if(place_workers(gs,gs->current_player,ui->sel_location,ui->sel_count)){
            ui->sel_location=-1;ui->sel_count=0;
            if(all_placed(gs)){
                gs->phase=PHASE_ACTIONS;
                gs->current_player=gs->start_player;
                ui->eval_player=gs->start_player;
                memset(ui->eval_done,0,sizeof(ui->eval_done));
                game_log(gs,"Actions phase begins.");
            } else {
                gs->current_player=(gs->current_player+1)%gs->player_count;
            }
        } else ui_show_overlay(ui,"Cannot place there!");
    }
    if(ui_button_hit(ui,BTN_SAVE))
        game_save(gs,"savegame.dat")?ui_show_overlay(ui,"Game saved!"):ui_show_overlay(ui,"Save failed.");
    if(ui_button_hit(ui,BTN_LOAD))
        game_load(gs,"savegame.dat")?ui_show_overlay(ui,"Game loaded!"):ui_show_overlay(ui,"Load failed.");
}

/* ══ HANDLE ACTIONS ══════════════════════════════════════════════════ */
static void handle_actions(UIState *ui, GameState *gs){
    /* If current eval player is CPU, handle automatically */
    if(is_cpu(ui, ui->eval_player)){
        ui_clear_buttons(ui);
        ui_draw_text(ui,"CPU is taking actions...", AX, AY, C_CPU, ui->font_sm);
        if(ui->cpu_thinking>0){ui->cpu_thinking--;return;}
        cpu_do_actions(ui, gs, ui->eval_player);
        ui->cpu_thinking=20;
        ui->eval_done[ui->eval_player]=1;
        int found=0;
        for(int i=1;i<=gs->player_count;i++){
            int next=(ui->eval_player+i)%gs->player_count;
            if(!ui->eval_done[next]){ui->eval_player=next;gs->current_player=next;found=1;break;}
        }
        if(!found){gs->phase=PHASE_FEEDING;game_log(gs,"Feeding phase.");}
        return;
    }

    Player *p=&gs->players[ui->eval_player];

    if(ui->action_state==ACTION_NONE){
        build_actions(ui,gs);
        if(ui_button_hit(ui,BTN_DONE)){
            ui->eval_done[ui->eval_player]=1;
            int found=0;
            for(int i=1;i<=gs->player_count;i++){
                int next=(ui->eval_player+i)%gs->player_count;
                if(!ui->eval_done[next]){ui->eval_player=next;gs->current_player=next;found=1;break;}
            }
            if(!found){gs->phase=PHASE_FEEDING;game_log(gs,"Feeding phase.");}
            return;
        }
        for(int loc=0;loc<LOC_COUNT;loc++){
            if(!ui_button_hit(ui,BTN_LOC(loc)))continue;
            if(p->workers_at[loc]==0)continue;
            ui->sel_location=(LocationType)loc;
            if(loc==LOC_HUNT||(loc>=LOC_FOREST&&loc<=LOC_RIVER)){
                ui->action_state=ACTION_EVAL_SELECT_TOOLS;
                ui->temp_extra_tool=0;
                build_tools(ui,gs);return;
            }
            if(loc>=LOC_CIV_CARD_0&&loc<=LOC_CIV_CARD_3){
                int slot=loc-LOC_CIV_CARD_0;
                if(slot<gs->civ_display_count){
                    ui->sel_slot=slot;
                    ui->action_state=ACTION_EVAL_CIV_RESOURCE;
                    memset(ui->temp_res,0,sizeof(ui->temp_res));
                    build_payment(ui,gs,gs->civ_display[slot].cost);
                }return;
            }
            if(loc>=LOC_BUILDING_0&&loc<=LOC_BUILDING_3){
                int stack=loc-LOC_BUILDING_0;
                if(gs->building_top_valid[stack]){
                    Building *b=&gs->building_top[stack];
                    ui->sel_stack=stack;
                    memset(ui->temp_res,0,sizeof(ui->temp_res));
                    if(b->type==BUILD_SPECIFIC){
                        if(validate_resource_payment(p,b->required))eval_building(gs,ui->eval_player,stack,b->required);
                        else ui_show_overlay(ui,"Not enough resources!");
                        ui->action_state=ACTION_NONE;
                    } else {
                        ui->action_state=ACTION_EVAL_BUILD_RESOURCE;
                        build_payment(ui,gs,(b->type==BUILD_SELECTED)?4:1);
                    }
                }return;
            }
            if(loc==LOC_TOOL_MAKER){eval_tool_maker(gs,ui->eval_player);return;}
            if(loc==LOC_HUT){eval_hut(gs,ui->eval_player);return;}
            if(loc==LOC_FIELD){eval_field(gs,ui->eval_player);return;}
        }
    }

    if(ui->action_state==ACTION_EVAL_SELECT_TOOLS){
        build_tools(ui,gs);
        if(ui_button_hit(ui,BTN_NOTOOL)){
            LocationType loc=ui->sel_location;
            if(loc==LOC_HUNT)eval_hunt_with_tools(gs,ui->eval_player,0);
            else eval_resource_with_tools(gs,ui->eval_player,loc,0);
            ui->action_state=ACTION_NONE;return;
        }
        for(int t=0;t<p->tool_count;t++)
            if(!p->tools[t].used&&ui_button_hit(ui,BTN_TOOL(t))){p->tools[t].used=1;ui->temp_extra_tool+=p->tools[t].value;}
        if(ui_button_hit(ui,BTN_CONFIRM)){
            LocationType loc=ui->sel_location;
            if(loc==LOC_HUNT)eval_hunt_with_tools(gs,ui->eval_player,ui->temp_extra_tool);
            else eval_resource_with_tools(gs,ui->eval_player,loc,ui->temp_extra_tool);
            ui->action_state=ACTION_NONE;ui->temp_extra_tool=0;
        }
    }

    if(ui->action_state==ACTION_EVAL_CIV_RESOURCE){
        int slot=ui->sel_slot;
        int cost=(slot<gs->civ_display_count)?gs->civ_display[slot].cost:0;
        build_payment(ui,gs,cost);
        for(int r=RES_WOOD;r<RES_COUNT;r++){
            if(ui_button_hit(ui,BTN_RMINUS(r))&&ui->temp_res[r]>0)ui->temp_res[r]--;
            if(ui_button_hit(ui,BTN_RPLUS(r))&&p->resources[r]>ui->temp_res[r])ui->temp_res[r]++;
        }
        if(ui_button_hit(ui,BTN_CONFIRM)){eval_civ_card_pay(gs,ui->eval_player,slot,ui->temp_res);ui->action_state=ACTION_NONE;}
        if(ui_button_hit(ui,BTN_SKIP)){
            LocationType loc=(LocationType)(LOC_CIV_CARD_0+slot);
            gs->workers_at[loc]--;p->workers_at[loc]=0;
            ui->action_state=ACTION_NONE;game_log(gs,"%s skips civ card.",p->name);
        }
    }

    if(ui->action_state==ACTION_EVAL_BUILD_RESOURCE){
        int stack=ui->sel_stack;
        Building *b=gs->building_top_valid[stack]?&gs->building_top[stack]:NULL;
        int req=b?(b->type==BUILD_SELECTED?4:1):1;
        build_payment(ui,gs,req);
        for(int r=RES_WOOD;r<RES_COUNT;r++){
            if(ui_button_hit(ui,BTN_RMINUS(r))&&ui->temp_res[r]>0)ui->temp_res[r]--;
            if(ui_button_hit(ui,BTN_RPLUS(r))&&p->resources[r]>ui->temp_res[r])ui->temp_res[r]++;
        }
        if(ui_button_hit(ui,BTN_CONFIRM)){eval_building(gs,ui->eval_player,stack,ui->temp_res);ui->action_state=ACTION_NONE;}
        if(ui_button_hit(ui,BTN_SKIP)){
            LocationType loc=(LocationType)(LOC_BUILDING_0+stack);
            gs->workers_at[loc]--;p->workers_at[loc]=0;ui->action_state=ACTION_NONE;
        }
    }
}

/* ══ HANDLE FEEDING ══════════════════════════════════════════════════ */
static void handle_feeding(UIState *ui, GameState *gs){
    /* Auto-feed when all players are CPU */
    int all_cpu=1;
    for(int i=0;i<gs->player_count;i++)
        if(ui->player_types[i]==PLAYER_HUMAN){all_cpu=0;break;}
    if(all_cpu){
        if(ui->cpu_thinking>0){ui->cpu_thinking--;return;}
        feed_players(gs);
        if(check_game_over(gs)){score_final(gs);ui->screen=SCREEN_GAMEOVER;}
        else round_end(gs);
        ui->cpu_thinking=15;
        return;
    }
    ui_clear_buttons(ui);
    ui_draw_text(ui,"Feed your people - each person needs 1 food.",AX,AY-14,C_GOLD,ui->font_xs);
    ui_add_button(ui,AX,AY,180,38,"Feed All Players",BTN_FEED,1);
    if(ui_button_hit(ui,BTN_FEED)){
        feed_players(gs);
        if(check_game_over(gs)){score_final(gs);ui->screen=SCREEN_GAMEOVER;}
        else round_end(gs);
    }
}

/* ══ MAIN EVENT HANDLER ══════════════════════════════════════════════ */
void ui_handle_events(UIState *ui, GameState *gs, int *quit){
    ui->mouse_clicked=0;
    for(int i=0;i<ui->button_count;i++){
        SDL_Point pt={ui->mouse_x,ui->mouse_y};
        ui->buttons[i].hovered=SDL_PointInRect(&pt,&ui->buttons[i].rect);
    }

    SDL_Event ev;
    while(SDL_PollEvent(&ev)){
        switch(ev.type){
            case SDL_QUIT:*quit=1;break;
            case SDL_MOUSEMOTION:
                ui->mouse_x=ev.motion.x;ui->mouse_y=ev.motion.y;break;
            case SDL_MOUSEBUTTONDOWN:
                if(ev.button.button==SDL_BUTTON_LEFT){
                    ui->mouse_x=ev.button.x;ui->mouse_y=ev.button.y;
                    ui->mouse_clicked=1;
                }break;
            case SDL_KEYDOWN:
                if(ui->screen==SCREEN_SETUP&&ui->setup_name_focus>=0){
                    int f=ui->setup_name_focus;
                    int len=(int)strlen(ui->setup_names[f]);
                    if(ev.key.keysym.sym==SDLK_BACKSPACE&&len>0)ui->setup_names[f][len-1]='\0';
                    if(ev.key.keysym.sym==SDLK_RETURN||ev.key.keysym.sym==SDLK_TAB)
                        ui->setup_name_focus=(f+1)%ui->setup_player_count;
                }
                if(ev.key.keysym.sym==SDLK_ESCAPE&&ui->screen==SCREEN_GAME)ui->screen=SCREEN_MENU;
                break;
            case SDL_TEXTINPUT:
                if(ui->screen==SCREEN_SETUP&&ui->setup_name_focus>=0){
                    int f=ui->setup_name_focus;
                    int len=(int)strlen(ui->setup_names[f]);
                    if(len<30)strncat(ui->setup_names[f],ev.text.text,30-len);
                }break;
        }
    }

    /* ── Screen logic ──────────────────────────────────────────── */
    if(ui->screen==SCREEN_MENU){
        int cx=WINDOW_W/2;
        ui_clear_buttons(ui);
        ui_add_button(ui,cx-130,200,260,50,"New Game",1,1);
        ui_add_button(ui,cx-130,263,260,50,"Load Game",2,1);
        ui_add_button(ui,cx-130,326,260,50,"Quit",3,1);
        if(ui->mouse_clicked){
            SDL_Point pt={ui->mouse_x,ui->mouse_y};
            for(int i=0;i<ui->button_count;i++){
                if(!SDL_PointInRect(&pt,&ui->buttons[i].rect))continue;
                int id=ui->buttons[i].id;
                if(id==1){ui->screen=SCREEN_SETUP;SDL_StartTextInput();}
                if(id==2){
                    if(game_load(gs,"savegame.dat")){ui->screen=SCREEN_GAME;ui_show_overlay(ui,"Loaded!");}
                    else ui_show_overlay(ui,"No save file found.");
                }
                if(id==3)*quit=1;
            }
        }
    }
    else if(ui->screen==SCREEN_SETUP){
        int cx=WINDOW_W/2;
        ui_clear_buttons(ui);
        /* Player count buttons */
        for(int n=2;n<=4;n++){
            char l[4];snprintf(l,4,"%d",n);
            SDL_Rect r={cx-90+(n-2)*90,148,70,40};
            ui_add_button(ui,r.x,r.y,r.w,r.h,l,10+n,1);
        }
        /* Name + type buttons per player */
        int row_start=202;
        for(int i=0;i<ui->setup_player_count;i++){
            int ry=row_start+i*70;
            for(int t=0;t<4;t++)
                ui_add_button(ui,cx-160+t*100,ry+36,94,32,"",30+i*10+t,1);
        }
        int by=row_start+ui->setup_player_count*70+20;
        ui_add_button(ui,cx-134,by,120,44,"Back",5,1);
        ui_add_button(ui,cx+14,by,120,44,"Start Game",6,1);
        SDL_StartTextInput();

        if(ui->mouse_clicked){
            SDL_Point pt={ui->mouse_x,ui->mouse_y};
            for(int i=0;i<ui->button_count;i++){
                if(!SDL_PointInRect(&pt,&ui->buttons[i].rect))continue;
                int id=ui->buttons[i].id;
                if(id>=10&&id<=14)ui->setup_player_count=id-10;
                /* names fixed */
                /* Player type buttons: 30 + player*10 + type */
                if(id>=30&&id<=73){
                    int player=(id-30)/10;
                    int type=(id-30)%10;
                    if(player<MAX_PLAYERS&&type<4)
                        ui->setup_player_types[player]=type;
                }
                if(id==5){ui->screen=SCREEN_MENU;ui->setup_name_focus=-1;SDL_StopTextInput();}
                if(id==6){
                    for(int j=0;j<ui->setup_player_count;j++)
                        if(!strlen(ui->setup_names[j])){char d[32];snprintf(d,32,"Player %d",j+1);strncpy(ui->setup_names[j],d,31);}
                    game_init(gs,ui->setup_player_count,(const char(*)[32])ui->setup_names);
                    /* Copy player types to active game */
                    for(int j=0;j<MAX_PLAYERS;j++)
                        ui->player_types[j]=ui->setup_player_types[j];
                    ui->screen=SCREEN_GAME;
                    ui->sel_location=-1;ui->sel_count=0;
                    ui->action_state=ACTION_NONE;
                    ui->eval_player=gs->start_player;
                    ui->cpu_thinking=0;
                    memset(ui->eval_done,0,sizeof(ui->eval_done));
                    ui->setup_name_focus=-1;
                    SDL_StopTextInput();
                }
            }
        }
    }
    else if(ui->screen==SCREEN_GAME){
        switch(gs->phase){
            case PHASE_PLACEMENT:handle_placement(ui,gs);break;
            case PHASE_ACTIONS:  handle_actions(ui,gs);  break;
            case PHASE_FEEDING:  handle_feeding(ui,gs);  break;
            default:break;
        }
    }
    else if(ui->screen==SCREEN_GAMEOVER){
        ui_clear_buttons(ui);
        int cx=WINDOW_W/2;
        int sy=166+30+gs->player_count*50+20;
        ui_add_button(ui,cx-134,sy,120,44,"Main Menu",1,1);
        ui_add_button(ui,cx+14,sy,120,44,"Quit",3,1);
        if(ui->mouse_clicked){
            SDL_Point pt={ui->mouse_x,ui->mouse_y};
            for(int i=0;i<ui->button_count;i++){
                if(!SDL_PointInRect(&pt,&ui->buttons[i].rect))continue;
                if(ui->buttons[i].id==1)ui->screen=SCREEN_MENU;
                if(ui->buttons[i].id==3)*quit=1;
            }
        }
    }
}
