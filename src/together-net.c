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
#include <stddef.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#include "together-net.h"
#include "together-error.h"
#include "together-poco.h"
#include "together-codec.h"
#include "together-util.h"

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")

  typedef SOCKET socket_t;
  #define poll WSAPoll
  #define IS_INVALID_SOCKET(s) ((s) == INVALID_SOCKET)
  #define CLOSE_SOCKET(s) closesocket(s)
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <poll.h>
  #include <errno.h>

  typedef int socket_t;
  #define IS_INVALID_SOCKET(s) ((s) < 0)
  #define CLOSE_SOCKET(s) close(s)
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR (-1)
#endif

// 最大并发执行的 Shell 任务数（设为 1 表示全局单任务互斥，多出的请求立即回复 BUSY）
#define MAX_CONCURRENT_TASKS          1
#define CLIENT_RECV_TIMEOUT_SEC       (60 * 5)
#define MAX_CLIENTS                   128
#define BUFFER_SIZE                   4096

// 原子计数器：记录当前正在执行 Shell 的工作线程数
static atomic_int g_active_tasks = 0;

// 派发给独立工作线程的任务上下文
typedef struct {
  socket_t client_fd;
  int seq;
  size_t cmd_len;
  char cmd[2048];
} tgr_task_t;

static void 
tgr_net_init(void) {
#ifdef _WIN32
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    fprintf(stderr, "WSAStartup failed\n");
    exit(1);
  }
#endif
}

static void 
tgr_net_cleanup(void) {
#ifdef _WIN32
  WSACleanup();
#endif
}

