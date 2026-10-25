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

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "together-poco.h"

tgr_frame_p
tgr_frame_init(void)
{
  tgr_frame_p ret = (tgr_frame_p) malloc(sizeof(tgr_frame_t));
  strcpy(ret->type_name, "tgr_frame_p");
  ret->sender_host[0] = '\0';
  ret->sequence_id = INT_MIN;
  ret->error = INT_MIN;
  ret->payload_len = INT_MIN;
  return ret;
}

void
tgr_frame_free(tgr_frame_p frame)
{
  if (frame->payload != NULL) 
    free(frame->payload);
  free(frame);
}

void
tgr_frame_set_magic(tgr_frame_p frame, uint16_t value)
{
  frame->magic = value;
}

void
tgr_frame_set_version(tgr_frame_p frame, uint8_t value)
{
  frame->version = value;
}

void
tgr_frame_set_message_type(tgr_frame_p frame, uint8_t value)
{
  frame->message_type = value;
}

void
tgr_frame_set_sender_host(tgr_frame_p frame, char* value)
{
  if (value == NULL) return;
  int len = strlen(value);
  strcpy(frame->sender_host, value);
  frame->sender_host[len] = '\0';
}

void
tgr_frame_set_sender_port(tgr_frame_p frame, uint16_t value)
{
  frame->sender_port = value;
}

void
tgr_frame_set_sequence_id(tgr_frame_p frame, int value)
{
  frame->sequence_id = value;
}

void
tgr_frame_set_error(tgr_frame_p frame, int value)
{
  frame->error = value;
}

void
tgr_frame_set_payload_len(tgr_frame_p frame, int value)
{
  frame->payload_len = value;
}

void
tgr_frame_set_payload(tgr_frame_p frame, uint8_t* value)
{
  frame->payload = value;
}
