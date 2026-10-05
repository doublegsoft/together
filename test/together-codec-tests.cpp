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

#include "together-error.h"
#include "together-codec.h"
#include "together-poco.h"
#include "together-util.h"

TEST(codec, encode_decode_request) {
  unsigned char* bytes = NULL;
  const char* msg = "tail /log/test.log";
  int seq = tgr_millis_get();
  size_t frame_size = 0;
  size_t parsed_size = 0;

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

  tgr_frame_encode(frame, &bytes, &frame_size);
  tgr_frame_free(frame);

  frame = NULL;
  frame = tgr_frame_decode(bytes, &parsed_size);

  ASSERT_EQ(frame_size, parsed_size);
  ASSERT_EQ(TGR_MAGIC, frame->magic);
  ASSERT_EQ(TGR_VERSION, frame->version);
  ASSERT_EQ(TGR_MSG_REQ, frame->message_type);
  ASSERT_EQ(TGR_OK, frame->error);
  ASSERT_EQ(seq, frame->sequence_id);
  ASSERT_STREQ("192.168.0.210", frame->sender_host);
  ASSERT_EQ(TGR_SERVER_PORT, frame->sender_port);
  ASSERT_EQ(strlen(msg), frame->payload_len);
  ASSERT_STREQ(msg, frame->payload);

  free(bytes);
  tgr_frame_free(frame);
}

TEST(codec, encode_decode_response) {
  unsigned char* bytes = NULL;
  const char* msg = "hello world";
  int seq = tgr_millis_get();
  size_t frame_size = 0;
  size_t parsed_size = 0;

  tgr_frame_p frame = tgr_frame_init();
  frame->magic = TGR_MAGIC;
  frame->version = TGR_VERSION;
  frame->message_type = TGR_MSG_RESP;
  frame->sequence_id = seq;
  frame->error = TGR_OK;
  strcpy(frame->sender_host, "192.168.0.210");
  frame->sender_port = TGR_SERVER_PORT;
  frame->payload_len = strlen(msg);
  frame->payload = strdup(msg);

  tgr_frame_encode(frame, &bytes, &frame_size);
  tgr_frame_free(frame);

  frame = NULL;
  frame = tgr_frame_decode(bytes, &parsed_size);

  ASSERT_EQ(frame_size, parsed_size);
  ASSERT_EQ(TGR_MAGIC, frame->magic);
  ASSERT_EQ(TGR_VERSION, frame->version);
  ASSERT_EQ(TGR_MSG_RESP, frame->message_type);
  ASSERT_EQ(TGR_OK, frame->error);
  ASSERT_EQ(seq, frame->sequence_id);
  ASSERT_STREQ("192.168.0.210", frame->sender_host);
  ASSERT_EQ(TGR_SERVER_PORT, frame->sender_port);
  ASSERT_EQ(strlen(msg), frame->payload_len);
  ASSERT_STREQ(msg, frame->payload);

  free(bytes);
  tgr_frame_free(frame);
}

TEST(codec, encode_decode_response_with_error) {
  unsigned char* bytes = NULL;
  int error = TGR_ERR_FILE_READ_FAILED;
  const char* msg = strdup(tgr_error_str(error));
  int seq = tgr_millis_get();
  size_t frame_size = 0;
  size_t parsed_size = 0;

  tgr_frame_p frame = tgr_frame_init();
  frame->magic = TGR_MAGIC;
  frame->version = TGR_VERSION;
  frame->message_type = TGR_MSG_RESP;
  frame->sequence_id = seq;
  frame->error = error;
  strcpy(frame->sender_host, "192.168.0.210");
  frame->sender_port = TGR_SERVER_PORT;
  frame->payload_len = strlen(msg);
  frame->payload = strdup(msg);

  tgr_frame_encode(frame, &bytes, &frame_size);
  tgr_frame_free(frame);

  frame = NULL;
  frame = tgr_frame_decode(bytes, &parsed_size);

  ASSERT_EQ(frame_size, parsed_size);
  ASSERT_EQ(TGR_MAGIC, frame->magic);
  ASSERT_EQ(TGR_VERSION, frame->version);
  ASSERT_EQ(TGR_MSG_RESP, frame->message_type);
  ASSERT_EQ(error, frame->error);
  ASSERT_EQ(seq, frame->sequence_id);
  ASSERT_STREQ("192.168.0.210", frame->sender_host);
  ASSERT_EQ(TGR_SERVER_PORT, frame->sender_port);
  ASSERT_EQ(strlen(msg), frame->payload_len);
  ASSERT_STREQ(msg, frame->payload);

  printf("error message: %.*s\n", (int) frame->payload_len, frame->payload);
  free(bytes);
  tgr_frame_free(frame);
}
