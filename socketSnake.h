#ifndef SOCKETSNAKE_H_
#define SOCKETSNAKE_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "uthash.h"

#define WIDE 80
#define HIGH 30

typedef struct FOOD {
    int X;
    int Y;
} Food;

typedef struct BODY {
    int X;
    int Y;
} Body;

typedef struct SNAKE {
    struct BODY body[HIGH * WIDE];
    int size;
} Snake;

typedef struct mapSnake {
    int snakeUid;
    Snake* snake;
    UT_hash_handle hh;
} MapSnake;
MapSnake* mapSnake = NULL;

typedef struct arguments {
    int snakeUid;
    char key;
    int sockfd;
} Arguments;

Food food;

int kx = 0;
int ky = 0;

char key = 'd';

void initSnake(Snake* snake);
void initFood();
void addSnake();
void delAllSnake();
Snake* findSnake();
void* playGame(void* arg);
void gameEnd(int snakeUid, int sockfd);
void removeSnake(int snakeUid);
void sendMap(int sockfd);

void initFood() {
    srand(time(NULL));
    food.X = rand() % 76 + 2;  // 2-78
    food.Y = rand() % 26 + 2;  // 2-28
}

void initSnake(Snake* snake) {
    int x = rand() % 45 + 5;
    int y = rand() % 24 + 3;

    snake->body[0].X = x;
    snake->body[0].Y = y; /*蛇头*/
    snake->body[1].X = x - 1;
    snake->body[1].Y = y; /*蛇尾*/
    snake->size = 2;
}

void gameEnd(int snakeUid, int sockfd) {
    if (snakeUid == 0) {
        close(sockfd);
    } else {
        Snake* snake = findSnake(snakeUid);
        if (snake == NULL) {
            return;
        }
        FILE* fp = fdopen(sockfd, "w");
        if (fp == NULL) {
            perror("end error");
            return;
        }
        fprintf(fp, "蛇的长度为: %d\n", snake->size);
        fflush(fp);
        fclose(fp);
        removeSnake(snakeUid);
        close(sockfd);  //关闭与该客户端的链接
    }
}

void sendMap(int sockfd) {
    char screen[HIGH][WIDE + 1];
    int i, j;
    for (i = 0; i < HIGH; i++) {
        for (j = 0; j < WIDE + 1; j++) {
            if (j == WIDE) {
                screen[i][WIDE] = 0;
            } else {
                screen[i][j] = ' ';
            }
        }
    }
    //食物
    screen[food.Y][food.X] = '#';
    MapSnake *current, *temp;
    HASH_ITER(hh, mapSnake, current, temp) {
        Snake* snake = current->snake;
        int snakeUid = current->snakeUid;
        //蛇头
        screen[snake->body[0].Y][snake->body[0].X] = '@';
        //蛇尾
        int k;
        for (k = 1; k < snake->size; k++) {
            screen[snake->body[k].Y][snake->body[k].X] = snakeUid + 48;
        }
    }
    FILE* fp = fdopen(sockfd, "w");
    //画墙
    for (j = 0; j < WIDE; j++) {
        screen[0][j] = '_';
        screen[HIGH - 1][j] = '_';
    }
    for (i = 1; i < HIGH; i++) {
        screen[i][0] = '|';
        screen[i][WIDE - 1] = '|';
    }
    for (i = 0; i < HIGH; i++) {
        fprintf(fp, "%s\n", screen[i]);
        fflush(fp);
    }
}

void* playGame(void* arg) {
    Arguments* arguments = (Arguments*)arg;
    int snakeUid = arguments->snakeUid;
    char key = arguments->key;
    int sockfd = arguments->sockfd;
    Snake* snake = findSnake(snakeUid);

    if (snake == NULL) {
        gameEnd(0, sockfd);
        return;
    }
    if (snake->size < (HIGH - 1) * (WIDE - 1)) {
        switch (key) {
            case 'w':
                kx = 0;
                ky = -1;
                break;
            case 'a':
                kx = -1;
                ky = 0;
                break;
            case 's':
                kx = 0;
                ky = 1;
                break;
            case 'd':
                kx = 1;
                ky = 0;
                break;
            default:
                break;
        }
        int i;
        //判断是否撞墙
        if (snake->body[0].X == 0 || snake->body[0].X == WIDE - 1 ||
            snake->body[0].Y == 0 || snake->body[0].Y == HIGH - 1) {
            gameEnd(snakeUid, sockfd);
            return;
        }
        //判断是否撞到蛇身
        for (i = 1; i < snake->size; i++) {
            if (snake->body[0].X == snake->body[i].X &&
                snake->body[0].Y == snake->body[i].Y) {
                gameEnd(snakeUid, sockfd);
                return;
            }
        }
        //判断是否撞到食物
        if (snake->body[0].X == food.X && snake->body[0].Y == food.Y) {
            initFood();
            snake->size++;
        }

        for (i = snake->size - 1; i > 0; i--) {
            snake->body[i].X = snake->body[i - 1].X;
            snake->body[i].Y = snake->body[i - 1].Y;
        }
        snake->body[0].X = snake->body[0].X + kx;
        snake->body[0].Y = snake->body[0].Y + ky;
        //向客户端发送信息
        sendMap(sockfd);
    }
}

//添加一条蛇
void addSnake(int snakeUid, Snake* snake) {
    MapSnake* temp;
    HASH_FIND_INT(mapSnake, &snakeUid, temp);
    if (temp == NULL) {
        temp = (MapSnake*)malloc(sizeof(MapSnake));
        temp->snakeUid = snakeUid;
        temp->snake = snake;
        HASH_ADD_INT(mapSnake, snakeUid, temp);
    }
}

//删除一条蛇
void removeSnake(int snakeUid) {
    MapSnake* temp;
    HASH_FIND_INT(mapSnake, &snakeUid, temp);
    if (mapSnake != NULL) {
        HASH_DEL(mapSnake, temp);
        free(temp);
    }
}

//根据snakeUid查找对应蛇
Snake* findSnake(int snakeUid) {
    MapSnake* temp;
    HASH_FIND_INT(mapSnake, &snakeUid, temp);
    if (temp != NULL) {
        return temp->snake;
    }
    return NULL;
}

//清空所有蛇
void delAllSnake() {
    MapSnake *current, *temp;
    HASH_ITER(hh, mapSnake, current, temp) {
        HASH_DEL(mapSnake, current);
        free(current);
    }
}

#endif