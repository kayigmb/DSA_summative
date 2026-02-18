#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_EVENTS 20 // Max events to keep in memory
#define MSG_LEN 128   // Max message length

typedef struct Event {
  int id;
  char time[32];
  char msg[MSG_LEN];
  struct Event *prev;
  struct Event *next;
} Event;

typedef struct {
  Event *head;   // Oldest
  Event *tail;   // Newest
  Event *cursor; // Operator position
  int count;
  int next_id;
  int live;
  int running;
  pthread_mutex_t lock;
} Log;

Log logg;

void timestamp(char *buf) {
  time_t t = time(NULL);
  strftime(buf, 32, "%Y-%m-%d %H:%M:%S", localtime(&t));
}

void print_event(Event *e) {
  if (!e) {
    printf("No event.\n");
    return;
  }
  printf("[ID:%03d | %s] %s\n", e->id, e->time, e->msg);
}

void remove_oldest() {
  if (!logg.head)
    return;

  Event *tmp = logg.head;
  if (logg.cursor == tmp)
    logg.cursor = tmp->next;

  logg.head = tmp->next;
  if (logg.head)
    logg.head->prev = NULL;
  else
    logg.tail = NULL;

  free(tmp);
  logg.count--;
}

void add_event(const char *msg) {
  pthread_mutex_lock(&logg.lock);

  if (logg.count == MAX_EVENTS)
    remove_oldest();

  Event *e = malloc(sizeof(Event));
  e->id = logg.next_id++;
  timestamp(e->time);
  strncpy(e->msg, msg, MSG_LEN);
  e->prev = logg.tail;
  e->next = NULL;

  if (logg.tail)
    logg.tail->next = e;
  else
    logg.head = e;

  logg.tail = e;
  logg.count++;

  if (logg.count == 1)
    logg.cursor = e; // Start at oldest

  if (logg.live) {
    printf("\n[LIVE] ");
    print_event(e);
  }

  pthread_mutex_unlock(&logg.lock);
}

void cmd_next() {
  pthread_mutex_lock(&logg.lock);
  if (logg.cursor && logg.cursor->next) {
    logg.cursor = logg.cursor->next;
    print_event(logg.cursor);
  } else
    printf("Already at newest.\n");
  pthread_mutex_unlock(&logg.lock);
}

void cmd_prev() {
  pthread_mutex_lock(&logg.lock);
  if (logg.cursor && logg.cursor->prev) {
    logg.cursor = logg.cursor->prev;
    print_event(logg.cursor);
  } else
    printf("Already at oldest.\n");
  pthread_mutex_unlock(&logg.lock);
}

void cmd_clear() {
  pthread_mutex_lock(&logg.lock);
  Event *cur = logg.head;
  while (cur) {
    Event *n = cur->next;
    free(cur);
    cur = n;
  }
  logg.head = logg.tail = logg.cursor = NULL;
  logg.count = 0;
  printf("All events cleared.\n");
  pthread_mutex_unlock(&logg.lock);
}

void cmd_exit() {
  pthread_mutex_lock(&logg.lock);
  logg.running = 0;
  printf("\nShutting down. Events stored: %d\n", logg.count);
  pthread_mutex_unlock(&logg.lock);
}

void *producer(void *arg) {
  const char *msgs[] = {"Power: 3.4 kW", "Voltage: 230 V", "Frequency: 50 Hz",
                        "Fault: Overcurrent"};
  int i = 0;

  while (logg.running) {
    add_event(msgs[i % 4]);
    i++;
    sleep(2);
  }
  return NULL;
}

int main() {

  memset(&logg, 0, sizeof(logg));
  pthread_mutex_init(&logg.lock, NULL);
  logg.running = 1;

  /* Initial events */
  add_event("System Boot");
  add_event("Meter Connected");

  pthread_t tid;
  pthread_create(&tid, NULL, producer, NULL);

  printf(" Smart Energy Gateway \n");
  printf("n=next  p=prev  r=live  h=hold  c=clear  x=exit\n\n");

  print_event(logg.cursor); // Start at oldest

  char cmd;
  while (logg.running) {
    printf("\n> ");
    scanf(" %c", &cmd);

    switch (cmd) {
    case 'n':
      cmd_next();
      break;
    case 'p':
      cmd_prev();
      break;
    case 'r':
      logg.live = 1;
      printf("Live ON\n");
      break;
    case 'h':
      logg.live = 0;
      printf("Live paused\n");
      break;
    case 'c':
      cmd_clear();
      break;
    case 'x':
      cmd_exit();
      break;
    default:
      printf("Invalid command\n");
    }
  }

  pthread_join(tid, NULL);
  cmd_clear();
  pthread_mutex_destroy(&logg.lock);
  return 0;
}
