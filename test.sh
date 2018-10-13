export LD_LIBRARY_PATH=`pwd`:"$LD_LIBRARY_PATH"
export LD_PRELOAD=`pwd`/memory.so 
export MEMORY_DEBUG=yes
libreoffice
echo $LD_LIBRARY_PATH
