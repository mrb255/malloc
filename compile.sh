    gcc -fPIC -c -Wall -g -O0 *.c -lpthread
    gcc -fPIC -shared -o memory.so *.o -lpthread -lrt -pthread