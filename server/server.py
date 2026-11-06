"""
센서 노드가 HTTP POST로 보낸 JSON을 받아 로그 파일과 CSV 파일로 저장하는 Flask 서버.
랩미팅 발표자료(240305 로그, 240319 로그 회전, 240503 CSV)에 나온 코드를 합쳐 복원했습니다.
'# [복원 시 추가]' 표시가 있는 줄은 발표자료에 없어 실행에 필요한 만큼만 채운 부분입니다.
"""
from flask import Flask, request, render_template, jsonify
import json
import csv  # [복원 시 추가]
import logging, logging.handlers
from datetime import datetime

now = datetime.now().strftime('%d-%m-%Y_%H-%M-%S')

logger = logging.getLogger('server')
logger.setLevel(logging.INFO)  # INFO 이상의 로그가 발생했을 때만 로그가 출력
formatter = logging.Formatter(fmt="%(asctime)s %(levelname)s %(message)s",
                              datefmt='[%d/%m/%Y %H:%M:%S]')
# file_handler = logging.FileHandler('./log_files/' + now + '.log')  # 파일명은 프로그램 작동 시간
file_handler = logging.handlers.TimedRotatingFileHandler(
    filename="./log_files/log", when='H', interval=5, backupCount=30  # backupCount=0 이면 파일 개수 제한 x
)
file_handler.setFormatter(formatter)
logger.addHandler(file_handler)

received_data = None  # [복원 시 추가]
count = 0             # [복원 시 추가] CSV 헤더 작성 여부
filename = now        # [복원 시 추가] CSV 파일명 (원본 값은 자료에 없음)

app = Flask(__name__)


def write_csv_file(data):
    """생성된 csv 파일에 센서 데이터 값을 추가하는 함수"""
    data = dict(data)
    date = datetime.now().strftime("%Y-%m-%d")
    time = datetime.now().strftime("%H:%M:%S")
    global count

    with open('./csv_files/' + filename + '.csv', mode='a', newline='') as file:
        writer = csv.writer(file)

        if (count == 0):
            writer.writerow(['Date', 'Time', 'x', 'y', 'z', 'Humidity', 'Temperature', 'Lux'])  # 헤더 작성
            count += 1  # csv 파일이 처음 생성됐을 때만 헤더 작성

        for item in data['iot2tangle']:  # 데이터 작성
            sensor = item['sensor']
            sensor_data = item['data'][0]
            if sensor == 'Accelerometer':
                x = sensor_data.get('x', '')
                y = sensor_data.get('y', '')
                z = sensor_data.get('z', '')
            if sensor == 'Environmental':
                humidity = sensor_data.get('Humidity', '')
                temperature = sensor_data.get('Temperature', '')
            if sensor == 'Lux':
                lux = sensor_data.get('Lux', '')

        writer.writerow([date, time, x, y, z, humidity, temperature, lux])


def format_data(data):  # [복원 시 추가] 원본 함수 본문은 자료에 없음
    return json.dumps(data, indent=2, ensure_ascii=False)


@app.route('/', methods=['GET', 'POST'])
def index():
    global received_data
    try:
        if request.method == 'POST' and request.is_json:
            received_data = request.json
            print("Received JSON data:", received_data)
            logger.info("Received JSON data: " + str(received_data))
            write_csv_file(received_data)
            return jsonify({"message": "Data received successfully"})
        elif request.method == 'GET':
            formatted_data = format_data(received_data) if received_data else "No data received yet"
            return render_template('main.html', data=formatted_data)  # templates/main.html은 자료에 없음

    except Exception as e:
        logger.error('error: ' + str(e))
        return jsonify({"error": str(e)}), 400


if __name__ == "__main__":
    app.run(host='0.0.0.0')  # [복원 시 추가] 실제 포트는 config.h의 PORTgw와 맞춰야 함
