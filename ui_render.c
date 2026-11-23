#include "ui.h"
#include "game.h"
#include <stdio.h>
#include <string.h>

static SDL_Color player_colors[4]={C_P0,C_P1,C_P2,C_P3};
static void hline(UIState *ui,int x,int y,int w,SDL_Color c){
    SDL_SetRenderDrawColor(ui->renderer,c.r,c.g,c.b,c.a);
    SDL_RenderDrawLine(ui->renderer,x,y,x+w,y);
}
static void vline(UIState *ui,int x,int y,int h,SDL_Color c){
    SDL_SetRenderDrawColor(ui->renderer,c.r,c.g,c.b,c.a);
    SDL_RenderDrawLine(ui->renderer,x,y,x,y+h);
}

/* ══ MENU ════════════════════════════════════════════════════════════ */
void ui_render_menu(UIState *ui){
    SDL_SetRenderDrawColor(ui->renderer,C_BG.r,C_BG.g,C_BG.b,255);
    SDL_RenderClear(ui->renderer);
    int cx=WINDOW_W/2;
    ui_draw_text_centered(ui,"STONE AGE",(SDL_Rect){0,70,WINDOW_W,56},C_GOLD,ui->font_lg);
    ui_draw_text_centered(ui,"The Board Game",(SDL_Rect){0,134,WINDOW_W,26},C_MUTED,ui->font_md);
    hline(ui,cx-180,168,360,C_OUTLINE);
    ui_clear_buttons(ui);
    ui_add_button(ui,cx-130,200,260,50,"New Game",1,1);
    ui_add_button(ui,cx-130,263,260,50,"Load Game",2,1);
    ui_add_button(ui,cx-130,326,260,50,"Quit",3,1);
    ui_draw_buttons(ui);
    const char *rnames[]={"Food","Wood","Brick","Stone","Gold"};
    ResourceType rr[]={RES_FOOD,RES_WOOD,RES_BRICK,RES_STONE,RES_GOLD};
    int ly=430;
    ui_draw_text_centered(ui,"Resources",(SDL_Rect){0,ly,WINDOW_W,20},C_MUTED,ui->font_sm);
    for(int i=0;i<5;i++){
        int rx=cx-240+i*120;
        ui_draw_resource_icon(ui,rr[i],rx+18,ly+38,22);
        ui_draw_text_centered(ui,rnames[rr[i]],(SDL_Rect){rx,ly+54,36,14},C_TEXT,ui->font_xs);
    }
}

/* ══ SETUP ════════════════════════════════════════════════════════════ */
void ui_render_setup(UIState *ui){
    SDL_SetRenderDrawColor(ui->renderer,C_BG.r,C_BG.g,C_BG.b,255);
    SDL_RenderClear(ui->renderer);
    int cx=WINDOW_W/2;
    ui_draw_text_centered(ui,"GAME SETUP",(SDL_Rect){0,40,WINDOW_W,50},C_GOLD,ui->font_lg);
    hline(ui,cx-200,98,400,C_OUTLINE);

    /* Player count */
    ui_draw_text_centered(ui,"Number of Players:",(SDL_Rect){0,116,WINDOW_W,24},C_TEXT,ui->font_md);
    ui_clear_buttons(ui);
    for(int n=2;n<=4;n++){
        char l[4];snprintf(l,4,"%d",n);
        int bx=cx-90+(n-2)*90;
        SDL_Rect r={bx,148,70,40};
        SDL_Color bg=(ui->setup_player_count==n)?C_BTN_SEL:C_BTN;
        ui_fill_rounded(ui,r,bg,5);
        ui_draw_rect_outline(ui,r,(ui->setup_player_count==n)?C_GOLD:C_OUTLINE,2);
        ui_draw_text_centered(ui,l,r,C_TEXT,ui->font_md);
        ui_add_button(ui,r.x,r.y,r.w,r.h,l,10+n,1);
    }

    /* Player rows */
    SDL_Color pcs[]={C_P0,C_P1,C_P2,C_P3};
    const char *type_labels[]={"Human","CPU: Res","CPU: Food","CPU: Bal"};
    int row_start=202;
    for(int i=0;i<ui->setup_player_count;i++){
        int ry=row_start+i*70;
        SDL_Color pc=pcs[i];
        SDL_Rect sw={cx-220,ry+10,16,16};
        ui_fill_rounded(ui,sw,pc,3);
        ui_draw_text(ui,ui->setup_names[i],cx-196,ry+8,pc,ui->font_md);
        for(int t=0;t<4;t++){
            SDL_Rect tb={cx-160+t*100,ry+36,94,32};
            int sel=(ui->setup_player_types[i]==t);
            SDL_Color tbg=sel?(SDL_Color){90,65,14,255}:C_BTN;
            ui_fill_rounded(ui,tb,tbg,5);
            ui_draw_rect_outline(ui,tb,sel?C_GOLD:C_DIM,sel?2:1);
            ui_draw_text_centered(ui,type_labels[t],tb,sel?C_GOLD:C_MUTED,ui->font_sm);
            ui_add_button(ui,tb.x,tb.y,tb.w,tb.h,type_labels[t],30+i*10+t,1);
        }
    }
    int by=row_start+ui->setup_player_count*70+20;
    ui_add_button(ui,cx-134,by,120,44,"Back",5,1);
    ui_add_button(ui,cx+14,by,120,44,"Start Game",6,1);
    ui_draw_buttons(ui);
    ui_draw_text_centered(ui,"Click a type button to choose Human or CPU for each player.",
                          (SDL_Rect){0,by+54,WINDOW_W,16},C_MUTED,ui->font_xs);
}

