/*
 * Flash.h
 *
 *  Created on: 2026年2月21日
 *      Author: 36315
 */

#ifndef DRIVERS_FLASH_H_
#define DRIVERS_FLASH_H_

typedef struct st_action_frame
{
    uint16_t dt_ms;//距离上一帧的时间
    uint8_t  a;
    uint8_t  b;
    uint8_t  c;
    uint8_t  d;
    uint8_t  e;
    uint8_t  reserved;
} action_frame_t;

fsp_err_t action_flash_save(action_frame_t const * frames, uint32_t frame_count);
fsp_err_t action_flash_load(action_frame_t * frames, uint32_t max_frames, uint32_t * out_frame_count);
fsp_err_t action_flash_clear(void);

#endif /* DRIVERS_FLASH_H_ */