static int 
tgr_set_nonblocking(socket_t fd) {
#ifdef _WIN32
  u_long mode = 1;
  return ioctlsocket(fd, FIONBIO, &mode);
#else
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

/*!
** @brief 获取当前本地时间的格式化字符串（精确到毫秒）
**
** @param[out] buf  输出缓冲区
** @param[in]  size 缓冲区长度（建议至少 24 字节）
** @return 成功返回 buf 指针，失败返回 NULL
*/
static const char*
tgr_time_now(char* buf, size_t size) {
  if (!buf || size < 24) {
    return NULL;
  }

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  struct tm tm_info;
  localtime_r(&ts.tv_sec, &tm_info);

  char base_time[20];
  strftime(base_time, sizeof(base_time), "%Y-%m-%d %H:%M:%S", &tm_info);
  snprintf(buf, size, "%s.%03ld", base_time, ts.tv_nsec / 1000000);

  return buf;
}

/*!
** @brief 直接打印当前时间到控制台并换行
*/
static void
tgr_time_print(void) {
  char buf[32];
  if (tgr_time_now(buf, sizeof(buf))) {
    printf("%s\n", buf);
  }
}

/*!
** @brief 直接通过指定的客户端套接字回复数据帧
**
** @param[in] client_fd 目标客户端套接字
** @param[in] seq       原请求数据帧序号
** @param[in] error     返回的状态/错误码
*/
static int
tgr_net_reply(socket_t client_fd, int seq, int error) {
  char* bytes = NULL;
  size_t bytes_size = 0;

  tgr_frame_p frame = tgr_frame_init();
  frame->version = TGR_VERSION;
  frame->message_type = TGR_MSG_RESP;
  frame->sequence_id = seq;
  frame->error = error;

  if (error == TGR_OK) {
    if (tgr_file_read("output.log", &bytes, &bytes_size) == TGR_OK) {
      frame->payload_len = bytes_size;
      frame->payload = bytes;
    }
  } else {
    const char* msg = tgr_error_str(error);
    frame->payload_len = strlen(msg);
    frame->payload = strdup(msg);
  }

  unsigned char* send_bytes = NULL;
  size_t send_size = 0;
  tgr_frame_encode(frame, &send_bytes, &send_size);

  if (send_bytes && send_size > 0) {
    send(client_fd, (const char*)send_bytes, (int)send_size, 0);
    free(send_bytes); 
  }

  tgr_frame_free(frame);
  return TGR_OK;
}

/*!
** @brief 独立任务执行工作线程
*/
static void*
tgr_task_worker(void* arg) 
{
  tgr_task_t* task = (tgr_task_t*)arg;
  printf("[Worker %lu] Started executing: %s\n", (unsigned long)pthread_self(), task->cmd);

  int error = tgr_shell_exec(task->cmd, task->cmd_len);
  printf("[Worker %lu] Finished command, exit code: %d\n", (unsigned long)pthread_self(), error);
  tgr_net_reply(task->client_fd, task->seq, error);
  CLOSE_SOCKET(task->client_fd);
  atomic_fetch_sub(&g_active_tasks, 1);
  free(task);
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                               HEADER FUNCTIONS                             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
int
tgr_net_serve(int port) {
  tgr_net_init();

  socket_t listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (IS_INVALID_SOCKET(listen_fd)) {
    perror("socket creation failed");
    tgr_net_cleanup();
    return TGR_ERR_NET_SERVER_CREATE;
  }

  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
  tgr_set_nonblocking(listen_fd);

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
    perror("bind failed");
    CLOSE_SOCKET(listen_fd);
    tgr_net_cleanup();
    return TGR_ERR_NET_SERVER_BIND;
  }

  listen(listen_fd, SOMAXCONN);
  printf("Server listening on port %d (Max concurrent tasks: %d)...\n", 
         port, MAX_CONCURRENT_TASKS);

  struct pollfd fds[MAX_CLIENTS + 1];
  memset(fds, 0, sizeof(fds));

  fds[0].fd = listen_fd;
  fds[0].events = POLLIN;

  for (int i = 1; i <= MAX_CLIENTS; i++) {
    fds[i].fd = INVALID_SOCKET;
  }

  int active_fds = 1;

  while (1) {
    int activity = poll(fds, active_fds, -1);
    if (activity < 0) {
      perror("poll error");
      break;
    }

    // 1. 新连接到达
    if (fds[0].revents & POLLIN) {
      struct sockaddr_in client_addr;
      socklen_t client_len = sizeof(client_addr);

      socket_t client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
      if (!IS_INVALID_SOCKET(client_fd)) {
        tgr_set_nonblocking(client_fd);

        int added = 0;
        for (int i = 1; i <= MAX_CLIENTS; i++) {
          if (fds[i].fd == INVALID_SOCKET) {
            fds[i].fd = client_fd;
            fds[i].events = POLLIN;
            if (i >= active_fds) {
              active_fds = i + 1;
            }
            added = 1;
            break;
          }
        }

        if (!added) {
          printf("Server full, dropping client\n");
          tgr_net_reply(client_fd, 0, TGR_ERR_NET_SERVER_CONN_LIMIT);
          CLOSE_SOCKET(client_fd);
        }
      }
    }

    // 2. 客户端数据到达
    for (int i = 1; i < active_fds; i++) {
      if (fds[i].fd == INVALID_SOCKET) continue;

      if (fds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
        unsigned char buffer[BUFFER_SIZE];
        int bytes = recv(fds[i].fd, (char*)buffer, sizeof(buffer), 0);

        if (bytes > 0) {
          size_t available_bytes = (size_t)bytes;
          size_t offset = 0;

          while (offset < available_bytes) {
            size_t remaining = available_bytes - offset;
            size_t frame_bytes_consumed = remaining;

            tgr_frame_p req = tgr_frame_decode(buffer + offset, &frame_bytes_consumed);
            if (!req) break;

            if (req->magic == TGR_MAGIC && req->message_type == TGR_MSG_REQ) {
              
              // =================================================================
              // 【核心方案】：原子检查并发容量（Fast-Fail 快速失败判断）
              // =================================================================
              if (atomic_load(&g_active_tasks) >= MAX_CONCURRENT_TASKS) {
                // 超载/正在执行：主线程纳秒级响应，直接返回 SERVER_BUSY！
                tgr_time_print();
                printf("[Main] Server BUSY (active: %d >= limit: %d), rejecting seq: %d\n", 
                       atomic_load(&g_active_tasks), MAX_CONCURRENT_TASKS, req->sequence_id);

                tgr_net_reply(fds[i].fd, req->sequence_id, TGR_ERR_NET_SERVER_BUSY);
              } else {
                // 未超载：自增并发计数
                atomic_fetch_add(&g_active_tasks, 1);

                // 创建独立任务上下文
                tgr_task_t* task = (tgr_task_t*)malloc(sizeof(tgr_task_t));
                task->client_fd = fds[i].fd;
                task->seq = req->sequence_id;
                task->cmd_len = req->payload_len < sizeof(task->cmd) - 1 ? 
                                req->payload_len : sizeof(task->cmd) - 1;
                memcpy(task->cmd, req->payload, task->cmd_len);
                task->cmd[task->cmd_len] = '\0';

                // 启动工作线程并发处理
                pthread_t tid;
                if (pthread_create(&tid, NULL, tgr_task_worker, task) == 0) {
                  pthread_detach(tid);
                  // 套接字所有权移交给工作线程，主线程从 poll 摘除
                  fds[i].fd = INVALID_SOCKET;
                } else {
                  atomic_fetch_sub(&g_active_tasks, 1);
                  free(task);
                  tgr_net_reply(fds[i].fd, req->sequence_id, TGR_ERR_OS_THREAD_CREATE_FAILED);
                }
              }

            } else {
              tgr_net_reply(fds[i].fd, req->sequence_id, TGR_ERR_PROTO_HDR_MAGIC);
            }

            offset += frame_bytes_consumed;
            tgr_frame_free(req);
          }
        } else {
          CLOSE_SOCKET(fds[i].fd);
          fds[i].fd = INVALID_SOCKET;
        }
      }
    }
  }

  // 清理退出
  for (int i = 0; i < active_fds; i++) {
    if (!IS_INVALID_SOCKET(fds[i].fd)) {
      CLOSE_SOCKET(fds[i].fd);
    }
  }

  tgr_net_cleanup();
  return TGR_OK;
}

int 
tgr_net_send(const char* addr, 
             int port, 
             tgr_frame_p frame, 
             tgr_frame_p* resp) 
{
  // 1. 入参校验
  if (!addr || port <= 0 || port > 65535 || !frame) {
    return TGR_ERR_PARAM_INVALID;
  }
  if (resp != NULL) {
    *resp = NULL; // 初始化输出指针
  }

  // 2. 序列化请求帧
  unsigned char* send_bytes = NULL;
  size_t send_size = 0;
  tgr_frame_encode(frame, &send_bytes, &send_size);

  if (!send_bytes || send_size == 0) {
    return TGR_ERR_MEM_ALLOC_FAILED;
  }

  // 3. 创建套接字
  socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
  if (IS_INVALID_SOCKET(sock)) {
    perror("tgr_net_send: socket creation failed");
    free(send_bytes);
    return TGR_ERR_NET_SERVER_CREATE;
  }

  // 4. 解析目标地址
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
    fprintf(stderr, "tgr_net_send: invalid server IP address: %s\n", addr);
    free(send_bytes);
    CLOSE_SOCKET(sock);
    return TGR_ERR_PARAM_INVALID;
  }

  // 5. 连接服务器
  printf("Connecting to %s:%d...\n", addr, port);
  if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
    perror("tgr_net_send: connect failed");
    free(send_bytes);
    CLOSE_SOCKET(sock);
    return TGR_ERR_NET_CLIENT_CONNECT;
  }

  // 6. 发送请求字节流
  size_t total_sent = 0;
  int ret_code = TGR_OK;

  while (total_sent < send_size) {
    int bytes_sent = send(sock, 
                          (const char*)(send_bytes + total_sent), 
                          (int)(send_size - total_sent), 
                          0);
    if (bytes_sent == SOCKET_ERROR) {
      perror("tgr_net_send: send frame failed");
      ret_code = TGR_ERR_NET_STREAM_SEND;
      break;
    }
    total_sent += (size_t)bytes_sent;
  }
  
  // 发送完毕，释放发送缓冲区
  free(send_bytes);

  // 7. 如果发送失败，或调用方不需要响应，直接退出
  if (ret_code != TGR_OK || resp == NULL) {
    CLOSE_SOCKET(sock);
    return ret_code;
  }

  // =========================================================================
  // 8. 同步等待并接收服务端的 Response 回复
  // =========================================================================
  // 设置读取超时时间，防止服务端无响应导致永久挂起
