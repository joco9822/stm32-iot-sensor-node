# STM32 IoT Sensor Node

B-L475E-IOT01A(STM32L475) 보드의 내장 센서값을 읽어 JSON으로 만들고, 보드 내장 Wi-Fi 모듈(ISM43362)을 **SPI로 직접 제어**해 Flask 서버로 보내는 센서 노드 펌웨어입니다.

- 기간: 2024.01 - 2024.05
- 소속: 학부연구생, 임베디드제어 시스템 연구실 (태양광 센서노드 과제)
- 환경: STM32CubeIDE, STM32 HAL, ST BSP 드라이버

> **참고:** 이 저장소는 당시 노션에 정리해 둔 자료(PDF)에서 핵심 코드를 복원한 것입니다.
> 원본 프로젝트 파일이 남아 있지 않아 **그대로 빌드할 수 있는 전체 프로젝트가 아닙니다.**
> 빠진 파일은 아래 [복원되지 않은 부분](#복원되지-않은-부분)에 정리했습니다.

## 동작 흐름

```
HAL/주변장치 초기화 → 센서 초기화(BSP_Sensor_Init) → Wi-Fi 모듈 리셋·준비 확인(WIFI_Init)
→ AP 접속·서버 TCP 연결(startWifi, AT 명령)
→ [반복 700ms] 센서 측정 → JSON 변환(convert_json) → UART 출력 → 서버 전송(sendData)
```

## 사용한 센서와 핀

| 구분 | 부품 | 인터페이스 | 비고 |
| --- | --- | --- | --- |
| 온도·습도 | HTS221 | I2C2 (BSP) | BSP 초기화 + HTS221 드라이버 직접 호출 |
| 가속도·자이로 | LSM6DSL | I2C2 (BSP) | |
| 조도 | VEML7700 (외부 모듈) | I2C1 | BSP 드라이버가 없어 레지스터를 직접 설정 (Gain 1/8, IT 100ms) |
| Wi-Fi | Inventek ISM43362 (eS-WiFi) | SPI3 | AT 명령 기반 |
| 디버그 출력 | ST-LINK VCP | USART1 | `__io_putchar` 재정의로 `printf` 사용 |

Wi-Fi 모듈 제어 핀: `PE8` 리셋, `PE0` SSN(CSN), `PE1` CMD/DATA READY(EXTI1 인터럽트)

## 구현 포인트

- **ES-WiFi SPI 프로토콜 직접 구현:** DataRdy(PE1) 상승 엣지를 인터럽트로 받아 Command Phase와 Data Phase를 구분하고, 응답을 받을 때는 Full-duplex 특성상 더미 바이트(`0x0A`)를 보내며 수신합니다.
- **엔디언 변환:** 모듈이 16비트 단위 LSB First로 주고받기 때문에 송신 전 `bigToLittle`, 수신 후 `littleToBig`로 바이트 순서를 바꿉니다.
- **VEML7700 수동 드라이버:** 데이터시트를 보고 ALS_CONF 레지스터 설정과 원시값 → Lux 변환(×1.8432)을 구현했습니다.
- **JSON 직렬화:** `setSensor` / `setDataInt` / `setDataFloat`로 센서별 객체를 문자열로 이어 붙입니다.

서버에서 받은 데이터 예시:

```
{'sensor': 'Accelerometer', 'data': [{'x': '9', 'y': '0', 'z': '244'}]}, ...
```

## 파일 구성

| 파일 | 내용 |
| --- | --- |
| `src/main.c` | 초기화 순서, 메인 루프, UART 출력 |
| `src/sensor.c` | 센서 초기화와 측정 (VEML7700 I2C 직접 제어 포함) |
| `src/wifi.c` | Wi-Fi 모듈 리셋·준비 확인, AT 명령 송수신, JSON 변환 |
| `src/stm32l4xx_it.c` | DataRdy(EXTI1) 인터럽트 핸들러 (발췌) |

## 복원되지 않은 부분

정리 자료에 코드가 없어 이 저장소에 포함하지 못한 부분입니다.

| 빠진 부분 | 역할 |
| --- | --- |
| `sendData()` | JSON(`payloadsensors`)을 HTTP POST로 감싸 서버에 전송 |
| `checkWifiEvent()` | 수신 버퍼에서 모듈 프롬프트(`ready[]`) 확인 |
| `bigToLittle()`, `littleToBig()` | 16비트 단위 바이트 순서 변환 |
| `config.h` | Wi-Fi(SSID·비밀번호·보안), 서버 IP·포트 AT 명령 문자열, VEML7700 주소(`0x10`)·레지스터 정의 |
| `sensor.h`, `wifi.h` | 함수 선언 |
| CubeMX 생성 코드 | `SystemClock_Config`, `MX_*_Init`, `.ioc` 설정 |
| ST BSP 드라이버 | `stm32l475e_iot01_*` (STM32CubeL4 패키지에 포함) |
| Flask 서버 | 센서 JSON 수신·저장 |

복원하면서 줄바꿈과 들여쓰기를 정리했고, `setDataInt`/`setDataFloat`의 중복된 분기를 하나로 합쳤습니다(동작은 같음).
