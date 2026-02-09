@echo off

echo 正在编译设置
windres res\WindowsBetter.rc o\rc.o

echo.
echo 正在编译主程序
g++ -c res\WindowsBetter.cpp -o o\WindowsBetter.o

echo.
echo 正在链接主程序
g++ o\WindowsBetter.o o\rc.o -o WindowsBetter\WindowsBetter.exe -luser32 -lgdi32 -lkernel32 -lcomctl32

echo.
echo 正在编译并链接监控器
g++ res\Wmonitor.cpp -o WindowsBetter\Wmonitor.exe -luser32 -lgdi32 -lkernel32

echo.
echo 正在压缩安装包
"C:\Program Files\WinRAR\WinRAR.exe" a download.zip WindowsBetter

echo.
<nul set /p="打包完成，"
pause