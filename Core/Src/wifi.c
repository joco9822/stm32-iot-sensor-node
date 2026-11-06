/*
 * wifi.c (발췌)
 * 보드 내장 Wi-Fi 모듈(Inventek ISM43362, eS-WiFi)을 SPI3로 직접 제어해
 * AT 명령으로 AP·서버(Flask)에 연결하고, 센서값을 JSON으로 만들어 보냅니다.
 *   - PE8: Wi-Fi 모듈 리셋, PE0: SSN(CSN), PE1: CMD/DATA READY (EXTI1)
 * 노션 정리 자료와 랩미팅 발표자료에서 복원한 코드입니다. checkWifiEvent(),
 * bigToLittle(), littleToBig()와 AT 명령 문자열 일부(config.h)는 원본 자료에 없어 빠져 있습니다.
 */
#include "main.h"
#include "string.h"
#include "stdio.h"
#include "wifi.h"
#include "config.h"

extern SPI_HandleTypeDef hspi3;  // TODO: wifi.h로 옮기기
extern int wifi_init_re;

/* Wi-Fi 통신 및 JSON 데이터 전송에 필요한 버퍼와 플래그 */
int flagData = 0;
int flagReady = 0;
int wifiReady = 0;
int flagConnected = 0;
int flagConnectedFinal = 0;

uint8_t buffer[1000];
uint8_t bufferBig[1000];
uint8_t dummy[] = { 0x0a, 0x0a };
uint8_t ready[] = { 0x15, 0x15, 0x0a, 0x0d, 0x20, 0x3e }; // ready signal

int index = 0;
uint8_t commandbuffer[500];
int indexdata = 0;
int indexsensor = 0;

uint8_t payloadsensors[1000];

/* Wi-Fi 모듈을 하드웨어 리셋한 뒤 동작 준비가 됐는지 확인 */
uint8_t WIFI_Init() {
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET); // Wi-Fi 모듈 리셋
    HAL_Delay(1000);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET);   // Wi-Fi 모듈 리셋 해제

    do {
        wifi_init_re = getWifiModuleReady(); // Wi-Fi 모듈 초기화 확인
    } while (!wifi_init_re);

    return HAL_OK;
}

/*
 * DataRdy(PE1) 인터럽트를 기다린 뒤 SSN(PE0)을 Low로 내리고,
 * Full-duplex SPI 특성상 더미(0x0A)를 보내면서 PE1이 1인 동안 응답을 받는다.
 * 받은 데이터가 ready[] 프롬프트와 일치하면 모듈 준비 완료.
 */
int getWifiModuleReady() {
    while (!flagData)
        ;
    flagData = 0; // wifi data ready interrupt

    memset(buffer, 0, sizeof(buffer)); // buffer clear
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_RESET); // WIFI_ENABLE_NSS receive operate
    do {
        HAL_SPI_TransmitReceive(&hspi3, dummy, buffer + index, 2, 1);
        index += 2;
    } while (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == 1); // wifi data ready interrupt

    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET); // WIFI_disable_NSS not receive operate

    if (checkWifiEvent(buffer, ready, sizeof(ready))) {
        return 1; // wifi module ok
    }
    index = 0;
    return 0;
}

/* AP 접속과 서버 TCP 연결을 위한 AT 명령을 차례로 보낸다 (main()에서 WIFI_Init() 다음에 호출) */
void startWifi() {
    sendCommand((uint8_t*) SSIDcommand, strlen(SSIDcommand));       // SSID 설정
    sendCommand((uint8_t*) PSWcommand, strlen(PSWcommand));         // 비밀번호 설정
    sendCommand((uint8_t*) SECcommad, strlen(SECcommad));           // 보안 방식 설정 (예: WPA2)
    sendCommand((uint8_t*) CONNECTcommand, strlen(CONNECTcommand)); // SSID에 연결 요청
    sendCommand((uint8_t*) TCPcommand, strlen(TCPcommand));         // TCP 명령 전송
    sendCommand((uint8_t*) IPcommand, strlen(IPcommand));           // 서버 IP 설정
    sendCommand((uint8_t*) PORTcommand, strlen(PORTcommand));       // 서버 포트 설정 (Flask 서버용)
}

/*
 * Command Phase: DataRdy 상승 엣지를 기다린 뒤 명령을 LSB First(16비트 단위)로 바꿔 전송.
 * Data Phase: 다음 상승 엣지 후 더미를 보내며 응답 수신, MSB First로 되돌려 출력.
 */
void sendCommand(uint8_t *command, int size) {
    while (!flagData);
    flagData = 0; // wifi data ready interrupt

    memset(buffer, 0, sizeof(buffer));                    // reset buffer
    memset(commandbuffer, 0x0a, sizeof(commandbuffer));   // reset 0x0a command buffer

    bigToLittle(command, commandbuffer, size); // msb to lsb
    index = 0;

    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_RESET); // WIFI_ENABLE_NSS receive operate

    do {
        HAL_SPI_TransmitReceive(&hspi3, commandbuffer + index, buffer + index, 2, 1);
        index += 2;
    } while (index < size); // at command size

    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET); // WIFI_ENABLE_NSS not receive operate

    while (!flagData);
    flagData = 0; // wifi data ready interrupt

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
    for (int a = 0; a != index; a += 2) {
        printf("\'%c\' \'%c\' ", bufferBig[a], bufferBig[a + 1]);
    }
    printf("\n\r------Command transfer complete------\n\r");

    HAL_Delay(1);
}

/*
 * TCP 통신을 시작해 HTTP POST로 센서 데이터(JSON)를 보내고 연결을 종료.
 *   DATAcommand: HTTP POST 요청 전체 (헤더 + JSON 본문)
 *   DATAsend   : DATAcommand를 Wi-Fi 모듈로 보내기 위해 AT 명령 형식에 맞춘 문자열
 */
void sendData() {
    sendCommand((uint8_t*) STARTTCPcommand, strlen(STARTTCPcommand)); // TCP client 시작

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

/* 센서값을 JSON 형식 문자열(payloadsensors)로 변환 */
void convert_json(uint8_t *pDataXYZ, float *pGyroDataXYZ, float *temp_value,
                  float *humidity_value, uint32_t *lux) {
    setSensor("Accelerometer");
    setDataInt("x", pDataXYZ[0]);
    setDataInt("y", pDataXYZ[1]);
    setDataInt("z", pDataXYZ[2]); // convert int
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

/* 새 센서 항목(JSON 객체)을 시작 */
void setSensor(uint8_t *name) {
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

/* 정수 값을 "키":"값" 형식으로 추가 */
void setDataInt(uint8_t *payload, int data) {
    uint8_t value[10];
    if (indexdata != 0) {
        sprintf(value, ",");
        strcat(payloadsensors, value);
    }
    strcat(payloadsensors, "\"");
    strcat(payloadsensors, payload);
    strcat(payloadsensors, "\"");
    strcat(payloadsensors, ":");
    sprintf(value, "\"%d\"", data);
    strcat(payloadsensors, value);
    indexdata++;
}

/* 실수 값을 소수점 둘째 자리까지 "키":"값" 형식으로 추가 */
void setDataFloat(uint8_t *payload, float data) {
    uint8_t value[10];
    if (indexdata != 0) {
        sprintf(value, ",");
        strcat(payloadsensors, value);
    }
    strcat(payloadsensors, "\"");
    strcat(payloadsensors, payload);
    strcat(payloadsensors, "\"");
    strcat(payloadsensors, ":");
    sprintf(value, "\"%.2f\"", data);
    strcat(payloadsensors, value);
    indexdata++;
}
