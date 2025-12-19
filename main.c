
//  full_game.c
//  project_test
//
//  Created by Mateusz Ciesielczyk on 06/12/2025.
//

#include <stdio.h>      // Standard input/output (printf, fprintf)
#include <stdlib.h>     // Standard library (malloc, free, exit)
#include <string.h>     // String operations (memset, strcpy)
#include <unistd.h>     // Unix standard (usleep for timing)
#include <ncurses.h>    // Text-based UI library
#include <time.h>
#include<math.h>


//=================================//
//    STRUCT AND DEFINITIONS      //
//===============================//

// Keyboard controls
#define QUIT        'q'        // Key to quit the game
#define NOKEY        ERR        // ncurses constant for "no key pressed"
#define UP          'w'
#define DOWN        's'
#define LEFT        'a'
#define RIGHT       'd'
#define ACTIVATE_TAXI 't'
// Timing and speed
#define FRAME_TIME    50   // Milliseconds per frame (100ms = 0.1 sec)
#define SPEED_UP     'p'
#define SPEED_DOWN    'o'

#define BORDER 1
// Color pair identifiers (used with ncurses color system)

//characters
// Window dimensions and position
#define STAT_HEIGHT   5
#define BORDER        1        // Border width (in characters)
#define OFFY        2        // Y offset from top of screen
#define OFFX        5        // X offset from left of screen

#define MAX_COMMAND_SIZE 50 // max command size for the configuration.txt
#define THRESHOLD    3 // used for the birds speed

// ranking system
#define NUM_PLAYERS 10
#define RANKING_FILE "ranking.txt"

//TAXI DEFINES
#define SAFE_ZONEW 20
#define SAFE_ZONEH 10
#define BONUS_STARS 15

#define MAX_STARS 10
#define MAX_HUNTERS 6
#define BORDER      1
//COLORS
#define MAIN_COLOR    1        // Main window color
#define STAT_COLOR    2        // Status bar color
#define PLAY_COLOR    3        // Play area color
#define BIRD_COLOR    4        // Bird color
#define STAR_COLOR    8     //star color
#define HUNTER_COLOR  5
#define INJURED_BIRD  6
#define TAXI_COLOR    7

typedef struct {
    WINDOW* window;        // ncurses window pointer
    int x, y;        // position on screen
    int rows, cols;        // size of window
    int color;        // color scheme
} WIN;

typedef struct {
    WIN* win;        // window containing the bird
    int x, y;        // current position
    int dx, dy;        // velocity direction vector
    int speed;        // movement speed
    int counter;
    char *symbol;        // character to display
    int color;      // color scheme
    int score;
    int life;
    int max_life;
    int on_taxi;  //0 false , 1 true
} BIRD;

typedef struct{
    WIN* win;
    double x , y;
    double dx , dy;
    double speed;
    int bounces;
    int damage;
    int height;
    int width;
    int color;
    int active ;  // 1 - is active , 0 - is dead
    int wait_dash;
} HUNTER;

typedef struct {
    WIN* win;
    int x ;
    int y;
    int dx;
    int dy;
    int interval;
    int counter;
    char symbol;
    int color;
} STAR ;

typedef struct{
    WIN* win;
    int x , y;
    int dx , dy ;
    int speed;
    char *symbol;
    int color;
    int counter_of_taxis;
    int active;
    int state ; // 0 waiting time on the bird
    int bonusx[BONUS_STARS]; //position of the bonus points that will appear on the road
    int bonusa[BONUS_STARS]; //1 is a visible bonus 0 is an already collected one
} TAXI;

typedef struct {
    int screen_width;
    int screen_height;
    int star_quota;          // Stars needed to win
    double time_limit;          // Max time in seconds
    int swallow_speed_min;
    int swallow_speed_max;
    int hunter_spawn_rate;   // How often a hunter appears
    int seed;                // Random seed for replayability
    int damage_penalty;      // Life lost when hit
    double hunter_speed;        // Base speed for hunters
    char player_name[50];
    int curr_level;
    int hunter_bounces;
    int hunter_width;
    int hunter_height;
    int hunter_num;
    int available_taxis;
} GameConfig;

typedef struct{
    char name[50];
    int stars_collected;
    int star_quota;
    double time_used;
    int total_score;
} RANKING;


//============================//
// ACTORS AND PHYSICS        //
//==========================//

// ___________BIRD___________//


BIRD* InitBird(WIN* w, int x, int y, int dx, int dy , GameConfig *config)
{
    // Allocate memory for BIRD structure
    BIRD* b = (BIRD*)malloc(sizeof(BIRD));

    // Set bird properties
    b->win = w;            // window containing bird
    b->x = x;            // initial x position
    b->y = y;            // initial y position
    b->dx = dx;            // direction: -1=left, 1=right
    b->dy = dy;            // direction: -1=up, 1=down
    b->speed = config->swallow_speed_min;  // movement speed from the configuration
    b->counter = 0;
    b->symbol = "/|O|\\";        // display character
    b->color = BIRD_COLOR;    // color scheme
    b->score = 0;
    b->life = 100;
    b->max_life = 100;
    return b;
}

void DrawBird(BIRD* b)
{
    // Set bird color
    wattron(b->win->window, COLOR_PAIR(b->color));

    // Draw bird symbol at current position
    mvwprintw(b->win->window, b->y, b->x, "%s", b->symbol);

    // Restore window color
    wattron(b->win->window, COLOR_PAIR(b->win->color));
}

