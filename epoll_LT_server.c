/*
    运用epoll函数，IO多路复用
    水平触发
*/

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

int main() {
    // 创建监听socket
    int lfd = socket(PF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket");
        exit(-1);
    }
    struct sockaddr_in saddr;
    saddr.sin_port = htons(9999);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;

    // 端口复用，防止上一次的连接不上
    int optval = 1;// 1复用、0不复用
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // 绑定
    int ret = bind(lfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if (ret == -1) {
        perror("bind");
        exit(-1);
    }

    // 监听
    ret = listen(lfd, 8);
    if (ret == -1) {
        perror("listen");
        exit(-1);
    }

    // 创建一个epoll示例
    int epfd = epoll_create(1);

    // 将监听的文件描述符相关信息加入epoll实例
    struct epoll_event epev;
    epev.events = EPOLLIN;  // 如果检测的不止读事件，需要对每个检测类型分别处理
    epev.data.fd = lfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &epev);

    // 最大客户端数目
    const int maxClientNum = 1024;
    struct epoll_event epevs[maxClientNum];

    while (1) {
        // 调用epoll函数
        ret = epoll_wait(epfd, epevs, maxClientNum, -1);  // -1是阻塞
        if (ret == -1) {
            perror("poll_wait");
            exit(-1);
        } else if (ret == 0) {
            // 没检测到
            continue;
        }
        // 现在ret > 0,检测到有ret个fd对应的缓冲区有数据到达
        printf("ret = %d\n", ret);

        for (int i = 0; i < ret; ++i) {
            if (epevs[i].data.fd == lfd) {
                // 新连接
                struct sockaddr_in clientaddr;
                socklen_t size = sizeof(clientaddr);
                // 接受新连接
                int cfd = accept(lfd, (struct sockaddr*)&clientaddr, &size);
                if (cfd == -1) {
                    perror("accept");
                    exit(-1);
                }

                // 将新连接文件描述符加入epoll实例
                epev.events = EPOLLIN;
                epev.data.fd = cfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &epev);

                // 获取新客户端信息
                char clientIp[16] = {0};
                inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientIp, sizeof(clientIp));
                unsigned short clientPort = ntohs(clientaddr.sin_port);
                printf("client ip is %s, port is %d\n", clientIp, clientPort);

            } else {
                // 不是新连接,对应客户端发来数据
                int fd = epevs[i].data.fd;
                char recvBuf[5] = {0}; //减小缓冲区，演示水平触发
                // sizeof(recvBuf) - 1, 这样就保留缓冲区最后位置的结束符
                int len = read(fd, recvBuf, sizeof(recvBuf) - 1);//第三个参数是读取的字节数
                if (len == -1) {
                    perror("read");
                    exit(-1);
                } else if (len > 0) {
                    printf("recv client data: %s\n", recvBuf);
                    // 给客户端发送数据
                    int ret1 = write(fd, recvBuf, strlen(recvBuf) + 1);
                    if (ret1 == -1) {
                        perror("write");
                        exit(-1);
                    }
                } else if (len == 0) {
                    printf("client closed...\n");
                    // 从epoll实例中移除该文件描述符
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                }
            }
        }
    }
    close(lfd);
    close(epfd);
    return 0;
}