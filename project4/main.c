#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NODES 20 // Max servers/switches in the network
#define NAME_LEN 16  // e.g., "S1", "SwitchX"
#define INF INT_MAX  // Represents no connection

typedef struct {
  char names[MAX_NODES][NAME_LEN];
  int adj[MAX_NODES][MAX_NODES];
  int count;
} Network;

Network net;

int find_node(const char *name) {
  for (int i = 0; i < net.count; i++)
    if (strcmp(net.names[i], name) == 0)
      return i;
  return -1;
}

int add_node(const char *name) {
  if (net.count >= MAX_NODES)
    return -1;
  strcpy(net.names[net.count], name);
  return net.count++;
}

int ensure_node(const char *name) {
  int idx = find_node(name);
  if (idx != -1)
    return idx;
  return add_node(name);
}

void add_edge(const char *a, const char *b, int weight) {
  int u = ensure_node(a);
  int v = ensure_node(b);
  if (u == -1 || v == -1)
    return;
  net.adj[u][v] = weight;
  net.adj[v][u] = weight;
}

void load_topology() {
  add_edge("S1", "S2", 8);
  add_edge("S1", "S4", 20);
  add_edge("S2", "S3", 7);
  add_edge("S3", "S6", 12);
  add_edge("S4", "S5", 4);
  add_edge("S5", "S6", 6);
  add_edge("S2", "SwitchX", 3);
  add_edge("SwitchX", "S5", 5);
}

void dijkstra(int src, int dist[], int prev[]) {
  int visited[MAX_NODES] = {0};

  for (int i = 0; i < net.count; i++) {
    dist[i] = INF;
    prev[i] = -1;
  }

  dist[src] = 0;

  for (int i = 0; i < net.count; i++) {
    int u = -1;
    for (int j = 0; j < net.count; j++)
      if (!visited[j] && (u == -1 || dist[j] < dist[u]))
        u = j;

    if (u == -1 || dist[u] == INF)
      break;

    visited[u] = 1;

    for (int v = 0; v < net.count; v++) {
      if (net.adj[u][v] > 0 && !visited[v]) {
        if (dist[u] + net.adj[u][v] < dist[v]) {
          dist[v] = dist[u] + net.adj[u][v];
          prev[v] = u;
        }
      }
    }
  }
}

void print_path(int src, int dst, int dist[], int prev[]) {
  if (dist[dst] == INF) {
    printf("No path found.\n");
    return;
  }

  int path[MAX_NODES], len = 0;
  for (int at = dst; at != -1; at = prev[at])
    path[len++] = at;

  printf("\nOptimal Path: ");
  for (int i = len - 1; i >= 0; i--) {
    printf("%s", net.names[path[i]]);
    if (i > 0)
      printf(" -> ");
  }

  printf("\nTotal Latency: %d ms\n", dist[dst]);
}

void print_network() {
  printf("\nNetwork Topology (Adjacency Matrix):\n    ");
  for (int i = 0; i < net.count; i++)
    printf("%10s", net.names[i]);
  printf("\n");

  for (int i = 0; i < net.count; i++) {
    printf("%4s", net.names[i]);
    for (int j = 0; j < net.count; j++) {
      if (net.adj[i][j])
        printf("%10d", net.adj[i][j]);
      else
        printf("%10s", "-");
    }
    printf("\n");
  }
}

int main() {
  memset(&net, 0, sizeof(net));
  load_topology();

  printf("Network Routing Simulator\n");
  print_network();

  char src_name[NAME_LEN], dst_name[NAME_LEN];

  while (1) {
    printf("\nEnter source (or 'quit' or 'x'): ");
    if (!fgets(src_name, sizeof(src_name), stdin))
      break;
    src_name[strcspn(src_name, "\n")] = 0;

    // Allow quitting with "quit" or "x"
    if (strcmp(src_name, "quit") == 0 || strcmp(src_name, "x") == 0)
      break;

    printf("Enter destination: ");
    if (!fgets(dst_name, sizeof(dst_name), stdin))
      break;
    dst_name[strcspn(dst_name, "\n")] = 0;

    int src = find_node(src_name);
    int dst = find_node(dst_name);

    if (src == -1) {
      printf("Unknown server: %s\n", src_name);
      continue;
    }

    if (dst == -1) {
      printf("Unknown server: %s\n", dst_name);
      continue;
    }

    int dist[MAX_NODES], prev[MAX_NODES];
    dijkstra(src, dist, prev);
    print_path(src, dst, dist, prev);
  }

  printf("Simulator terminated.\n");
  return 0;
}
