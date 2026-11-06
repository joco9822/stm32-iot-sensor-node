#include "wifi.h"
#include "config.h"
#include "main.h"

extern SPI_HandleTypeDef hspi3;

int flagData = 0;
int flagReady = 0;
int wifiReady = 0;
int flagConnected = 0;
int flagConnectedFinal = 0;

uint8_t buffer[1000];
uint8_t bufferBig[1000];
uint8_t dummy[] = { 0x0a, 0x0a };
uint8_t ready[] = { 0x15, 0x15, 0x0a, 0x0d, 0x20, 0x3e }; //ready buffer

int index = 0;
uint8_t commandbuffer[500];
int indexdata = 0;
int indexsensor = 0;

uint8_t payloadsensors[1000];

extern int wifi_init_re; // used in WIFI_Init()

uint8_t WIFI_Init() {
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET);  // wifi module reset
	do {
		wifi_init_re = getWifiModuleReady();
	} while (!wifi_init_re);
	return HAL_OK;
}

void convert_json(uint8_t *pDataXYZ, float *pGyroDataXYZ, float *temp_value,
		float *humidity_value, uint32_t *lux) {
	setSensor("Accelerometer");
	setDataInt("x", pDataXYZ[0]);
	setDataInt("y", pDataXYZ[1]);
	setDataInt("z", pDataXYZ[2]);  //convert int
	setSensor("Magnetometer");
	setDataInt("x", pGyroDataXYZ[0]);
	setDataInt("y", pGyroDataXYZ[1]);
	setDataInt("z", pGyroDataXYZ[2]);
	setSensor("Environmental");
	setDataFloat("Humidity", *humidity_value);
	setDataFloat("Temperature", *temp_value);
	setSensor("Lux");
	setDataInt("Lux", *lux);
}

int checkWifiEvent(uint8_t *input, uint8_t *event, int size) {

	int i = 0;
	for (i = 0; i < size; i++) {
		if (input[i] != event[i]) {
			return 0;  // data invaild = not working wifi module
		}
	}
	return 1;
}

int getWifiModuleReady() {
	while (!flagData)
		;
	flagData = 0; // wifi data ready interrupt

	memset(buffer, 0, sizeof(buffer)); // buffer clear
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_RESET); //WIFI_ENABLE_NSS receive operate
	do {

		HAL_SPI_TransmitReceive(&hspi3, dummy, buffer + index, 2, 1);

		index += 2;

	} while (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == 1); //wifi data ready interrupt

	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET); //WIFI_disable_NSS not receive operate

	if (checkWifiEvent(buffer, ready, sizeof(ready))) {
		return 1; // wifi module ok
	}
	index = 0;

	return 0;
}

void sendCommand(uint8_t *command, int size) {

	while (!flagData);
	flagData = 0; // wifi data ready interrupt

	memset(buffer, 0, sizeof(buffer));  //reset buffer
	memset(commandbuffer, 0x0a, sizeof(commandbuffer)); // reset 0x0a command buffer

	bigToLittle(command, commandbuffer, size); // msb to lsb
	index = 0;

	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_RESET); //WIFI_ENABLE_NSS receive operate
	do {
		HAL_SPI_TransmitReceive(&hspi3, commandbuffer + index, buffer + index,
				2, 1);
		index += 2;
	} while (index < size); // at command size

	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET); //WIFI_ENABLE_NSS not receive operate

	while (!flagData);
	flagData = 0;  // wifi data ready interrupt

	index = 0;
	memset(buffer, 0, sizeof(buffer));

	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_RESET);

	do {
		HAL_SPI_TransmitReceive(&hspi3, dummy, buffer + index, 2, 1);
		index += 2;
	} while (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == 1);

	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET);
	memset(bufferBig, 0, sizeof(bufferBig));
	littleToBig(buffer, bufferBig, sizeof(buffer)); // lsb to msb
	for ( int a = 0; a != index; a+=2) {
		printf("\'%c\' \'%c\' ", bufferBig[a], bufferBig[a+1]);
	}
	printf("\n\r------Command transfer complete------\n\r");

	HAL_Delay(1);
}

