# make clean
# make check

make clean
make
cd build/
clear

<<<<<<< Updated upstream

make tests/userprog/exec-arg.result

# make tests/userprog/exec-read.result
# make tests/userprog/wait-killed.result
# make tests/userprog/multi-recurse.result
# make tests/userprog/multi-child-fd.result
# make tests/userprog/rox-child.result
# make tests/userprog/rox-multichild.result
=======
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-exit:mmap-exit -p tests/vm/child-mm-wrt:child-mm-wrt --swap-disk=4 -- -q   -f run mmap-exit
make tests/vm/mmap-exit.result
>>>>>>> Stashed changes

cd ..