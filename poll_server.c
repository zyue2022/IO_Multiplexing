/*
    运用poll函数，IO多路复用
*/

#include <arpa/inet.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    // 初始化检测的结构体数组
    const int maxClientNum = 1024;
    struct pollfd fds[maxClientNum];
    for (int i = 0; i < maxClientNum; ++i) {
        fds[i].fd = -1;
        // 检测读事件
        fds[i].events = POLLIN;
    }
    fds[0].fd = lfd;

    // 最大文件描述符索引
    int nfds = 0;

    while (1) {
        // 调用poll函数
        ret = poll(fds, nfds + 1, -1);  // -1是阻塞

        if (ret == -1) {
            perror("poll");
            exit(-1);
        } else if (ret == 0) {
            // 没检测到
            continue;
        } else if (ret > 0) {
            // 说明检测到有fds中某fd对应的缓冲区有数据到达

            // 如果是新连接
            if (fds[0].revents & POLLIN) {
                struct sockaddr_in clientaddr;
                socklen_t size = sizeof(clientaddr);
                // 接受新连接
                int cfd = accept(lfd, (struct sockaddr*)&clientaddr, &size);
                if (cfd == -1) {
                    perror("accept");
                    exit(-1);
                }

                // 获取新客户端信息
                char clientIp[16];
                inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientIp,
                          sizeof(clientIp));
                unsigned short clientPort = ntohs(clientaddr.sin_port);
                printf("client ip is %s, port is %d\n", clientIp, clientPort);

                // 将新连接文件描述符加入结构体数组
                for (int i = 1; i < maxClientNum; ++i) {
                    if (fds[i].fd == -1) {
                        fds[i].fd = cfd;
                        fds[i].events = POLLIN;
                        break;
                    }
                }

                // 更新最大文件描述符索引
                nfds = nfds > cfd ? nfds : cfd;
            }

            // 不是新连接
            for (int i = 1; i <= nfds; ++i) {
                if (fds[i].revents & POLLIN) {
                    // 对应客户端发来数据
                    int fd = fds[i].fd;
                    char recvBuf[1024] = {0};
                    int len = read(fd, recvBuf, sizeof(recvBuf));
                    if (len == -1) {
                        perror("read");
                        exit(-1);
                    } else if (len > 0) {
                        printf("recv client data: %s\n", recvBuf);
                        // 给客户端发送数据
                        char* data = recvBuf;
                        int ret = write(fd, data, strlen(data));
                        if (ret == -1) {
                            perror("write");
                            exit(-1);
                        }
                    } else if (len == 0) {
                        printf("client closed...\n");
                        close(fd);
                        // 从集合中移除该文件描述符
                        fds[i].fd = -1;
                    }
                }
            }
        }
    }
    close(lfd);
    return 0;
}