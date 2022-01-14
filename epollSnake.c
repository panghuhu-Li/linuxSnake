#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <sys/epoll.h>
#include "socketSnake.h"

#define MAXLINE 8192
#define SERV_PORT 8000
#define OPEN_MAX 5000

void perr_exit(const char* s) {
    perror(s);
    exit(-1);
}

int setnonblocking(int sockfd) {
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);
    return 0;
}

int main(int argc, char* argv[]) {

    int listenfd, i, connfd, sockfd;
    int n, num = 0;
    struct sockaddr_in addr, cliaddr;
    struct epoll_event tep, ep[OPEN_MAX];
    ssize_t efd, res, nready;
    socklen_t clilen;
    char buf[MAXLINE];
    char uid[512];
    char key[3];

    initFood();

    if (argc < 2) {
        perror("cmd port");
        exit(1);
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, (const struct sockaddr*)&addr,
             sizeof(struct sockaddr_in)) == -1) {
        perror("cannot bind");
        exit(1);
    }

    listen(listenfd, 20);
    efd = epoll_create(OPEN_MAX);  //创建epoll模型, efd指向红黑树根节点
    if (efd == -1) {
        perr_exit("epoll_create error");
    }

    tep.events = EPOLLIN;
    tep.data.fd = listenfd;  //指定lfd的监听时间为"读"
    res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd,
                    &tep);  //将lfd及对应的结构体设置到树上,efd可找到该树
    if (res == -1) {
        perr_exit("epoll_ctl error");
    }
    while (1) {
        nready = epoll_wait(efd, ep, OPEN_MAX, -1);
        if (nready == -1) {
            perr_exit("epoll_wait error");
        }
        for (i = 0; i < nready; i++) {
            if (!(ep[i].events & EPOLLIN)) {
                continue;
            }
            /*判断是客户端发来消息还是客户端进行初次连接 */
            if (ep[i].data.fd == listenfd) {
                clilen = sizeof(cliaddr);
                connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
                printf("cfd %d---client %d\n", connfd, ++num);
                setnonblocking(connfd);
                tep.events = EPOLLIN;
                tep.data.fd = connfd;
                res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
                if (res == -1) {
                    perr_exit("epoll_ctl error");
                }
            } else {
                sockfd = ep[i].data.fd;
                n = read(sockfd, buf, MAXLINE);
                if (n == 0) {  //读到0,说明客户端关闭链接
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd,
                                    NULL);  //将该文件描述符从红黑树摘除
                    if (res == -1)
                        perr_exit("epoll_ctl error");
                    close(sockfd);  //关闭与该客户端的链接
                    printf("client[%d] closed connection\n", sockfd);

                } else if (n < 0) {  //出错
                    perror("read n < 0 error: ");
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
                    close(sockfd);

                } else {
                    //读取客户端发来的消息
                    sscanf(buf, "%s%s", uid, key);
                    int snakeUid = atoi(uid);
                    //根据snakeUid来判断当对应蛇是否存在
                    Snake* snake = findSnake(snakeUid);
                    if (snake == NULL) {
                        snake = (Snake*)malloc(sizeof(snake));
                        initSnake(snake);
                        addSnake(snakeUid, snake);
                    }
                    //传递参数
                    Arguments* arguments =
                        (Arguments*)malloc(sizeof(Arguments));
                    arguments->key = key[0];
                    arguments->snakeUid = snakeUid;
                    arguments->sockfd = sockfd;
                    playGame((void*)arguments);
                }
            }
        }
    }
    close(listenfd);
    close(efd);
}