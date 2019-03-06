#ifndef BUFFER_H
#define BUFFER_H
typedef struct b{
    int* buff;
    int cap;
    int size;
    int top;
    int bottom;
}buffer;

buffer* createBuffer(int capacity);

//return -1 if full
int add(buffer* b, int element);

//return -1 if empty
int get(buffer* b);

void freeBuffer(buffer* b);


#endif