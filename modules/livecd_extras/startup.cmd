@echo off
cd \extras
start winvnc.exe
ipconfig

timeout 8 > nul
echo.
netstat -ano

echo.
echo VNC server is started on port 5900. Press any key to continue...
pause>NUL
