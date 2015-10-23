# BSDATA - standalone BSREAD data serialization library 

## Quick start

Simply include bsdata.cc and json.cc and link against zmq library. 

E.g in cmake: 

	add_executable (bsdata_example ./example.cpp ../json.cc ../json.h ../bsdata.h ../bsdata.cc)
	target_link_libraries(bsdata_example zmq)


Or manually using c++:

	g++ -o test example.cpp ../json.cc ../bsdata.cc -I.. -lzmq


## Example: 

(see src/bsdata_example/example.cpp)


	/**
	 * @brief example Example showing the correct of BSDATA in application that manually manages
	 * its databuffer. (e.g. buffer access is local to thread that is sending the data)
	 *
	 * @param buffer_len length of integer buffer to be sent. a sensible value is 300e3 which corresponds to 1.2Mb per message
	 * or about 960MBps bandwidth
	 *
	 * @param sleep time in seconds to wait before sending a next message
	 */
	void example(size_t buffer_len, double sleep=0.01){
	    //DAQ buffer
	    unsigned int* buffer = new unsigned int[buffer_len];

	    //Create a channel for this buffer, 4byte long unsigned int data
	    bsread::BSDataChannel daq_channel("BSREADTEST:DAQ_DATA",bsread::BSDATA_INT);

	    //Since the daq_channel data and the bsread sending is preformed from the same
	    //thread no locking of data is needed, we only need to set the channels data.
	    //Whenever a message that contians this channel will be sent, the data will be
	    //fetched from buffer and sent out.
	    daq_channel.set_data(buffer,buffer_len);

	    //Lets create a second channel that will hold time needed to send the last message

	    double time_spent;
	    bsread::BSDataChannel time_channel("BSREADTESTS:TIME_SPENT",bsread::BSDATA_DOUBLE);
	    time_channel.set_data(&time_spent,1);
	    time_channel.m_meta_modulo=100;

	    //Now we need to create a bsdata message that will capture our channels
	    bsread::BSDataMessage message;

	    //Add both channels to the message
	    message.add_channel(&daq_channel);
	    message.add_channel(&time_channel);


	    //We now need to create a bsread sender that will serialize and send out the message
	    bsread::BSDataSenderZmq zmq_sender("tcp://*:9999",4096*4096);

	    //Lets send one message every 10ms, as in SwissFEL
	    long pulse_id=0;
	    timespec global_timestamp;

	    double t; //Used for time measurment
	    size_t sent;

	    while(true){
	        pulse_id++;
	        //Set a message, assign it a pulse id and prepare it for sending
	        message.set(pulse_id,global_timestamp);

	        t = dbltime_get();
	        //Send the message
	        sent = zmq_sender.send_message(message);

	        time_spent = (dbltime_get() - t)*1e3;
	        //time_spent, which is a buffer for time_channel was updated
	        //it makes sense to update its timestamp as well
	        time_channel.set_timestamp();

	        cout << "Send " << sent/1024 <<" kb " << "took " << time_spent<< "ms" << endl;

	        //Sleep 10ms
	        time_nanosleep(10/1e3);

	    }

	}


## A few more details

###Callbacks

For each channel a callback can be registed. This is usefull to handle situations where data is updated from a different thread than it is send and we want to avoid locking all channels at once (e.g. when using EPICS records as channels). Callback is invoked:

1. Before the channel data will be accessed (argument acquire==1), this can be used to either lock the data or generate/copy data into channel buffer. 
2. After the data was sent/copied into zmq stack (argument acquire==0), this can be used to release lock.

Note that the callback is called for each channel indiviudally. 


### Using BSDataSenderZmq with custom zmq socket. 

BSDataSenderZmq provides a static version of send_message function that can be invoked with any ZMQ socket. 

	bsread::BSDataSenderZmq::send_message(message,&my_custom_socket);