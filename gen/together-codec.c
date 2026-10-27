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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <winsock2.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <arpa/inet.h>
#endif

#include "together-codec.h"
#include "together-error.h"

tgr_frame_p 
tgr_frame_decode(const unsigned char* bytes, 
                 size_t* size)
{
  size_t offset = 0;
  short magic = 0;
  memcpy((void*)&magic, bytes, 2);
  magic = ntohs(magic);
  offset += 2;
  if (magic != TGR_MAGIC) 
    return NULL;
  tgr_frame_p ret = tgr_frame_init();
  size_t block_bytes = 0;

  ret->magic = magic;
  // version
  memcpy((void*)&ret->version, bytes + offset, 1);
  offset += 1;
  // message_type
  memcpy((void*)&ret->message_type, bytes + offset, 1);
  offset += 1;
  // sender_host
  memcpy((void*)ret->sender_host, bytes + offset, 16);
  offset += 16;
  // sender_port
  uint16_t raw_sender_port = 0;
  memcpy(&raw_sender_port, bytes + offset, 2);
  ret->sender_port = ntohs(raw_sender_port);
  offset += 2;
  // sequence_id
  int raw_sequence_id = 0;
  memcpy(&raw_sequence_id, bytes + offset, 4);
  ret->sequence_id = ntohl(raw_sequence_id);
  offset += 4;
  // error
  int raw_error = 0;
  memcpy(&raw_error, bytes + offset, 4);
  ret->error = ntohl(raw_error);
  offset += 4;
  // payload_len
  int raw_payload_len = 0;
  memcpy(&raw_payload_len, bytes + offset, 4);
  ret->payload_len = ntohl(raw_payload_len);
  offset += 4;
  // payload
  ret->payload = (uint8_t*)malloc(ret->payload_len);  
  memcpy((void*)ret->payload, bytes + offset, ret->payload_len);
  offset += ret->payload_len;
  *size = offset;
  return ret;
}

void
tgr_frame_encode(const tgr_frame_p frame, 
                 unsigned char** bytes,
                 size_t* size)
{
  size_t offset = 0;
  size_t block_bytes = 0;
  size_t total_bytes = 0;

  tgr_frame_bytes(frame, &total_bytes);
  *bytes = (unsigned char*)malloc(total_bytes);
  // magic
  short raw_magic = htons(frame->magic);
  memcpy((*bytes) + offset, &raw_magic, 2);   
  offset += 2;
  // version
  memcpy((*bytes) + offset, &frame->version, 1);
  offset += 1;
  // message_type
  memcpy((*bytes) + offset, &frame->message_type, 1);
  offset += 1;
  // sender_host
  memcpy((*bytes) + offset, frame->sender_host, 16);
  offset += 16;
  // sender_port
  short raw_sender_port = htons(frame->sender_port);
  memcpy((*bytes) + offset, &raw_sender_port, 2);   
  offset += 2;
  // sequence_id
  int raw_sequence_id = htonl(frame->sequence_id);
  memcpy((*bytes) + offset, &raw_sequence_id, 4);  
  offset += 4; 
  // error
  int raw_error = htonl(frame->error);
  memcpy((*bytes) + offset, &raw_error, 4);  
  offset += 4; 
  // payload_len
  int raw_payload_len = htonl(frame->payload_len);
  memcpy((*bytes) + offset, &raw_payload_len, 4);  
  offset += 4; 
  // payload
  memcpy((*bytes) + offset, frame->payload, frame->payload_len);
  offset += frame->payload_len;
  *size = total_bytes;
}

void 
tgr_frame_bytes(const tgr_frame_p obj, 
                size_t* size)
{
  size_t total_bytes = 0;
  size_t block_bytes = 0;
  // magic    
  total_bytes += 2; 
  // version    
  total_bytes += 1; 
  // message_type    
  total_bytes += 1; 
  // sender_host    
  total_bytes += 16; 
  // sender_port    
  total_bytes += 2; 
  // sequence_id    
  total_bytes += 4; 
  // error    
  total_bytes += 4; 
  // payload_len    
  total_bytes += 4; 
  // payload    
  total_bytes += obj->payload_len;
  *size = total_bytes;
}
