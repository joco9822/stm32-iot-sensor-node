/*
 * sensor.c
 *
 *  Created on: Mar 14, 2024
 *      Author: tkjty
 */

#include "main.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_accelero.h"
#include "sensor.h"

TSENSOR_InitTypeDef gTemp;  // BSP 드라이버. 온도 함수 정의를 위해 사용됨
extern I2C_HandleTypeDef hi2c1;


uint8_t BSP_Sensor_Init()
{
	BSP_TSENSOR_Init();								//BSP Init
	BSP_HSENSOR_Init();								//BSP Init
	BSP_GYRO_Init();									//Gyro sensor Init
	HTS221_H_Init((uint16_t)HTS221_DEVICE_ID);			//Temp sensor Init
	HTS221_T_Init((uint16_t)HTS221_DEVICE_ID, &gTemp);	//Humi sensor Init
	BSP_ACCELERO_Init();								//Accelero Init
	VEML7700_Init();
	return HAL_OK;
}

void VEML7700_Init()
{
	uint8_t inita[2];
	inita[0] = 0b00000000; //  (LSB)
	inita[1] = 0b00010011; //  (MSB)
	HAL_I2C_Mem_Write(&hi2c1, VEML7700_ADDR, VEML7700_ALS_CONFIG, 1, (uint8_t*)inita, 2, 0xFFFF);
	// setting veml init setting : power on , gain 1/8 , intergration 100ms , power saving off , no interrupt
}


void Accelero_sensor(uint8_t *pDataXYZ)
{
	BSP_ACCELERO_AccGetXYZ(pDataXYZ);
}

void GYRO_sensor(float *pGyroDataXYZ)
{
	BSP_GYRO_GetXYZ(pGyroDataXYZ);
}

void HTS221_sensor(float *temp_value, float *humidity_value)
{
    *temp_value = HTS221_T_ReadTemp((uint16_t)HTS221_DEVICE_ID);
    *humidity_value = HTS221_H_ReadHumidity((uint16_t)HTS221_DEVICE_ID);

}

int VEML7700_sensor(uint32_t *lux, uint8_t *rData)

{
	uint16_t temp[2] = {0};
	uint16_t outtemp;
	HAL_I2C_Mem_Read(&hi2c1, VEML7700_ADDR, VEML7700_ALS_DATA, 1, rData, 2, 0xFFFF);  //VEML Memory read ALS Data
	for(uint8_t i = 0; i<2; i++)
	{
		temp[i] = rData[i];
	}
	outtemp = (temp[1]<<8)|temp[0];
	*lux = (uint32_t)(outtemp * 1.8432);//lux data trasform

	return *lux;
}
