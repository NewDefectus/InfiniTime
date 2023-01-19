rm -rvf //home/user/ab/b/InfiniTime/build/ 
docker run --rm -it -v ${PWD}:/sources --user $(id -u):$(id -g) infinitime-build
nautilus `pwd`/build/
