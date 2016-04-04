#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

pthread_mutex_t output_mutex;
pthread_mutex_t partner_choose_mutex;

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
  int id;
  char name;
  pthread_t thread;
  Position position;
  Dancer *partner;
};

Config app_config;

Dancer *guys_array;
Dancer *girls_array;

void guy_init(Dancer *guy, char name){
  guy->name = name;
}

void girl_init(Dancer *girl, char name){
  girl->name = name;
}

void displayDancer(Dancer *dancer){
  pthread_mutex_lock(&output_mutex);

  printf("\033[%d;%dH", dancer->position.y, dancer->position.x);
  printf("%c", dancer->name);

  fflush(stdout);
  pthread_mutex_unlock(&output_mutex);
}

void hideDancer(Dancer *dancer){
  pthread_mutex_lock(&output_mutex);

  printf("\033[%d;%dH", dancer->position.y, dancer->position.x);
  printf("%c", ' ');

  pthread_mutex_unlock(&output_mutex);
}

void move(Dancer *dancer, Position moveTo, int delay){
  do
  {
    hideDancer(dancer);

    (moveTo.y >= dancer->position.y) ? dancer->position.y++ : dancer->position.y--;
    (moveTo.x >= dancer->position.x) ? dancer->position.x++ : dancer->position.x--;
    displayDancer(dancer);
    usleep(delay);

  }
  while(dancer->position.y != moveTo.y || dancer->position.x != moveTo.x);
}

int free_girls_count(){
  int free_cnt = 0;

  for(int i = 0; i < app_config.girls_cnt; i++){
    if(girls_array[i].partner == NULL)
      free_cnt++;
  }

  return free_cnt;
}

Dancer *get_free_girls()
{
  Dancer *girls;
  girls = (Dancer*)malloc(free_girls_count() * sizeof(Dancer));

  for(int i = 0, p = 0; i < app_config.girls_cnt; i++){
    if(girls_array[i].partner == NULL){
      girls[p] = girls_array[i];
      p++;
    }
  }

  return girls;
}

void guy_behavior(void *args){
  Dancer *guy = args;

  guy->position.y = 0;
  guy->position.x = guy->id * 7;

  displayDancer(guy);

  Position pos;
  pos.y = 20;
  pos.x = guy->id * 7;

  move(guy, pos, 200000);

  pthread_mutex_lock(&partner_choose_mutex);

  printf("\033[%d;%dH", guy->position.y, guy->position.x);
  get_success_alert("%c", guy->name);
  usleep(2000000);
  pthread_mutex_unlock(&partner_choose_mutex);

  Dancer *girls = get_free_girls();

  printf("\033[%d;%dH", girls[0].position.y, girls[1].position.x);
  get_warning_alert("%c", girls[0].name);
  girls[0].partner = guy;

  usleep(2000000);
  pthread_mutex_unlock(&partner_choose_mutex);


}

void girl_behavior(void *args){
  Dancer *girl = args;

  girl->position.y = 20;
  girl->position.x = girl->id * 10;

  displayDancer(girl);

  Position pos;
  pos.y = 0;
  pos.x = girl->position.x = girl->id * 10;

  move(girl, pos, 200000);
}

void app_boot(Config config, Dancer *guys_array, Dancer *girls_array){
  //To refactor
}

void main(){

  app_config.guys_cnt = 10;
  app_config.girls_cnt = 7;
  app_config.dances_cnt = 5;

  pthread_mutex_init(&output_mutex, NULL);
  pthread_mutex_init(&partner_choose_mutex, NULL);

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

  printf("\033[2J\n");

  getchar();
}
