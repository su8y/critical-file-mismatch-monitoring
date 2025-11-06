## 개요

파일(GoldenParameter, SystemCriticalFileList)에 대한 변경(수정, 추가 ,삭제)에 대한 활동들을 **monitoring**하여 **Database**에 알람을 적재 및 **SNMP Trap**을 보냅니다.

## 주요 기능
- 파일 간 비교 후 알람 및 SNMP 적재
- 파일의 **Checksum**비교후 알람 및 SNMP 적재

## 기술 스택
**internal**
- Cpp17
- Cmake

**external**
- flatten_json_diff([github](#))
- jbt-log
- nlohmann/json

## 시작 하기
```bash
# build
mkdir -p build && cd build
cmake .. && cmake --build .

# run
./bin/mis_mon
```