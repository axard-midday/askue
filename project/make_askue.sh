#!/bin/sh

# необходимо на моём компьютере для осуществления компилирования
# с помощью gcc-3.4
export LIBRARY_PATH=/usr/lib/i386-linux-gnu:$LIBRARY_PATH

# создать папку для бинарников
if [ ! -d "../bin" ]
then
    mkdir "../bin"
fi

# скомпилировать файл
make

# переместить исполняемый файл в папку для бинарников
if [ -e "./askue" ]
then
    cp ./askue ../bin/askue
fi