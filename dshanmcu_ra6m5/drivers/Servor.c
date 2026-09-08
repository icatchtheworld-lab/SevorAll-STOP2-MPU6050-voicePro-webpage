#include "Servor.h"
#include "hal_data.h"
#include <stdio.h>
#include "drv_uart.h"

// ========== 宏定义 ==========
#define GRIP_START_ANGLE    180U
#define GRIP_CLOSE_ANGLE    90U
#define GRIP_STEP           1U
#define GRIP_DELAY_MS       30U

// ========== 全局变量 ==========
static uint32_t servo_period = 0;
static uint32_t current_servo_angle = GRIP_START_ANGLE;
static volatile uint8_t g_emergency_stop = 0U;

static uint32_t g_servo_a_deg = 90U;
static uint32_t g_servo_b_deg = GRIP_START_ANGLE;
static uint32_t g_servo_c_deg = 90U;
static uint32_t g_servo_d_deg = 90U;
static uint32_t g_servo_e_deg = 90U;

// ========== 初始化 ==========
void servo_init(void)
{

    timer_info_t info;

    // 1. 打开 GPT0和GPT1和GPT2
             R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
             R_GPT_Open(&g_timer1_ctrl, &g_timer1_cfg);
             R_GPT_Open(&g_timer2_ctrl, &g_timer2_cfg);
    // 2. 获取 GPT0 的周期信息 GPT1同周期 复用该值
             R_GPT_InfoGet(&g_timer0_ctrl, &info);
             servo_period = info.period_counts;
             printf("Servo: Period = %lu counts (20ms)\r\n", servo_period);

    // 3. 启动 GPT012
        R_GPT_Start(&g_timer0_ctrl);
        R_GPT_Start(&g_timer1_ctrl);
        R_GPT_Start(&g_timer2_ctrl);
    // 4. 初始化到中位（90°）
    servo_set_angleB(90U);
    servo_set_angleA(90U);
    servo_set_angleC(90U);
    servo_set_angleD(90U);
    servo_set_angleE(90U);
    current_servo_angle = 90U;
    printf("Servo: Initialized successfully\r\n");
}

// ========== 舵机A控制 ==========
void servo_set_angleA(uint32_t angle)
{
    // 边界检查
    if (angle > 180U) angle = 180U;

    g_servo_a_deg = angle;

    // 角度映射到脉宽（500~2500us 对应 0~180°）
    uint32_t pulse_us = 500U + (angle * 2000U / 180U);

    // 脉宽转换为占空比计数
    // 20ms = 20000us
    // servo_period 对应 20000us
    // pulse_us 对应多少计数？
    uint32_t duty_counts = (uint32_t)(((uint64_t)pulse_us * servo_period) / 20000U);


    // 更新 PWM 占空比
    fsp_err_t err = R_GPT_DutyCycleSet(
        &g_timer0_ctrl,
        duty_counts,
        GPT_IO_PIN_GTIOCA  // ← 输出引脚 A
    );

    if (FSP_SUCCESS != err)
    {
        printf("Servo-A: DutyCycleSet Error\r\n");
    }
}

// ========== 舵机B控制 ==========
void servo_set_angleB(uint32_t angle)
{
    // 边界检查
    if (angle > 180U) angle = 180U;

    g_servo_b_deg = angle;

    // 角度映射到脉宽（500~2500us 对应 0~180°）
    uint32_t pulse_us = 500U + (angle * 2000U / 180U);

    // 脉宽转换为占空比计数
    // 20ms = 20000us
    // servo_period 对应 20000us
    // pulse_us 对应多少计数？
    uint32_t duty_counts = (uint32_t)(((uint64_t)pulse_us * servo_period) / 20000U);


    // 更新 PWM 占空比
    fsp_err_t err = R_GPT_DutyCycleSet(
        &g_timer0_ctrl,
        duty_counts,
        GPT_IO_PIN_GTIOCB  // ← 输出引脚 B
    );

    if (FSP_SUCCESS != err)
    {
        printf("Servo-B: DutyCycleSet Error\r\n");
    }
}
// ========== 舵机C控制 ==========
void servo_set_angleC(uint32_t angle)
{
    // 边界检查
    if (angle > 180U) angle = 180U;

    g_servo_c_deg = angle;

    // 角度映射到脉宽（500~2500us 对应 0~180°）
    uint32_t pulse_us = 500U + (angle * 2000U / 180U);

    // 脉宽转换为占空比计数
    // 20ms = 20000us
    // servo_period 对应 20000us
    // pulse_us 对应多少计数？
    uint32_t duty_counts = (uint32_t)(((uint64_t)pulse_us * servo_period) / 20000U);


    // 更新 PWM 占空比
    fsp_err_t err = R_GPT_DutyCycleSet(
        &g_timer1_ctrl,
        duty_counts,
        GPT_IO_PIN_GTIOCB // ← 输出引脚 B
    );

    if (FSP_SUCCESS != err)
    {
        printf("Servo-C: DutyCycleSet Error\r\n");
    }
}
// ========== 舵机D控制 ==========
void servo_set_angleD(uint32_t angle)
{
    // 边界检查
    if (angle > 180U) angle = 180U;

    g_servo_d_deg = angle;

    // 角度映射到脉宽（500~2500us 对应 0~180°）
    uint32_t pulse_us = 500U + (angle * 2000U / 180U);

    // 脉宽转换为占空比计数
    // 20ms = 20000us
    // servo_period 对应 20000us
    // pulse_us 对应多少计数？
    uint32_t duty_counts = (uint32_t)(((uint64_t)pulse_us * servo_period) / 20000U);

    // 更新 PWM 占空比
    fsp_err_t err = R_GPT_DutyCycleSet(
        &g_timer1_ctrl,
        duty_counts,
        GPT_IO_PIN_GTIOCA // ← 输出引脚 A
    );

    if (FSP_SUCCESS != err)
    {
        printf("Servo-D: DutyCycleSet Error\r\n");
    }
}

