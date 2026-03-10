# Windows Memcached Min Smoke

Generated: 2026-03-09 23:15:20 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/run_win_memcached_min_smoke.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_smoke.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| check | result |
| --- | --- |
| version | PASS |
| set/get | PASS |

Config:
- exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- listen: `127.0.0.1:11411`
- threads: `2`
- mem_mb: `64`

Version response:
```text
VERSION 1.6.40
```

Set/Get response:
```text
STORED
VALUE k 0 5
value
END
```

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\smoke\20260309_231519_memcached_min_smoke.log`
