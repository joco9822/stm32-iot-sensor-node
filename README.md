# STM32 IoT Sensor Node

B-L475E-IOT01A(STM32L475) 보드의 내장 센서값을 읽어 JSON으로 만들고, 보드 내장 Wi-Fi 모듈(ISM43362)을 **SPI로 직접 제어**해 HTTP POST로 Flask 서버에 보내는 센서 노드입니다. 서버는 받은 데이터를 로그 파일과 CSV로 저장합니다.

- 기간: 2024.01 - 2024.05
- 소속: 학부연구생, 임베디드제어 시스템 연구실 (태양광 센서노드 과제)
- 환경: STM32CubeIDE, STM32 HAL, ST BSP 드라이버, Python Flask

> **참고:** 원본 프로젝트 파일이 남아 있지 않아, 직접 코드를 분석해 발표한 **랩미팅 자료(2024.02 - 05)** 와 **노션 정리 자료**에서 핵심 코드를 복원했습니다.
> **그대로 빌드할 수 있는 전체 프로젝트가 아닙니다.** 빠진 부분은 아래 [복원되지 않은 부분](#복원되지-않은-부분)에 정리했습니다.

## 동작 흐름

```
[센서 노드]
HAL/주변장치 초기화 → 센서 초기화(BSP_Sensor_Init) → Wi-Fi 모듈 리셋·준비 확인(WIFI_Init)
→ AP 접속·서버 설정(startWifi, AT 명령)
→ [반복] 센서 측정 → JSON 변환(convert_json) → UART 출력
        → TCP 연결 → HTTP POST 전송 → 연결 종료(sendData)

[Flask 서버]
POST 수신 → 로그 기록(5시간마다 파일 회전, 최대 30개) → CSV 한 줄 추가
```

## 사용한 센서와 핀

| 구분 | 부품 | 인터페이스 | 비고 |
| --- | --- | --- | --- |
| 온도·습도 | HTS221 | I2C2 (BSP) | BSP 초기화 + HTS221 드라이버 직접 호출 |
| 가속도·자이로 | LSM6DSL | I2C2 (BSP) | |
| 조도 | VEML7700 (외부 모듈) | I2C1 | BSP 함수가 없어 데이터시트를 보고 레지스터 직접 설정 |
| Wi-Fi | Inventek ISM43362 (eS-WiFi) | SPI3 | AT 명령 기반 |
| 디버그 출력 | ST-LINK VCP | USART1 | `__io_putchar` 재정의로 `printf` 사용 |

Wi-Fi 모듈 제어 핀: `PE8` 리셋, `PE0` SPI3 CSN, `PE1` CMD/DATA READY(EXTI1 인터럽트)

## 구현 포인트

- **eS-WiFi SPI 프로토콜 직접 구현:** CMD/DATA READY(PE1)가 High가 되면 인터럽트로 알리고, Command Phase(명령 전송)와 Data Phase(응답 수신)를 차례로 진행합니다. 응답을 받을 때는 Full-duplex 특성상 더미 바이트(`0x0A`)를 보내며 수신합니다.
- **엔디언 변환:** 모듈이 16비트 단위로 주고받기 때문에 송신 전 `bigToLittle`, 수신 후 `littleToBig`로 바이트 순서를 바꿉니다.
- **HTTP POST 직접 조립:** `sendData()`가 `Content-Length`를 계산해 POST 요청 문자열을 만들고, AT 명령 형식으로 감싸 TCP로 보냅니다.
- **VEML7700 수동 드라이버:** ALS_CONF 레지스터 설정과 원시값 → Lux 변환을 구현했습니다.
- **서버 로그·CSV:** `TimedRotatingFileHandler`로 로그 파일을 시간 단위로 회전시키고, 받은 JSON을 날짜·시간과 함께 CSV로 저장합니다.

Wi-Fi 모듈에 보낸 AT 명령 순서와 응답:

| 명령 | 의미 | 응답 |
| --- | --- | --- |
| `C1=<SSID>` | Set Network SSID | OK |
| `C2=<Passphrase>` | Set Network Passphrase | OK |
| `C3=<Security Mode>` | Set Network Security Type | OK |
| `C0` | Join a Network | `JOIN <SSID> <IP> OK` |
| `P1=<Protocol>` | Set Transport Protocol | OK |
| `P3=<IP>` | Set Transport Remote Host IP Address | OK |
| `P4=<Port>` | Set Transport Remote Port Number | OK |

전송하는 JSON 예시 (`payloadsensors`):

```
{"iot2tangle": [{"sensor": "Accelerometer", "data": [{"x": "9", "y": "0", "z": "244"}]},
                {"sensor": "Magnetometer",  "data": [{"x": "770", "y": "-1610", "z": "0"}]},
                {"sensor": "Environmental", "data": [{"Humidity": "35.57", "Temperature": "28.23"}]},
                {"sensor": "Lux",           "data": [{"Lux": "245"}]}],
 "device": "...", "timestamp": 0}
```

서버가 저장한 CSV 예시:

```
Date,Time,x,y,z,Humidity,Temperature,Lux
2024-05-03,19:50:44,236,255,4,30.37,30.89,449
2024-05-03,19:50:46,237,255,3,30.4,30.89,812
```

## 파일 구성

원본 STM32CubeIDE 프로젝트(`BSP_TEST`)의 폴더 구조를 따랐습니다.

| 파일 | 내용 |
| --- | --- |
| `Core/Src/main.c` | 초기화 순서, 메인 루프, UART 출력 |
| `Core/Src/sensor.c` | 센서 초기화와 측정 (VEML7700 I2C 직접 제어 포함) |
| `Core/Src/wifi.c` | Wi-Fi 모듈 리셋·준비 확인, AT 명령 송수신, JSON 변환, HTTP POST 전송 |
| `Core/Src/stm32l4xx_it.c` | CMD/DATA READY(EXTI1) 인터럽트 핸들러 (발췌) |
| `Core/Inc/sensor.h` | 센서 함수 원형, 장치 주소·레지스터 정의 |
| `Core/Inc/wifi.h` | Wi-Fi 함수 원형 |
| `Core/Inc/config.h` | HTTP POST 문자열 (발췌) |
| `server/server.py` | Flask 수신 서버 (로그 회전, CSV 저장) |
| `docs/pseudo-code.md` | `sensor.c`를 나누기 전에 작성한 의사 코드 |

## 복원 과정에서 확인한 점

- **VEML7700 적분 시간:** 코드 주석에는 100ms라고 되어 있지만, 설정값 MSB `0b00010011`은 ALS_IT 비트(9:6)가 `1100`이라 실제로는 **25ms**입니다. 변환 계수 1.8432도 Gain 1/8 · 25ms 분해능과 일치합니다. 초기 버전(240227)은 `0x1000`(100ms)에 0.4608을 썼습니다.
- **"Magnetometer" 항목:** JSON에서 `Magnetometer`라는 이름으로 보내는 값은 실제로는 LSM6DSL **자이로** 값입니다 (`pGyroDataXYZ`).
- **함수 반환형:** 발표자료(240319) 버전은 `Accelero_sensor`가 `uint8_t`, `HTS221_sensor`가 `float`를 반환했으나, 이후 노션 정리 버전은 `void`입니다. 이 저장소는 노션 버전을 따릅니다.

## 복원되지 않은 부분

| 빠진 부분 | 역할 |
| --- | --- |
| `checkWifiEvent()` | 수신 버퍼에서 모듈 프롬프트(`ready[]`) 확인 |
| `bigToLittle()`, `littleToBig()` | 16비트 단위 바이트 순서 변환 |
| `config.h` 일부 | AT 명령 문자열, 서버 IP·포트(`IPgw`, `PORTgw`), `deviceid`, `TIME_INTERVAL` |
| CubeMX 생성 코드 | `SystemClock_Config`, `MX_*_Init`, `BSP_TEST.ioc` |
| ST BSP 드라이버 | `stm32l475e_iot01_*` (STM32CubeL4 패키지에 포함) |
| 서버 일부 | `format_data()` 본문, `templates/main.html` (`server.py`에는 실행에 필요한 최소한만 `[복원 시 추가]`로 표시해 채움) |

복원하면서 줄바꿈과 들여쓰기를 정리했고, `setDataInt`/`setDataFloat`의 중복된 분기를 하나로 합쳤습니다(동작은 같음).