#ifdef _WIN32
  DWORD timeout_ms = CLIENT_RECV_TIMEOUT_SEC * 1000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
#else
  struct timeval tv;
  tv.tv_sec = CLIENT_RECV_TIMEOUT_SEC;
  tv.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif

  unsigned char recv_buf[BUFFER_SIZE];
  size_t total_recv = 0;
  int response_decoded = 0;

  while (total_recv < sizeof(recv_buf)) {
    int bytes_recv = recv(sock, 
                          (char*)(recv_buf + total_recv), 
                          (int)(sizeof(recv_buf) - total_recv), 
                          0);
    if (bytes_recv > 0) {
      total_recv += (size_t)bytes_recv;

      // 尝试解码收到的字节流
      size_t consumed = total_recv;
      tgr_frame_p res = tgr_frame_decode(recv_buf, &consumed);
      if (res != NULL) {
        *resp = res;
        response_decoded = 1;
        break; // 成功解析出完整响应帧，结束接收
      }
    } else if (bytes_recv == 0) {
      // 对端关闭了连接
      break;
    } else {
      // 错误或超时检查
#ifdef _WIN32
      if (WSAGetLastError() == WSAETIMEDOUT) {
        ret_code = TGR_ERR_TIME_TIMEOUT;
      }
#else
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        ret_code = TGR_ERR_TIME_TIMEOUT;
      }
#endif
      else {
        ret_code = TGR_ERR_NET_STREAM_RECV;
      }
      break;
    }
  }

  if (ret_code == TGR_OK && !response_decoded) {
    ret_code = TGR_ERR_PROTO_FRAME_CORRUPTED;
  }

  // 9. 关闭套接字
  CLOSE_SOCKET(sock);
  return ret_code;
}