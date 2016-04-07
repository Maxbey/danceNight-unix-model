#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYELL  "\x1b[33m"
#define KNRM  "\x1B[0m"

pthread_mutex_t output_mutex;
pthread_mutex_t partner_choose_mutex;

pthread_cond_t next_stage;
pthread_mutex_t stage_mutex;

pthread_cond_t event;
pthread_mutex_t event_mutex;

pthread_cond_t fuck;
pthread_mutex_t fuck_mutex;

pthread_t render;

typedef struct config_t Config;
typedef struct dancer_t Dancer;
typedef struct position_t Position;

struct config_t {
  int guys_cnt;
  int girls_cnt;
  int dances_cnt;
};

struct position_t {
  int x;
  int y;
};

struct dancer_t {
  pthread_t thread;

  int id;
  char name;
  char color[255];

  Position position;
  Position dance_position;

  Dancer *partner;
};

Config app_config;

Dancer *guys_array;
Dancer *girls_array;

int pairs_cnt = 0;
int ready_for_dance = 0;
int lets_dance = 0;

void displayDancer(Dancer *dancer){
  pthread_mutex_lock(&output_mutex);

  printf("%s", dancer->color);
  printf("\033[%d;%dH", dancer->position.y, dancer->position.x);
  printf("%c", dancer->name);
  fflush(stdout);

  pthread_mutex_unlock(&output_mutex);
}

void render_thread(void *args){
  int i;

  while(1)
  {
    printf("\033[2J\n");
    for(i = 0; i < app_config.guys_cnt; i++){
      displayDancer(&guys_array[i]);
    }

    for(i = 0; i < app_config.girls_cnt; i++){
      displayDancer(&girls_array[i]);
    }
    usleep(50000);
  }

}

void guy_init(Dancer *guy, char name){
  guy->name = name;
}

void girl_init(Dancer *girl, char name){
  girl->name = name;
}

void setDancerColor(Dancer *dancer, char *color){
  strcpy(dancer->color, color);
}


void move(Dancer *dancer, Position moveTo, int delay){
  do
  {
    if(moveTo.y != dancer->position.y)
    {
        (moveTo.y > dancer->position.y) ? dancer->position.y++ : dancer->position.y--;
    }

    if(moveTo.x != dancer->position.x)
    {
        (moveTo.x > dancer->position.x) ? dancer->position.x++ : dancer->position.x--;
    }

    usleep(delay);
  }
  while(dancer->position.y != moveTo.y || dancer->position.x != moveTo.x);
}

void calc_dance_positions(){
  int rows_cnt = 1;
  int horizontal_margin;

  if(app_config.girls_cnt / 4.0 > 2.0 || app_config.girls_cnt / 4.0 == 3.0)
    rows_cnt = 3;

  else if(app_config.girls_cnt / 4.0 > 1.0 || app_config.girls_cnt / 4.0 == 2.0)
    rows_cnt = 2;

  int vertical_margin = 24 / (rows_cnt + 1);
  int elements_in_last_row_cnt = (app_config.girls_cnt / 4.0 - (double)(rows_cnt - 1)) / 0.25;

  horizontal_margin = 80 / 5;

  for(int row = 1; row <= rows_cnt; row++)
  {
    if(row == rows_cnt)
    {
      horizontal_margin = 80 / (elements_in_last_row_cnt + 1);
    }

    for(int j = (4 * row) - 4, step = 0; j < 4 * row && j < app_config.girls_cnt; j++)
    {
      Position girl_pos;
      girl_pos.y = vertical_margin * row;
      step += horizontal_margin;
      girl_pos.x = step - 1;

      girls_array[j].dance_position = girl_pos;

      Position guy_pos;
      guy_pos.y = girl_pos.y;
      guy_pos.x = girl_pos.x + 2;

      girls_array[j].partner->dance_position = guy_pos;
    }
  }
}

int free_girls_count(){
  int free_cnt = 0;

  for(int i = 0; i < app_config.girls_cnt; i++){
    if(girls_array[i].partner == NULL)
      free_cnt++;
  }

  return free_cnt;
}

Dancer **get_free_girls()
{
  Dancer **girls;
  girls = (Dancer**)malloc(free_girls_count() * sizeof(Dancer*));

  for(int i = 0, p = 0; i < app_config.girls_cnt; i++){
    if(girls_array[i].partner == NULL){
      girls[p] = &girls_array[i];
      p++;
    }
  }

  return girls;
}

int bind_partners(Dancer *guy, Dancer *girl){
  srand(time(NULL));
  int random = rand() % 100;

  if(random <= 30)
    return 0;

  guy->partner = girl;
  girl->partner = guy;

  return 1;
}

