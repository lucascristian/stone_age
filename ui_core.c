#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

int ui_init(UIState *ui) {
    memset(ui, 0, sizeof(*ui));
    if (SDL_Init(SDL_INIT_VIDEO)!=0){fprintf(stderr,"SDL:%s\n",SDL_GetError());return 0;}
    if (TTF_Init()!=0){fprintf(stderr,"TTF:%s\n",TTF_GetError());return 0;}
    ui->window=SDL_CreateWindow("Stone Age",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        WINDOW_W,WINDOW_H,SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);
    if(!ui->window)return 0;
    SDL_RaiseWindow(ui->window);
    SDL_SetWindowInputFocus(ui->window);
    ui->renderer=SDL_CreateRenderer(ui->window,-1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH,"1");
    if(!ui->renderer)return 0;
    int dw,dh;
    SDL_GL_GetDrawableSize(ui->window,&dw,&dh);
    SDL_RenderSetScale(ui->renderer,(float)dw/WINDOW_W,(float)dh/WINDOW_H);
    const char *fonts[]={
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        NULL};
    for(int i=0;fonts[i]&&!ui->font_lg;i++){
        ui->font_lg=TTF_OpenFont(fonts[i],26);
        if(ui->font_lg){
            ui->font_md=TTF_OpenFont(fonts[i],17);
            ui->font_sm=TTF_OpenFont(fonts[i],13);
            ui->font_xs=TTF_OpenFont(fonts[i],11);
        }
    }
    if(!ui->font_lg){fprintf(stderr,"No font:%s\n",TTF_GetError());return 0;}
    ui->screen=SCREEN_MENU;
    ui->setup_player_count=2;
    ui->setup_name_focus=-1;
    ui->sel_location=-1;
    for(int i=0;i<MAX_PLAYERS;i++){
        char n[32];snprintf(n,32,"Player %d",i+1);
        strncpy(ui->setup_names[i],n,31);
        ui->setup_player_types[i]=PLAYER_HUMAN;
    }
    return 1;
}

void ui_destroy(UIState *ui){
    if(ui->font_xs)TTF_CloseFont(ui->font_xs);
    if(ui->font_sm)TTF_CloseFont(ui->font_sm);
    if(ui->font_md)TTF_CloseFont(ui->font_md);
    if(ui->font_lg)TTF_CloseFont(ui->font_lg);
    if(ui->renderer)SDL_DestroyRenderer(ui->renderer);
    if(ui->window)SDL_DestroyWindow(ui->window);
    TTF_Quit();SDL_Quit();
}

