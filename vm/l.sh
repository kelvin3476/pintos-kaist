# make clean
# make check

make clean
make
cd build/
clear

# # pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-exit:mmap-exit -p tests/vm/child-mm-wrt:child-mm-wrt --swap-disk=4 -- -q   -f run mmap-exit
# # make tests/vm/mmap-exit.result

pintos -v -k -T 300 -m 20   --fs-disk=10 -p tests/filesys/base/syn-read:syn-read -p tests/filesys/base/child-syn-read:child-syn-read --swap-disk=4 -- -q   -f run syn-read
make tests/filesys/base/syn-read.result

cd ..