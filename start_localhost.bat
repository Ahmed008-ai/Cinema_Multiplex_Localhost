@echo off
setlocal
cd /d "%~dp0"
title CINE - Local Cinema Management Studio

echo.
echo  ================================================
echo       CINE - CINEMA MANAGEMENT STUDIO
echo  ================================================
echo.

where py >nul 2>nul
if %errorlevel%==0 (
    py -3 server.py
    goto :end
)

where python >nul 2>nul
if %errorlevel%==0 (
    python server.py
    goto :end
)

echo Python 3 was not found.
echo Please install Python 3 from https://www.python.org/downloads/
echo During installation, tick "Add Python to PATH".
echo Then double-click this file again.

:end
echo.
pause
endlocal