void ClearBird(BIRD* b)
{
    // Overwrite bird with a space character
    mvwprintw(b->win->window, b->y, b->x,"     ");
}

void UpdateBirdColor(BIRD* b)
{
    // We change the color ID based on how much life is left.
    // Note: These color pair IDs (4, 6, 5) must match what we defined in screen.c/Start()
    
    if (b->life > 66) {
        b->color = BIRD_COLOR;
    } else if (b->life > 33) {
        b->color = INJURED_BIRD;
    } else {
        b->color = HUNTER_COLOR; // Pair 5 = Red (Critical)
    }
}

void MoveBird(BIRD* b)
{
    b->counter += b->speed;
    while(b->counter >= THRESHOLD){
        
        
        b->counter -= THRESHOLD;
        // Step 1: Erase bird from old position
        ClearBird(b);
        int bird_size = strlen(b->symbol);
        // Step 2: Check if bird is already at boundary
        // If at boundary, only reverse direction - don't move!
        int at_x_boundary = (b->x <= BORDER) || (b->x >= b->win->cols - BORDER - 1);
        int at_y_boundary = (b->y <= BORDER) || (b->y >= b->win->rows - BORDER - 1);
        
        // Step 3: Handle horizontal movement
        if (at_x_boundary) {
            // Already at X boundary - just reverse direction if needed
            if (b->x <= BORDER) {
                b->dx = 1;     // Change direction to right
            }
            else if (b->x >= b->win->cols - BORDER - bird_size) {
                b->dx = -1;    // Change direction to left
            }
            // Don't change X position!
        }
        // Not at boundary - calculate new position
        int new_x = b->x + b->dx;
        
        // Check if new position would hit boundary
        if (new_x <= BORDER) {
            b->x = BORDER;
            b->dx = 1;
        }
        else if (new_x >= b->win->cols - BORDER - bird_size) {
            b->x = b->win->cols - BORDER - bird_size;
            b->dx = -1;
        }
        else {
            b->x = new_x;    // Accept new position
        }
        
        if (at_y_boundary) {
            // Already at Y boundary - just reverse direction if needed
            if (b->y <= BORDER) {
                b->dy = 1;    // Change direction to down
            }
            else if (b->y >= b->win->rows - BORDER - 1) {
                b->dy = -1;    // Change direction to up
            }
            // Don't change Y position!
        }
        // Not at boundary - calculate new position
        int new_y = b->y + b->dy;
        
        // Check if new position would hit boundary
        if (new_y <= BORDER) {
            b->y = BORDER;
            b->dy = 1;
        }
        else if (new_y >= b->win->rows - BORDER - 1) {
            b->y = b->win->rows - BORDER - 1;
            b->dy = -1;
        }
        else {
            b->y = new_y;    // Accept new position
        }
        
        
        // Step 5: Draw bird at new position
        UpdateBirdColor(b);
        DrawBird(b);
    }
}

void SpeedUp(BIRD* b , GameConfig *config)
{
    if(b->speed < config->swallow_speed_max){
        b->speed++;
    }
}

void SpeedDown(BIRD* b , GameConfig *config)
{
    if(b->speed > config->swallow_speed_min){
        b->speed--;
    }
}

void UpBird(BIRD* b)
{
    b->dy = -1;
    b->dx = 0;
}

void DownBird(BIRD* b)
{
    b->dy = 1;
    b->dx = 0;

}

void RightBird(BIRD* b)
{
    b->dx = 1;
    b->dy = 0;
}

void LeftBird(BIRD* b)
{
    b->dx = -1;
    b->dy = 0;
}

//_________________HUNTER________//

//
//  hunter.c
//  project_test
//
//  Created by Mateusz Ciesielczyk on 29/11/2025.
//


HUNTER* InitHunter(WIN *w, BIRD* b , GameConfig *config)
{
    HUNTER* h = (HUNTER*)malloc(sizeof(HUNTER));
    h->win = w;
    h->speed = config->hunter_speed;
    h->color = HUNTER_COLOR;
    h->damage = config->damage_penalty;
    
    // Using config value for bounces
    h->bounces = (rand() % 3) + config->hunter_bounces;
    h->width = config->hunter_width;
    h->height = config->hunter_height;
    h->active = 1;
    h->wait_dash = 0;
    
    // Spawn Logic
    int side = rand() % 4;
    if(side == 0) {  // Top
           h->y = BORDER + 1;
           h->x = (rand() % (w->cols - 2 * BORDER - 2 - h->width)) + BORDER + 1;
       }
       else if(side == 1) {  // Right
           h->x = w->cols - BORDER - h->width;
           h->y = (rand() % (w->rows - 2 * BORDER - 2 - h->height)) + BORDER + 1;
       }
       else if(side == 2) {  // Bottom
           h->y = w->rows - BORDER - h->height;
           h->x = (rand() % (w->cols - 2 * BORDER - 2 - h->width)) + BORDER + 1;
       }
       else {  // Left
           h->x = BORDER + 1;
           h->y = (rand() % (w->rows - 2 * BORDER - 2 - h->height)) + BORDER + 1;
       }
    double diffx = b->x - h->x;
    double diffy = b->y - h->y;
    double length = sqrt( (b->x - h->x)*(b->x - h->x) +  (b->y - h->y)*(b->y - h->y));
    if (length != 0) {
            h->dx = (diffx / length) ;
            h->dy = (diffy / length) ;
        } else {
            h->dx = 0; h->dy = 0;
        }
    return h;
}

