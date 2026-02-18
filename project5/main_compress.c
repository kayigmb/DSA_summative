#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct HNode {
  unsigned char symbol;
  uint64_t freq;
  struct HNode *left;
  struct HNode *right;
} HNode;

typedef struct {
  HNode **data;
  int size;
  int capacity;
} MinHeap;

typedef struct {
  uint32_t bits;
  int len;
} Code;

Code codes[256];

MinHeap *heap_new(int capacity) {
  MinHeap *h = malloc(sizeof(MinHeap));
  h->data = malloc(sizeof(HNode *) * capacity);
  h->size = 0;
  h->capacity = capacity;
  return h;
}

void heap_push(MinHeap *h, HNode *node) {
  int i = h->size++;
  h->data[i] = node;
  while (i > 0) {
    int parent = (i - 1) / 2;
    if (h->data[parent]->freq <= h->data[i]->freq)
      break;
    HNode *temp = h->data[parent];
    h->data[parent] = h->data[i];
    h->data[i] = temp;
    i = parent;
  }
}

HNode *heap_pop(MinHeap *h) {
  HNode *top = h->data[0];
  h->data[0] = h->data[--h->size];

  int i = 0;
  while (1) {
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    int smallest = i;

    if (left < h->size && h->data[left]->freq < h->data[smallest]->freq)
      smallest = left;
    if (right < h->size && h->data[right]->freq < h->data[smallest]->freq)
      smallest = right;
    if (smallest == i)
      break;

    HNode *temp = h->data[i];
    h->data[i] = h->data[smallest];
    h->data[smallest] = temp;
    i = smallest;
  }

  return top;
}

HNode *new_node(unsigned char sym, uint64_t freq, HNode *left, HNode *right) {
  HNode *node = calloc(1, sizeof(HNode));
  node->symbol = sym;
  node->freq = freq;
  node->left = left;
  node->right = right;
  return node;
}

void free_tree(HNode *root) {
  if (!root)
    return;
  free_tree(root->left);
  free_tree(root->right);
  free(root);
}

void build_codes(HNode *node, uint32_t bits, int len) {
  if (!node->left && !node->right) {
    codes[node->symbol].bits = bits;
    codes[node->symbol].len = (len > 0) ? len : 1;
    return;
  }
  if (node->left)
    build_codes(node->left, bits << 1, len + 1);
  if (node->right)
    build_codes(node->right, (bits << 1) | 1, len + 1);
}

typedef struct {
  FILE *f;
  uint8_t buf;
  int bit_pos;
} BitWriter;

void bw_init(BitWriter *bw, FILE *f) {
  bw->f = f;
  bw->buf = 0;
  bw->bit_pos = 0;
}

void bw_write_bit(BitWriter *bw, int bit) {
  bw->buf = (bw->buf << 1) | (bit & 1);
  bw->bit_pos++;
  if (bw->bit_pos == 8) {
    fputc(bw->buf, bw->f);
    bw->buf = 0;
    bw->bit_pos = 0;
  }
}

int bw_flush(BitWriter *bw) {
  if (bw->bit_pos > 0) {
    bw->buf <<= (8 - bw->bit_pos);
    fputc(bw->buf, bw->f);
    return bw->bit_pos;
  }
  return 8;
}

void write_table(FILE *f, uint64_t freq[256]) {
  uint32_t count = 0;
  for (int i = 0; i < 256; i++)
    if (freq[i])
      count++;
  fwrite(&count, sizeof(uint32_t), 1, f);

  for (int i = 0; i < 256; i++) {
    if (!freq[i])
      continue;
    uint8_t sym = i;
    uint8_t len = codes[i].len;
    fwrite(&sym, 1, 1, f);
    fwrite(&len, 1, 1, f);

    uint8_t cbytes[4] = {0};
    int nbytes = (len + 7) / 8;
    for (int b = 0; b < len; b++)
      if ((codes[i].bits >> (len - 1 - b)) & 1)
        cbytes[b / 8] |= (1 << (7 - b % 8));
    fwrite(cbytes, 1, nbytes, f);
  }
}

int compress_file(const char *input, const char *output) {
  FILE *fin = fopen(input, "rb");
  if (!fin) {
    perror("Cannot open input");
    return -1;
  }

  uint64_t freq[256] = {0};
  int ch;
  uint64_t orig_size = 0;
  while ((ch = fgetc(fin)) != EOF) {
    freq[ch]++;
    orig_size++;
  }

  if (orig_size == 0) {
    fclose(fin);
    printf("Input file is empty.\n");
    return -1;
  }

  rewind(fin);

  MinHeap *heap = heap_new(256);
  for (int i = 0; i < 256; i++)
    if (freq[i])
      heap_push(heap, new_node(i, freq[i], NULL, NULL));

  if (heap->size == 1) {
    HNode *only = heap_pop(heap);
    heap_push(heap, new_node(0, only->freq, only, NULL));
  }

  while (heap->size > 1) {
    HNode *a = heap_pop(heap);
    HNode *b = heap_pop(heap);
    heap_push(heap, new_node(0, a->freq + b->freq, a, b));
  }

  HNode *root = heap_pop(heap);
  free(heap->data);
  free(heap);

  for (int i = 0; i < 256; i++)
    codes[i].bits = 0, codes[i].len = 0;
  build_codes(root, 0, 0);

  FILE *fout = fopen(output, "wb");
  if (!fout) {
    perror("Cannot open output");
    free_tree(root);
    fclose(fin);
    return -1;
  }

  write_table(fout, freq);
  fwrite(&orig_size, sizeof(uint64_t), 1, fout);

  long valid_pos = ftell(fout);
  uint32_t placeholder = 0;
  fwrite(&placeholder, sizeof(uint32_t), 1, fout);

  BitWriter bw;
  bw_init(&bw, fout);

  while ((ch = fgetc(fin)) != EOF) {
    Code cd = codes[ch];
    for (int b = cd.len - 1; b >= 0; b--)
      bw_write_bit(&bw, (cd.bits >> b) & 1);
  }

  int last_valid = bw_flush(&bw);

  fseek(fout, valid_pos, SEEK_SET);
  uint32_t lv = (uint32_t)last_valid;
  fwrite(&lv, sizeof(uint32_t), 1, fout);

  fclose(fin);
  fclose(fout);
  free_tree(root);

  printf("\nCompression Complete\n");
  printf("Original size   : %llu bytes\n", (unsigned long long)orig_size);
  printf("Compressed size : %ld bytes\n", ftell(fout));
  printf("Valid bits last byte : %d\n", last_valid);

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <input.txt>\n", argv[0]);
    return 1;
  }

  compress_file(argv[1], "compressed.log");
  return 0;
}