void ui_draw_rect(UIState *ui,SDL_Rect r,SDL_Color c){
    SDL_SetRenderDrawColor(ui->renderer,c.r,c.g,c.b,c.a);
    SDL_RenderFillRect(ui->renderer,&r);
}
void ui_draw_rect_outline(UIState *ui,SDL_Rect r,SDL_Color c,int t){
    SDL_SetRenderDrawColor(ui->renderer,c.r,c.g,c.b,c.a);
    for(int i=0;i<t;i++){SDL_Rect b={r.x+i,r.y+i,r.w-2*i,r.h-2*i};SDL_RenderDrawRect(ui->renderer,&b);}
}
void ui_fill_rounded(UIState *ui,SDL_Rect r,SDL_Color c,int rad){
    if(rad<1){ui_draw_rect(ui,r,c);return;}
    SDL_SetRenderDrawColor(ui->renderer,c.r,c.g,c.b,c.a);
    SDL_Rect h={r.x,r.y+rad,r.w,r.h-2*rad};
    SDL_Rect v={r.x+rad,r.y,r.w-2*rad,r.h};
    SDL_RenderFillRect(ui->renderer,&h);
    SDL_RenderFillRect(ui->renderer,&v);
    for(int dy=-rad;dy<=rad;dy++){
        int dx=(int)sqrtf((float)(rad*rad-dy*dy));
        SDL_RenderDrawLine(ui->renderer,r.x+rad-dx,r.y+rad+dy,r.x+rad,r.y+rad+dy);
        SDL_RenderDrawLine(ui->renderer,r.x+r.w-rad,r.y+rad+dy,r.x+r.w-rad+dx,r.y+rad+dy);
        SDL_RenderDrawLine(ui->renderer,r.x+rad-dx,r.y+r.h-rad+dy,r.x+rad,r.y+r.h-rad+dy);
        SDL_RenderDrawLine(ui->renderer,r.x+r.w-rad,r.y+r.h-rad+dy,r.x+r.w-rad+dx,r.y+r.h-rad+dy);
    }
}
void ui_draw_text(UIState *ui,const char *txt,int x,int y,SDL_Color c,TTF_Font *f){
    if(!txt||!f||!txt[0])return;
    SDL_Surface *s=TTF_RenderText_Blended(f,txt,c);
    if(!s)return;
    SDL_Texture *t=SDL_CreateTextureFromSurface(ui->renderer,s);
    SDL_Rect d={x,y,s->w,s->h};
    SDL_RenderCopy(ui->renderer,t,NULL,&d);
    SDL_DestroyTexture(t);SDL_FreeSurface(s);
}
void ui_draw_text_centered(UIState *ui,const char *txt,SDL_Rect b,SDL_Color c,TTF_Font *f){
    if(!txt||!f||!txt[0])return;
    int tw,th;TTF_SizeText(f,txt,&tw,&th);
    ui_draw_text(ui,txt,b.x+(b.w-tw)/2,b.y+(b.h-th)/2,c,f);
}
void ui_draw_resource_icon(UIState *ui,ResourceType res,int x,int y,int sz){
    SDL_Color cols[]={C_FOOD,C_WOOD,C_BRICK,C_STONE,C_RGOLD};
    const char *lbl[]={"F","W","B","S","G"};
    SDL_Color c=(res<RES_COUNT)?cols[res]:(SDL_Color){100,100,100,255};
    SDL_Rect r={x-sz/2,y-sz/2,sz,sz};
    ui_fill_rounded(ui,r,c,3);
    SDL_SetRenderDrawColor(ui->renderer,0,0,0,200);
    SDL_RenderDrawRect(ui->renderer,&r);
    if(res<RES_COUNT){
        int tw,th;TTF_SizeText(ui->font_xs,lbl[res],&tw,&th);
        ui_draw_text(ui,lbl[res],x-tw/2,y-th/2,(SDL_Color){255,255,255,255},ui->font_xs);
    }
}
void ui_draw_worker(UIState *ui,int col,int x,int y,int radius){
    SDL_Color pcs[]={C_P0,C_P1,C_P2,C_P3};
    SDL_Color c=(col>=0&&col<4)?pcs[col]:(SDL_Color){150,150,150,255};
    SDL_SetRenderDrawColor(ui->renderer,c.r,c.g,c.b,255);
    for(int dy=-radius;dy<=radius;dy++){
        int dx=(int)sqrtf((float)(radius*radius-dy*dy));
        SDL_RenderDrawLine(ui->renderer,x-dx,y+dy,x+dx,y+dy);
    }
    SDL_SetRenderDrawColor(ui->renderer,0,0,0,255);
    for(int a=0;a<360;a+=15){
        float rad=a*3.14159f/180.f;
        SDL_RenderDrawPoint(ui->renderer,x+(int)(radius*cosf(rad)),y+(int)(radius*sinf(rad)));
    }
}
Button *ui_add_button(UIState *ui,int x,int y,int w,int h,const char *lbl,int id,int en){
    if(ui->button_count>=MAX_BUTTONS)return NULL;
    Button *b=&ui->buttons[ui->button_count++];
    b->rect=(SDL_Rect){x,y,w,h};strncpy(b->label,lbl,63);
    b->id=id;b->enabled=en;b->hovered=0;return b;
}
void ui_clear_buttons(UIState *ui){ui->button_count=0;}
void ui_draw_buttons(UIState *ui){
    for(int i=0;i<ui->button_count;i++){
        Button *b=&ui->buttons[i];
        SDL_Color bg,tc;
        if(!b->enabled){bg=C_BTN_DIS;tc=C_MUTED;}
        else if(b->hovered){bg=C_BTN_HOV;tc=C_TEXT;}
        else{bg=C_BTN;tc=C_TEXT;}
        ui_fill_rounded(ui,b->rect,bg,5);
        ui_draw_rect_outline(ui,b->rect,b->enabled?C_OUTLINE:C_DIM,1);
        ui_draw_text_centered(ui,b->label,b->rect,tc,ui->font_sm);
    }
}
int ui_button_hit(UIState *ui,int id){
    if(!ui->mouse_clicked)return 0;
    SDL_Point pt={ui->mouse_x,ui->mouse_y};
    for(int i=0;i<ui->button_count;i++)
        if(ui->buttons[i].id==id&&ui->buttons[i].enabled&&SDL_PointInRect(&pt,&ui->buttons[i].rect))return 1;
    return 0;
}
void ui_show_overlay(UIState *ui,const char *msg){strncpy(ui->overlay_msg,msg,127);ui->overlay_timer=120;}
