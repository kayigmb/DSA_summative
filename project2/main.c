#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LEN 64
#define REVIEW_FILE "rejected_commands.log"
#define THRESHOLD 3

typedef struct Node {
  char cmd[MAX_LEN];
  struct Node *left, *right;
} Node;

Node *new_node(const char *cmd) {
  Node *n = malloc(sizeof(Node));
  strcpy(n->cmd, cmd);
  n->left = n->right = NULL;
  return n;
}

Node *insert(Node *root, const char *cmd) {
  if (!root)
    return new_node(cmd);
  int cmp = strcmp(cmd, root->cmd);
  if (cmp < 0)
    root->left = insert(root->left, cmd);
  else if (cmp > 0)
    root->right = insert(root->right, cmd);
  return root;
}

int search(Node *root, const char *cmd) {
  if (!root)
    return 0;
  int cmp = strcmp(cmd, root->cmd);
  if (cmp == 0)
    return 1;
  return (cmp < 0) ? search(root->left, cmd) : search(root->right, cmd);
}

int edit_distance(const char *a, const char *b) {
  int la = strlen(a), lb = strlen(b);
  int dp[65][65];

  for (int i = 0; i <= la; i++)
    for (int j = 0; j <= lb; j++) {
      if (i == 0)
        dp[i][j] = j;
      else if (j == 0)
        dp[i][j] = i;
      else if (a[i - 1] == b[j - 1])
        dp[i][j] = dp[i - 1][j - 1];
      else {
        int min = dp[i - 1][j];
        if (dp[i][j - 1] < min)
          min = dp[i][j - 1];
        if (dp[i - 1][j - 1] < min)
          min = dp[i - 1][j - 1];
        dp[i][j] = 1 + min;
      }
    }
  return dp[la][lb];
}

void find_closest(Node *root, const char *input, char *best, int *best_dist) {
  if (!root)
    return;

  find_closest(root->left, input, best, best_dist);

  int d = edit_distance(input, root->cmd);
  if (d < *best_dist) {
    *best_dist = d;
    strcpy(best, root->cmd);
  }

  find_closest(root->right, input, best, best_dist);
}

void log_rejected(const char *cmd) {
  FILE *f = fopen(REVIEW_FILE, "a");
  if (!f)
    return;
  time_t t = time(NULL);
  fprintf(f, "[%ld] REJECTED: %s\n", t, cmd);
  fclose(f);
}

void free_tree(Node *root) {
  if (!root)
    return;
  free_tree(root->left);
  free_tree(root->right);
  free(root);
}

int main() {

  Node *root = NULL;
  FILE *file = fopen("commands.txt", "r");
  char buffer[MAX_LEN];

  if (file) {
    while (fgets(buffer, sizeof(buffer), file)) {
      buffer[strcspn(buffer, "\n")] = 0;
      root = insert(root, buffer);
    }
    fclose(file);
  } else {
    const char *defaults[] = {
        "START_UP",       "STOP_CONVEYOR",  "RESET_ALARM",  "PAUSE_PROCESS",
        "RESUME_PROCESS", "EMERGENCY_STOP", "CHECK_STATUS", NULL};
    for (int i = 0; defaults[i]; i++)
      root = insert(root, defaults[i]);
  }

  printf(" Command Authorization Simulator \n");
  printf("Type 'quit' or 'exit' to exit.\n");

  char input[MAX_LEN];

  while (1) {
    printf("\nCommand> ");
    if (!fgets(input, sizeof(input), stdin))
      break;
    input[strcspn(input, "\n")] = 0;

    if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0)
      break;

    if (search(root, input)) {
      printf("[AUTHORIZED] Command executed: %s\n", input);
    } else {
      char best[MAX_LEN] = "";
      int best_dist = INT_MAX;

      find_closest(root, input, best, &best_dist);

      if (best_dist > 0 && best_dist <= THRESHOLD) {
        printf("[SUGGESTION] Did you mean: %s ?\n", best);
      } else {
        printf("[REJECTED] Command not recognized.\n");
        log_rejected(input);
      }
    }
  }

  free_tree(root);
  printf("Simulator terminated.\n");
  return 0;
}