void getIPAddress() {
	sendCommand((uint8_t*) "AT+CIFSR\r\n", strlen("AT+CIFSR\r\n"));
	// Wi-Fi 모듈로부터 IP 주소 정보를 읽어옵니다.
	while (!flagData) {
		// 대기
	}
	flagData = 0;
	index = 0;

	// 응답 정보를 버퍼에서 찾아서 IP 주소 정보를 추출합니다.
	printf("\r\nIP :\r\n%s\r\n", buffer);

	HAL_Delay(1);
}

void littleToBig(uint8_t *input, uint8_t *output, int size) {
	for (int i = 0; i < size; i += 2) {
		output[i] = input[i + 1];
		output[i + 1] = input[i];
	}
}

void bigToLittle(uint8_t *input, uint8_t *output, int size) {
	for (int i = 0; i < size; i += 2) {
		output[i] = input[i + 1];
		output[i + 1] = input[i];
	}
}

void setSensor(uint8_t *name) {
	uint8_t value[50];
	if (indexsensor != 0) {
		strcat(payloadsensors, "} ] },{ \"sensor\":");
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, name);
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, ", \"data\": [ {");
	} else {
		memset(payloadsensors, 0, sizeof(payloadsensors));
		strcpy(payloadsensors, "\"");
		strcat(payloadsensors, name);
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, ", \"data\": [ {");
	}
	indexsensor++;
	indexdata = 0;

}

void setDataInt(uint8_t *payload, int data) {
	uint8_t value[10];
	if (indexdata != 0) {
		sprintf(value, ",");
		strcat(payloadsensors, value);
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, payload);
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, ":");
		sprintf(value, "\"%d\"", data);
		strcat(payloadsensors, value);

	} else {
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, payload);
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, ":");
		sprintf(value, "\"%d\"", data);
		strcat(payloadsensors, value);

	}
	indexdata++;

}

void setDataFloat(uint8_t *payload, float data) {
	uint8_t value[10];
	if (indexdata != 0) {
		sprintf(value, ",");
		strcat(payloadsensors, value);
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, payload);
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, ":");
		sprintf(value, "\"%.2f\"", data);
		strcat(payloadsensors, value);
	} else {
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, payload);
		strcat(payloadsensors, "\"");
		strcat(payloadsensors, ":");
		sprintf(value, "\"%.2f\"", data);
		strcat(payloadsensors, value);

	}
	indexdata++;

}

void sendData() {
	sendCommand((uint8_t*) STARTTCPcommand, strlen(STARTTCPcommand)); // send post cmd

	int payloadsize = strlen(POSTmid) + strlen(payloadsensors)
			+ strlen(POSTclose) - 4;
	uint8_t DATAcommand[1000];
	uint8_t DATAsend[1000];

	memset(DATAcommand, 0, sizeof(DATAcommand));
	memset(DATAsend, 0, sizeof(DATAsend));

	strcpy(DATAcommand, POSTinit);
	uint8_t data[10];
	memset(data, 0, sizeof(data));
	sprintf(data, "%d", payloadsize);
	strcat(DATAcommand, data);
	strcat(DATAcommand, POSTmid);
	strcat(DATAcommand, payloadsensors);
	strcat(DATAcommand, POSTclose);
	strcpy(DATAsend, TCPSENDcommand);
	memset(data, 0, sizeof(data));
	sprintf(data, "%d", strlen(DATAcommand) - 1);
	strcat(DATAsend, data);
	strcat(DATAsend, DATAcommand);

	sendCommand(DATAsend, strlen(DATAsend));
	sendCommand((uint8_t*) CLOSETCPcommand, strlen(CLOSETCPcommand));

	HAL_Delay(TIME_INTERVAL);
	indexdata = 0;
	indexsensor = 0;

	printf("http transefer complete\r\n"); // send complete
}

void startWifi() {
	sendCommand((uint8_t*) SSIDcommand, strlen(SSIDcommand)); //ssid + ssid data length
	sendCommand((uint8_t*) PSWcommand, strlen(PSWcommand));   //pw
	sendCommand((uint8_t*) SECcommad, strlen(SECcommad));     //WPA2
	sendCommand((uint8_t*) CONNECTcommand, strlen(CONNECTcommand)); //connect at solab sever
	sendCommand((uint8_t*) TCPcommand, strlen(TCPcommand));	   // set tcp cmd ip
	sendCommand((uint8_t*) IPcommand, strlen(IPcommand));	   // set server ip
	sendCommand((uint8_t*) PORTcommand, strlen(PORTcommand)); // set server port  // Flask
}

