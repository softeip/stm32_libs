/*
 * mpu6050_i2c.h
 *
 *  Created on: 13 Jul 2020
 *      Author: olegkaliuzhnyi
 */


#include "mpu6050/mpu6050_i2c.h"

#define I2C_REQUEST_WRITE 0x00
#define I2C_REQUEST_READ 0x01

void I2C_WriteData(I2C_TypeDef* I2Cx, uint8_t data)
{
	LL_I2C_TransmitData8(I2Cx, data);
	while(!LL_I2C_IsActiveFlag_TXE(I2Cx));
}

uint8_t I2C_ReadData(I2C_TypeDef* I2Cx)
{
	while(!LL_I2C_IsActiveFlag_RXNE(I2Cx));
	return LL_I2C_ReceiveData8(I2Cx);
}


void MPU6050_WakeUp(I2C_TypeDef* I2Cx, MPU6050Data* data)
{
	LL_I2C_DisableBitPOS(I2Cx);
	LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_ACK);

	LL_I2C_GenerateStartCondition(I2Cx);
	while(!LL_I2C_IsActiveFlag_SB(I2Cx));

	LL_I2C_ClearFlag_ADDR(I2Cx);

	LL_I2C_TransmitData8(I2Cx, data->Address << 1 | I2C_REQUEST_WRITE);
	while(!LL_I2C_IsActiveFlag_ADDR(I2Cx)); // wait until finished data tx
	LL_I2C_ClearFlag_ADDR(I2Cx);

	LL_I2C_TransmitData8(I2Cx, 0x6B); // PWR_MGMT_1 register
	while(!LL_I2C_IsActiveFlag_TXE(I2Cx));

	LL_I2C_TransmitData8(I2Cx, 0); // set to zero (wakes up the MPU-6050)
	while(!LL_I2C_IsActiveFlag_TXE(I2Cx));

	LL_I2C_GenerateStopCondition(I2Cx);
}

void MPU6050_ReadAll(I2C_TypeDef* I2Cx, MPU6050Data* data)
{
	while(LL_I2C_IsActiveFlag_BUSY(I2Cx));

	LL_I2C_DisableBitPOS(I2Cx);
	LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_ACK);

	// S
	LL_I2C_GenerateStartCondition(I2Cx);
	while(!LL_I2C_IsActiveFlag_SB(I2Cx)); // wait until Start Bit appeared

	LL_I2C_ClearFlag_ADDR(I2Cx); // reset address matching by reading registers SR1 and SR2

	// AD+W
	LL_I2C_TransmitData8(I2Cx, data->Address << 1 | I2C_REQUEST_WRITE);
	while(!LL_I2C_IsActiveFlag_ADDR(I2Cx)); // wait until address sent
	LL_I2C_ClearFlag_ADDR(I2Cx); // why to clear here ?

	// RA
	LL_I2C_TransmitData8(I2Cx, 0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
	while (!LL_I2C_IsActiveFlag_TXE(I2Cx));

	// S
	LL_I2C_GenerateStartCondition(I2Cx);
	while(!LL_I2C_IsActiveFlag_SB(I2Cx));

	LL_I2C_ClearFlag_ADDR(I2Cx); // reset address matching by reading registers SR1 and SR2

	// AD+R
	LL_I2C_TransmitData8(I2Cx, data->Address << 1 | I2C_REQUEST_READ);
	while(!LL_I2C_IsActiveFlag_ADDR(I2Cx)); // wait until address sent
	LL_I2C_ClearFlag_ADDR(I2Cx); // why to clear here ?


	uint8_t buffer[14];
	uint32_t bytes = 14;
	for(uint32_t i = 0; i < bytes; i++)
	{
		if (i < bytes - 1)
		{
			buffer[i] = I2C_ReadData(I2Cx);
		}
		else
		{
			LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);
			LL_I2C_GenerateStopCondition(I2Cx);

			buffer[i] = I2C_ReadData(I2Cx);
		}
	}

	data->AccelerometerX = buffer[0] << 8 | buffer[1]; // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
	data->AccelerometerY = buffer[2] << 8 | buffer[3]; // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
	data->AccelerometerZ = buffer[4] << 8 | buffer[5]; // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
	data->Temperature = buffer[6] << 8 | buffer[7]; // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
	data->GyroscopeX = buffer[8] << 8 | buffer[9]; // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
	data->GyroscopeY = buffer[10] << 8 | buffer[11]; // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
	data->GyroscopeZ = buffer[12] << 8 | buffer[13]; // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

}
