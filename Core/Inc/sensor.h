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
/* 발표자료(240319) 버전은 Accelero_sensor가 uint8_t, HTS221_sensor가 float를 반환했으나
 * 이후 노션 정리 버전(sensor.c)에 맞춰 void로 맞춤 */
void Accelero_sensor(uint8_t *pDataXYZ);
void GYRO_sensor(float *pGyroDataXYZ);
void HTS221_sensor(float *temp_value, float *humidity_value);
int VEML7700_sensor(uint32_t *lux, uint8_t *rData);

#endif /* INC_SENSOR_H_ */
