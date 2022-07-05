#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    // 创建用于连接的套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(-1);
    }

    // 连接服务器
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr.s_addr);
    saddr.sin_port = htons(9999);
    int ret = connect(fd, (struct sockaddr*)&saddr, sizeof(saddr));
    if (ret == -1) {
        perror("connect");
        exit(-1);
    }

    char recvBuf[1024] = {0};
    int i = 0;
    while (1) {
        // 发送数据
        char data[128] = {0};
        sprintf(data, "i = %d\n", i++);
        write(fd, data, strlen(data));

        // sleep(1);// 放这服务端会出现read: Connection reset by peer

        // 接收数据
        int len = read(fd, recvBuf, sizeof(recvBuf));
        if (len == -1) {
            perror("read");
            exit(-1);
        } else if (len > 0) {
            printf("recv server data: %s\n", recvBuf);
        } else if (len == 0) {
            printf("server closed...\n");
            break;
        }

        usleep(1000);
    }

    // 关闭文件描述符
    close(fd);

    return 0;
}