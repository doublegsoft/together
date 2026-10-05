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

#ifndef __TOGETHER_UTIL_H__
#define __TOGETHER_UTIL_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*!
** 【获取当前系统时间（秒级）】
** 
** 获取当前系统的 Unix 时间戳（秒数）。
**
** @return 当前时间的秒数整型值
*/
int 
tgr_sec_get(void);

/*!
** 【获取当前系统时间（毫秒级截断序号）】
** 
** 获取高精度的系统时间并转换为毫秒，截取低 31 位保证为非负数。
** 循环周期约为 24.8 天，适合作为通信请求与响应配对的消息序号。
**
** @return 毫秒级非负整型序号 (0 ~ 2147483647)
*/
int 
tgr_millis_get(void);

/*!
** 【获取当前本机活动 IPv4 地址】
** 
** 通过 UDP 内核路由探测技术获取当前用于对外通信的本机网卡 IP 地址。
** 无需真实发包即可自动跳过本地回环 (127.0.0.1) 及虚拟网卡。
**
** @param out_ip  存储输出 IP 字符串的目标缓冲区 (建议至少 16 字节)
** @param max_len 目标缓冲区的最大字节长度
** @return 成功返回 0，失败返回 -1
*/
int 
tgr_ip_get(char* out_ip, size_t max_len);

/*!
** @brief 执行指定的 Shell 命令或脚本缓冲区
**
** 启动 Shell 子进程并执行 \p shell 中长度为 \p size 字节的命令内容。
** 该函数为阻塞调用，直到子进程执行结束返回。
**
** @param[in] shell 指向待执行 Shell 命令或脚本内容的缓冲区指针
** @param[in] size  命令缓冲区的字节长度
**
** @return 命令执行成功返回子进程退出码（通常 0 为成功），
**         创建或执行失败返回负数错误码（如 -1）。
*/
int
tgr_shell_exec(const char* shell, size_t size);

/*!
** @brief 读取指定文件的全部内容到新分配的内存缓冲区中
**
** 以二进制只读模式打开文件，自动计算文件字节大小，分配对应大小的堆内存
** （并在末尾额外多分配 1 字节填充 '\0' 保证作为字符串处理时的安全性），
** 将文件内容一次性读入缓冲区。
**
** @param[in]  filepath 待读取的目标文件路径字符串（不可为 NULL）
** @param[out] bytes    二级指针，用于接收动态分配的字节缓冲区地址。
**                      注意：该内存的所有权归调用方所有，使用完毕后必须调用 free() 释放。
** @param[out] size     指针，用于接收成功读取的文件总字节数。
**
** @return 执行状态码：
**         - 0  : 读取成功
**         - -1 : 读取失败（入参无效、文件打开/寻址失败、内存不足或读取不完整）
*/
int
tgr_file_read(const char* filepath, char** bytes, size_t* size);

#ifdef __cplusplus
}
#endif

#endif // __TOGETHER_UTIL_H__