/* ══ GAME BOARD ══════════════════════════════════════════════════════ */
static SDL_Rect loc_rect(LocationType loc){
    int g=4,lw=119,lh=86;
    switch(loc){
        case LOC_HUNT:      return (SDL_Rect){g+0*(lw+g),g,lw,lh};
        case LOC_FOREST:    return (SDL_Rect){g+1*(lw+g),g,lw,lh};
        case LOC_CLAY_PIT:  return (SDL_Rect){g+2*(lw+g),g,lw,lh};
        case LOC_QUARRY:    return (SDL_Rect){g+3*(lw+g),g,lw,lh};
        case LOC_RIVER:     return (SDL_Rect){g+4*(lw+g),g,lw,lh};
        case LOC_TOOL_MAKER:return (SDL_Rect){g+5*(lw+g),g,lw,lh};
        case LOC_HUT:       return (SDL_Rect){g+6*(lw+g),g,lw,lh};
        case LOC_FIELD:     return (SDL_Rect){g+7*(lw+g),g,lw,lh};
        case LOC_CIV_CARD_0:return (SDL_Rect){g+0*244,g+lh+g,240,86};
        case LOC_CIV_CARD_1:return (SDL_Rect){g+1*244,g+lh+g,240,86};
        case LOC_CIV_CARD_2:return (SDL_Rect){g+2*244,g+lh+g,240,86};
        case LOC_CIV_CARD_3:return (SDL_Rect){g+3*244,g+lh+g,240,86};
        case LOC_BUILDING_0:return (SDL_Rect){g+0*254,g+2*(lh+g),250,98};
        case LOC_BUILDING_1:return (SDL_Rect){g+1*254,g+2*(lh+g),250,98};
        case LOC_BUILDING_2:return (SDL_Rect){g+2*254,g+2*(lh+g),250,98};
        case LOC_BUILDING_3:return (SDL_Rect){g+3*254,g+2*(lh+g),250,98};
        default:            return (SDL_Rect){0,0,0,0};
    }
}
static SDL_Color loc_bg(LocationType loc){
    if(loc==LOC_HUNT)return C_LOC_SPE;
    if(loc>=LOC_FOREST&&loc<=LOC_RIVER)return C_LOC_RES;
    if(loc==LOC_TOOL_MAKER||loc==LOC_HUT||loc==LOC_FIELD)return C_LOC_SPE;
    if(loc>=LOC_CIV_CARD_0&&loc<=LOC_CIV_CARD_3)return C_LOC_CIV;
    return C_LOC_BLD;
}
static const char *loc_title(LocationType loc){
    const char *n[]={"HUNT","FOREST","CLAY PIT","QUARRY","RIVER","TOOL MAKER","HUT","FIELD",
        "CIV CARD 1","CIV CARD 2","CIV CARD 3","CIV CARD 4",
        "BUILDING 1","BUILDING 2","BUILDING 3","BUILDING 4"};
    return loc<LOC_COUNT?n[loc]:"?";
}
static const char *loc_sub(LocationType loc){
    if(loc==LOC_HUNT)return "div 2 -> food";
    if(loc==LOC_FOREST)return "div 3 -> wood";
    if(loc==LOC_CLAY_PIT)return "div 4 -> brick";
    if(loc==LOC_QUARRY)return "div 5 -> stone";
    if(loc==LOC_RIVER)return "div 6 -> gold";
    if(loc==LOC_TOOL_MAKER)return "get a tool";
    if(loc==LOC_HUT)return "+1 person";
    if(loc==LOC_FIELD)return "+1 food prod";
    return "";
}
static void draw_loc(UIState *ui,const GameState *gs,LocationType loc,int sel){
    SDL_Rect r=loc_rect(loc);
    ui_fill_rounded(ui,r,loc_bg(loc),6);
    ui_draw_rect_outline(ui,r,sel?C_GOLD:C_OUTLINE,sel?3:1);
    ui_draw_text(ui,loc_title(loc),r.x+6,r.y+4,C_GOLD,ui->font_xs);
    const char *sub=loc_sub(loc);
    if(sub[0])ui_draw_text(ui,sub,r.x+6,r.y+16,C_MUTED,ui->font_xs);
    if(loc>=LOC_CIV_CARD_0&&loc<=LOC_CIV_CARD_3){
        int slot=loc-LOC_CIV_CARD_0;
        if(slot<gs->civ_display_count){
            char buf[32];snprintf(buf,sizeof(buf),"Cost: %d",gs->civ_display[slot].cost);
            ui_draw_text(ui,buf,r.x+6,r.y+16,C_TEXT,ui->font_xs);
            const char *eff="";
            switch(gs->civ_display[slot].top_type){
                case CIV_TOP_FOOD:eff="[+Food]";break;
                case CIV_TOP_RESOURCE:eff="[+Resource]";break;
                case CIV_TOP_POINTS:eff="[+3 Points]";break;
                case CIV_TOP_TOOL:eff="[+Tool]";break;
                case CIV_TOP_FOOD_PROD:eff="[+Food Prod]";break;
                case CIV_TOP_ONETIME_TOOL:eff="[One-time Tool]";break;
                case CIV_TOP_TWO_RESOURCES:eff="[2 Resources]";break;
                default:eff="[Effect]";break;
            }
            ui_draw_text(ui,eff,r.x+6,r.y+30,C_GREEN,ui->font_xs);
            const char *bot="";
            switch(gs->civ_display[slot].bot_type){
                case CIV_BOT_GREEN:bot="Culture symbol";break;
                case CIV_BOT_FARMER:bot="x Food track";break;
                case CIV_BOT_TOOL_MAKER:bot="x Tools";break;
                case CIV_BOT_HUT_BUILDER:bot="x Buildings";break;
                case CIV_BOT_SHAMAN:bot="x People";break;
            }
            ui_draw_text(ui,bot,r.x+6,r.y+44,C_MUTED,ui->font_xs);
        } else ui_draw_text_centered(ui,"(empty)",r,C_MUTED,ui->font_xs);
    }
    if(loc>=LOC_BUILDING_0&&loc<=LOC_BUILDING_3){
        int stack=loc-LOC_BUILDING_0;
        if(stack<gs->stack_count&&gs->building_top_valid[stack]){
            const Building *b=&gs->building_top[stack];
            char pts[32];
            if(b->type==BUILD_SPECIFIC)snprintf(pts,sizeof(pts),"%d points",b->points);
            else if(b->type==BUILD_SELECTED)snprintf(pts,sizeof(pts),"4 res, 2 kinds");
            else snprintf(pts,sizeof(pts),"1-7 res (flex)");
            ui_draw_text(ui,pts,r.x+6,r.y+16,C_GOLD,ui->font_xs);
            if(b->type==BUILD_SPECIFIC){
                int ix=r.x+6;
                for(int ri=RES_WOOD;ri<RES_COUNT;ri++)
                    for(int q=0;q<b->required[ri];q++){ui_draw_resource_icon(ui,(ResourceType)ri,ix+9,r.y+50,16);ix+=20;}
            } else ui_draw_text(ui,"Choose resources",r.x+6,r.y+44,C_MUTED,ui->font_xs);
            char left[24];snprintf(left,sizeof(left),"Left:%d",gs->building_stack_size[stack]);
            ui_draw_text(ui,left,r.x+r.w-46,r.y+4,C_MUTED,ui->font_xs);
        } else ui_draw_text_centered(ui,"EMPTY",r,C_MUTED,ui->font_sm);
    }
    /* Worker dots */
    int wx=r.x+6,wy=r.y+r.h-14;
    for(int p=0;p<gs->player_count;p++){
        int wc=gs->players[p].workers_at[loc];
        for(int k=0;k<wc;k++){
            ui_draw_worker(ui,gs->players[p].color,wx+7,wy,7);
            wx+=18;if(wx>r.x+r.w-14){wx=r.x+6;wy-=18;}
        }
    }
}

