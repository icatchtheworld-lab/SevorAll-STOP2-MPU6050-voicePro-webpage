/*
 * VoiceControl.c
 *
 *  Created on: 2026年2月10日
 *      Author: 36315
 */
#include "hal_data.h"
#include "Servor.h"
#include "VoiceControl.h"
#include <string.h>
#include "drv_uart.h"

// 串口接收缓冲区配置（适配115200波特率）
#define UART_BUF_SIZE       32     // 缓冲区足够存指令（比如"RELEASE\n"是8个字符）
uint8_t uart_recv_buf[UART_BUF_SIZE] = {0};
uint8_t uart_recv_len = 0;         // 已接收字符长度

 volatile uint8_t g_uart2_rx_byte = 0;//串口2接收的一个临时数据 用于传送

/* UART2 RX ring buffer (filled in ISR, drained in main loop) */
#define UART2_RB_SIZE 64U
static volatile uint8_t  g_uart2_rb[UART2_RB_SIZE];
static volatile uint16_t g_uart2_rb_head = 0U;
static volatile uint16_t g_uart2_rb_tail = 0U;

static volatile uint8_t g_voice_extend_mode = 0U;//延展标志位
static volatile uint8_t g_voice_extend_step_pending = 0U;

static volatile uint8_t g_voice_mem_record = 0U;//是否正在记录的标志
static volatile uint8_t g_voice_mem_play = 0U;////是否正在复现动作的标志
static volatile uint8_t g_voice_mem_start_pending = 0U;//「开始记录」的待处理指令计数器
static volatile uint8_t g_voice_mem_end_pending = 0U;//「停止记录」的待处理指令计数器
static volatile uint8_t g_voice_mem_play_pending = 0U;//「复现动作」的待处理指令计数器

static volatile uint8_t g_voice_wireless_control = 0U;

//延展模式标志位开启
uint8_t voice_extend_mode_is_set(void)
{
    return g_voice_extend_mode;
}
//延展模式标志位清除
void voice_extend_mode_clear(void)
{
    g_voice_extend_mode = 0U;
    g_voice_extend_step_pending = 0U;
}

uint8_t voice_extend_step_try_consume(void)
{
    if (g_voice_extend_step_pending)
    {
        g_voice_extend_step_pending--;
        return 1U;
    }
    return 0U;
}

uint8_t voice_mem_record_is_set(void)
{
    return g_voice_mem_record;
}

uint8_t voice_mem_play_is_set(void)
{
    return g_voice_mem_play;
}

static uint8_t consume_u8(volatile uint8_t * p_flag)
{
    if (*p_flag)
    {
        (*p_flag)--;
        return 1U;
    }
    return 0U;
}

uint8_t voice_mem_start_try_consume(void)
{
    return consume_u8(&g_voice_mem_start_pending);
}

uint8_t voice_mem_end_try_consume(void)
{
    return consume_u8(&g_voice_mem_end_pending);
}

uint8_t voice_mem_play_try_consume(void)
{
    return consume_u8(&g_voice_mem_play_pending);
}

uint8_t voice_wireless_control_is_set(void)
{
    return g_voice_wireless_control;
}

void voice_uart2_rx_byte(uint8_t byte)
{
    /* emergency STOP detection: "S\n" / "s\n" */
    static volatile uint8_t s_prev = 0U;//静态变量
    if (((s_prev == (uint8_t) 'S') || (s_prev == (uint8_t) 's')) && (byte == (uint8_t) '\n'))
    {
        servo_emergency_stop();
    }
    s_prev = byte;
//好巧妙的设计
    uint16_t head = g_uart2_rb_head;
    uint16_t next = (uint16_t) ((head + 1U) % UART2_RB_SIZE);
    if (next != g_uart2_rb_tail)
    {
        g_uart2_rb[head] = byte;//将byte写入环形缓冲区
        g_uart2_rb_head = next;//head往前移动
    }
}

static int uart2_rb_pop(uint8_t * out)
{
    uint16_t tail = g_uart2_rb_tail;
    if (tail == g_uart2_rb_head)
    {
        return 0;
    }

    *out = g_uart2_rb[tail];// 把缓冲区的字节，赋值给“out指向的变量”
    g_uart2_rb_tail = (uint16_t) ((tail + 1U) % UART2_RB_SIZE);
    return 1;
}

// 串口接收并解析语音指令（核心：触发舵机动作）
void uart_recv_asr_cmd(void)
{
    uint8_t recv_char;
    while (uart2_rb_pop(&recv_char))
    {
        if ('\r' != (char) recv_char)
        {
            if (uart_recv_len < UART_BUF_SIZE - 1)
            {
                uart_recv_buf[uart_recv_len++] = recv_char;
            }
        }

        if ('\n' == (char) recv_char)
        {
            uart_recv_buf[uart_recv_len] = '\0';

            if (strcmp((char*)uart_recv_buf, "UnCon\n") == 0)
            {
                g_voice_wireless_control = 1U;
            }
            else if (strcmp((char*)uart_recv_buf, "Con\n") == 0)
            {
                g_voice_wireless_control = 0U;
            }
            else if (!g_voice_wireless_control)
            {
                if (strcmp((char*)uart_recv_buf, "GRIP\n") == 0)
                {
                    servo_catch_target();
                }
                else if (strcmp((char*)uart_recv_buf, "RELEASE\n") == 0)
                {
                    servo_release_target();
                }
                else if (strcmp((char*)uart_recv_buf, "for\n") == 0)
                {
                    g_voice_extend_mode = 1U;//开启标志位
                    if (g_voice_extend_step_pending < 250U)
                    {
                        g_voice_extend_step_pending++;
                    }
                }
                else if (strcmp((char*)uart_recv_buf, "Com\n") == 0)//完成微调
                {
                    g_voice_extend_mode = 0U;
                    g_voice_extend_step_pending = 0U;
                }
                else if (strcmp((char*)uart_recv_buf, "Rec\n") == 0)//记录
                {
                    g_voice_mem_play = 0U;//关闭复现
                    g_voice_mem_record = 1U;//开启正在记录
                    if (g_voice_mem_start_pending < 250U)//存储指令防止被吞掉
                    {
                        g_voice_mem_start_pending++;
                    }
                }
                else if (strcmp((char*)uart_recv_buf, "End\n") == 0)//结束
                {
                    g_voice_mem_record = 0U;
                    if (g_voice_mem_end_pending < 250U)
                    {
                        g_voice_mem_end_pending++;
                    }
                }
                else if (strcmp((char*)uart_recv_buf, "Play\n") == 0)//复现
                {
                    g_voice_mem_record = 0U;
                    g_voice_mem_play = 1U;
                    if (g_voice_mem_play_pending < 250U)
                    {
                        g_voice_mem_play_pending++;
                    }
                }
            }

            uart_recv_len = 0;
            memset(uart_recv_buf, 0, UART_BUF_SIZE);
        }
    }
}