void InitMultipleHunter(HUNTER* h[] , WIN* w , BIRD* b , GameConfig *config){
    for(int i =0 ; i < MAX_HUNTERS ; i++){
        h[i] = InitHunter(w,b,config);
        if (i < config->hunter_num) {
            h[i]->active = 1;
        }else {
            h[i]->active = 0;
        }
    }
}

void DrawHunter(HUNTER* h)
{
    wattron(h->win->window, COLOR_PAIR(h->color));
    int numposx = h->width / 2;
    int numposy = h->height / 2;
    for(int i =0 ; i < h->height ; i++){
        for(int j = 0; j < h->width ; j++){
            if(i == numposy && j == numposx){
                mvwprintw(h->win->window, h->y + i, h->x + j, "%d", h->bounces);
            }
            else
            {
                mvwprintw(h->win->window, h->y + i, h->x + j, "#");
            }
        }
    }
    wattron(h->win->window, COLOR_PAIR(h->win->color));

}

void DrawMultipleHunters(HUNTER* h[] , GameConfig *config){
    for(int i =0 ; i < config->hunter_num ; i++){
        DrawHunter(h[i]);
    }
}

void ClearHunter(HUNTER* h)
{
    for(int i =0 ; i < h->height ; i++){
        for(int j = 0; j < h->width ; j++){
            mvwprintw(h->win->window, (int)(h->y + i), (int)(h->x + j)," ");

        }
    }

}

void CheckHunterBird(HUNTER* h , BIRD* b ){
    int bird_width = strlen(b->symbol);
    if(b->on_taxi == 1){
        return;
    }
    else if ((int)h->y <= (int)b->y && (int)h->y + h->height >=  (int)b->y) {
        if ((int)h->x <= (int)b->x + bird_width && (int)h->x + h->width >= (int)b->x) {
            h->active = 0;
            b->life -= h->damage;
            ClearHunter(h);
            if(b->life < 0) b->life = 0;
        }
    }
}

void CheckHunterTaxi(HUNTER* h  , TAXI* t)
{
    if (!t->active || !t->state) return ;
    if (h->x < t->x + SAFE_ZONEW && h->x + h->width > t->x){
        if(h->y < t->y + SAFE_ZONEH && h->y + h->height > t->y){
            h->active = 0;
            ClearHunter(h);
        }
    }
}


void Bounce(HUNTER* h,BIRD* b ,TAXI* t, int width , int height){
    int hit = 0;
    double diffx = b->x - h->x;
    double diffy = b->y - h->y;
    double length = sqrt( (b->x - h->x)*(b->x - h->x) +  (b->y - h->y)*(b->y - h->y));
    if (h->x < BORDER ) {
            h->x = BORDER;
            hit = 1;
        if(t->active && t->state) h->dx = -h->dx;
        } else if (h->x > width - BORDER - h->width) {
            h->x = width - BORDER - h->width ;
            hit = 1;
        }
        
        if (h->y < BORDER) {
            h->y = BORDER;
            hit = 1;
        } else if (h->y > height - BORDER  - h->height) {
            h->y = height - BORDER - h->height;
            hit = 1;
        }
    
    // IF WE HIT A WALL: Stop and Start Timer
        if (hit) {
                h->dx = 0;
                h->dy = 0;
                h->wait_dash = 30; // Wait for 20 frames (approx 1 sec)
                h->bounces--;
            
        }
}

void MoveHunter(HUNTER* h , BIRD* b , TAXI* t){
    if(!h->active) return;
    CheckHunterBird(h , b);
    if(!h->active) return;
    CheckHunterTaxi(h,t);
    if(!h->active) return;
    ClearHunter(h);
    if(h->wait_dash > 0){
        h->wait_dash--;
        if(h->wait_dash == 0){
            double length = sqrt( (b->x - h->x)*(b->x - h->x) +  (b->y - h->y)*(b->y - h->y));
            if (length != 0) {
                h->dx = ((b->x - h->x) / length) ;
                h->dy = ((b->y - h->y) / length) ;
            }
        }
        DrawHunter(h);
        return;
    }
    h->x += (h->dx * h->speed);
    h->y += (h->dy * h->speed);
    Bounce(h, b , t, h->win->cols, h->win->rows);
    if (h->bounces < 0) {
        h->active = 0;
    } else {
        DrawHunter(h);
    }
   
    CheckHunterBird(h , b);
}

void MoveMultipleHunter(HUNTER* h[] , BIRD* b , TAXI* t, WIN* w , GameConfig *config)
{
    for(int i = 0 ; i < config->hunter_num ; i++){
        
        if(h[i]->active){
            MoveHunter(h[i] , b , t);
        }else {
            if(rand() % config->hunter_spawn_rate == 0){
                ClearHunter(h[i]);
                free(h[i]);
                h[i] = InitHunter(w , b , config);
                h[i]->active = 1;
                
            }
        }
    }
}

void free_hunter(HUNTER* h[] , GameConfig *config)
{
    for(int i = 0 ; i < config->hunter_num ; i++){
        free(h[i]);
    }
}

//____________STARS_______________//

//
//  star.c
//  project_test
//
//  Created by Mateusz Ciesielczyk on 29/11/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

