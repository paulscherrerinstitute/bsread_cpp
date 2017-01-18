# Test bsread application

See contents of `test_app.cpp` for details


## Usage

After building simply run the executable and observe the output. 

	optional flags:

	   -addr [tcp://*:9090]
	   -rpc_addr [tcp://*:9091]
	   -num_chan [100]   num of channels
	   -num_elem [1024]  num of elements per channels [type: double]
	   -num_runs [100]   stop sending and destroy bsread after num_runs
	   -single           if set the bsread instance is not recreated after num_runs
	   -auto_enable      enable all channels on start



## Building

To build, create a build directory and run `cmake <path to test_app dir>` and finally make
e.g.

	mkdir build
	cd build
	cmake ..
	make 


Different distributions give different names to `libzmq`. By default it should `zmq` (with lowercase letters) however on PSI it is for some reason installed with uppercase letters (libZMQ). If you get a linker error complaining that libzmq can not be found, simply correct line 22 in CMakeLists.txt
from
	
	target_link_libraries(test_app zmq Com)

to
	
	target_link_libraries(test_app ZMQ Com)