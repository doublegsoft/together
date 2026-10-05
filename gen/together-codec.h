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

#ifndef __TGR_CODEC_H__
#define __TGR_CODEC_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "together-poco.h"

/*!
** Decodes a serialized raw byte buffer into an object instance.
**
** @param bytes The raw byte array containing the serialized data.
** @return      A pointer to the newly allocated and decoded object 
**              (tgr_frame_p). The caller is typically 
**              responsible for freeing this memory.
*/
tgr_frame_p 
tgr_frame_decode(const unsigned char* buf, 
                 size_t* size);

/*!
** Serializes (encodes) an object instance into a dynamically allocated byte buffer.
** 
** Note: The function name here is generated as '_decode', but the parameters 
** suggest this is meant to be an '_encode' or '_serialize' function.
**
** @param obj   The object instance to be serialized.
** @param bytes A pointer to a byte array pointer. The function will allocate 
**              the required memory for the serialized data and set *bytes to 
**              point to it. The caller is responsible for freeing this buffer.
** @param size  A pointer to an unsigned int where the function will write 
**              the total size (in bytes) of the newly allocated buffer.
*/
void 
tgr_frame_encode(const tgr_frame_p obj, 
                 unsigned char** bytes, 
                 size_t* size);

/*!
** Calculates the total byte size required to serialize (encode) a frame instance.
** 
** This function traverses the object hierarchy (including primitive fields, 
** variable-length payloads, and nested child structures) to compute the exact 
** memory footprint needed for binary encoding.
**
** @param obj  The constant tgr_frame_p instance to be measured.
** @param size A pointer to a size_t variable where the function will write 
**             the total required size (in bytes).
*/
void 
tgr_frame_bytes(const tgr_frame_p obj, 
                size_t* size);

#ifdef __cplusplus
}
#endif

#endif // __TGR_PKT_CODEC_H__