/* ── Player panel ──────────────────────────────────────────────────── */
static void draw_player_panel(UIState *ui,const GameState *gs,int idx){
    int ph=WINDOW_H/gs->player_count;
    int py=idx*ph;
    int px=BOARD_W+6, pw=PANEL_W-12;
    SDL_Rect bg={BOARD_W,py,PANEL_W,ph};
    ui_draw_rect(ui,bg,C_PANEL);
    if(idx==gs->current_player){
        SDL_Color hl=player_colors[idx];
        SDL_Rect bar={BOARD_W,py,5,ph};
        ui_draw_rect(ui,bar,hl);
    }
    const Player *p=&gs->players[idx];
    SDL_Color pc=player_colors[idx];

    /* Name + CPU badge */
    SDL_Rect sw={px,py+7,13,13};
    ui_fill_rounded(ui,sw,pc,3);
    ui_draw_text(ui,p->name,px+18,py+6,pc,ui->font_md);

    /* Show CPU type badge */
    if(idx<MAX_PLAYERS&&ui->player_types[idx]!=PLAYER_HUMAN){
        const char *badges[]={"","[CPU:Res]","[CPU:Food]","[CPU:Bal]"};
        ui_draw_text(ui,badges[ui->player_types[idx]],px+18+100,py+8,C_CPU,ui->font_xs);
    }

    char buf[64];
    snprintf(buf,sizeof(buf),"Score: %d",p->score);
    ui_draw_text(ui,buf,px,py+26,C_TEXT,ui->font_sm);
    snprintf(buf,sizeof(buf),"Food prod: %d",p->food_track);
    ui_draw_text(ui,buf,px+pw/2,py+26,C_FOOD,ui->font_sm);
    snprintf(buf,sizeof(buf),"People: %d  (avail: %d)",p->people_total,p->people_available);
    ui_draw_text(ui,buf,px,py+44,C_TEXT,ui->font_sm);

    hline(ui,px,py+64,pw,C_DIVIDER);
    int rx=px;
    for(int r2=0;r2<RES_COUNT;r2++){
        ui_draw_resource_icon(ui,(ResourceType)r2,rx+11,py+78,15);
        char rv[8];snprintf(rv,sizeof(rv),"%d",p->resources[r2]);
        ui_draw_text_centered(ui,rv,(SDL_Rect){rx,py+88,22,13},C_TEXT,ui->font_xs);
        rx+=pw/5;
    }
    hline(ui,px,py+106,pw,C_DIVIDER);
    ui_draw_text(ui,"Tools:",px,py+110,C_MUTED,ui->font_xs);
    if(p->tool_count==0)ui_draw_text(ui,"none",px+44,py+110,C_MUTED,ui->font_xs);
    for(int t=0;t<p->tool_count;t++){
        char tv[4];snprintf(tv,sizeof(tv),"%d",p->tools[t].value);
        SDL_Color tc=p->tools[t].used?(SDL_Color){70,58,28,255}:C_GOLD;
        SDL_Rect tr={px+44+t*26,py+108,22,17};
        ui_fill_rounded(ui,tr,(SDL_Color){36,26,10,255},3);
        ui_draw_rect_outline(ui,tr,tc,1);
        ui_draw_text_centered(ui,tv,tr,tc,ui->font_xs);
    }
    hline(ui,px,py+130,pw,C_DIVIDER);
    snprintf(buf,sizeof(buf),"Buildings: %d    Cards: %d",p->building_count,p->civ_card_count);
    ui_draw_text(ui,buf,px,py+134,C_TEXT,ui->font_xs);

    if(idx==gs->current_player){
        const char *ph_str="";
        if(gs->phase==PHASE_PLACEMENT)ph_str=(ui->player_types[idx]!=PLAYER_HUMAN)?">> CPU placing...":">> Placing workers";
        else if(gs->phase==PHASE_ACTIONS)ph_str=(ui->player_types[idx]!=PLAYER_HUMAN)?">> CPU acting...":">> Taking actions";
        else if(gs->phase==PHASE_FEEDING)ph_str=">> Feeding";
        ui_draw_text(ui,ph_str,px,py+ph-18,C_GOLD,ui->font_xs);
    }
    hline(ui,BOARD_W,py+ph-1,PANEL_W,C_DIVIDER);
}

