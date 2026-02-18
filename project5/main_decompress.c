#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct HNode {
    unsigned char symbol;
    struct HNode *left;
    struct HNode *right;
} HNode;

HNode* new_node(unsigned char sym) {
    HNode *node = calloc(1, sizeof(HNode));
    node->symbol = sym;
    return node;
}

void free_tree(HNode *root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

typedef struct {
    FILE *f;
    uint8_t buf;
    int bit_pos;  // bits left in buffer
} BitReader;

void br_init(BitReader *br, FILE *f) {
    br->f = f;
    br->buf = 0;
    br->bit_pos = 0;
}

int br_read_bit(BitReader *br) {
    if (br->bit_pos == 0) {
        int c = fgetc(br->f);
        if (c == EOF) return -1;
        br->buf = (uint8_t)c;
        br->bit_pos = 8;
    }
    int bit = (br->buf >> 7) & 1;
    br->buf <<= 1;
    br->bit_pos--;
    return bit;
}

HNode* read_table(FILE *f) {
    uint32_t n_symbols;
    fread(&n_symbols, sizeof(uint32_t), 1, f);

    HNode *root = new_node(0);

    for (uint32_t i = 0; i < n_symbols; i++) {
        uint8_t sym, len;
        fread(&sym, 1, 1, f);
        fread(&len, 1, 1, f);

        int nbytes = (len + 7) / 8;
        uint8_t cbytes[4] = {0};
        fread(cbytes, 1, nbytes, f);

        HNode *cur = root;
        for (int b = 0; b < len; b++) {
            int bit = (cbytes[b / 8] >> (7 - b % 8)) & 1;
            if (bit == 0) {
                if (!cur->left) cur->left = new_node(0);
                cur = cur->left;
            } else {
                if (!cur->right) cur->right = new_node(0);
                cur = cur->right;
            }
        }
        cur->symbol = sym;
    }

    return root;
}

int decompress_file(const char *input, const char *output) {
    FILE *fin = fopen(input, "rb");
    if (!fin) { perror("Cannot open input"); return -1; }

    HNode *root = read_table(fin);
    if (!root) { fclose(fin); return -1; }

    uint64_t orig_size;
    uint32_t last_valid_bits;
    fread(&orig_size, sizeof(uint64_t), 1, fin);
    fread(&last_valid_bits, sizeof(uint32_t), 1, fin);

    FILE *fout = fopen(output, "wb");
    if (!fout) { perror("Cannot open output"); fclose(fin); free_tree(root); return -1; }

    // Calculate total bits in compressed data
    long start_pos = ftell(fin);
    fseek(fin, 0, SEEK_END);
    long end_pos = ftell(fin);
    fseek(fin, start_pos, SEEK_SET);
    long data_bytes = end_pos - start_pos;
    uint64_t total_bits = (data_bytes > 0) ? (uint64_t)(data_bytes - 1) * 8 + last_valid_bits : 0;

    BitReader br;
    br_init(&br, fin);

    HNode *cur = root;
    uint64_t decoded = 0;
    uint64_t bits_read = 0;

    while (decoded < orig_size && bits_read < total_bits) {
        int bit = br_read_bit(&br);
        if (bit < 0) break;
        bits_read++;

        cur = bit ? cur->right : cur->left;
        if (!cur) {
            printf("ERROR: Corrupt compressed data.\n");
            break;
        }

        if (!cur->left && !cur->right) {
            fputc(cur->symbol, fout);
            decoded++;
            cur = root;
        }
    }

    fclose(fin);
    fclose(fout);
    free_tree(root);

    printf("\nDecompression Complete\n");
    printf("Decoded bytes: %llu\n", (unsigned long long)decoded);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <compressed.log>\n", argv[0]);
        return 1;
    }

    decompress_file(argv[1], "decompressed.log");
    return 0;
}
