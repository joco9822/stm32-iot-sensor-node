#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

/*
 * config.h (발췌)
 * 발표자료(240409)에 나온 HTTP POST 문자열만 복원했습니다.
 * 아래 항목은 원본 자료에 값이 없어 빠져 있습니다.
 *   - IPgw, PORTgw, deviceid       : Flask 서버 주소·포트, 장치 ID
 *   - TIME_INTERVAL                : 전송 간격(ms)
 *   - eS-WiFi AT 명령 (끝은 <CR>)
 *       SSIDcommand     C1=<SSID>                 Set Network SSID
 *       PSWcommand      C2=<Passphrase>           Set Network Passphrase
 *       SECcommad       C3=<Security Mode>        Set Network Security Type
 *       CONNECTcommand  C0                        Join a Network
 *       TCPcommand      P1=<Protocol>             Set Transport Protocol
 *       IPcommand       P3=<xxx.xxx.xxx.xxx>      Set Transport Remote Host IP Address
 *       PORTcommand     P4=<Port>                 Set Transport Remote Port Number
 *       STARTTCPcommand, TCPSENDcommand, CLOSETCPcommand : TCP client 시작·데이터 전송·종료
 */

/* HTTP POST 요청: POSTinit + <본문 길이> + POSTmid + payloadsensors + POSTclose */
#define POSTinit  "\rPOST / HTTP/1.1\nHost: " IPgw ":" PORTgw "\nContent-Type: application/json\nContent-Length:"
#define POSTmid   "\n\n{ \"iot2tangle\": [ { \"sensor\":"
#define POSTclose "} ] }], \"device\": \"" deviceid "\", \"timestamp\": 0  }\r\n\r\n"

#endif /* INC_CONFIG_H_ */
