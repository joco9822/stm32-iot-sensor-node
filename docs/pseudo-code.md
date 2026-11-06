# sensor.c 의사 코드

`main.c`를 `sensor.c`/`wifi.c`로 나누기 전에, 센서 관련 함수를 의사 코드로 먼저 정리한 내용입니다 (랩미팅 240312 발표자료).

## A. 조도 센서 VEML7700의 configuration register를 초기화하는 함수

```
void VEML7700_Init()
    configuration register는 16비트이므로 8비트씩 LSB와 MSB로 작성
    configuration register는 gain 1/8, integration time setting은 100ms,
        persistence protection number setting은 1, interrupt disable, power on으로 초기화
    I2C1을 이용하여 configuration register에 값을 전송. HAL 함수를 이용
```

## B. 3D 가속도 센서 및 3D 자이로스코프 LSM6DSL의 가속도 값을 읽는 함수

```
8비트 정수형 포인터를 반환 Accelero_sensor(가속도 센서의 XYZ 값을 저장하는 변수의 포인터)
    3D 가속도 센서의 XYZ 값을 순서대로 읽어오는 BSP 함수(가속도 포인터)
    가속도 센서 포인터를 반환
```

## C. 온습도 센서 HTS211의 온도 값과 습도 값을 읽는 함수

```
4byte 실수형 반환 HTS211_sensor(온도 값을 저장하는 변수의 포인터, 습도 값을 저장하는 변수의 포인터)
    온도 포인터가 참조하는 값 = HTS211의 온도 값을 읽어오는 BSP 함수(HTS211의 DEVICE ID)
    습도 포인터가 참조하는 값 = HTS211의 습도 값을 읽어오는 BSP 함수(HTS211의 DEVICE ID)
    온도 값과 습도 값을 반환
```

## D. 조도 센서 VEML7700의 lux 값을 읽는 함수

```
정수형 포인터를 반환 VEML7700_sensor(lux 값을 저장하는 변수의 포인터, VEML7700의 data register의 값을 저장. LSB와 MSB 각각 8비트)
    data register에서 읽어온 값을 임시로 저장할 변수. data register은
        LSB와 MSB 각각 8비트로 2개의 register이고 이를 비트 연산하여 합쳐야 하므로 16비트 정수형 데이터 2개가 필요
    data register의 LSB와 MSB를 비트 연산하여 합친 값을 임시로 저장할 변수
    HAL 함수와 I2C1을 이용하여 VEML7700의 data register의 값을 읽음.
        읽은 값은 8비트 정수형 배열 rData[0]에 LSB 저장, rData[1]에 MSB 저장
    for(배열의 인덱스 = 0; 배열의 인덱스<2; 배열의 인덱스++)
    {
        16비트이며 data register의 LSB와 MSB를 임시로 저장한 배열 = 8비트이며 data register의 값의 LSB와 MSB
    }
    data registers의 MSB와 LSB를 합친 값 = MSB와 LSB를 저장한 변수를 서로 비트 연산하여 합침
    lux 값을 저장할 변수의 포인터 = data registers의 MSB와 LSB를 합친 값 * (gain * responsivity = 1.8432)
    lux 값을 반환
```

## E. 3D 가속도 센서 및 3D 자이로스코프 LSM6DSL의 3D 자이로스코프 값을 읽는 함수

```
GYRO_sensor(XYZ 값을 저장할 변수의 포인터)
    포인터를 인수로 주면 3D 자이로스코프 XYZ 값을 읽어오는 BSP 함수
```

## F. 센서를 초기화 하는 함수를 묶어둔 함수

```
8비트 정수형 BSP_Sensor_Init()
    BSP 함수로 LSM6DSL의 3D 자이로스코프를 초기화
    BSP 함수로 HTS211의 온도 센서를 초기화
    BSP 함수로 HTS211의 습도 센서를 초기화
    BSP 함수로 LSM6DSL의 3D 가속도센서를 초기화
    사용자 정의 함수 VEML7700_Init 함수로 VEML7700 센서를 초기화
    HAL_OK를 반환
```
