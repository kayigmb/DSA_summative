#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERS 50 // Max users in the graph
#define ID_LEN 8 // e.g., "U12345"

typedef struct {
  char users[MAX_USERS][ID_LEN];
  int adj[MAX_USERS][MAX_USERS];
  int count;
} Graph;

Graph g;

int find_user(const char *id) {
  for (int i = 0; i < g.count; i++)
    if (strcmp(g.users[i], id) == 0)
      return i;
  return -1;
}

int add_user(const char *id) {
  if (g.count >= MAX_USERS) {
    printf("User limit reached.\n");
    return -1;
  }
  if (find_user(id) != -1) {
    printf("User already exists.\n");
    return -1;
  }
  strcpy(g.users[g.count], id);
  g.count++;

  printf("User %s added.\n", id);
  return g.count - 1;
}

int ensure_user(const char *id) {
  int idx = find_user(id);
  if (idx == -1)
    idx = add_user(id);
  return idx;
}

void remove_user(const char *id) {
  int idx = find_user(id);
  if (idx == -1) {
    printf("User not found.\n");
    return;
  }

  for (int i = idx; i < g.count - 1; i++) {
    strcpy(g.users[i], g.users[i + 1]);
    for (int j = 0; j < g.count; j++)
      g.adj[i][j] = g.adj[i + 1][j];
  }

  for (int i = 0; i < g.count - 1; i++)
    for (int j = idx; j < g.count - 1; j++)
      g.adj[i][j] = g.adj[i][j + 1];

  g.count--;
  printf("User %s removed.\n", id);
}

void add_interaction(const char *from, const char *to) {
  int f = ensure_user(from);
  int t = ensure_user(to);
  if (f == -1 || t == -1 || f == t)
    return;

  g.adj[f][t] = 1;
  printf("Interaction %s -> %s added.\n", from, to);
}

void remove_interaction(const char *from, const char *to) {
  int f = find_user(from);
  int t = find_user(to);
  if (f == -1 || t == -1) {
    printf("User not found.\n");
    return;
  }
  g.adj[f][t] = 0;
  printf("Interaction %s -> %s removed.\n", from, to);
}

void query_user(const char *id) {
  int idx = find_user(id);
  if (idx == -1) {
    printf("Unknown user ID.\n");
    return;
  }

  printf("\nUser %s\n", id);

  printf("Outgoing:\n");
  int found = 0;
  for (int j = 0; j < g.count; j++)
    if (g.adj[idx][j]) {
      printf("  -> %s\n", g.users[j]);
      found = 1;
    }
  if (!found)
    printf("  None\n");

  printf("Incoming:\n");
  found = 0;
  for (int i = 0; i < g.count; i++)
    if (g.adj[i][idx]) {
      printf("  <- %s\n", g.users[i]);
      found = 1;
    }
  if (!found)
    printf("  None\n");
}

void print_matrix() {
  if (g.count == 0) {
    printf("Graph empty.\n");
    return;
  }

  printf("\nAdjacency Matrix:\n    ");
  for (int i = 0; i < g.count; i++)
    printf("%7s", g.users[i]);
  printf("\n");

  for (int i = 0; i < g.count; i++) {
    printf("%4s", g.users[i]);
    for (int j = 0; j < g.count; j++)
      printf("%7d", g.adj[i][j]);
    printf("\n");
  }
}

void load_initial() {
  const char *edges[][2] = {
      {"U101", "U102"}, {"U101", "U103"}, {"U102", "U104"},
      {"U103", "U105"}, {"U104", "U105"}, {"U104", "U106"},
      {"U105", "U107"}, {"U106", "U108"}, {NULL, NULL}};

  for (int i = 0; edges[i][0]; i++)
    add_interaction(edges[i][0], edges[i][1]);
}

int main() {
  memset(&g, 0, sizeof(g));

  printf("Interaction Mapping Tool \n");
  load_initial();
  print_matrix();

  char choice[8], a[ID_LEN], b[ID_LEN];

  while (1) {
    printf("\n1) Query\n2) Matrix\n3) Add User\n4) Remove User\n");
    printf("5) Add Interaction\n6) Remove Interaction\n0) Exit\nChoice: ");

    if (!fgets(choice, sizeof(choice), stdin))
      break;

    switch (choice[0]) {
    case '1':
      printf("User ID: ");
      fgets(a, sizeof(a), stdin);
      a[strcspn(a, "\n")] = 0;
      query_user(a);
      break;
    case '2':
      print_matrix();
      break;
    case '3':
      printf("New User ID: ");
      fgets(a, sizeof(a), stdin);
      a[strcspn(a, "\n")] = 0;
      add_user(a);
      break;
    case '4':
      printf("User ID: ");
      fgets(a, sizeof(a), stdin);
      a[strcspn(a, "\n")] = 0;
      remove_user(a);
      break;
    case '5':
      printf("From: ");
      fgets(a, sizeof(a), stdin);
      a[strcspn(a, "\n")] = 0;
      printf("To: ");
      fgets(b, sizeof(b), stdin);
      b[strcspn(b, "\n")] = 0;
      add_interaction(a, b);
      break;
    case '6':
      printf("From: ");
      fgets(a, sizeof(a), stdin);
      a[strcspn(a, "\n")] = 0;
      printf("To: ");
      fgets(b, sizeof(b), stdin);
      b[strcspn(b, "\n")] = 0;
      remove_interaction(a, b);
      break;
    case '0':
      printf("Exiting.\n");
      return 0;
    default:
      printf("Invalid choice.\n");
    }
  }
  return 0;
}