STAR* InitStar(WIN* w)
{
    STAR* s = (STAR*)malloc(sizeof(STAR));
    s->win = w;
    s->x = (rand() % (w->cols - 2)) + 1;
    s->y = 1;
    s->dx = 0;
    s->dy = 1;
    s->symbol = '*';
    s->interval = (rand() % 4) + 2; //random intervaal 1 to 4 1-fast , 4 - slow
    s->counter = s->interval; //starting counter at full interval
    s->color = STAR_COLOR;
    return s;
    
}

void InitMultipleStar(STAR* s[] , WIN* w)
{
    for(int i = 0; i < MAX_STARS ; i++){
        s[i] = InitStar(w);
    }
}

void DrawStar(STAR* s)
{
    wattron(s->win->window, COLOR_PAIR(s->color));
    mvwprintw(s->win->window , s->y , s->x , "%c" , s->symbol);
    wattron(s->win->window, COLOR_PAIR(s->win->color));

}

void DrawMultipleStar(STAR* s[])
{
    for(int i =0 ; i<MAX_STARS ; i++){
        DrawStar(s[i]);
    }
}

void ClearStar(STAR* s)
{
    mvwprintw(s->win->window, s->y, s->x," "); //clear star for movement
}

 void IfTouchedBird(STAR* s , BIRD* b)
{
     int bird_width = strlen(b->symbol);
     if(s->y == b->y || s->y == b-> y + 1){
         if(s->x >= b->x  && s->x < b->x + bird_width){
             ClearStar(s);
             s->y = 1;
             s->x = (rand() % (s->win->cols - 2)) + 1;
             s->interval = (rand() % 4) + 1;
             s->counter = s->interval;
             b->score++;
         }
     }
 }

void MoveStar(STAR* s , BIRD* b)
{
    s->counter--;
    if(s->counter <= 0){
        s->counter = s->interval;
        ClearStar(s);
        s->y +=1;
        if(s->y >= s->win->rows - 1){
            s->x = (rand() % (s->win->cols - 2)) + 1;
            s->y = 1;
            s->counter = s->interval;
        }
    }
    IfTouchedBird(s , b);
    DrawStar(s);
}
    
void MoveMultipleStar(STAR* s[] , BIRD* b){
    for(int i = 0 ; i < MAX_STARS ; i++){
        MoveStar(s[i] , b);
    }
}

void free_star(STAR* s[])
{
    for(int i = 0 ; i< MAX_STARS ; i++){
        free(s[i]);
    }
}

//____________TAXI_____________//

//
//  taxi.c
//  project_test
//
//  Created by Mateusz Ciesielczyk on 03/12/2025.
//

TAXI* InitTaxi(WIN* w , GameConfig* config)
{
    TAXI* t = (TAXI*)malloc(sizeof(TAXI));
    t->win = w;
    t->y = config->screen_height - SAFE_ZONEH - 1;
    t->x = 2;
    t->dx = 1;
    t->dy = 0;
    t->symbol = "o\\__/o" ;
    t->speed = 1;
    t->color = TAXI_COLOR;
    t->counter_of_taxis = 0;
    t->active = 0;
    t->state = 0;
    for(int i=0; i<BONUS_STARS; i++) t->bonusa[i] = 0;
    return t;
}

void InitBonus(TAXI* t)
{
    for(int i = 0 ; i < BONUS_STARS ; i++){
        t->bonusx[i] = t->x + 20 + (i*10);
        t->bonusa[i] = 1;
    }
}

void CheckTaxiBonus(TAXI* t , BIRD* b)
{
    for(int i = 0 ; i< BONUS_STARS ; i++){
        if (t->bonusa[i] == 1) {
            if(t->x + SAFE_ZONEW >= t->bonusx[i]){
                b->score++;
                t->bonusa[i] = 0;
            }
        }
    }
}

void DrawBonus(TAXI* t)
{
    wattron(t->win->window, COLOR_PAIR(INJURED_BIRD));
    for(int i = 0 ; i < BONUS_STARS ; i++)
    {
        if(t->bonusa[i] == 1){
            mvwprintw(t->win->window, t->y + SAFE_ZONEH/2 , t->bonusx[i], "*");
        }
    }
    wattroff(t->win->window, COLOR_PAIR(INJURED_BIRD));
}

void DrawTaxiSafeZone(TAXI* t)
{
    // Only draw if the shield is active
    if (!t->active || !t->state) return;
   wattron(t->win->window, COLOR_PAIR(BIRD_COLOR)); // Use a specific color (e.g., Green or Cyan)

        // Draw Top and Bottom borders
    for (int i = 0; i < SAFE_ZONEW ; i++) {
        // Top Edge
        mvwprintw(t->win->window, t->y, t->x + i, "-");
        // Bottom Edge
        mvwprintw(t->win->window, t->y + SAFE_ZONEH - 1, t->x + i, "-");
    }

        // Draw Left and Right borders
    for (int i = 0; i < SAFE_ZONEH ; i++) {
        // Left Edge
        mvwprintw(t->win->window, t->y + i, t->x, "|");
        // Right Edge
        mvwprintw(t->win->window, t->y + i, t->x + SAFE_ZONEW - 1, "|");
    }
    wattroff(t->win->window, COLOR_PAIR(BIRD_COLOR));
}

void DrawTaxi(TAXI* t)
{
    int taxi_width = strlen(t->symbol);
    wattron(t->win->window, COLOR_PAIR(t->color));
    wattron(t->win->window, A_BOLD);
    mvwprintw(t->win->window , t->y + SAFE_ZONEH - 2, t->x + SAFE_ZONEW/2 - taxi_width/2, "%s" , t->symbol);
    wattroff(t->win->window, A_BOLD);
    wattroff(t->win->window, COLOR_PAIR(t->win->color));
}

