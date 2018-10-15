export LD_LIBRARY_PATH=`pwd`:"$LD_LIBRARY_PATH"
export LD_PRELOAD=`pwd`/memory.so 
export MEMORY_DEBUG=yes
export CPUPROFILE=/media/sf_Shared/malloc/prof.out
export CPUPFOFILE_FREQUENCY=10000
libreoffice
