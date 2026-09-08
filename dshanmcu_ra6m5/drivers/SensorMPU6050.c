/*
 * Sensor.c
 *
 *  Created on: 2026年2月13日
 *      Author: 36315
 */
#include "SensorMPU6050.h"
#include <string.h>

#define MPU6050_REG_WHO_AM_I      (0x75U)//读取设备（MPU60050)的设备id  0x75这个寄存器的地址存储设备的id  返回的是1字节
#define MPU6050_REG_PWR_MGMT_1    (0x6BU)//电源管理：唤醒设备
#define MPU6050_REG_SMPLRT_DIV    (0x19U)//采样率分频寄存器
#define MPU6050_REG_CONFIG        (0x1AU)//设置陀螺仪 / 加速度计的低通滤波（LPF）带宽，过滤噪声
#define MPU6050_REG_GYRO_CONFIG   (0x1BU)//设置陀螺仪的量程  我记得返回是两字节
#define MPU6050_REG_ACCEL_CONFIG  (0x1CU)//设置加速的量程
#define MPU6050_REG_ACCEL_XOUT_H  (0x3BU)//设置加速度 X 轴高字节寄存器（只要拿到首地址 后面的yz的地址是连续的）


//用回调函数的全局变量 生成一个新的变量 g_mpu_i2c_event
static volatile i2c_master_event_t g_mpu_i2c_event = (i2c_master_event_t) 0;

void MPU6050_I2C_EventSet(i2c_master_event_t event)
{
    g_mpu_i2c_event = event;
}

static fsp_err_t mpu_i2c_wait(i2c_master_event_t expected)
{
    uint32_t timeout_ms = 200U;
    while (timeout_ms--)
    {
        //如果不等于0说明有事件发生了
        if ((i2c_master_event_t) 0 != g_mpu_i2c_event)
        {
            i2c_master_event_t e = g_mpu_i2c_event;
            g_mpu_i2c_event = (i2c_master_event_t) 0;
            if (expected == e)
            {
                return FSP_SUCCESS;
            }
            if (I2C_MASTER_EVENT_ABORTED == e)
            {
                return FSP_ERR_ABORTED;
            }
            return FSP_ERR_INTERNAL;
        }

        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
    }

    return FSP_ERR_TIMEOUT;
}
//一个目标地址 一个数据
static fsp_err_t mpu6050_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = { reg, data };
    // 2. 清空I2C事件标志（避免之前的事件干扰本次判断）
    g_mpu_i2c_event = (i2c_master_event_t) 0;
    fsp_err_t err = g_i2c_master0.p_api->write(g_i2c_master0.p_ctrl, buf, sizeof(buf), false);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return mpu_i2c_wait(I2C_MASTER_EVENT_TX_COMPLETE);
}
//读取寄存器   出入三个变量   1起始寄存器地址  2数据存储的目标缓冲区指针 3要读取的字节数 ，即从起始寄存器开始连续读取的寄存器数量
static fsp_err_t mpu6050_read_regs(uint8_t start_reg, uint8_t * p_dst, uint32_t len)
{
    fsp_err_t err;

    g_mpu_i2c_event = (i2c_master_event_t) 0;
    err = g_i2c_master0.p_api->write(g_i2c_master0.p_ctrl, &start_reg, 1U, true);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = mpu_i2c_wait(I2C_MASTER_EVENT_TX_COMPLETE);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    g_mpu_i2c_event = (i2c_master_event_t) 0;
    err = g_i2c_master0.p_api->read(g_i2c_master0.p_ctrl, p_dst, len, false);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return mpu_i2c_wait(I2C_MASTER_EVENT_RX_COMPLETE);
}

fsp_err_t MPU6050_Init(void)
{
    fsp_err_t err;

    err = g_i2c_master0.p_api->open(g_i2c_master0.p_ctrl, g_i2c_master0.p_cfg);
    if ((FSP_SUCCESS != err) && (FSP_ERR_ALREADY_OPEN != err))
    {
        return err;
    }

//写向寄存器
    err = mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x00U);//对MPU6050复位
    if (FSP_SUCCESS != err) { return err; }
    err = mpu6050_write_reg(MPU6050_REG_SMPLRT_DIV, 0x07U);
    if (FSP_SUCCESS != err) { return err; }
    err = mpu6050_write_reg(MPU6050_REG_CONFIG, 0x06U);
    if (FSP_SUCCESS != err) { return err; }
    err = mpu6050_write_reg(MPU6050_REG_GYRO_CONFIG, 0x00U);//设置陀螺仪量程+-2000度/s
    if (FSP_SUCCESS != err) { return err; }
    err = mpu6050_write_reg(MPU6050_REG_ACCEL_CONFIG, 0x00U);//设置加速度计的量程+-2g
    if (FSP_SUCCESS != err) { return err; }


    return FSP_SUCCESS;
}

fsp_err_t MPU6050_WhoAmI(uint8_t * p_who)
{
    if (NULL == p_who)
    {
        return FSP_ERR_ASSERTION;
    }

    return mpu6050_read_regs(MPU6050_REG_WHO_AM_I, p_who, 1U);
}

fsp_err_t MPU6050_ReadRaw(mpu6050_raw_t * p_raw)
{
    if (NULL == p_raw)
    {
        return FSP_ERR_ASSERTION;
    }

    uint8_t buf[14];//每个参数 两个字节 一共7个参数
    fsp_err_t err = mpu6050_read_regs(MPU6050_REG_ACCEL_XOUT_H, buf, sizeof(buf));//读取寄存器的值 存到buf1中
    if (FSP_SUCCESS != err)
    {
        return err;
    }
    //每个参数 占两字节  每个字节8位  转换为16为 则原高字节左移8位然后+第二个字节 合并一共16位
    p_raw->ax   = (int16_t) ((buf[0] << 8) | buf[1]);
    p_raw->ay   = (int16_t) ((buf[2] << 8) | buf[3]);
    p_raw->az   = (int16_t) ((buf[4] << 8) | buf[5]);
    p_raw->temp = (int16_t) ((buf[6] << 8) | buf[7]);
    p_raw->gx   = (int16_t) ((buf[8] << 8) | buf[9]);
    p_raw->gy   = (int16_t) ((buf[10] << 8) | buf[11]);
    p_raw->gz   = (int16_t) ((buf[12] << 8) | buf[13]);
    return FSP_SUCCESS;
}