void ClearTaxi(TAXI* t)
{
    for(int i = 0; i < SAFE_ZONEH; i++) {
            for(int j = 0; j < SAFE_ZONEW; j++) {
                 mvwprintw(t->win->window, t->y + i, t->x + j, " ");
            }
        }
}

void SafeBirdTaxi(TAXI* t , BIRD* b)
{
    int bird_width = strlen(b->symbol);
    if (b->x + bird_width >= t->x && b->x <= t->x + SAFE_ZONEW) {
        if(b->y >= t->y && b->y <= t->y + SAFE_ZONEH) {
            t->state = 1;    // Switch Taxi to MOVING mode
            b->on_taxi = 1;  // Tell Bird it is riding
            InitBonus(t);
        }
    }
}

void MoveTaxi(TAXI* t, BIRD* b)
{
    if (!t->active) return;
    ClearTaxi(t);
    
    if(t->state == 0) //waiting mode
    {
        t->x = 2;
        t->y = t->win->rows - SAFE_ZONEH - 1;
        DrawTaxi(t);
        SafeBirdTaxi(t ,b);
    }
    else if(t->state == 1){
        t->x += (t->dx * t->speed);
        ClearBird(b);
        if (b->life < 100) {
            b->life++;  // +1 HP every frame (gradual healing)
        }
        CheckTaxiBonus(t , b);
        int bird_width = strlen(b->symbol);
        b->x = t->x + (SAFE_ZONEW / 2) - (bird_width / 2); // attaching the bird to sit on the taxi
        b->y = t->y + SAFE_ZONEH - 3;
        if(t->x >= t->win->cols - SAFE_ZONEW - 1){
            t->active = 0;
            t->state = 0;
            b->on_taxi = 0;
            ClearTaxi(t);
            return;
        }
        DrawTaxiSafeZone(t);
        DrawTaxi(t);
        DrawBonus(t);
    }
}


//__________RANKING SYSTEM_____________//


//
//  ranking.c
//  project_test
//
//  Created by Mateusz Ciesielczyk on 05/12/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

//helper function for sorting the rankings
int CompareScores(const void* x, const void* y){
    RANKING* scorex = (RANKING*)x;
    RANKING* scorey = (RANKING*)y;
    return scorey->total_score - scorex->total_score;
}

int CalculateScore(BIRD* b , GameConfig* config)
{
    return (int)(config->time_limit * 50) + b->score * 100 + b->life * 5 + config->curr_level * 500 ;
}

int FindPlayerIndex(RANKING scores[], int count, const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(scores[i].name, name) == 0) {
            return i; // Player found at this index
        }
    }
    return -1; // Player not found
}

void SetPlayerStats(RANKING* r, BIRD* b, GameConfig* cfg, double t_used, int score) {
    r->stars_collected = b->score;
    r->star_quota = cfg->star_quota;
    r->time_used = t_used;
    r->total_score = score;
}

void UpdateRanking(BIRD* b, GameConfig* config, double time_used, int total_score)
{
    RANKING scores[NUM_PLAYERS + 1];
    int count = 0;
    FILE* file = fopen(RANKING_FILE, "r");
    if(file){
        while(count < NUM_PLAYERS && fscanf(file, "%s %d %d %lf %d",scores[count].name,&scores[count].stars_collected,&scores[count].star_quota,&scores[count].time_used,&scores[count].total_score) == 5){
            count++;
        }
        fclose(file);
    }
    int found_index = FindPlayerIndex(scores, count, config->player_name);
    if(found_index != -1) {
        // Player found! Only update if the NEW score is better.
        if(total_score > scores[found_index].total_score) {
            SetPlayerStats(&scores[found_index], b, config, time_used, total_score);
        }
    }
    else{
        strcpy(scores[count].name, config->player_name);
        SetPlayerStats(&scores[count], b, config, time_used, total_score);
        count++;
    }
    
    qsort(scores, count, sizeof(RANKING), CompareScores);
    file = fopen(RANKING_FILE, "w");
        if (!file) return;
    int limit;
    if (count < NUM_PLAYERS) {
        limit = count;
    } else {
        limit = NUM_PLAYERS;
    }
    for (int i = 0; i < limit; i++) {
        fprintf(file, "%s %d %d %.2f %d\n",
                scores[i].name,
                scores[i].stars_collected,
                scores[i].star_quota,
                scores[i].time_used,
                scores[i].total_score);
    }
    fclose(file);
    
}

void ShowRanking(WINDOW* ranking, int rows, int cols)
{
    int h = 16, w = 60;
    int y = (rows - h) / 2;
    int x = (cols - w) / 2;
    WINDOW* win = newwin(h, w, y, x);
    box(win, 0, 0);
    wbkgd(win, COLOR_PAIR(1));
    wattron(win, A_BOLD | A_UNDERLINE);
    mvwprintw(win, 1, 20, "BEST OF THE BEST");
    wattroff(win, A_BOLD | A_UNDERLINE);
    wattron(win, A_REVERSE);
    mvwprintw(win, 3, 2, "RK  NAME         STARS   TIME(s)  SCORE  ");
    wattroff(win, A_REVERSE);
    FILE* file = fopen(RANKING_FILE, "r");
    char name[50];
    int s, q, score;
    double t;
    int line = 4;
    if (file) {
           int rank = 1;
           while (fscanf(file, "%s %d %d %lf %d", name, &s, &q, &t, &score) == 5 && rank <= NUM_PLAYERS) {
               mvwprintw(win, line, 2, "#%-2d %-12s %2d/%-2d   %6.1f   %6d",
                         rank, name, s, q, t, score);
               line++;
               rank++;
           }
           fclose(file);
       }else {
           mvwprintw(win, 6, 20, "No Records Yet!");
       }
    mvwprintw(win, h-2, 18, "Press Any Key");
    wrefresh(win);
    nodelay(win, FALSE);
    wgetch(win);
    delwin(win);
    
}






