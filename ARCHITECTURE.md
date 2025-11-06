## 프로젝트 구조
To Be Updated

## 폴더 구조
```
src/        
├── config/                     # 환경구성 파일(ex: 전역변수, 상태, connection 초기화 등)
│   └── gis_properties          # mis_mon 환경 구성
├── application/                # 애플리케이션 로직
│   └── mismatch_service        # mismatch 서비스
├── infrastructure/             # 외부 
│   └── jdbc_alarm_repository   # jdbc alarm 저장소
│   └── snmp_alarm_client       # snmp alarm 클라이언트
├── support/                    # 서포트 
│   └── utils.hpp               # 유틸 함수
└── mis_mon.cpp                 # bootstrap
bin/        
├── flatten_json_diff           # flatten_json_diff 실행 파일
├── mis_mon_off                 #  
└── mis_mon                     # mis_mon 실행 파일
```