/* ── Log ───────────────────────────────────────────────────────────── */
static void draw_log(UIState *ui,const GameState *gs){
    int lx=0,ly=490,lw=BOARD_W,lh=WINDOW_H-ly;
    SDL_Rect lb={lx,ly,lw,lh};
    ui_draw_rect(ui,lb,C_LOG_BG);
    hline(ui,lx,ly,lw,C_OUTLINE);
    ui_draw_text(ui,"Game Log:",lx+8,ly+4,C_GOLD,ui->font_xs);
    char tmp[4096];strncpy(tmp,gs->log,sizeof(tmp)-1);
    char *lines[300];int nl=0;
    char *tok=strtok(tmp,"\n");
    while(tok&&nl<300){lines[nl++]=tok;tok=strtok(NULL,"\n");}
    int line_h=13,max_lines=(lh-22)/line_h;
    int start=(nl>max_lines)?nl-max_lines:0;
    for(int i=start;i<nl;i++)
        ui_draw_text(ui,lines[i],lx+8,ly+20+(i-start)*line_h,C_LOG_TEXT,ui->font_xs);
}

static void draw_statusbar(UIState *ui,const GameState *gs){
    SDL_Rect sb={0,296,BOARD_W,18};
    ui_draw_rect(ui,sb,C_ACT_BG);
    hline(ui,0,296,BOARD_W,C_OUTLINE);
    char buf[128];
    const char *ptype="";
    if(ui->player_types[gs->current_player]==PLAYER_CPU_RES) ptype=" [CPU: Greedy Resources]";
    else if(ui->player_types[gs->current_player]==PLAYER_CPU_FOOD) ptype=" [CPU: Greedy Food]";
    else if(ui->player_types[gs->current_player]==PLAYER_CPU_BAL) ptype=" [CPU: Balanced]";
    snprintf(buf,sizeof(buf),"Round %d  |  %s  |  Current: %s%s",
        gs->round,
        gs->phase==PHASE_PLACEMENT?"PLACEMENT":
        gs->phase==PHASE_ACTIONS?"ACTIONS":
        gs->phase==PHASE_FEEDING?"FEEDING":"END",
        gs->players[gs->current_player].name, ptype);
    ui_draw_text(ui,buf,8,298,C_GOLD,ui->font_xs);
}