//--------------------------------------//
//          CONFIGURATION INPUT
//--------------------------------------//


//Loads configuration of a text file
//will return 1 if succesfull and 0 if failed

int LoadConfig(const char* filename, GameConfig* config);
void DefaultValues(GameConfig* config);

void AssignValue(GameConfig* config, const char* key, int value)
{
    if (strcmp(key, "SCREEN_WIDTH") == 0) config->screen_width = value;
    else if (strcmp(key, "SCREEN_HEIGHT") == 0) config->screen_height = value;
    else if (strcmp(key, "STAR_QUOTA") == 0) config->star_quota = value;
    else if (strcmp(key, "TIME_LIMIT") == 0) config->time_limit = value;
    else if (strcmp(key, "SWALLOW_SPEED_MIN") == 0) config->swallow_speed_min = value;
    else if (strcmp(key, "SWALLOW_SPEED_MAX") == 0) config->swallow_speed_max = value;
    else if (strcmp(key, "HUNTER_SPAWN_RATE") == 0) config->hunter_spawn_rate = value;
    else if (strcmp(key, "SEED") == 0) config->seed = value;
    else if (strcmp(key, "DAMAGE_PENALTY") == 0) config->damage_penalty = value;
    else if (strcmp(key, "HUNTER_SPEED") == 0) config->hunter_speed = value;
    else if (strcmp(key, "LEVEL") == 0) config->curr_level = value;
    else if (strcmp(key, "HUNTER_BOUNCES") == 0) config->hunter_bounces = value;
    else if (strcmp(key, "HUNTER_NUM") == 0) config->hunter_num = value;
    else if (strcmp(key, "AVAILABLE_TAXIS") == 0) config->available_taxis = value;
}

int LoadConfig(const char* filename, GameConfig* config) {
    FILE* file = fopen(filename, "r");
    
    // Safety check: Make sure the file actually exists
    if (!file) {
        fprintf(stderr, "Error: Could not open config file %s\n", filename);
        return 0; // Return 0 to indicate failure
    }
    DefaultValues(config);
    char key[MAX_COMMAND_SIZE];
    int value;
    while (fscanf(file, "%s", key) == 1) {
        if (strcmp(key, "PLAYER_NAME") == 0) {
            fscanf(file, "%s", config->player_name);}
        else if (strcmp(key, "HUNTER_SPEED") == 0) {
            fscanf(file, "%lf", &config->hunter_speed);
                    }
        else if (strcmp(key, "TIME_LIMIT") == 0) {
            fscanf(file, "%lf", &config->time_limit);
                    }
        else if (strcmp(key , "HUNTER_SHAPE") == 0){
            fscanf(file, "%dx%d" , &config->hunter_width , &config->hunter_height);
        }
        else{
            int value;
            fscanf(file , "%d" , &value);
            AssignValue(config , key , value);

        }
        }
    fclose(file);
    return 1;
    
}

void DefaultValues(GameConfig* config)
{
    //default values if the fil doesnt work or is incomplete
    config->screen_width = 100;
    config->screen_height = 35;
    config->star_quota = 10;
    config->time_limit = 60;
    config->swallow_speed_min = 1;
    config->swallow_speed_max = 5;
    config->hunter_spawn_rate = 100;
    config->seed = 1234;
    config->damage_penalty = 1;
    config->hunter_speed = 1;
    strcpy(config->player_name , "PLAYER1");
    config->hunter_bounces = 3;
    config->curr_level = 1;
}

//-----------------------------------//
//  SCREEN SETTINGS AND I/O          //
//-----------------------------------//


WINDOW* Start();
void CleanWin(WIN* W, int bo);
WIN* InitWin(WINDOW* parent, int rows, int cols, int y, int x, int color, int bo, int delay);
void ShowStatus(WIN* W, void* bird , GameConfig *config ); //we use void so the program and the includes wont stay in an infinite loop
void EndGameWin(WIN* W);
void EndGameLose(WIN* W);
void EndGameQuit(WIN* W);


