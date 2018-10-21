#include <stdint.h>
#include <stdlib.h>
#include "queue.h"


// 构造一个空队列Q
void Q_Init(SqQueue* Q)
{
    Q->base = (int *)malloc(MAX_QSIZE * sizeof(int));
    // 存储分配失败
    if (!Q->base){
        return;
    }
    Q->front = Q->rear = 0;
    return;
}

// 销毁队列Q，Q不再存在
void Q_Destroy(SqQueue *Q)
{
    if (Q->base)
        free(Q->base);
    Q->base = NULL;
    Q->front = Q->rear = 0;
}

// 将Q清为空队列
void Q_Clear(SqQueue *Q)
{
    Q->front = Q->rear = 0;
}

// 若队列Q为空队列，则返回1；否则返回-1
int Q_Empty(SqQueue Q)
{
    if (Q.front == Q.rear) // 队列空的标志
        return 1;
    else
        return -1;
}

// 返回Q的元素个数，即队列的长度
int Q_Length(SqQueue Q)
{
    return (Q.rear - Q.front + MAX_QSIZE) % MAX_QSIZE;
}

// 若队列不空，则用e返回Q的队头元素，并返回OK；否则返回ERROR
int Q_GetHead(SqQueue Q, int *e) {
    if (Q.front == Q.rear) // 队列空
        return -1;
    *e = Q.base[Q.front];
    return 1;
}

//// 打印队列中的内容
//void Q_Print(SqQueue Q) {
//    int p = Q.front;
//    while (Q.rear != p) {
//        printf("%d\n", Q.base[p]);
//        p += 1;
//    }
//}

// 插入元素e为Q的新的队尾元素
int Q_Put(SqQueue *Q, int e)
{
    if ((Q->rear + 1) % MAX_QSIZE == Q->front) // 队列满
        return -1;
    Q->base[Q->rear] = e;
    Q->rear = (Q->rear + 1) % MAX_QSIZE;
    return 1;
}

// 若队列不空，则删除Q的队头元素，用e返回其值，并返回1；否则返回-1
int Q_Poll(SqQueue *Q, int *e)
{
    if (Q->front == Q->rear) // 队列空
        return -1;
    *e = Q->base[Q->front];
    Q->front = (Q->front + 1) % MAX_QSIZE;
    return 1;
}


//QHAL

int queue_try_put(SqQueue *queue, void *p)
{
	if(Q_Put(queue, (int)p) == 1) {
		return 1;
	} else {
		return 0;
	}
}


void *queue_try_get(SqQueue *queue)
{
	int e;
	if(Q_Poll(queue, &e) == 1) {
		return (void *)e;
	} else {
		return NULL;
	}
}

