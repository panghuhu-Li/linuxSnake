#include <curses.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
int count = 1;

void connectServer();
void sendAndGet(int signum);
int set_ticker(int n_msecs);

int sockfd;
char sendbuf[50];
char dir[1];

int main() {
    int delay = 500;
    int ndelay;

    connectServer();

    sendbuf[0] = getchar();
    getchar();
    sendbuf[1] = ' ';
    sendbuf[2] = 'w';
    system("stty -icanon");
    system("stty -echo");

    signal(SIGALRM, sendAndGet);
    set_ticker(delay);

    while (1) {
        ndelay = 0;
        sendbuf[2] = getchar();
        if (sendbuf[2] == 'q')
            break;
    }

    system("stty icanon");
    system("stty echo");

    return 0;
}

void connectServer() {
    struct sockaddr_in server_addr;
    struct hostent* host;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //客户端发出请求
    if (connect(sockfd, (struct sockaddr*)(&server_addr),
                sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "Connect failed\n");
        exit(EXIT_FAILURE);
    }
}

//设置信号发送时间
int set_ticker(int n_msecs) {
    struct itimerval new_timeset;
    long n_sec, n_usecs;

    n_sec = n_msecs / 1000;
    n_usecs = (n_msecs % 1000) * 1000L;

    new_timeset.it_interval.tv_sec = n_sec;    /* set reload  */
    new_timeset.it_interval.tv_usec = n_usecs; /* new ticker value */
    new_timeset.it_value.tv_sec = n_sec;       /* store this   */
    new_timeset.it_value.tv_usec = n_usecs;    /* and this     */

    return setitimer(ITIMER_REAL, &new_timeset, NULL);
}

void sendAndGet(int signum) {
    signal(SIGALRM, sendAndGet);

    char recvbuf[3072];

    system("clear");

    send(sockfd, sendbuf, strlen(sendbuf), 0);

    int nread = recv(sockfd, recvbuf, sizeof(recvbuf), 0);

    fputs(recvbuf, stdout);

    memset(recvbuf, 0, sizeof(recvbuf));
}