// ========== 舵机E控制 ==========
void servo_set_angleE(uint32_t angle)
{
    // 边界检查
    if (angle > 180U) angle = 180U;

    g_servo_e_deg = angle;

    // 角度映射到脉宽（500~2500us 对应 0~180°）
    uint32_t pulse_us = 500U + (angle * 2000U / 180U);

    // 脉宽转换为占空比计数
    // 20ms = 20000us
    // servo_period 对应 20000us
    // pulse_us 对应多少计数？
    uint32_t duty_counts = (uint32_t)(((uint64_t)pulse_us * servo_period) / 20000U);

    // 更新 PWM 占空比
    fsp_err_t err = R_GPT_DutyCycleSet(
        &g_timer2_ctrl,
        duty_counts,
        GPT_IO_PIN_GTIOCB
    );

    if (FSP_SUCCESS != err)
    {
        printf("Servo-A: DutyCycleSet Error\r\n");
    }
}

// ========== 获取当前角度 ==========
uint32_t servo_get_current_angle(void)
{
    return current_servo_angle;
}

// ========== 设置当前角度 ==========
void servo_set_current_angle(uint32_t angle)
{
    if (angle > 180U) angle = 180U;
    current_servo_angle = angle;
}

uint32_t servo_get_angleA(void)
{
    return g_servo_a_deg;
}

uint32_t servo_get_angleB(void)
{
    return g_servo_b_deg;
}

uint32_t servo_get_angleC(void)
{
    return g_servo_c_deg;
}

uint32_t servo_get_angleD(void)
{
    return g_servo_d_deg;
}

uint32_t servo_get_angleE(void)
{
    return g_servo_e_deg;
}

// ========== 通用的平滑转动函数 ==========
void servo_move_smooth(uint32_t target_angle, uint32_t speed_ms)
{
    // 验证目标角度
    if (target_angle > 180U) target_angle = 180U;

    // 如果已经在目标位置
    if (current_servo_angle == target_angle)
    {
        printf(" 已在目标角度: %lu°\r\n", target_angle);
        return;
    }

    printf("舵机转动: %lu° → %lu° (速度: %lums/步)\r\n",
           current_servo_angle, target_angle, speed_ms);

    // 从当前角度转到目标角度
    if (current_servo_angle < target_angle)
    {
        // 需要增大角度
        for (uint32_t angle = current_servo_angle; angle <= target_angle; angle += GRIP_STEP)
        {
            if (g_emergency_stop)
                       {
                           printf(" 转动被中断! 当前角度: %lu°\r\n", angle);
                           current_servo_angle = angle;
                           servo_clear_emergency_stop();
                           return;  // 立刻返回，停止转动
                       }
            servo_set_angleB(angle);
            current_servo_angle = angle;
            R_BSP_SoftwareDelay(speed_ms, BSP_DELAY_UNITS_MILLISECONDS);
        }
    }
    else if (current_servo_angle > target_angle)
    {
        // 需要减小角度
        for (uint32_t angle = current_servo_angle; angle >= target_angle; angle -= GRIP_STEP)
        {
            if (g_emergency_stop)
                       {
                           printf("  转动被中断! 当前角度: %lu°\r\n", angle);
                           current_servo_angle = angle;
                           servo_clear_emergency_stop();
                           return;  // 立刻返回，停止转动
                       }
            servo_set_angleB(angle);
            current_servo_angle = angle;
            R_BSP_SoftwareDelay(speed_ms, BSP_DELAY_UNITS_MILLISECONDS);
        }
    }

    current_servo_angle = target_angle;  // 确保精确到达
    printf("转动完成，当前角度: %lu°\r\n\r\n", current_servo_angle);
}

// ========== 夹取动作 ==========
void servo_catch_target(void)
{
    servo_move_smooth(GRIP_CLOSE_ANGLE, GRIP_DELAY_MS);
}

// ========== 释放动作 ==========
void servo_release_target(void)
{
    servo_move_smooth(GRIP_START_ANGLE, GRIP_DELAY_MS);
}

//============紧急停止=========
void servo_emergency_stop(void)
{
    g_emergency_stop = 1U;
}
// 清除停止标志
void servo_clear_emergency_stop(void)
{
    g_emergency_stop = 0U;
}

uint8_t servo_emergency_stop_is_set(void)
{
    return g_emergency_stop;
}
