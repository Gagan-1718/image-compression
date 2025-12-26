#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node {
    int value;
    int freq;
    struct node *left, *right;
} Node;

typedef struct {
    int size;
    Node* data[256];
} MinHeap;

unsigned char header[54];
int width, height;

void swap(Node **a, Node **b) {
    Node *temp = *a;
    *a = *b;
    *b = temp;
}

void insertMinHeap(MinHeap *heap, Node *newNode) {
    int i = heap->size++;
    heap->data[i] = newNode;
    while (i && heap->data[i]->freq < heap->data[(i - 1) / 2]->freq) {
        swap(&heap->data[i], &heap->data[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

Node* extractMin(MinHeap *heap) {
    Node *root = heap->data[0];
    heap->data[0] = heap->data[--heap->size];

    int i = 0;
    while (2 * i + 1 < heap->size) {
        int smallest = i;
        if (heap->data[2 * i + 1]->freq < heap->data[smallest]->freq)
            smallest = 2 * i + 1;
        if (2 * i + 2 < heap->size && heap->data[2 * i + 2]->freq < heap->data[smallest]->freq)
            smallest = 2 * i + 2;
        if (smallest == i) break;
        swap(&heap->data[i], &heap->data[smallest]);
        i = smallest;
    }
    return root;
}

Node* buildHuffmanTree(int freq[]) {
    MinHeap heap = {0};
    for (int i = 0; i < 256; i++)
        if (freq[i]) {
            Node *newNode = (Node*)malloc(sizeof(Node));
            newNode->value = i;
            newNode->freq = freq[i];
            newNode->left = newNode->right = NULL;
            insertMinHeap(&heap, newNode);
        }

    while (heap.size > 1) {
        Node *left = extractMin(&heap);
        Node *right = extractMin(&heap);
        Node *merged = (Node*)malloc(sizeof(Node));
        merged->value = -1;
        merged->freq = left->freq + right->freq;
        merged->left = left;
        merged->right = right;
        insertMinHeap(&heap, merged);
    }
    return extractMin(&heap);
}

void generateCodes(Node *root, char *code, char *codes[256]) {
    if (!root) return;
    if (root->value != -1) {
        codes[root->value] = strdup(code);
        return;
    }
    char leftCode[256], rightCode[256];
    strcpy(leftCode, code);
    strcpy(rightCode, code);
    strcat(leftCode, "0");
    strcat(rightCode, "1");
    generateCodes(root->left, leftCode, codes);
    generateCodes(root->right, rightCode, codes);
}

void writeBMP(const char *filename, int **image) {
    FILE *fp = fopen(filename, "wb");
    fwrite(header, sizeof(unsigned char), 54, fp);
    for (int i = height - 1; i >= 0; i++)
        for (int j = 0; j < width; j++) {
            unsigned char pixel = (unsigned char)image[i][j];
            fwrite(&pixel, sizeof(unsigned char), 1, fp);
            fwrite(&pixel, sizeof(unsigned char), 1, fp);
            fwrite(&pixel, sizeof(unsigned char), 1, fp);
        }
    fclose(fp);
}

int main() {
    FILE *bmp = fopen("D:\\Gagan\\inp.bmp", "rb");
    fread(header, sizeof(unsigned char), 54, bmp);
    width = *(int*)&header[18];
    height = *(int*)&header[22];

    int **image = (int **)malloc(height * sizeof(int *));
    for (int i = 0; i < height; i++)
        image[i] = (int *)malloc(width * sizeof(int));

    for (int i = height - 1; i >= 0; i--)
        for (int j = 0; j < width; j++) {
            unsigned char rgb[3];
            fread(rgb, sizeof(unsigned char), 3, bmp);
            image[i][j] = rgb[0];
        }
    fclose(bmp);

    int freq[256] = {0};
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            freq[image[i][j]]++;

    Node *root = buildHuffmanTree(freq);

    char *codes[256] = {0};
    char empty[1] = "";
    generateCodes(root, empty, codes);

    FILE *enc = fopen("D:\\Gagan\\enc.txt", "w");
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            fprintf(enc, "%s", codes[image[i][j]]);
    fclose(enc);

    FILE *cb = fopen("D:\\Gagan\\codebook.txt", "w");
    for (int i = 0; i < 256; i++)
        if (codes[i])
            fprintf(cb, "%d %s\n", i, codes[i]);
    fclose(cb);

    FILE *dim = fopen("D:\\Gagan\\dimensions.txt", "w");
    fprintf(dim, "%d %d\n", width, height);
    fclose(dim);

    printf("Compression complete!\n");
    return 0;
}
