#ifndef INC_WIFI_H_
#define INC_WIFI_H_

/* 발표자료(240319)에는 WIFI_Init(), convert_json() 원형만 나와 있어
 * 나머지 원형은 wifi.c의 정의를 보고 채웠습니다. */

#include "main.h"

uint8_t WIFI_Init();
int getWifiModuleReady();
void startWifi();
void sendCommand(uint8_t *command, int size);
void sendData();

void convert_json(uint8_t *pDataXYZ, float *pGyroDataXYZ, float *temp_value,
        float *humidity_value, uint32_t *lux);
void setSensor(uint8_t *name);
void setDataInt(uint8_t *payload, int data);
void setDataFloat(uint8_t *payload, float data);

#endif /* INC_WIFI_H_ */
