#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node {
    int value;
    struct node *left, *right;
} Node;

Node* createNode() {
    Node *newNode = (Node*)malloc(sizeof(Node));
    newNode->value = -1;
    newNode->left = newNode->right = NULL;
    return newNode;
}

void insertCode(Node *root, char *code, int value) {
    Node *curr = root;
    while (*code) {
        if (*code == '0') {
            if (!curr->left) curr->left = createNode();
            curr = curr->left;
        } else {
            if (!curr->right) curr->right = createNode();
            curr = curr->right;
        }
        code++;
    }
    curr->value = value;
}

void save_bmp(const char *filename, int **image, int width, int height) {
    FILE *fp = fopen(filename, "wb");

    unsigned char header[54] = {
        0x42, 0x4D, 0,0,0,0, 0,0, 0,0, 54,0,0,0,
        40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0
    };

    int rowSize = (3 * width + 3) & (~3);
    int imageSize = rowSize * height;
    *(int*)&header[2] = 54 + imageSize;
    *(int*)&header[18] = width;
    *(int*)&header[22] = height;
    *(int*)&header[34] = imageSize;

    fwrite(header, sizeof(unsigned char), 54, fp);
    unsigned char *row = (unsigned char*)malloc(rowSize);
    for (int i = height - 1; i >= 0; i--) {
        for (int j = 0; j < width; j++) {
            unsigned char val = (unsigned char)image[i][j];
            row[j * 3] = row[j * 3 + 1] = row[j * 3 + 2] = val;
        }
        fwrite(row, 1, rowSize, fp);
    }
    free(row);
    fclose(fp);
}

int main() {
    int width, height;
    FILE *dim = fopen("D:\\Gagan\\dimensions.txt", "r");
    fscanf(dim, "%d %d", &width, &height);
    fclose(dim);

    Node *root = createNode();

    FILE *cb = fopen("D:\\Gagan\\codebook.txt", "r");
    int val;
    char code[256];
    while (fscanf(cb, "%d %s", &val, code) != EOF)
        insertCode(root, code, val);
    fclose(cb);

    FILE *enc = fopen("D:\\Gagan\\enc.txt", "r");
    fseek(enc, 0, SEEK_END);
    long length = ftell(enc);
    rewind(enc);

    char *bits = (char*)malloc(length + 1);
    fread(bits, 1, length, enc);
    bits[length] = '\0';
    fclose(enc);

    int **image = (int**)malloc(height * sizeof(int*));
    for (int i = 0; i < height; i++)
        image[i] = (int*)malloc(width * sizeof(int));

    Node *curr = root;
    int i = 0, row = 0, col = 0;
    while (bits[i] && row < height) {
        curr = (bits[i] == '0') ? curr->left : curr->right;
        if (curr->value != -1) {
            image[row][col++] = curr->value;
            curr = root;
            if (col == width) {
                col = 0;
                row++;
            }
        }
        i++;
    }

    save_bmp("D:\\Gagan\\decompressed.bmp", image, width, height);
    printf("Decompression complete!\n");
    return 0;
}
