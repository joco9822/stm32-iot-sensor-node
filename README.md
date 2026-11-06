# STM32 IoT Sensor Node

B-L475E-IOT01A(STM32L475) 보드의 내장 센서값을 읽어 JSON으로 만들고, 보드 내장 Wi-Fi 모듈(ISM43362)을 **SPI로 직접 제어**해 HTTP POST로 Flask 서버에 보내는 센서 노드입니다. 서버는 받은 데이터를 로그 파일과 CSV로 저장합니다.

- 기간: 2024.01 - 2024.05
- 소속: 학부연구생, 임베디드제어 시스템 연구실 (태양광 센서노드 과제)
- 환경: STM32CubeIDE 1.14, STM32 HAL, ST BSP 드라이버, Python Flask

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

- **eS-WiFi SPI 프로토콜 직접 구현:** CMD/DATA READY(PE1)가 High가 되면 인터럽트(`EXTI1_IRQHandler`)로 `flagData`를 세우고, Command Phase(명령 전송)와 Data Phase(응답 수신)를 차례로 진행합니다. 응답을 받을 때는 Full-duplex 특성상 더미 바이트(`0x0A`)를 보내며 수신합니다.
- **엔디언 변환:** 모듈이 16비트 단위로 주고받기 때문에 송신 전 `bigToLittle`, 수신 후 `littleToBig`로 두 바이트씩 순서를 바꿉니다.
- **HTTP POST 직접 조립:** `sendData()`가 `Content-Length`를 계산해 POST 요청 문자열을 만들고, `S3=<길이>` AT 명령으로 감싸 TCP로 보냅니다.
- **VEML7700 수동 드라이버:** ALS_CONF 레지스터 설정과 원시값 → Lux 변환을 구현했습니다.
- **서버 로그·CSV:** `TimedRotatingFileHandler`로 로그 파일을 시간 단위로 회전시키고, 받은 JSON을 날짜·시간과 함께 CSV로 저장합니다.

Wi-Fi 모듈에 보내는 AT 명령 (`Core/Inc/wifi.h`):

| 명령 | 의미 |
| --- | --- |
| `C1=<SSID>`, `C2=<Passphrase>`, `C3=4` | SSID, 비밀번호, 보안 방식(WPA2) 설정 |
| `C0` | AP 접속 |
| `P1=0`, `P3=<IP>`, `P4=<Port>` | TCP, 서버 IP, 서버 포트 설정 |
| `P6=1` / `P6=0` | TCP client 시작 / 종료 |
| `S3=<길이><데이터>` | 데이터 전송 |

전송하는 JSON 예시:

```
{"iot2tangle": [{"sensor": "Accelerometer", "data": [{"x": "9", "y": "0", "z": "244"}]},
                {"sensor": "Magnetometer",  "data": [{"x": "770", "y": "-1610", "z": "0"}]},
                {"sensor": "Environmental", "data": [{"Humidity": "35.57", "Temperature": "28.23"}]},
                {"sensor": "Lux",           "data": [{"Lux": "245"}]}],
 "device": "456", "timestamp": 0}
```

서버가 저장한 CSV 예시:

```
Date,Time,x,y,z,Humidity,Temperature,Lux
2024-05-03,19:50:44,236,255,4,30.37,30.89,449
2024-05-03,19:50:46,237,255,3,30.4,30.89,812
```

## 파일 구성

STM32CubeIDE 프로젝트(`BSP_TEST`) 그대로입니다. 직접 작성한 코드는 아래 파일들이고, 나머지는 CubeMX 생성 코드와 ST 드라이버입니다.

| 파일 | 내용 |
| --- | --- |
| `Core/Src/main.c` | 초기화 순서, 메인 루프, UART 출력 (`USER CODE` 영역) |
| `Core/Src/sensor.c`, `Core/Inc/sensor.h` | 센서 초기화와 측정 (VEML7700 I2C 직접 제어 포함) |
| `Core/Src/wifi.c`, `Core/Inc/wifi.h` | Wi-Fi 모듈 리셋·준비 확인, AT 명령 송수신, JSON 변환, HTTP POST 전송 |
| `Core/Inc/config.h` | Wi-Fi·서버 설정 |
| `Core/Src/stm32l4xx_it.c` | `EXTI1_IRQHandler`에서 `flagData` 설정 |
| `server/server.py` | Flask 수신 서버 (로그 회전, CSV 저장) |
| `docs/pseudo-code.md` | `sensor.c`를 나누기 전에 작성한 의사 코드 |

Wi-Fi 통신 부분(`config.h`, `wifi.c`의 JSON·HTTP 전송 구조)은 `config.h` 머리말에 남아 있듯이 IOT2TANGLE 프로젝트의 STM32 예제(Raul Rosa, 2021)를 바탕으로 했고, 이를 분석해 센서 구성과 서버에 맞게 바꿨습니다.

> `server/server.py`는 원본 파일이 남아 있지 않아 랩미팅 발표자료의 코드 캡처에서 복원했습니다. `[복원 시 추가]` 표시가 있는 줄은 자료에 없어 실행에 필요한 만큼만 채운 부분이고, `templates/main.html`은 없습니다.

## 빌드

1. STM32CubeIDE에서 `File > Import > Existing Projects into Workspace`로 이 폴더를 엽니다.
2. `Core/Inc/config.h`의 `SSID`, `PSW`, `IPgw`, `PORTgw`를 환경에 맞게 바꿉니다.
3. 빌드 후 B-L475E-IOT01A에 올리고, `server/server.py`를 같은 네트워크에서 실행합니다.

## 코드 분석 노트

- **VEML7700 적분 시간:** 주석에는 100ms라고 되어 있지만, 설정값 MSB `0b00010011`은 ALS_IT 비트(9:6)가 `1100`이라 실제로는 **25ms**입니다. 변환 계수 1.8432도 Gain 1/8 · 25ms 분해능과 일치합니다. 초기 버전(240227 발표)은 `0x1000`(100ms)에 0.4608을 썼습니다.
- **"Magnetometer" 항목:** JSON에서 `Magnetometer`라는 이름으로 보내는 값은 실제로는 LSM6DSL **자이로** 값입니다 (`pGyroDataXYZ`).
- **개선할 점:**
  - 인터럽트에서 바꾸는 `flagData`에 `volatile`이 없어 최적화 수준에 따라 `while(!flagData)`가 끝나지 않을 수 있습니다.
  - Wi-Fi 응답 대기 루프에 타임아웃이 없어, 모듈이 응답하지 않으면 시스템이 멈춥니다.
  - `setDataFloat`의 `value[10]`은 따옴표를 포함해 10자를 넘는 값에서 넘칠 수 있습니다. `snprintf`와 더 큰 버퍼가 필요합니다.
  - HAL 함수의 반환값을 확인하지 않습니다.
  - `getIPAddress()`의 `AT+CIFSR`는 ESP8266용 명령이라 eS-WiFi 모듈에서는 동작하지 않습니다 (호출되지 않는 함수).
