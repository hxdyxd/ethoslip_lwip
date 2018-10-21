#ifndef __QUEUE_H__
#define __QUEUE_H__

// 队列的顺序存储结构(循环队列)
#define MAX_QSIZE 100 // 最大队列长度+1
typedef struct {
    int *base; // 初始化的动态分配存储空间
    int front; // 头指针，若队列不空，指向队列头元素
    int rear; // 尾指针，若队列不空，指向队列尾元素的下一个位置
} SqQueue;


void Q_Init(SqQueue *Q);
void Q_Destroy(SqQueue *Q);
int Q_Empty(SqQueue Q);
int Q_Length(SqQueue Q);
int Q_Put(SqQueue *Q, int e);
int Q_Poll(SqQueue *Q, int *e);

//QHAL
int queue_try_put(SqQueue *queue, void *p);
void *queue_try_get(SqQueue *queue);

#endif