WINDOW* Start(){
    WINDOW* win;

    // Initialize ncurses - sets up terminal for text UI
    if ( (win = initscr()) == NULL ) {
        fprintf(stderr, "Error initialising ncurses.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize color system
    start_color();

    // Define color pairs: init_pair(ID, FOREGROUND, BACKGROUND)
    init_pair(MAIN_COLOR, COLOR_WHITE, COLOR_BLACK);
    init_pair(PLAY_COLOR, COLOR_CYAN, COLOR_BLACK);
    init_pair(STAT_COLOR, COLOR_YELLOW, COLOR_BLUE);
    //ACTORS
    init_pair(BIRD_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(STAR_COLOR, COLOR_MAGENTA , COLOR_BLACK);
    init_pair(HUNTER_COLOR , COLOR_RED , COLOR_BLACK);
    init_pair(INJURED_BIRD, COLOR_YELLOW, COLOR_BLACK);
    init_pair(TAXI_COLOR , COLOR_CYAN , COLOR_BLACK);
    // Don't echo typed characters to screen
    noecho();
    
    // Make cursor invisible (curs_set: 0=invisible, 1=normal, 2=very visible)
    curs_set(0);

    return win;
}

void CleanWin(WIN* W, int bo)
{
    int i, j;

    // Set window color
    wattron(W->window, COLOR_PAIR(W->color));

    // Draw border if requested
    if (bo) box(W->window, 0, 0);

    // Fill window with spaces (clearing it)
    for (i = bo; i < W->rows - bo; i++)
        for (j = bo; j < W->cols - bo; j++)
            mvwprintw(W->window, i, j, " ");

    // Refresh to show changes
    wrefresh(W->window);
}

WIN* InitWin(WINDOW* parent, int rows, int cols, int y, int x, int color, int bo, int delay)
{
    // Allocate memory for WIN structure
    WIN* W = (WIN*)malloc(sizeof(WIN));

    // Store window properties
    W->x = x;
    W->y = y;
    W->rows = rows;
    W->cols = cols;
    W->color = color;

    // Create ncurses subwindow
    W->window = subwin(parent, rows, cols, y, x);

    // Clear the window
    CleanWin(W, bo);

    // Set input mode: delay==0 means non-blocking (for real-time games)
    if (delay == 0) nodelay(W->window, TRUE);

    // Display the window
    wrefresh(W->window);

    return W;
}

void ShowStatus(WIN* W, void* bird , GameConfig* config)
{
    BIRD* b = (BIRD*)bird;
    // Set status bar color
    
    wattron(W->window, COLOR_PAIR(BIRD_COLOR) | A_BOLD | A_REVERSE);
    mvwprintw(W->window, 1, 2, "   SCORE: %d/%d     Time Left : %.1f  Life = %d   ", b->score , config->star_quota , config->time_limit ,  b->life);
    wattroff(W->window, COLOR_PAIR(BIRD_COLOR) | A_BOLD | A_REVERSE);
    
    wattron(W->window, COLOR_PAIR(W->color));

    // Draw border around status bar
    box(W->window, 0, 0);
    const char* controls = "[W]Up [S]Dn [A]Lft [D]Rgt [Q]Quit";
    int pos_x = W->cols - strlen(controls) - 2; // -2 for right margin
    mvwprintw(W->window, 1, pos_x, "%s", controls);
    
    // Display controls
    
    mvwprintw(W->window, 2, pos_x, "Position: x=%d y=%d " ,b->x, b->y  );
    mvwprintw(W->window, 3, pos_x - 30, "Press t to activate shield taxi and bonus points");
    mvwprintw(W->window, 2, 2, "PLAYER: %s   LEVEL: %d  TAXIS AVAILABLE: %d", config->player_name, config->curr_level , config->available_taxis);
    
    mvwprintw(W->window , 3 , 2 , "SPEED = %d" , b->speed );
    // Update display
    wrefresh(W->window);
    //sleep(2);
}

void EndGameWin(WIN* W)
{
    // Clear the window
    CleanWin(W, 1);
    nodelay(W->window , FALSE);
    // Display goodbye message
    wattron(W->window, COLOR_PAIR(STAT_COLOR));
    mvwprintw(W->window, W->rows / 2 - 1 , W->cols / 2 - 18, "MISSION ACCOMPLISHED! SWALLOW SAVED!");
    wattroff(W->window, COLOR_PAIR(STAT_COLOR));
    sleep(1);
    mvwprintw(W->window, W->rows / 2 + 1 , W->cols / 2 - 12, "Press q to exit ");
    wrefresh(W->window);

    wgetch(W->window);
  
}

void EndGameLose(WIN* W)
{
    CleanWin(W, 1);
    nodelay(W->window , FALSE);
    wattron(W->window, COLOR_PAIR(HUNTER_COLOR));
    mvwprintw(W->window, W->rows / 2 - 1 , W->cols / 2 - 18 , "GAME OVER.");
    wattroff(W->window, COLOR_PAIR(HUNTER_COLOR));
    sleep(1);

    mvwprintw(W->window, W->rows / 2 + 1 , W->cols / 2 - 12, "Press q to exit ");
    wrefresh(W->window);
   

    int ch;
    while((ch = wgetch(W->window)) != 'q') {
        }

}

void EndGameQuit(WIN* W)
{
    CleanWin(W, 1);
    mvwprintw(W->window, 2, 2, "Game Aborted.");
    wrefresh(W->window);
    sleep(1);
}

//__MAIN_GAME_LOOP_AND_MECHANICS___//
//==================================//
//--------------------------------//

void Difficulty(GameConfig *config , double time)
{
    double time_passed = time - config->time_limit;
    // 4 levels , level 1 from 10 to 35 level 2 : from 35 to 60 level 3 : from 60 to 85 and level 4 : from 85 to 90
    //spaw rate of hunter lowers , and the bounces increases so the live longer at the last level speed also increases
    if (time_passed > 0 && time_passed <= config->time_limit/6.0) {
            config->hunter_spawn_rate = 50;
            config->hunter_bounces = 3;
            config->curr_level = 1;
            config->hunter_num = 3;
        }
        else if (time_passed > config->time_limit/6.0 && time_passed <= 2*config->time_limit/6.0) {
            config->hunter_spawn_rate = 25;
            config->hunter_bounces = 8;
            config->curr_level = 2;
            config->hunter_num = 4;

        }
        else if (time_passed > 2*config->time_limit/4.0 && time_passed <= 3.5 * config->time_limit/4.0) {
            config->hunter_spawn_rate = 20;
            config->hunter_bounces = 5;
            config->curr_level = 3;
            config->hunter_num = 5;
            config->hunter_speed = 1.0;
        }
        else if(time_passed > 3.5 * config->time_limit/4.0){
            config->hunter_spawn_rate = 10;
            config->hunter_bounces = 4;
            config->curr_level = 4;
            config->hunter_num = 6;
            config->hunter_speed = 1.2;

        }
}

int MainLoop(WIN* playwin, WIN* statwin, BIRD* bird , TAXI* taxi, STAR* star[] , HUNTER* hunter[] , GameConfig *config)
{
    int ch;        // Variable to store key press
    double max_time = config->time_limit;
    // Infinite loop - runs until player quits
    while (1)
    {
        // Read keyboard input (non-blocking due to nodelay(TRUE))
        ch = wgetch(statwin->window);
        Difficulty(config , max_time);
        config->time_limit -= (FRAME_TIME / 1000.0);
        // Check if player wants to quit
        if (ch == QUIT) return 0;
        if (bird->life == 0 || config->time_limit <= 0) return 1; //defeat
        if(bird->score >= config->star_quota) return 2; //win
        if (ch == UP) {
            UpBird(bird);
        }else if(ch == DOWN){
            DownBird(bird);
        }else if(ch == RIGHT){
            RightBird(bird);
        }else if(ch == LEFT){
            LeftBird(bird);
        }else if(ch == SPEED_UP){
            SpeedUp(bird , config);
        }else if(ch == SPEED_DOWN){
            SpeedDown(bird , config);
        }else if(ch == ACTIVATE_TAXI && !taxi->active && config->available_taxis > 0){
            taxi->active = 1;
            taxi->state = 0;
            config->available_taxis--;
        }
        
        // Move bird (automatic movement every frame)
        if (taxi->active) {
            MoveTaxi(taxi, bird);
        }
        if (bird->on_taxi == 0) {
            MoveBird(bird);
        } else {
            UpdateBirdColor(bird);
            DrawBird(bird);
        }
        
        MoveMultipleStar(star , bird);
        MoveMultipleHunter(hunter , bird , taxi, playwin , config);
        mvwprintw(playwin->window, 1, playwin->cols - 2, "Z");
        
        // Update status bar with current position
        ShowStatus(statwin, bird , config);
        
        // Refresh play window to show changes
        wrefresh(playwin->window);

        // Clear input buffer to avoid key press accumulation
        flushinp();

        // Sleep to control frame rate
        // FRAME_TIME is in milliseconds, usleep needs microseconds
        usleep(FRAME_TIME * 1000);
    }
}

void EndGameResult(int result, WIN* statwin)
{
    if (result == 2) {
            EndGameWin(statwin);
        }
        else if (result == 1) {
            EndGameLose(statwin);
        }
        else if(result == 0){
            EndGameQuit(statwin);
        }
}

void CleanUpMemory(WINDOW* mainwin, WIN* playwin, WIN* statwin, BIRD* bird, TAXI* taxi, STAR* star[], HUNTER* hunter[], GameConfig* config){
    // Delete ncurses windows
    delwin(playwin->window);
    delwin(statwin->window);
    delwin(mainwin);

    // Close ncurses
    endwin();
    refresh();
    // Free allocated memory
    free_hunter(hunter , config);
    free_star(star);
    free(bird);
    free(taxi);
    free(playwin);
    free(statwin);
}



int main()
{
    GameConfig config;
    if (!LoadConfig("config.txt", &config)) {
            return EXIT_FAILURE;
        }
    double initial_time = config.time_limit;
    srand(config.seed);
    STAR* s[MAX_STARS];
    HUNTER* h[MAX_HUNTERS];

    WINDOW *mainwin = Start();
    WIN* playwin = InitWin(mainwin, config.screen_height, config.screen_width, OFFY, OFFX,
                           PLAY_COLOR, BORDER, 0);
    WIN* statwin = InitWin(mainwin, STAT_HEIGHT, config.screen_width , config.screen_height+OFFY, OFFX,
                           STAT_COLOR, BORDER, 0);
    BIRD* b = InitBird(playwin, config.screen_width/2, config.screen_height/2, 1, 0 , &config);
    TAXI* t = InitTaxi(playwin , &config);
    InitMultipleStar(s , playwin );
    InitMultipleHunter(h , playwin , b , &config);
    // Step 4: Initial display
   
    DrawBird(b);            // Draw bird
    ShowStatus(statwin, b , &config);    // Update status bar
    wrefresh(playwin->window);    // Refresh play window
    int result = MainLoop(playwin, statwin, b ,t , s , h , &config);
    double time_used = initial_time - config.time_limit;
        if(time_used < 0) time_used = 0;
    
    UpdateRanking(b, &config, time_used, CalculateScore(b , &config) );
    
    // Step 5: Run main game loop (returns when player quits)
    
    EndGameResult(result , statwin);

    ShowRanking(mainwin, config.screen_height, config.screen_width);
    // Step 6: Cleanup - free resources and close ncurses
    // Display game over message
    
    CleanUpMemory(mainwin, playwin, statwin, b , t, s, h , &config);
    
    return EXIT_SUCCESS;
}



