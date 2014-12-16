import zmq
import bunchData_pb2

context = zmq.Context();


zmq_sock = context.socket(zmq.PULL);
zmq_sock.bind("tcp://*:9999")

## Create a protocolBuffer container
bd = bunchData_pb2.BunchData()
while True:
    print "Waiting for msg"
    ## Try to decode protocol buffer
    bd.ParseFromString(zmq_sock.recv())    
    print bd
    #print zmq_sock.recv();