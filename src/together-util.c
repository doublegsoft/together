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
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <iphlpapi.h>
  #pragma comment(lib, "ws2_32.lib")
  #pragma comment(lib, "iphlpapi.lib")
  typedef SOCKET socket_t;
  #define CLOSE_SOCKET(s) closesocket(s)
#else
  /* macOS / Linux 严格依赖以下头文件顺序 */
  #include <sys/types.h>
  #include <sys/time.h>
  #include <sys/socket.h>
  #include <net/if.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <ifaddrs.h>
  #include <unistd.h>
  typedef int socket_t;
  #define CLOSE_SOCKET(s) close(s)

  /* 防御性兼容宏 */
  #ifndef IFF_UP
    #define IFF_UP 0x1
  #endif
  #ifndef IFF_LOOPBACK
    #define IFF_LOOPBACK 0x8
  #endif
  #ifndef IFF_RUNNING
    #define IFF_RUNNING 0x40
  #endif
#endif

#include "together-error.h"
#include "together-util.h"

/*!
** 【获取当前系统时间（秒级）】
*/
int 
tgr_sec_get(void) 
{
  return (int)time(NULL);
}

/*!
** 【获取当前系统时间（毫秒级截断序号）】
*/
int 
tgr_millis_get(void) 
{
#ifdef _WIN32
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  uint64_t total = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
  uint64_t ms = total / 10000;
  return (int)(ms & 0x7FFFFFFF);
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t ms = (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
  return (int)(ms & 0x7FFFFFFF);
#endif
}

/*!
** 【辅助函数：判断是否属于 RFC 1918 标准私网 IP】
*/
static int 
is_private_lan_ip(uint32_t ip_host_order) 
{
  uint8_t b1 = (ip_host_order >> 24) & 0xFF;
  uint8_t b2 = (ip_host_order >> 16) & 0xFF;

  // 10.x.x.x
  if (b1 == 10) return 1;

  // 172.16.x.x ~ 172.31.x.x
  if (b1 == 172 && (b2 >= 16 && b2 <= 31)) return 1;

  // 192.168.x.x
  if (b1 == 192 && b2 == 168) return 1;

  return 0;
}

/*!
** 【获取当前本机活动局域网 IPv4 地址】
*/
int 
tgr_ip_get(char* out_ip, size_t max_len) 
{
  if (!out_ip || max_len < 16) return -1;

#ifdef _WIN32
  ULONG buf_size = 15000;
  PIP_ADAPTER_ADDRESSES adapters = (PIP_ADAPTER_ADDRESSES)malloc(buf_size);
  if (!adapters) return -1;

  ULONG ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, adapters, &buf_size);
  if (ret == ERROR_BUFFER_OVERFLOW) {
    free(adapters);
    adapters = (PIP_ADAPTER_ADDRESSES)malloc(buf_size);
    if (!adapters) return -1;
    ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, adapters, &buf_size);
  }

  if (ret != NO_ERROR) {
    free(adapters);
    return -1;
  }

  int found = 0;
  PIP_ADAPTER_ADDRESSES curr = adapters;

  while (curr) {
    if (curr->IfType != IF_TYPE_SOFTWARE_LOOPBACK && curr->OperStatus == IfOperStatusUp) {
      PIP_ADAPTER_UNICAST_ADDRESS uni = curr->FirstUnicastAddress;
      while (uni) {
        struct sockaddr_in* sa = (struct sockaddr_in*)uni->Address.lpSockaddr;
        if (sa && sa->sin_family == AF_INET) {
          uint32_t ip_h = ntohl(sa->sin_addr.s_addr);
          if (is_private_lan_ip(ip_h)) {
            inet_ntop(AF_INET, &sa->sin_addr, out_ip, (socklen_t)max_len);
            found = 1;
            break;
          }
        }
        uni = uni->Next;
      }
    }
    if (found) break;
    curr = curr->Next;
  }

  free(adapters);
  return found ? 0 : -1;

