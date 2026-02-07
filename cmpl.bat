g++ -c res\watermark.cpp -o o\watermark.o
g++ res\monitor.cpp -o WindowsBetter\Wmonitor.exe -luser32 -lgdi32 -lkernel32
windres res\winbetter.rc o\winbetter.o
g++ o\watermark.o o\winbetter.o -o WindowsBetter\WindowsBetter.exe -luser32 -lgdi32 -lkernel32