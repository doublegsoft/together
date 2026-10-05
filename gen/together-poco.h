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

#ifndef __TOGETHER_POCO_H__
#define __TOGETHER_POCO_H__

#ifdef __cplusplus
extern "C"
{
#endif
  
/*!
** 【Together通用消息帧】对象。
*/
typedef struct tgr_frame_s      tgr_frame_t;
typedef        tgr_frame_t*     tgr_frame_p;

/*!
** 【Together通用消息帧】数据结构定义。
*/
struct tgr_frame_s 
{
  /*!
  ** 指明这个对象的类型名称。
  */
  char type_name[64];

  /*!
  ** 【协议魔数，0x5447】
  */
  short magic;

  /*!
  ** 【协议版本号】
  */
  char version;

  /*!
  ** 【消息类型，0x01: Request, 0x02: Response, 0x03: Event】
  */
  char message_type;

  /*!
  ** 【发送方接收地址】
  */
  char sender_host[16] ;  

  /*!
  ** 【发送方接收端口】
  */
  short sender_port;

  /*!
  ** 【消息序号，用于匹配请求与响应】
  */
  int sequence_id;

  /*!
  ** 【错误码，成功则为0，错误payload就是错误信息】
  */
  int error;

  /*!
  ** 【后续载荷的实际长度 (4 Bytes)】
  */
  int payload_len;

  /*!
  ** 【实际的业务数据（对应下方具体的 Payload 结构序列化后的二进制）】
  */
  char* payload;  
}; 

/*!
** 创建并初始化【Together通用消息帧】对象。
*/
tgr_frame_p
tgr_frame_init(void);

/*!
** 释放【Together通用消息帧】对象所占用的内存。
*/
void
tgr_frame_free(tgr_frame_p);

/*!
** 设置【Together通用消息帧】的【协议魔数，0x5447】属性值。
*/
void
tgr_frame_set_magic(tgr_frame_p, short);

/*!
** 设置【Together通用消息帧】的【协议版本号】属性值。
*/
void
tgr_frame_set_version(tgr_frame_p, char);

/*!
** 设置【Together通用消息帧】的【消息类型，0x01: Request, 0x02: Response, 0x03: Event】属性值。
*/
void
tgr_frame_set_message_type(tgr_frame_p, char);

/*!
** 设置【Together通用消息帧】的【发送方接收地址】属性值。
*/
void
tgr_frame_set_sender_host(tgr_frame_p, char*);

/*!
** 设置【Together通用消息帧】的【发送方接收端口】属性值。
*/
void
tgr_frame_set_sender_port(tgr_frame_p, short);

/*!
** 设置【Together通用消息帧】的【消息序号，用于匹配请求与响应】属性值。
*/
void
tgr_frame_set_sequence_id(tgr_frame_p, int);

/*!
** 设置【Together通用消息帧】的【错误码，成功则为0，错误payload就是错误信息】属性值。
*/
void
tgr_frame_set_error(tgr_frame_p, int);

/*!
** 设置【Together通用消息帧】的【后续载荷的实际长度 (4 Bytes)】属性值。
*/
void
tgr_frame_set_payload_len(tgr_frame_p, int);

/*!
** 设置【Together通用消息帧】的【实际的业务数据（对应下方具体的 Payload 结构序列化后的二进制）】属性值。
*/
void
tgr_frame_set_payload(tgr_frame_p, char*);

#ifdef __cplusplus
}
#endif

#endif // __TOGETHER_POCO_H__
