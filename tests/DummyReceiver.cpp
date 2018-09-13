#include "DummyReceiver.h"

using namespace std;

bsread::DummyReceiver::DummyReceiver(string address, int rcvhwm, int sock_type) :
        m_ctx(1),
        m_sock(m_ctx, sock_type),
        m_address(address)
{
    m_sock.setsockopt(ZMQ_RCVHWM, &rcvhwm, sizeof(rcvhwm));
    m_sock.connect(address.c_str());
}

void bsread::DummyReceiver::receive() {
    zmq::message_t msg;
    int more;
    size_t more_size = sizeof(more);

    m_sock.recv(&msg);
    m_sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);

    while (more) {
        m_sock.recv(&msg);
        m_sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
    }
}