#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO: 可以设置两个线程，分别负责读或写

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
    char data[1024] = {0};
    while (1) {
        // 发送数据
        bzero(data, sizeof(data));
        // 键盘输入待发送数据
        fgets(data, sizeof(data), stdin);//在终端中,因为无可避免要回车进行确定,因此read函数会读入换行符
        write(fd, data, strlen(data) + 1);//发送过去的数据带有换行符 \n \0

        // 接收数据
        bzero(recvBuf, sizeof(recvBuf));
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
    }

    // 关闭文件描述符
    close(fd);

    return 0;
}