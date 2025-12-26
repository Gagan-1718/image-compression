#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct pixelfreq {
    int pix;
    float freq;
    struct pixelfreq *left, *right;
    char code[50];
} pixelfreq;

typedef struct huffcode {
    int pix, arrloc;
    float freq;
} huffcode;

pixelfreq *pixel_freq;
huffcode *huffcodes;

void strconcat(char *str, const char *parentcode, char add) {
    int i = 0;
    while (parentcode[i] != '\0') {
        str[i] = parentcode[i];
        i++;
    }
    if (add != '2') {
        str[i] = add;
        str[i + 1] = '\0';
    } else {
        str[i] = '\0';
    }
}

void build_huffman_tree(int *hist, int nodes, int totalnodes, int totpix) {
    int i, j, k, n = 0, nextnode = nodes;
    float sumprob;
    int sumpix;

    pixel_freq = (pixelfreq *)malloc(sizeof(pixelfreq) * totalnodes);
    huffcodes = (huffcode *)malloc(sizeof(huffcode) * nodes);

    for (i = 0, j = 0; i < 256; i++) {
        if (hist[i] != 0) {
            huffcodes[j].pix = i;
            pixel_freq[j].pix = i;
            huffcodes[j].arrloc = j;
            float tempprob = (float)hist[i] / (float)totpix;
            pixel_freq[j].freq = tempprob;
            huffcodes[j].freq = tempprob;
            pixel_freq[j].left = NULL;
            pixel_freq[j].right = NULL;
            pixel_freq[j].code[0] = '\0';
            j++;
        }
    }

    for (i = 0; i < nodes - 1; i++) {
        for (j = i + 1; j < nodes; j++) {
            if (huffcodes[i].freq > huffcodes[j].freq) {
                huffcode temp = huffcodes[i];
                huffcodes[i] = huffcodes[j];
                huffcodes[j] = temp;
            }
        }
    }

    while (n < nodes - 1) {
        sumprob = huffcodes[0].freq + huffcodes[1].freq;
        sumpix = huffcodes[0].pix + huffcodes[1].pix;

        pixel_freq[nextnode].pix = sumpix;
        pixel_freq[nextnode].freq = sumprob;
        pixel_freq[nextnode].left = &pixel_freq[huffcodes[0].arrloc];
        pixel_freq[nextnode].right = &pixel_freq[huffcodes[1].arrloc];
        pixel_freq[nextnode].code[0] = '\0';

        i = 0;
        while (i < nodes - n - 1 && sumprob > huffcodes[i].freq) i++;

        for (k = nodes - n - 1; k > i; k--) {
            huffcodes[k] = huffcodes[k - 1];
        }

        huffcodes[i].pix = sumpix;
        huffcodes[i].freq = sumprob;
        huffcodes[i].arrloc = nextnode;

        for (k = 0; k < nodes - n - 3; k++) {
            huffcodes[k] = huffcodes[k + 2];
        }

        n++;
        nextnode++;
    }

    char left = '0', right = '1';
    int total = 2 * nodes - 1;
    pixel_freq[total - 1].code[0] = '\0';

    for (i = total - 1; i >= nodes; i--) {
        if (pixel_freq[i].left != NULL)
            strconcat(pixel_freq[i].left->code, pixel_freq[i].code, left);
        if (pixel_freq[i].right != NULL)
            strconcat(pixel_freq[i].right->code, pixel_freq[i].code, right);
    }
}

void compress_channel(FILE *fenc, FILE *fcode, unsigned char **channel, int width, int height, char color) {
    int i, j;
    int hist[256] = {0}, nodes = 0, totpix = width * height;

    for (i = 0; i < height; i++)
        for (j = 0; j < width; j++)
            hist[channel[i][j]]++;

    for (i = 0; i < 256; i++)
        if (hist[i] != 0) nodes++;

    int totalnodes = 2 * nodes - 1;
    build_huffman_tree(hist, nodes, totalnodes, totpix);

    char *code_lookup[256] = {0};
    for (i = 0; i < nodes; i++) {
        code_lookup[pixel_freq[i].pix] = pixel_freq[i].code;
        fprintf(fcode, "%c %d %s\n", color, pixel_freq[i].pix, pixel_freq[i].code);
    }

    for (i = 0; i < height; i++)
        for (j = 0; j < width; j++) {
            int pix_val = channel[i][j];
            fprintf(fenc, "%s", code_lookup[pix_val]);
        }

    free(pixel_freq);
    free(huffcodes);
}

int read_bmp_image(const char *filename, unsigned char ***red, unsigned char ***green, unsigned char ***blue, int *width, int *height) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;

    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, f);
    *width = *(int *)&header[18];
    *height = *(int *)&header[22];
    int bpp = *(short *)&header[28];
    if (bpp != 24) return 0;

    int row_padded = (*width * 3 + 3) & (~3);
    unsigned char *row = (unsigned char *)malloc(row_padded);

    *red = (unsigned char **)malloc(*height * sizeof(unsigned char *));
    *green = (unsigned char **)malloc(*height * sizeof(unsigned char *));
    *blue = (unsigned char **)malloc(*height * sizeof(unsigned char *));
    for (int i = 0; i < *height; i++) {
        (*red)[i] = (unsigned char *)malloc(*width);
        (*green)[i] = (unsigned char *)malloc(*width);
        (*blue)[i] = (unsigned char *)malloc(*width);
    }

    for (int i = *height - 1; i >= 0; i--) {
        fread(row, sizeof(unsigned char), row_padded, f);
        for (int j = 0; j < *width; j++) {
            (*blue)[i][j] = row[j * 3 + 0];
            (*green)[i][j] = row[j * 3 + 1];
            (*red)[i][j] = row[j * 3 + 2];
        }
    }

    free(row);
    fclose(f);
    return 1;
}

int main() {
    int width, height;
    unsigned char **R, **G, **B;

    if (!read_bmp_image("D:\\MINI_PROJECT\\image.bmp", &R, &G, &B, &width, &height)) {
        printf("Error reading image.\n");
        return 1;
    }

    FILE *fenc = fopen("D:\\MINI_PROJECT\\compressed.huff", "w");
    FILE *fcode = fopen("D:\\MINI_PROJECT\\codebook.txt", "w");
    if (!fenc || !fcode) {
        printf("Error creating output files!\n");
        return 1;
    }

    compress_channel(fenc, fcode, R, width, height, 'R');
    compress_channel(fenc, fcode, G, width, height, 'G');
    compress_channel(fenc, fcode, B, width, height, 'B');

    fclose(fenc);
    fclose(fcode);

    printf("Compression complete! Files saved to D:\\MINI_PROJECT\\\n");

    for (int i = 0; i < height; i++) {
        free(R[i]); free(G[i]); free(B[i]);
    }
    free(R); free(G); free(B);

    return 0;
}