#else
  struct ifaddrs* ifaddr = NULL;
  if (getifaddrs(&ifaddr) == -1) {
    return -1;
  }

  int found = 0;
  char candidate_ip[16] = {0};

  for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr) continue;

    // 只检查 IPv4
    if (ifa->ifa_addr->sa_family != AF_INET) continue;

    // 排除回环网卡，且网卡必须激活运行
    if ((ifa->ifa_flags & IFF_LOOPBACK) ||
        !(ifa->ifa_flags & IFF_UP) ||
        !(ifa->ifa_flags & IFF_RUNNING)) {
      continue;
    }

    struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
    uint32_t ip_h = ntohl(sa->sin_addr.s_addr);

    // 优先提取 192.168.x.x / 10.x.x.x / 172.16~31.x.x
    if (is_private_lan_ip(ip_h)) {
      inet_ntop(AF_INET, &sa->sin_addr, out_ip, (socklen_t)max_len);
      found = 1;
      break;
    }

    // 备选记录（防止部分非标准局域网段）
    if (candidate_ip[0] == '\0') {
      inet_ntop(AF_INET, &sa->sin_addr, candidate_ip, sizeof(candidate_ip));
    }
  }

  freeifaddrs(ifaddr);

  if (found) {
    return 0;
  } else if (candidate_ip[0] != '\0') {
    strncpy(out_ip, candidate_ip, max_len - 1);
    out_ip[max_len - 1] = '\0';
    return 0;
  }

  return -1;
#endif
}

int
tgr_shell_exec(const char* shell, size_t size)
{
  char command[4096 * 16] = {'\0'};
  sprintf(command, "%.*s  > output.log 2>&1", (int)size, shell);
  FILE* fp = popen(command, "r");
  if (!fp) {
    perror("popen failed");
    return TGR_ERR_OS_PROC_START_FAILED;
  }

  pclose(fp);
  return TGR_OK;
}

int 
tgr_file_read(const char* filepath, char** bytes, size_t* size)
{
  if (!filepath || !bytes || !size) {
    return -1;
  }

  // Initialize output pointers
  *bytes = NULL;
  *size = 0;

  // 2. Open file in binary read mode
  FILE* fp = fopen(filepath, "rb");
  if (!fp) {
    perror("tgr_file_read: fopen failed");
    return -1;
  }

  // 3. Determine file size
  if (fseek(fp, 0, SEEK_END) != 0) {
    perror("tgr_file_read: fseek to end failed");
    fclose(fp);
    return -1;
  }

  long file_size = ftell(fp);
  if (file_size < 0) {
    perror("tgr_file_read: ftell failed");
    fclose(fp);
    return -1;
  }

  rewind(fp);

  // 4. Handle empty file case
  if (file_size == 0) {
    char* empty_buf = (char*)malloc(1);
    if (!empty_buf) {
      fclose(fp);
      return -1;
    }
    empty_buf[0] = '\0';
    fclose(fp);
    *bytes = empty_buf;
    *size = 0;
    return 0;
  }

  // 5. Allocate memory (+1 byte for null-terminator safety)
  char* buffer = (char*)malloc((size_t)file_size + 1);
  if (!buffer) {
    perror("tgr_file_read: malloc failed");
    fclose(fp);
    return -1;
  }

  // 6. Read the whole file into the buffer
  size_t bytes_read = fread(buffer, 1, (size_t)file_size, fp);
  fclose(fp);

  if (bytes_read != (size_t)file_size) {
    fprintf(stderr, "tgr_file_read: incomplete read (expected %ld, got %zu)\n", 
            file_size, bytes_read);
    free(buffer);
    return -1;
  }

  // Null-terminate buffer for safe string handling
  buffer[file_size] = '\0';

  // 7. Assign outputs
  *bytes = buffer;
  *size = (size_t)file_size;

  return TGR_OK;
}