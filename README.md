## 개요

파일(GoldenParameter, SystemCriticalFileList)에 대한 변경(수정, 추가 ,삭제)에 대한 활동들을 **monitoring**하여 **Database**에 알람을 적재 및 **SNMP Trap**을 보냅니다.

## 주요 기능
- 파일 간 비교 후 알람 발생 및 SNMP 적재
- 파일의 **Checksum**비교후 알람 및 SNMP 적재

## 기술 스택
**internal**
- Cpp20
- CMake

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
./bin/mis_mon --config <config_file_path> {off <off_target_file>}
```

### config file
[example setup.json](./example/setup.json)

| 필드 | 값 | 설명 |
|---|---|---|
| type | checksum or json | mismatch할때 사용할 알고리즘 설정 |
| target_type | file or listfile | mismatch target 파일의 타입 |
| cron | 크론 표현식 | mismatch를 언제 실행할지 설정 |

---

## 아키텍처
[문서 바로가기](./ARCHITECTURE.md)