gcc -fPIC -c -Wall -O3 *.c -lpthread
gcc -fPIC -shared -o memory.so *.o -lpthread -lrt -pthread
