/*
**    ▄                           ▄    █
**  ▄▄█▄▄   ▄▄▄    ▄▄▄▄   ▄▄▄   ▄▄█▄▄  █ ▄▄    ▄▄▄    ▄ ▄▄
**    █    █▀ ▀█  █▀ ▀█  █▀  █    █    █▀  █  █▀  █   █▀  ▀
**    █    █   █  █   █  █▀▀▀▀    █    █   █  █▀▀▀▀   █
**    ▀▄▄  ▀█▄█▀  ▀█▄▀█  ▀█▄▄▀    ▀▄▄  █   █  ▀█▄▄▀   █
**                 ▄  █
**                  ▀▀
** Copyright 2026 doublegsoft.dev
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the “Software”), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")

  typedef SOCKET socket_t;
  #define IS_INVALID_SOCKET(s) ((s) == INVALID_SOCKET)
  #define CLOSE_SOCKET(s) closesocket(s)
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>

  typedef int socket_t;
  #define IS_INVALID_SOCKET(s) ((s) < 0)
  #define CLOSE_SOCKET(s) close(s)
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR (-1)
#endif

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

void net_init(void) {
#ifdef _WIN32
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    fprintf(stderr, "WSAStartup failed\n");
    exit(1);
  }
#endif
}

void net_cleanup(void) {
#ifdef _WIN32
  WSACleanup();
#endif
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  net_init();

  socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
  if (IS_INVALID_SOCKET(sock)) {
    perror("socket creation failed");
    net_cleanup();
    return 1;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);

  if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
    fprintf(stderr, "Invalid server IP address\n");
    CLOSE_SOCKET(sock);
    net_cleanup();
    return 1;
  }

  // 3. 连接服务器
  printf("Connecting to %s:%d...\n", SERVER_IP, SERVER_PORT);
  if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
    perror("connect failed");
    CLOSE_SOCKET(sock);
    net_cleanup();
    return 1;
  }

  printf("Connected! Type messages to send. Type 'exit' or 'quit' to close.\n\n");

  // 4. 发送与接收循环
  char send_buf[BUFFER_SIZE];
  char recv_buf[BUFFER_SIZE];

  while (1) {
    printf("> ");
    fflush(stdout);

    if (fgets(send_buf, sizeof(send_buf), stdin) == NULL) {
      break;
    }

    // 检查退出指令
    if (strcmp(send_buf, "exit\n") == 0 || strcmp(send_buf, "quit\n") == 0) {
      printf("Closing connection...\n");
      break;
    }

    size_t send_len = strlen(send_buf);
    if (send_len <= 1) continue; // 忽略仅按回车

    // 发送消息
    int bytes_sent = send(sock, send_buf, (int)send_len, 0);
    if (bytes_sent == SOCKET_ERROR) {
      perror("send failed");
      break;
    }

    // 接收服务端 Echo 回复
    int bytes_recv = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
    if (bytes_recv > 0) {
      recv_buf[bytes_recv] = '\0';
      printf("Server Echo: %s", recv_buf);
    } else if (bytes_recv == 0) {
      printf("Server closed connection.\n");
      break;
    } else {
      perror("recv failed");
      break;
    }
  }

  // 5. 清理退出
  CLOSE_SOCKET(sock);
  net_cleanup();
  return 0;
}