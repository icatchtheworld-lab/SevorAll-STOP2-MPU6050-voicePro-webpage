/*
 * VoiceControl.h
 *
 *  Created on: 2026年2月10日
 *      Author: 36315
 */

#ifndef DRIVERS_VOICECONTROL_H_
#define DRIVERS_VOICECONTROL_H_

#include "hal_data.h"

// 1. 串口缓冲区宏定义（全局可见）
#define UART_BUF_SIZE       32     // 和voicecon.c里的定义保持一致

// 2. 全局变量声明（extern关键字放在头文件，定义放在.c文件）
extern uint8_t uart_recv_buf[UART_BUF_SIZE];  // 匹配voicecon.c里的数组定义
extern uint8_t uart_recv_len;                 // 匹配voicecon.c里的uint8_t类型
extern volatile uint8_t g_uart2_rx_byte;      // 把g_uart2_rx_byte也统一声明

void uart_recv_asr_cmd(void);

void voice_uart2_rx_byte(uint8_t byte);

uint8_t voice_extend_mode_is_set(void);
void voice_extend_mode_clear(void);
uint8_t voice_extend_step_try_consume(void);

uint8_t voice_mem_record_is_set(void);
uint8_t voice_mem_play_is_set(void);
uint8_t voice_mem_start_try_consume(void);
uint8_t voice_mem_end_try_consume(void);
uint8_t voice_mem_play_try_consume(void);

uint8_t voice_wireless_control_is_set(void);

#endif /* DRIVERS_VOICECONTROL_H_ */