static void draw_action_bg(UIState *ui){
    SDL_Rect ab={0,314,BOARD_W,WINDOW_H-314};
    ui_draw_rect(ui,ab,C_ACT_BG);
    hline(ui,0,314,BOARD_W,C_DIVIDER);
}

/* ══ MAIN GAME RENDER ═════════════════════════════════════════════════ */
void ui_render_game(UIState *ui,const GameState *gs){
    SDL_SetRenderDrawColor(ui->renderer,C_BG.r,C_BG.g,C_BG.b,255);
    SDL_RenderClear(ui->renderer);
    SDL_Rect board={0,0,BOARD_W,296};
    ui_draw_rect(ui,board,C_BOARD);
    for(int loc=0;loc<LOC_COUNT;loc++){
        if(loc>=LOC_CIV_CARD_0&&loc<=LOC_CIV_CARD_3&&loc-LOC_CIV_CARD_0>=CIV_DISPLAY_SIZE)continue;
        if(loc>=LOC_BUILDING_0&&loc<=LOC_BUILDING_3&&loc-LOC_BUILDING_0>=gs->stack_count)continue;
        int sel=(ui->sel_location==(LocationType)loc);
        draw_loc(ui,gs,(LocationType)loc,sel);
    }
    draw_statusbar(ui,gs);
    draw_action_bg(ui);
    SDL_Rect panel={BOARD_W,0,PANEL_W,WINDOW_H};
    ui_draw_rect(ui,panel,C_PANEL);
    vline(ui,BOARD_W,0,WINDOW_H,C_OUTLINE);
    for(int i=0;i<gs->player_count;i++)draw_player_panel(ui,gs,i);
    draw_log(ui,gs);
    ui_draw_buttons(ui);
    if(ui->overlay_timer>0){
        SDL_Rect ov={WINDOW_W/2-200,WINDOW_H/2-24,400,48};
        ui_draw_rect(ui,ov,(SDL_Color){20,12,5,240});
        ui_draw_rect_outline(ui,ov,C_GOLD,2);
        ui_draw_text_centered(ui,ui->overlay_msg,ov,C_GOLD,ui->font_md);
        ui->overlay_timer--;
    }
}

