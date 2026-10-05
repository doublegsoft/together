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
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include "together-error.h"
#include "together-codec.h"
#include "together-poco.h"
#include "together-util.h"
#include "together-net.h"

void* 
server_worker(void* arg) 
{
  tgr_net_serve(TGR_SERVER_PORT);
  return NULL;
}

void* 
client_worker(void* arg) 
{
  const char* msg = (const char*) arg;
  int seq = tgr_millis_get();

  tgr_frame_p frame = tgr_frame_init();
  frame->magic = TGR_MAGIC;
  frame->version = TGR_VERSION;
  frame->message_type = TGR_MSG_REQ;
  frame->sequence_id = seq;
  frame->error = TGR_OK;
  strcpy(frame->sender_host, "192.168.0.210");
  frame->sender_port = TGR_SERVER_PORT;
  frame->payload_len = strlen(msg);
  frame->payload = strdup(msg);

  tgr_frame_p resp = NULL;
  tgr_net_send("127.0.0.1", TGR_SERVER_PORT, frame, &resp);
  tgr_frame_free(frame);
  if (resp != NULL)
  {
    printf("%.*s", (int)resp->payload_len, resp->payload);
    tgr_frame_free(resp);
  }
  return NULL;
}

TEST(e2e, busy) {
  
  pthread_t server_tid;
  pthread_t client_first_tid;
  pthread_t client_second_tid;

  pthread_create(&server_tid, NULL, server_worker, NULL);
  usleep(1000000 * 1);

  pthread_create(&client_first_tid, NULL, client_worker, (void*) "sleep 8");
  usleep(1000000 * 1);
  pthread_create(&client_second_tid, NULL, client_worker, (void*) "ls /");
  usleep(1000000 * 8);
}
