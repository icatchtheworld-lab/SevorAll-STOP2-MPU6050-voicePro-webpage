/*
 * Sensor.h
 *
 *  Created on: 2026年2月13日
 *      Author: 36315
 */

#ifndef DRIVERS_SENSORMPU6050_H_
#define DRIVERS_SENSORMPU6050_H_
#include "hal_data.h"

fsp_err_t MPU6050_Init(void);

typedef struct st_mpu6050_raw
{
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t temp;
    int16_t gx;
    int16_t gy;
    int16_t gz;
} mpu6050_raw_t;

fsp_err_t MPU6050_WhoAmI(uint8_t * p_who);
fsp_err_t MPU6050_ReadRaw(mpu6050_raw_t * p_raw);

void MPU6050_I2C_EventSet(i2c_master_event_t event);

#endif /* DRIVERS_SENSORMPU6050_H_ */