/* ══ GAME OVER ════════════════════════════════════════════════════════ */
void ui_render_gameover(UIState *ui,const GameState *gs){
    SDL_SetRenderDrawColor(ui->renderer,C_BG.r,C_BG.g,C_BG.b,255);
    SDL_RenderClear(ui->renderer);
    ui_draw_text_centered(ui,"GAME OVER",(SDL_Rect){0,44,WINDOW_W,56},C_GOLD,ui->font_lg);
    if(gs->winner>=0){
        char msg[64];snprintf(msg,sizeof(msg),"Winner: %s!",gs->players[gs->winner].name);
        ui_draw_text_centered(ui,msg,(SDL_Rect){0,108,WINDOW_W,34},player_colors[gs->players[gs->winner].color],ui->font_lg);
    }
    hline(ui,WINDOW_W/2-280,150,560,C_OUTLINE);
    int sy=166;
    ui_draw_text_centered(ui,"Final Scores",(SDL_Rect){0,sy,WINDOW_W,22},C_TEXT,ui->font_md);
    sy+=30;
    for(int i=0;i<gs->player_count;i++){
        const Player *p=&gs->players[i];
        SDL_Rect row={WINDOW_W/2-320,sy,640,42};
        ui_fill_rounded(ui,row,(i==gs->winner)?(SDL_Color){48,36,10,255}:C_PANEL,5);
        if(i==gs->winner)ui_draw_rect_outline(ui,row,C_GOLD,1);
        SDL_Rect sw={row.x+10,sy+14,13,13};
        ui_fill_rounded(ui,sw,player_colors[i],3);
        ui_draw_text(ui,p->name,row.x+30,sy+12,player_colors[i],ui->font_md);
        const char *badges[]={"","[CPU:Res]","[CPU:Food]","[CPU:Bal]"};
        if(ui->player_types[i]!=PLAYER_HUMAN)
            ui_draw_text(ui,badges[ui->player_types[i]],row.x+30+120,sy+14,C_CPU,ui->font_xs);
        char sc[16];snprintf(sc,sizeof(sc),"%d pts",p->score);
        ui_draw_text(ui,sc,row.x+220,sy+12,(i==gs->winner)?C_GOLD:C_TEXT,ui->font_md);
        char det[128];
        snprintf(det,sizeof(det),"Food:%d  Tools:%d  Bldgs:%d  Cards:%d",
            p->food_track,p->tool_count,p->building_count,p->civ_card_count);
        ui_draw_text(ui,det,row.x+310,sy+14,C_MUTED,ui->font_xs);
        sy+=50;
    }
    ui_clear_buttons(ui);
    int cx=WINDOW_W/2;
    ui_add_button(ui,cx-134,sy+20,120,44,"Main Menu",1,1);
    ui_add_button(ui,cx+14,sy+20,120,44,"Quit",3,1);
    ui_draw_buttons(ui);
}

void ui_render(UIState *ui,const GameState *gs){
    SDL_SetRenderDrawColor(ui->renderer,C_BG.r,C_BG.g,C_BG.b,255);
    SDL_RenderClear(ui->renderer);
    switch(ui->screen){
        case SCREEN_MENU:     ui_render_menu(ui);         break;
        case SCREEN_SETUP:    ui_render_setup(ui);        break;
        case SCREEN_GAME:     ui_render_game(ui,gs);      break;
        case SCREEN_GAMEOVER: ui_render_gameover(ui,gs);  break;
    }
    SDL_RenderPresent(ui->renderer);
}
