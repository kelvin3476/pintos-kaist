# make clean
# make check

make clean
make
cd build/
clear


make tests/userprog/exec-arg.result

# make tests/userprog/exec-read.result
# make tests/userprog/wait-killed.result
# make tests/userprog/multi-recurse.result
# make tests/userprog/multi-child-fd.result
# make tests/userprog/rox-child.result
# make tests/userprog/rox-multichild.result

cd ..