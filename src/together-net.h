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

#ifndef __TOGETHER_NET_H__
#define __TOGETHER_NET_H__

#include <stddef.h>

#include "together-poco.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*!
** @brief 自定义协议包处理回调函数原型
**
** @param[in]  req       客户端发送的请求帧
** @param[out] resp      待回复给客户端的响应帧（预设了版本与序列号，可填充 payload 与 error）
** @param[in]  user_data 用户携带的上下文参数
** @return 状态码或错误码（如 TGR_OK）
*/
typedef int (*tgr_net_handler_cb)(const tgr_frame_p req, tgr_frame_p resp, void* user_data);

/*!
** @brief 启动 TCP 服务端，监听并基于多路复用与工作线程处理协议请求
**
** @details 该函数以非阻塞 I/O（poll）多路复用方式监听指定端口：
**          1. 接受客户端 TCP 连接并自动管理连接生命周期；
**          2. 接收并解析满足 TGR 协议规范的数据帧；
**          3. 采用并发控制（Fast-Fail）机制：若当前活动任务数达到 MAX_CONCURRENT_TASKS，
**             则立即向客户端返回 TGR_ERR_NET_SERVER_BUSY 状态帧；
**          4. 并发未满时，将请求派发至独立工作线程执行：
**             - 若指定了 @p handler 回调函数，则由回调完成自定义业务逻辑并填充响应帧；
**             - 若 @p handler 为 NULL，则降级为默认行为（执行 Shell 命令并将 output.log 作为载荷回传）；
**          5. 任务处理完毕后自动向客户端发送响应帧并断开连接。
**
** @param[in] port      服务器监听的本地端口号（1 ~ 65535）
** @param[in] handler   自定义协议包业务处理回调函数指针。若传入 NULL，则执行默认的 Shell 命令处理器
** @param[in] user_data 传递给 @p handler 的用户私有上下文指针（可选，可传 NULL）
**
** @return int 状态码：
**         - @c TGR_OK: 服务正常退出
**         - @c TGR_ERR_NET_SERVER_CREATE: 套接字创建失败
**         - @c TGR_ERR_NET_SERVER_BIND: 端口绑定失败
*/
int
tgr_net_serve(int port, tgr_net_handler_cb handler, void* user_data);

/*!
** @brief 连接服务器、发送请求数据帧，并同步等待接收服务端的响应数据帧
**
** @param[in]  addr  目标服务器 IP 地址（如 "127.0.0.1"）
** @param[in]  port  目标服务器端口号（1 ~ 65535）
** @param[in]  frame 待发送的请求数据帧
** @param[out] resp  二级指针，用于接收服务端回复的帧对象指针（调用方负责调用 tgr_frame_free 释放）；
**                   若传入 NULL，则表示仅发送不等待响应（Fire-and-Forget 模式）
**
** @return 执行状态码：
**         - TGR_OK (0): 成功发送并成功收到解析响应帧
**         - TGR_ERR_PARAM_INVALID: 入参非法或目标 IP 格式错误
**         - TGR_ERR_NET_CLIENT_CONNECT: 连接目标服务器失败
**         - TGR_ERR_NET_STREAM_SEND: 数据帧发送中断或失败
**         - TGR_ERR_TIME_TIMEOUT: 等待服务端响应超时
**         - TGR_ERR_NET_STREAM_RECV: 接收响应失败或对端异常关闭
**         - TGR_ERR_PROTO_FRAME_CORRUPTED: 响应数据帧损坏或解析失败
*/
int 
tgr_net_send(const char* addr, 
             int port, 
             tgr_frame_p frame, 
             tgr_frame_p* resp);

#ifdef __cplusplus
}
#endif

#endif // __TOGETHER_NET_H__