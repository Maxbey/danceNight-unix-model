#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

typedef struct mrevent {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool triggered;
} Event;

void mrevent_init(struct mrevent *ev) {
    pthread_mutex_init(&ev->mutex, 0);
    pthread_cond_init(&ev->cond, 0);
    ev->triggered = false;
}

void mrevent_trigger(struct mrevent *ev) {
    pthread_mutex_lock(&ev->mutex);
    ev->triggered = true;
    pthread_cond_broadcast(&ev->cond);
    pthread_mutex_unlock(&ev->mutex);
}

void mrevent_reset(struct mrevent *ev) {
    pthread_mutex_lock(&ev->mutex);
    ev->triggered = false;
    pthread_mutex_unlock(&ev->mutex);
}

void mrevent_wait(struct mrevent *ev) {
     pthread_mutex_lock(&ev->mutex);
     while (!ev->triggered)
         pthread_cond_wait(&ev->cond, &ev->mutex);
     pthread_mutex_unlock(&ev->mutex);
}

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYELL  "\x1b[33m"
#define KNRM  "\x1B[0m"

pthread_mutex_t output_mutex;
pthread_mutex_t partner_choose_mutex;

Event event_1;
Event event_2;
Event event_3;

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

  char name;
  char color[255];

  Position position;
  Position stand_position;
  Position dance_position;

  Dancer *partner;
};

Config app_config;

Dancer *guys_array;
Dancer *girls_array;

int pairs_cnt = 0;
int on_dance_position = 0;
int dance_part_1 = 0;
int dance_part_2 = 0;
int end_of_dance = 0;

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

void setDancerColor(Dancer *dancer, char *color){
  strcpy(dancer->color, color);
}

void init_dancer(Dancer *dancer, char name){
  dancer->name = name;
  dancer->partner = NULL;
  setDancerColor(dancer, KNRM);
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

// Calculate and set optimal dance positions for dancers
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
      Pos girl_pos;
      girls_array[j].dance_position.y = vertical_margin * row;
      step += horizontal_margin;
      girls_array[j].dance_position.x = step - 1;

      girls_array[j].partner->dance_position.y = girls_array[j].dance_position.y;
      girls_array[j].partner->dance_position.x = girls_array[j].dance_position.x + 2;
    }
  }
}

// Calculate and set optimal stand positions for dancers
void calc_stand_positions()
{
  int i, step;
  int guys_margin = 80 / app_config.guys_cnt + 1;
  int girls_margin = 80 / app_config.girls_cnt + 1;

  for(i = 0, step = 0; i < app_config.guys_cnt; i++)
  {
    guys_array[i].stand_position.y = 16;
    step += guys_margin;
    guys_array[i].stand_position.x = step;
  }

  for(i = 0, step = 0; i < app_config.girls_cnt; i++)
  {
    girls_array[i].stand_position.y = 8;
    step += girls_margin;
    girls_array[i].stand_position.x = step;
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

void dance_around_partner(Dancer *dancer, int direction)
{
  Position pos;

  pos.y = dancer->position.y + (4 * direction);
  pos.x = dancer->position.x;

  move(dancer, pos, 100000);

  pos.y = dancer->position.y;
  pos.x = dancer->position.x - 8;

  move(dancer, pos, 100000);

  pos.y = dancer->position.y - (8 * direction);
  pos.x = dancer->position.x;

  move(dancer, pos, 100000);

  pos.y = dancer->position.y;
  pos.x = dancer->position.x + 8;

  move(dancer, pos, 100000);

  pos.y = dancer->position.y + (4 * direction);
  pos.x = dancer->position.x;

  move(dancer, pos, 100000);
}

void guy_behavior(void *args){
  Position pos;
  Dancer *guy = args;

  guy->position = guy->stand_position;

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

  mrevent_wait(&event_1);

  if(guy->partner == NULL){
    pos.y = 25;
    move(guy, pos, 100000);
  }
  else{
    pos.y = 14;
    move(guy, pos, 100000);
  }

  on_dance_position++;

  mrevent_wait(&event_2);

  move(guy, guy->dance_position, 100000);

  dance_part_1++;

  mrevent_wait(&event_3);

  pos.y = guy->position.y;
  pos.x = guy->position.x + 2;

  move(guy, pos, 100000);

  dance_around_partner(guy, 1);

  dance_part_2++;

  mrevent_wait(&event_2);

  move(guy, guy->stand_position, 100000);
}

void girl_behavior(void *args){
  Position pos;
  Dancer *girl = args;

  girl->position = girl->stand_position;

  mrevent_wait(&event_1);

  pos.x = girl->partner->position.x;
  pos.y = 12;

  move(girl, pos, 100000);
  on_dance_position++;

  mrevent_wait(&event_2);

  move(girl, girl->dance_position, 100000);

  dance_part_1++;

  mrevent_wait(&event_1);

  pos.y = girl->position.y;
  pos.x = girl->partner->position.x + 3;

  move(girl, pos, 100000);

  dance_around_partner(girl, -1);

  end_of_dance++;

  mrevent_wait(&event_2);
  move(girl, girl->stand_position, 100000);
}

void run_dancers_threads()
{
  for(int i = 0; i < app_config.guys_cnt; i++){
    pthread_create(&guys_array[i].thread, NULL, (void *) guy_behavior, (void *) &guys_array[i]);
  }

  for(int i = 0; i < app_config.girls_cnt; i++){
    pthread_create(&girls_array[i].thread, NULL, (void *) girl_behavior, (void *) &girls_array[i]);
  }
}

void main(){

  app_config.guys_cnt = 12;
  app_config.girls_cnt = 12;
  app_config.dances_cnt = 5;

  pthread_mutex_init(&output_mutex, NULL);
  pthread_mutex_init(&partner_choose_mutex, NULL);

  mrevent_init(&event_1);
  mrevent_init(&event_2);
  mrevent_init(&event_3);

  char guys_dictionary[15] = "ABCDEFGHIJKLMNO";
  char girls_dictionary[15] = "abcdefghijklmno";

  guys_array = (Dancer*)malloc(app_config.guys_cnt * sizeof(Dancer));
  girls_array = (Dancer*)malloc(app_config.girls_cnt * sizeof(Dancer));

  for(int i = 0; i < app_config.guys_cnt; i++){
    init_dancer(&guys_array[i], guys_dictionary[i]);
  }

  for(int i = 0; i < app_config.girls_cnt; i++){
    init_dancer(&girls_array[i], girls_dictionary[i]);
  }

  //Calculate and set optimal stand positions for dancers
  calc_stand_positions();

  run_dancers_threads();

  // Run render thread
  pthread_create(&render, NULL, (void *) render_thread, (void *) 0);

  while(1){
    if(pairs_cnt == app_config.girls_cnt){
      calc_dance_positions();
      pairs_cnt = 0;
      mrevent_trigger(&event_1);
    }

    if(on_dance_position == app_config.girls_cnt * 2)
    {
      on_dance_position = 0;
      mrevent_reset(&event_1);
      mrevent_trigger(&event_2);
    }

    if(dance_part_1 == app_config.girls_cnt * 2)
    {
      dance_part_1 = 0;
      mrevent_reset(&event_2);
      mrevent_trigger(&event_3);
    }

    if(dance_part_2 == app_config.guys_cnt)
    {
      dance_part_2 = 0;
      mrevent_reset(&event_3);
      mrevent_trigger(&event_1);
    }

    if(end_of_dance == app_config.girls_cnt)
    {
      end_of_dance = 0;
      mrevent_trigger(&event_2);
      mrevent_reset(&event_1);
    }

    usleep(1000000);
  }


  getchar();
}
