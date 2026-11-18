@echo off
setlocal
cd /d "%~dp0\.."
if not exist tests\test_run mkdir tests\test_run
del /q tests\test_run\*.txt 2>nul
echo Building backend core tests...
gcc -std=c99 -Wall -Wextra -Wpedantic -DCINEMA_TEST -Ibackend tests\test_core.c backend\main.c -o tests\test_core.exe
if errorlevel 1 goto :failed
echo Running backend core tests...
cd /d "%~dp0\test_run"
..\test_core.exe
if errorlevel 1 goto :failed
cd /d "%~dp0\.."
echo Running console regression tests...
py -3 tests\test_cli.py
if errorlevel 1 goto :failed
echo Backend regression tests passed.
pause
exit /b 0
:failed
echo Backend regression tests failed.
pause
exit /b 1
endlocal
