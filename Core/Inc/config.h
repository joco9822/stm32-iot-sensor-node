/*
 * config.h
 *
 *  Created on: 19 feb 2021
 *      Author: Raul Rosa
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_



/* 공개 저장소용으로 실제 값은 자리표시자로 바꿨습니다. 사용 시 환경에 맞게 수정하세요. */
#define SSID "YOUR_WIFI_SSID"            //name of the wifi
#define PSW "YOUR_WIFI_PASSWORD"         //Wifi Password
#define IPgw "192.168.0.100" //Ip address of the gateway (Flask 서버 IP)
#define PORTgw "8000"         //Port of the address
#define deviceid "456"          //the whitelist of the device in the GW
#define TIME_INTERVAL 1000 //time in ms between each data sent to GW







#endif /* INC_CONFIG_H_ */
