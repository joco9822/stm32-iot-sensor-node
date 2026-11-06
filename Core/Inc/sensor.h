/*
 * sensor.h
 *
 *  Created on: Mar 14, 2024
 *      Author: tkjty
 */

#ifndef INC_SENSOR_H_
#define INC_SENSOR_H_

/* This file has functions that initialize HTS211, LSM6DSL and VEML7700 and read sensor's data*/

#include "main.h"

#define HTS221_DEVICE_ID 0x5F<<1
#define VEML7700_ADDR 0x10<<1
#define VEML7700_ALS_DATA 0x04
#define VEML7700_ALS_CONFIG 0x00

uint8_t BSP_Sensor_Init();
void VEML7700_Init();
uint8_t BSP_Sensor_Init();
void Accelero_sensor(uint8_t *pDataXYZ);
void GYRO_sensor(float *pGyroDataXYZ);
void HTS221_sensor(float *temp_value, float *humidity_value);
int VEML7700_sensor(uint32_t *lux, uint8_t *rData);




#endif /* INC_SENSOR_H_ */
