/*
 * mpu6050_i2c.h
 *
 *  Created on: 13 Jul 2020
 *      Author: olegkaliuzhnyi
 */


#include "stm32f1xx_ll_i2c.h"

typedef struct
{
	uint8_t Address;

	int16_t AccelerometerX;
	int16_t AccelerometerY;
	int16_t AccelerometerZ;
	int16_t Temperature;
	int16_t GyroscopeX;
	int16_t GyroscopeY;
	int16_t GyroscopeZ;
} MPU6050Data;



void MPU6050_WakeUp(I2C_TypeDef* I2Cx, MPU6050Data* data);

void MPU6050_ReadAll(I2C_TypeDef* I2Cx, MPU6050Data* data);

