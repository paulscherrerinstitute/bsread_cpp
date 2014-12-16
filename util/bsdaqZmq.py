import zmq

context = zmq.Context();


zmq_sock = context.socket(zmq.PULL);
zmq_sock.bind("tcp://*:9999")

while True:
    print "Waiting for msg"
    print zmq_sock.recv();