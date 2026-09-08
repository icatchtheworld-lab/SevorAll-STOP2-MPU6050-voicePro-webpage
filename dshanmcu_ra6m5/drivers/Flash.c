/*
 * Flash.c
 *
 *  Created on: 2026年2月21日
 *      Author: 36315
 */
#include "hal_data.h"
#include <string.h>
#include <stdio.h>
#include "Flash.h"

#define ACTION_FLASH_BASE_ADDR   (0x08000000U)//数据flash的起始地址
#define ACTION_FLASH_MAX_BYTES   (8192U)//数据Flash的总容量
#define ACTION_FLASH_BLOCK_SIZE  (64U)//数据Flash的最小擦除单元

typedef struct st_action_flash_header
{
    uint32_t magic;
    uint32_t version;
    uint32_t frame_count;
    uint32_t reserved;
} action_flash_header_t;

#define ACTION_FLASH_MAGIC       (0x4152434DU) /* 'ARCM' */
#define ACTION_FLASH_VERSION     (1U)
//打开flash
static fsp_err_t action_flash_open_once(void)
{
    fsp_err_t err = g_flash0.p_api->open(g_flash0.p_ctrl, g_flash0.p_cfg);
    if (FSP_ERR_ALREADY_OPEN == err)
    {
        return FSP_SUCCESS;
    }
    return err;
}
//清空flash
fsp_err_t action_flash_clear(void)
{
    fsp_err_t err = action_flash_open_once();
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    uint32_t blocks = (ACTION_FLASH_MAX_BYTES + (ACTION_FLASH_BLOCK_SIZE - 1U)) / ACTION_FLASH_BLOCK_SIZE;
    err = g_flash0.p_api->erase(g_flash0.p_ctrl, ACTION_FLASH_BASE_ADDR, blocks);
    return err;
}
//保存动作序列到 Flash
fsp_err_t action_flash_save(action_frame_t const * frames, uint32_t frame_count)
{
    if ((NULL == frames) || (0U == frame_count))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }
//总大小=帧头+动作帧数量*动作帧大小
    uint32_t bytes_needed = sizeof(action_flash_header_t) + frame_count * sizeof(action_frame_t);
    if (bytes_needed > ACTION_FLASH_MAX_BYTES)
    {
        return FSP_ERR_INVALID_SIZE;
    }

    fsp_err_t err = action_flash_clear();
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    action_flash_header_t header;
    header.magic = ACTION_FLASH_MAGIC;
    header.version = ACTION_FLASH_VERSION;
    header.frame_count = frame_count;
    header.reserved = 0U;

    err = g_flash0.p_api->write(g_flash0.p_ctrl,
                               (uint32_t) (uintptr_t) &header,
                               ACTION_FLASH_BASE_ADDR,
                               sizeof(header));
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = g_flash0.p_api->write(g_flash0.p_ctrl,
                               (uint32_t) (uintptr_t) frames,
                               ACTION_FLASH_BASE_ADDR + sizeof(header),
                               frame_count * sizeof(action_frame_t));
    return err;
}
//读取flash的值
fsp_err_t action_flash_load(action_frame_t * frames, uint32_t max_frames, uint32_t * out_frame_count)
{
    if ((NULL == frames) || (NULL == out_frame_count) || (0U == max_frames))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    action_flash_header_t const * header = (action_flash_header_t const *) (uintptr_t) ACTION_FLASH_BASE_ADDR;
    if ((header->magic != ACTION_FLASH_MAGIC) || (header->version != ACTION_FLASH_VERSION))
    {
        *out_frame_count = 0U;
        return FSP_ERR_NOT_FOUND;
    }
    //获取已保存数据帧的数量
    uint32_t frame_count = header->frame_count;
    if (frame_count > max_frames)
    {
        frame_count = max_frames;
    }
    //条过头标签 得到实际存储的动作帧
    uint8_t const * src = (uint8_t const *) (uintptr_t) (ACTION_FLASH_BASE_ADDR + sizeof(action_flash_header_t));
    memcpy(frames, src, frame_count * sizeof(action_frame_t));
    *out_frame_count = frame_count;
    return FSP_SUCCESS;
}
