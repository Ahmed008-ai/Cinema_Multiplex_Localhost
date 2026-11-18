@echo off
setlocal
cd /d "%~dp0\.."
where py >nul 2>nul
if %errorlevel%==0 (
  py -3 tests\test_local_api.py
) else (
  python tests\test_local_api.py
)
if errorlevel 1 (
  echo Tests failed.
  pause
  exit /b 1
)
echo All web bridge tests passed.
pause
endlocal
