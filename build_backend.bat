@echo off
setlocal
cd /d "%~dp0"
where gcc >nul 2>nul
if errorlevel 1 (
  echo GCC was not found on PATH. Run start_localhost.bat instead; it also checks the Dev-C++ GCC folder.
  pause
  exit /b 1
)
gcc -std=c99 -O2 -Wall -Wextra -shared -static-libgcc web_backend.c -o cinema_backend.dll -lm
if errorlevel 1 (
  echo Backend build failed.
  pause
  exit /b 1
)
echo Backend DLL built successfully.
pause
endlocal
