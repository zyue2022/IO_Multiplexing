/*
    不使用多进程或多线程进行多客户端连接；
    而是运用select函数，IO多路复用
*/

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
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

    // 创建一个fd_set集合，存放需检测的文件描述符
    fd_set rdset, tmp_rdset;
    FD_ZERO(&rdset);
    FD_SET(lfd, &rdset);

    // 找到最大的文件描述符
    int maxfd = lfd;

    while (1) {
        tmp_rdset = rdset;

        // 调用select，让内核检测那些文件描述符有数据
        // 传入内核的是tmp_rdset，就不用担心原rdset在未处理前被修改
        ret = select(maxfd + 1, &tmp_rdset, NULL, NULL,
                     NULL);  // 传入NULL，检测是阻塞的

        if (ret == -1) {
            perror("select");
            exit(-1);
        } else if (ret == 0) {
            continue;
        } else if (ret > 0) {
            // 说明检测到有fd对应的缓冲区有数据到达

            // 如果是新连接
            if (FD_ISSET(lfd, &tmp_rdset)) {
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

                // 将新连接文件描述符加入集合
                FD_SET(cfd, &rdset);
                // 更新maxfd
                maxfd = maxfd > cfd ? maxfd : cfd;
            }

            // 不是新连接
            for (int i = lfd + 1; i <= maxfd; ++i) {
                if (FD_ISSET(i, &tmp_rdset)) {
                    // 对应客户端发来数据
                    char recvBuf[1024] = {0};
                    int len = read(i, recvBuf, sizeof(recvBuf));
                    if (len == -1) {
                        perror("read");
                        exit(-1);
                    } else if (len > 0) {
                        printf("recv client data: %s\n", recvBuf);
                        // 给客户端发送数据
                        char* data = recvBuf;
                        int ret1 = write(i, data, strlen(data));
                        if (ret1 == -1) {
                            perror("write");
                            exit(-1);
                        }
                    } else if (len == 0) {
                        printf("client closed...\n");
                        close(i);
                        // 从集合中移除该文件描述符
                        FD_CLR(i, &rdset);
                    }
                }
            }
        }
    }
    close(lfd);
    return 0;
}