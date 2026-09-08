# Node-RED Flow

`flows.json` — 탭 2개(`ESP32 Home/Away`, `Sleep Mode`), Function 노드 20개, `node-red-dashboard@3.6.6`.

## 임포트

1. `npm i -g node-red` → `node-red` 실행
2. 팔레트 관리에서 `node-red-dashboard` 설치
3. 메뉴 → **Import** → `flows.json` 내용 붙여넣기 → Import
4. 아래 자리표시자를 본인 값으로 교체 후 **Deploy**
5. 대시보드: `http://<host>:1880/ui`

## 교체할 자리표시자

| 위치 | 자리표시자 | 넣을 값 |
|---|---|---|
| `KMA 초단기실황 파라미터` Function | `<KMA_API_KEY>` | 기상청 apihub 인증키 |
| Telegram `http request` node URL | `<TELEGRAM_BOT_TOKEN>` | `bot<봇토큰>` (예: `bot123456:ABC...`) |
| `경보 메시지` Function | `"<TELEGRAM_CHAT_ID>"` | 본인 텔레그램 채팅 ID(숫자) |
| `mqtt-broker` (Local Mosquitto) | `localhost` / `1883` | Mosquitto 주소·포트 |

> 원본에 있던 실제 토큰·인증키·채팅 ID 는 공개 저장소 반영을 위해 모두 자리표시자로 치환되어 있습니다.

## MQTT 토픽

| 토픽 | 방향 | 페이로드 |
|---|---|---|
| `home/sensors` | ESP32 → | `{temp,hum,light,gas,pir,loud,mode}` 단일 JSON |
| `home/alert` | ESP32 → | `"LOUD"` / `"INTRUSION"` |
| `home/mode` | → ESP32 | `"HOME"` / `"AWAY"` |
| `home/mode/SLEEP` · `home/mode/WAKE` | → | 수면 세션 시작 / 종료 |

## 컨텍스트 키

`currentMode`, `awaySince`, `intrusionActive`, `indoorHistory`, `sleepActive`, `sleepNoiseData`, `sleepStart`, `chartRange`
