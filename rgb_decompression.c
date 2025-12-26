#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    int pixel;
    struct Node *left, *right;
} Node;

Node* createNode() {
    Node* node = (Node*)malloc(sizeof(Node));
    node->pixel = -1;
    node->left = node->right = NULL;
    return node;
}

void insertCode(Node* root, const char* code, int pixel) {
    Node* curr = root;
    for (int i = 0; code[i] != '\0'; i++) {
        if (code[i] == '0') {
            if (!curr->left) curr->left = createNode();
            curr = curr->left;
        } else if (code[i] == '1') {
            if (!curr->right) curr->right = createNode();
            curr = curr->right;
        }
    }
    curr->pixel = pixel;
}

void freeTree(Node* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

int readCodebook(const char* filename, Node* rootR, Node* rootG, Node* rootB) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Cannot open codebook file.\n");
        return 0;
    }
    char line[100];
    while (fgets(line, sizeof(line), f)) {
        char color;
        int pixel;
        char code[50];
        if (sscanf(line, "%c %d %s", &color, &pixel, code) != 3) continue;
        if (color == 'R') insertCode(rootR, code, pixel);
        else if (color == 'G') insertCode(rootG, code, pixel);
        else if (color == 'B') insertCode(rootB, code, pixel);
    }
    fclose(f);
    return 1;
}

unsigned char **allocate2D(int width, int height) {
    unsigned char **arr = (unsigned char **)malloc(height * sizeof(unsigned char *));
    for (int i = 0; i < height; i++) {
        arr[i] = (unsigned char *)malloc(width);
    }
    return arr;
}

void free2D(unsigned char **arr, int height) {
    for (int i = 0; i < height; i++) free(arr[i]);
    free(arr);
}

int save_bmp(const char* filename, unsigned char **R, unsigned char **G, unsigned char **B, int width, int height) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("Cannot create output BMP file.\n");
        return 0;
    }

    unsigned char bmpfileheader[14] = {
        'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0
    };
    unsigned char bmpinfoheader[40] = {
        40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0
    };

    int filesize = 54 + 3 * width * height;
    bmpfileheader[2] = (unsigned char)(filesize);
    bmpfileheader[3] = (unsigned char)(filesize >> 8);
    bmpfileheader[4] = (unsigned char)(filesize >> 16);
    bmpfileheader[5] = (unsigned char)(filesize >> 24);

    bmpinfoheader[4] = (unsigned char)(width);
    bmpinfoheader[5] = (unsigned char)(width >> 8);
    bmpinfoheader[6] = (unsigned char)(width >> 16);
    bmpinfoheader[7] = (unsigned char)(width >> 24);

    bmpinfoheader[8] = (unsigned char)(height);
    bmpinfoheader[9] = (unsigned char)(height >> 8);
    bmpinfoheader[10] = (unsigned char)(height >> 16);
    bmpinfoheader[11] = (unsigned char)(height >> 24);

    fwrite(bmpfileheader, 1, 14, f);
    fwrite(bmpinfoheader, 1, 40, f);

    int padding = (4 - (width * 3) % 4) % 4;
    unsigned char pad[3] = {0,0,0};

    for (int i = height - 1; i >= 0; i--) {
        for (int j = 0; j < width; j++) {
            unsigned char pixel[3];
            pixel[0] = B[i][j]; // BMP stores color as BGR
            pixel[1] = G[i][j];
            pixel[2] = R[i][j];
            fwrite(pixel, 1, 3, f);
        }
        fwrite(pad, 1, padding, f);
    }

    fclose(f);
    return 1;
}

int decode_channel(FILE *f, Node* root, unsigned char **channel, int width, int height) {
    int pixels_decoded = 0;
    Node* curr = root;
    int c;

    while (pixels_decoded < width * height) {
        c = fgetc(f);
        if (c == EOF) {
            printf("Unexpected end of compressed file while decoding channel.\n");
            return 0;
        }
        if (c != '0' && c != '1') continue; // skip other chars

        if (c == '0') curr = curr->left;
        else curr = curr->right;

        if (!curr) {
            printf("Decoding error: Reached NULL node.\n");
            return 0;
        }

        if (curr->pixel != -1) {
            int row = pixels_decoded / width;
            int col = pixels_decoded % width;
            channel[row][col] = (unsigned char)curr->pixel;
            pixels_decoded++;
            curr = root;
        }
    }
    return 1;
}

int main() {
    int width = 0, height = 0;

    // You must set width and height same as original image:
    // You can hardcode or read from user or from a metadata file.
    // For demo, hardcode:
    width = 256;  // change accordingly
    height = 256; // change accordingly

    unsigned char **R = allocate2D(width, height);
    unsigned char **G = allocate2D(width, height);
    unsigned char **B = allocate2D(width, height);

    Node *rootR = createNode();
    Node *rootG = createNode();
    Node *rootB = createNode();

    if (!readCodebook("D:\\MINI_PROJECT\\codebook.txt", rootR, rootG, rootB)) {
        printf("Failed to read codebook.\n");
        return 1;
    }

    FILE *f = fopen("D:\\MINI_PROJECT\\compressed.huff", "r");
    if (!f) {
        printf("Cannot open compressed file.\n");
        return 1;
    }

    if (!decode_channel(f, rootR, R, width, height)) {
        printf("Error decoding Red channel.\n");
        fclose(f);
        return 1;
    }

    if (!decode_channel(f, rootG, G, width, height)) {
        printf("Error decoding Green channel.\n");
        fclose(f);
        return 1;
    }

    if (!decode_channel(f, rootB, B, width, height)) {
        printf("Error decoding Blue channel.\n");
        fclose(f);
        return 1;
    }

    fclose(f);

    if (!save_bmp("D:\\MINI_PROJECT\\decompressed.bmp", R, G, B, width, height)) {
        printf("Error saving BMP file.\n");
        return 1;
    }

    printf("Decompression complete! File saved as D:\\MINI_PROJECT\\decompressed.bmp\n");

    freeTree(rootR);
    freeTree(rootG);
    freeTree(rootB);

    free2D(R, height);
    free2D(G, height);
    free2D(B, height);

    return 0;
}
