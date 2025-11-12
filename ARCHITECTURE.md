## 폴더 구조
```
src/        
├── utils/                      # 서포트 기능
│   └── CommonUtil.hpp          # 공통 기능
│   └── FileUtil.h              # File 공통 기능
├── alarm_insert_client.{h,cpp} # 알람 적재 클라이언트 (custom)
├── snmp_trap_client.cpp        # snmp trap 클라이언트 (custom)
├── md5sum_diff.hpp             # 체크섬 비교 
├── flatten_json_diff.hpp       # Json 비교
├── main.cpp                    # bootstrap
└── CMakeList.txt               # package builder
bin/        
└── mis_mon                     # mis_mon 실행 파일
```