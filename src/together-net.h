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
** 启动网络服务端监听并运行主服务循环
**
** 该函数负责完成服务端的全流程初始化与运行：
** 1. 创建底层网络套接字；
** 2. 绑定指定的监听地址与端口；
** 3. 进入连接监听（listen）并处理接收到的客户端请求与协议数据帧；
** 4. 阻塞运行，直到服务被主动终止或遭遇致命错误。
**
** @param[in] port 本地监听的端口号（范围 1 ~ 65535，传 0 则由操作系统动态分配可用端口）
**
** @return 执行结果状态码：
**         - 0: 服务正常终止并退出
**         - 非0: 发生异常或启动失败（具体对应 TGR_ERR_* 错误码）
*/
int
tgr_net_serve(int port);

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