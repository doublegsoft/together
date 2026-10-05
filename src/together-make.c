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
#include <stdint.h>
#include <sys/types.h>

#include "together-error.h"
#include "together-poco.h"
#include "together-util.h"

#define MAX_PAYLOAD_LIMIT 1024 * 1024

static int 
tgr_frame_save(tgr_frame_p frame, const char* filepath) {
  if (!frame || !filepath) return -1;

  FILE* fp = fopen(filepath, "wb");
  if (!fp) {
    perror("fopen write failed");
    return -1;
  }

  // 1. 字段转换为网络大端字节序 (Big-Endian)
  uint16_t n_magic = htons((uint16_t)frame->magic);
  uint8_t  n_version = (uint8_t)frame->version;
  uint8_t  n_msg_type = (uint8_t)frame->message_type;
  uint32_t n_seq_id = htonl((uint32_t)frame->sequence_id);

  char n_host[16];
  memset(n_host, 0, sizeof(n_host));
  strncpy(n_host, frame->sender_host, sizeof(n_host) - 1);

  uint16_t n_port = htons((uint16_t)frame->sender_port);
  uint32_t n_payload_len = htonl((uint32_t)frame->payload_len);

  // 2. 依次写入 32 字节头部
  if (fwrite(&n_magic, sizeof(uint16_t), 1, fp) != 1 ||
      fwrite(&n_version, sizeof(uint8_t), 1, fp) != 1 ||
      fwrite(&n_msg_type, sizeof(uint8_t), 1, fp) != 1 ||
      fwrite(n_host, 1, 16, fp) != 16 ||
      fwrite(&n_port, sizeof(uint16_t), 1, fp) != 1 ||
      fwrite(&n_seq_id, sizeof(uint32_t), 1, fp) != 1 ||
      fwrite(&n_payload_len, sizeof(uint32_t), 1, fp) != 1) {
    fclose(fp);
    return -1;
  }

  // 3. 写入后续载荷 (若存在)
  if (frame->payload_len > 0 && frame->payload) {
    if (fwrite(frame->payload, 1, frame->payload_len, fp) != (size_t)frame->payload_len) {
      fclose(fp);
      return -1;
    }
  }

  fclose(fp);
  return 0;
}

static tgr_frame_p 
tgr_frame_load(const char* filepath) {
  if (!filepath) return NULL;

  FILE* fp = fopen(filepath, "rb");
  if (!fp) {
    perror("fopen read failed");
    return NULL;
  }

  // 初始化目标对象
  tgr_frame_p frame = tgr_frame_init();
  if (!frame) {
    fclose(fp);
    return NULL;
  }

  uint16_t n_magic;
  uint8_t  n_version;
  uint8_t  n_msg_type;
  uint32_t n_seq_id;
  uint16_t n_cmd_id;
  char n_host[16];
  uint16_t n_port;
  uint32_t n_payload_len;

  // 1. 读取固定 32 字节头部
  if (fread(&n_magic, sizeof(uint16_t), 1, fp) != 1 ||
      fread(&n_version, sizeof(uint8_t), 1, fp) != 1 ||
      fread(&n_msg_type, sizeof(uint8_t), 1, fp) != 1 ||
      fread(n_host, 1, 16, fp) != 16 ||
      fread(&n_port, sizeof(uint16_t), 1, fp) != 1 ||
      fread(&n_seq_id, sizeof(uint32_t), 1, fp) != 1 ||
      fread(&n_payload_len, sizeof(uint32_t), 1, fp) != 1) {
    fclose(fp);
    tgr_frame_free(frame);
    return NULL;
  }

  // 2. 转换为主机字节序并赋值
  tgr_frame_set_magic(frame, (short)ntohs(n_magic));
  tgr_frame_set_version(frame, (char)n_version);
  tgr_frame_set_message_type(frame, (char)n_msg_type);
  tgr_frame_set_sequence_id(frame, (int)ntohl(n_seq_id));
  tgr_frame_set_sender_host(frame, n_host);
  tgr_frame_set_sender_port(frame, (short)ntohs(n_port));

  int payload_len = (int)ntohl(n_payload_len);

  // 安全检查
  if (payload_len < 0 || payload_len > MAX_PAYLOAD_LIMIT) {
    fclose(fp);
    tgr_frame_free(frame);
    return NULL;
  }

  tgr_frame_set_payload_len(frame, payload_len);

  // 3. 读取 Payload 业务载荷
  if (payload_len > 0) {
    char* buf = (char*)malloc(payload_len);
    if (!buf) {
      fclose(fp);
      tgr_frame_free(frame);
      return NULL;
    }

    if (fread(buf, 1, payload_len, fp) != (size_t)payload_len) {
      free(buf);
      fclose(fp);
      tgr_frame_free(frame);
      return NULL;
    }

    tgr_frame_set_payload(frame, buf);
    free(buf); // tgr_frame_set_payload 内部已深拷贝，此处释放临时 buffer
  }

  fclose(fp);
  return frame;
}


int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  const char* filename = "frame.bin";
  char test_payload[] = "goo-mail-latest -s imap.qq.com -p 993 -u 598807227@qq.com";
  int payload_length = (int)strlen(test_payload);

  // 1. 初始化并构建帧对象
  char ip[16];
  tgr_ip_get(ip, 16);
  tgr_frame_p frame = tgr_frame_init();
  tgr_frame_set_magic(frame, TGR_MAGIC);
  tgr_frame_set_version(frame, TGR_VERSION);
  tgr_frame_set_message_type(frame, TGR_MSG_REQ);
  tgr_frame_set_sequence_id(frame, tgr_millis_get());
  tgr_frame_set_sender_host(frame, ip);
  tgr_frame_set_sender_port(frame, TGR_SERVER_PORT);
  tgr_frame_set_payload_len(frame, payload_length);
  tgr_frame_set_payload(frame, test_payload);

  // 2. 保存为二进制文件
  printf("Saving frame to '%s'...\n", filename);
  if (tgr_frame_save(frame, filename) == 0) {
    printf("Saved successfully! Total size: %d bytes (Header: 32B + Payload: %dB)\n\n",
           32 + payload_length, payload_length);
  }
  tgr_frame_free(frame);

  // 3. 读取该二进制文件并验证内容
  printf("Reading back from '%s' to verify...\n", filename);
  tgr_frame_p loaded_frame = tgr_frame_load(filename);
  if (loaded_frame) {
    printf("Magic:        0x%04X\n", (unsigned short)loaded_frame->magic);
    printf("Version:      %d\n", loaded_frame->version);
    printf("Msg Type:     0x%02X\n", loaded_frame->message_type);
    printf("Sequence ID:  %d\n", loaded_frame->sequence_id);
    printf("Sender Host:  %s\n", loaded_frame->sender_host);
    printf("Sender Port:  %d\n", loaded_frame->sender_port);
    printf("Payload Len:  %d\n", loaded_frame->payload_len);
    printf("Payload Data: %.*s\n", loaded_frame->payload_len, loaded_frame->payload);

    tgr_frame_free(loaded_frame);
  }
  return 0;
}