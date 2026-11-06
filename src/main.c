/*
 * main.c (발췌)
 * 노션 정리 자료에서 복원한 코드입니다. CubeMX가 생성한 초기화 함수
 * (SystemClock_Config, MX_*_Init)의 본문은 원본 자료에 없어 생략했습니다.
 */
#include "main.h"
#include "stdio.h"
#include "stm32l475e_iot01_hsensor.h"
#include "string.h"
#include "wifi.h"
#include "config.h"
#include "sensor.h"

#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

void system_status(uint8_t *pDataXYZ, float *pGyroDataXYZ, float *temp_value,
                   float *humidity_value, uint32_t *lux);

int wifi_init_re;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
SPI_HandleTypeDef hspi3;
UART_HandleTypeDef huart1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);

/* printf() 출력을 UART1로 보내기 위해 __io_putchar() 재정의 */
PUTCHAR_PROTOTYPE {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

int main(void)
{
    float humidity_value = 0;     // 습도
    float temp_value = 0;         // 온도
    float pGyroDataXYZ[3] = {0};  // 자이로센서
    uint8_t pDataXYZ[3] = {0};    // 가속도
    uint32_t lux = 0;             // 조도
    uint8_t rData[2];             // 조도 센서에서 읽어온 2바이트 원시 데이터 (MSB/LSB)

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C2_Init();
    MX_SPI3_Init();
    MX_USART1_UART_Init();
    MX_I2C1_Init();

    while (BSP_Sensor_Init() != HAL_OK)
    {
        Error_Handler();
    }
    printf("\r\nBSP init complete\r\n");

    while (WIFI_Init() != HAL_OK)
    {
        Error_Handler();
    }
    printf("\r\nwifi init complete\r\n");

    startWifi();

    while (1)
    {
        HTS221_sensor(&temp_value, &humidity_value);
        Accelero_sensor(&pDataXYZ);
        VEML7700_sensor(&lux, &rData);
        GYRO_sensor(&pGyroDataXYZ);
        convert_json(&pDataXYZ, &pGyroDataXYZ, &temp_value, &humidity_value, &lux);
        system_status(&pDataXYZ, &pGyroDataXYZ, &temp_value, &humidity_value, &lux);
        sendData();
        HAL_Delay(700);
    }
}

/* 측정한 센서값을 UART로 터미널에 출력 (디버깅 확인용) */
void system_status(uint8_t *pDataXYZ, float *pGyroDataXYZ, float *temp_value,
                   float *humidity_value, uint32_t *lux)
{
    printf("\r\nACCELERO_X = %d", pDataXYZ[0]);
    printf("\r\nACCELERO_Y = %d", pDataXYZ[1]);
    printf("\r\nACCELERO_Z = %d\r\n", pDataXYZ[2]);

    printf("\r\nTEMPERATURE is = %.2f °C", *temp_value);
    printf("\r\nHUMIDITY is = %.2f %%\r\n", *humidity_value);

    printf("\r\nlux is %lu \r\n", *lux);

    printf("\r\nGYRO_X = %.2f", pGyroDataXYZ[0]);
    printf("\r\nGYRO_Y = %.2f", pGyroDataXYZ[1]);
    printf("\r\nGYRO_Z = %.2f", pGyroDataXYZ[2]);
    printf("\r\n\n\n");
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