void guy_behavior(void *args){
  Dancer *guy = args;

  guy->position.y = 0;
  guy->position.x = guy->id * 8;
  setDancerColor(guy, KNRM);

  Position pos;
  pos.y = 20;
  pos.x = guy->id * 8;

  move(guy, pos, 200000);

  pthread_mutex_lock(&partner_choose_mutex);
  int free_cnt = free_girls_count();

  if(free_cnt)
  {
    setDancerColor(guy, KGRN);
    usleep(1000000);

    Dancer **girls = get_free_girls();

    for(int i = 0; 1; i++){
      if(i == free_cnt)
        i = 0;

      if(bind_partners(guy, girls[i])){
        pairs_cnt++;
        setDancerColor(girls[i], KGRN);
        usleep(1000000);
        setDancerColor(girls[i], KNRM);
        break;
      }
      else{
        setDancerColor(girls[i], KRED);
      }

      usleep(1000000);
      setDancerColor(girls[i], KNRM);
    }
  }

  setDancerColor(guy, KNRM);
  pthread_mutex_unlock(&partner_choose_mutex);

  pthread_cond_wait(&next_stage, &stage_mutex);
  pthread_mutex_unlock(&stage_mutex);

  if(guy->partner == NULL){
    pos.y = 25;
    move(guy, pos, 100000);
  }
  else{
    pos.y = 14;
    move(guy, pos, 100000);
  }

  ready_for_dance++;

  pthread_cond_wait(&event, &event_mutex);
  pthread_mutex_unlock(&event_mutex);
  move(guy, guy->dance_position, 100000);

  lets_dance++;

  pthread_cond_wait(&fuck, &fuck_mutex);
  pthread_mutex_unlock(&fuck_mutex);

  pos.y = guy->position.y;
  pos.x = guy->position.x + 2;

  move(guy, pos, 100000);

  for(int i = 0; i < 6; i++)
  {
    pos.y = guy->position.y + 4;
    pos.x = guy->position.x;

    move(guy, pos, 100000);

    pos.y = guy->position.y;
    pos.x = guy->position.x - 8;

    move(guy, pos, 100000);

    pos.y = guy->position.y - 8;
    pos.x = guy->position.x;

    move(guy, pos, 100000);

    pos.y = guy->position.y;
    pos.x = guy->position.x + 8;

    move(guy, pos, 100000);

    pos.y = guy->position.y + 4;
    pos.x = guy->position.x;

    move(guy, pos, 100000);
  }






}

void girl_behavior(void *args){
  Dancer *girl = args;

  girl->position.y = 20;
  girl->position.x = girl->id * 9;
  setDancerColor(girl, KNRM);

  Position pos;
  pos.y = 0;
  pos.x = girl->position.x = girl->id * 9;

  move(girl, pos, 200000);

  pthread_cond_wait(&next_stage, &stage_mutex);
  pthread_mutex_unlock(&stage_mutex);

  pos.x = girl->partner->position.x;
  pos.y = 12;

  move(girl, pos, 200000);
  ready_for_dance++;

  pthread_cond_wait(&event, &event_mutex);
  pthread_mutex_unlock(&event_mutex);

  move(girl, girl->dance_position, 100000);

  lets_dance++;
}

void main(){

  app_config.guys_cnt = 10;
  app_config.girls_cnt = 10;
  app_config.dances_cnt = 5;

  pthread_mutex_init(&output_mutex, NULL);
  pthread_mutex_init(&partner_choose_mutex, NULL);
  pthread_mutex_init(&stage_mutex, NULL);
  pthread_cond_init(&next_stage, NULL);

  pthread_mutex_init(&event_mutex, NULL);
  pthread_cond_init(&event, NULL);

  pthread_mutex_init(&fuck_mutex, NULL);
  pthread_cond_init(&fuck, NULL);

  char guys_dictionary[15] = "ABCDEFGHIJKLMNO";
  char girls_dictionary[15] = "abcdefghijklmno";

  guys_array = (Dancer*)malloc(app_config.guys_cnt * sizeof(Dancer));
  girls_array = (Dancer*)malloc(app_config.girls_cnt * sizeof(Dancer));

  for(int i = 0; i < app_config.guys_cnt; i++){
    guys_array[i].id = i + 1;
    guy_init(&guys_array[i], guys_dictionary[i]);
    guys_array[i].partner = NULL;

    pthread_create(&guys_array[i].thread, NULL, (void *) guy_behavior, (void *) &guys_array[i]);
  }

  for(int i = 0; i < app_config.girls_cnt; i++){
    girls_array[i].id = i + 1;
    girl_init(&girls_array[i], girls_dictionary[i]);
    girls_array[i].partner = NULL;

    pthread_create(&girls_array[i].thread, NULL, (void *) girl_behavior, (void *) &girls_array[i]);
  }

  pthread_create(&render, NULL, (void *) render_thread, (void *) 0);

  while(1){
    if(pairs_cnt == app_config.girls_cnt){
      calc_dance_positions();
      pthread_cond_broadcast(&next_stage);
    }

    if(ready_for_dance == app_config.girls_cnt * 2)
    {
      pthread_cond_broadcast(&event);
      ready_for_dance = 0;
    }

    if(lets_dance == app_config.girls_cnt * 2)
    {
      pthread_cond_broadcast(&fuck);
      lets_dance = 0;
    }


    usleep(1000000);
  }


  getchar();
}
