/*
 * sensor.c (발췌)
 * 온습도(HTS221), 가속도·자이로(LSM6DSL), 조도(VEML7700) 센서 초기화와 측정.
 * VEML7700을 제외한 센서는 ST BSP 드라이버 함수를 감싸서 사용합니다.
 * 노션 정리 자료에서 복원한 코드입니다.
 */
#include "main.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_accelero.h"
#include "sensor.h"

TSENSOR_InitTypeDef gTemp;      // HTS221 온도 센서 초기화 설정 구조체 (BSP 드라이버 제공)
extern I2C_HandleTypeDef hi2c1; // TODO: sensor.h로 옮기기 (변수 선언은 헤더 파일에)

/* 여러 센서 초기화를 한 번에 처리. main()에서 while(BSP_Sensor_Init() != HAL_OK)로 호출 */
uint8_t BSP_Sensor_Init()
{
    BSP_TSENSOR_Init();                                  // 온도 센서 초기화
    BSP_HSENSOR_Init();                                  // 습도 센서 초기화
    BSP_GYRO_Init();                                     // 자이로 센서 초기화
    HTS221_H_Init((uint16_t)HTS221_DEVICE_ID);           // HTS221 습도 센서 수동 초기화
    HTS221_T_Init((uint16_t)HTS221_DEVICE_ID, &gTemp);   // HTS221 온도 센서 수동 초기화
    BSP_ACCELERO_Init();                                 // 가속도 센서 초기화
    VEML7700_Init();                                     // 조도 센서 수동 초기화
    return HAL_OK;
}

/* 조도 센서 VEML7700은 BSP 드라이버가 없어 데이터시트를 보고 I2C로 직접 초기화 */
void VEML7700_Init()
{
    uint8_t inita[2];
    inita[0] = 0b00000000; // LSB
    inita[1] = 0b00010011; // MSB

    HAL_I2C_Mem_Write(&hi2c1, VEML7700_ADDR, VEML7700_ALS_CONFIG, 1, (uint8_t*)inita, 2, 0xFFFF);
    // 설정 요약: Power ON, Gain 1/8, Integration Time 100ms, Power Saving OFF, Interrupt OFF
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

/* VEML7700에서 2바이트(LSB First)를 읽어 16비트로 합친 뒤 Lux로 변환 */
int VEML7700_sensor(uint32_t *lux, uint8_t *rData)
{
    uint16_t temp[2] = {0};
    uint16_t outtemp;

    HAL_I2C_Mem_Read(&hi2c1, VEML7700_ADDR, VEML7700_ALS_DATA, 1, rData, 2, 0xFFFF);

    for (uint8_t i = 0; i < 2; i++) {
        temp[i] = rData[i];
    }

    outtemp = (temp[1] << 8) | temp[0];
    *lux = (uint32_t)(outtemp * 1.8432);  // 변환 계수 적용 (데이터시트)

    return *lux;
}
