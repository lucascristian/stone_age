/* Full game simulation: 100 games, all player counts, all agent combos */
#include "game.h"
#include "agents.h"
#include <stdio.h>
#include <string.h>

static int bugs=0, completed=0, run=0, total_rounds=0;
static int min_r=9999, max_r=0, starvations=0;

#define FAIL(msg,...) do{printf("[BUG] "msg"\n",##__VA_ARGS__);bugs++;}while(0)

static void validate(GameState *gs){
    for(int i=0;i<gs->player_count;i++){
        Player *p=&gs->players[i];
        for(int r=0;r<RES_COUNT;r++)
            if(p->resources[r]<0) FAIL("P%d neg res%d=%d",i,r,p->resources[r]);
        if(p->score<0) FAIL("P%d neg score",i);
        if(p->people_total<5||p->people_total>MAX_PEOPLE)
            FAIL("P%d bad people %d",i,p->people_total);
        if(p->people_available<0||p->people_available>p->people_total)
            FAIL("P%d bad avail %d/%d",i,p->people_available,p->people_total);
        if(p->food_track<0||p->food_track>10) FAIL("P%d bad food_track",i);
    }
}

static void place_all(GameState *gs, int np, AgentType *types){
    /* Place with stall detection: if no progress for 2 full rounds, force Hunt */
    int prev=-1, stalls=0;
    while(1){
        int avail=0;
        for(int i=0;i<np;i++) avail+=gs->players[i].people_available;
        if(!avail) break;
        if(avail==prev) stalls++;
        else stalls=0;
        if(stalls>np*2){
            for(int i=0;i<np;i++){
                int rem=gs->players[i].people_available;
                if(rem>0){
                    gs->players[i].workers_at[LOC_HUNT]+=rem;
                    gs->workers_at[LOC_HUNT]+=rem;
                    gs->players[i].people_available=0;
                }
            }
            break;
        }
        prev=avail;
        agent_place_workers(gs,gs->current_player,types[gs->current_player]);
        gs->current_player=(gs->current_player+1)%np;
    }
}

static void play(int np, AgentType *t){
    GameState gs; char n[4][32]={"A","B","C","D"};
    game_init(&gs,np,n); run++;
    int done=0;
    while(!gs.game_over && gs.round<=30){
        /* Placement */
        gs.phase=PHASE_PLACEMENT; gs.current_player=gs.start_player;
        for(int i=0;i<np;i++) gs.players[i].people_available=gs.players[i].people_total;
        place_all(&gs,np,t);
        validate(&gs);
        /* Actions */
        gs.phase=PHASE_ACTIONS;
        for(int i=0;i<np;i++) agent_eval_actions(&gs,i,t[i]);
        validate(&gs);
        /* Feeding */
        int mask=feed_players(&gs);
        for(int i=0;i<np;i++) if(mask&(1<<i)) starvations++;
        validate(&gs);
        /* Game over? */
        if(check_game_over(&gs)){score_final(&gs);done=1;break;}
        round_end(&gs); validate(&gs);
    }
    if(done){
        completed++; total_rounds+=gs.round;
        if(gs.round<min_r) min_r=gs.round;
        if(gs.round>max_r) max_r=gs.round;
        for(int i=0;i<np;i++){
            if(gs.players[i].score<0) FAIL("neg final score P%d",i);
            if(gs.players[i].score>500) FAIL("huge score P%d %d",i,gs.players[i].score);
        }
        if(gs.winner<0||gs.winner>=np) FAIL("bad winner %d",gs.winner);
    } else {
        FAIL("game %d did not complete (round %d)",run,gs.round);
    }
}

int main(void){
    printf("===========================================\n");
    printf("  Stone Age Full Simulation (100 games)\n");
    printf("===========================================\n\n");
    dice_init_rng();

    AgentType t2a[2]={AGENT_GREEDY_RESOURCES,AGENT_GREEDY_FOOD};
    AgentType t2b[2]={AGENT_BALANCED,AGENT_BALANCED};
    AgentType t2c[2]={AGENT_GREEDY_RESOURCES,AGENT_BALANCED};
    AgentType t2d[2]={AGENT_GREEDY_FOOD,AGENT_BALANCED};
    AgentType t3[3]={AGENT_GREEDY_RESOURCES,AGENT_GREEDY_FOOD,AGENT_BALANCED};
    AgentType t4[4]={AGENT_GREEDY_RESOURCES,AGENT_GREEDY_FOOD,AGENT_BALANCED,AGENT_BALANCED};

    printf("Running 20x Res vs Food ...\n");    for(int i=0;i<20;i++) play(2,t2a);
    printf("Running 20x Bal vs Bal ...\n");     for(int i=0;i<20;i++) play(2,t2b);
    printf("Running 15x Res vs Bal ...\n");     for(int i=0;i<15;i++) play(2,t2c);
    printf("Running 15x Food vs Bal ...\n");    for(int i=0;i<15;i++) play(2,t2d);
    printf("Running 15x 3-player mixed ...\n"); for(int i=0;i<15;i++) play(3,t3);
    printf("Running 15x 4-player mixed ...\n"); for(int i=0;i<15;i++) play(4,t4);

    printf("\n-------------------------------------------\n");
    printf("Run: %d   Completed: %d/%d\n",run,completed,run);
    if(completed>0){
        printf("Avg rounds: %.1f   Min: %d   Max: %d\n",
               (float)total_rounds/completed, min_r, max_r);
    }
    printf("Starvations: %d   Bugs: %d\n",starvations,bugs);
    printf("-------------------------------------------\n");
    if(!bugs && completed==run) printf("ALL 100 GAMES PASSED WITH NO BUGS\n");
    else printf("ISSUES FOUND\n");
    printf("-------------------------------------------\n");
    return (bugs==0&&completed==run)?0:1;
}
