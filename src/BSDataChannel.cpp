#include "BSDataChannel.h"

using namespace std;

string bsread::BSDataChannel::dump_header(){
    return get_data_header().toStyledString();
}

void bsread::BSDataChannel::set_enabled(bool enabled){
    m_enabled = enabled;
}

bool bsread::BSDataChannel::get_enabled(){
    return m_enabled;
}

string bsread::BSDataChannel::get_name(){
    return m_name;
}

bsread::BSDataChannel::BSDataChannel(const string &name, bsread::bsdata_type type):
        m_type(type),
        m_data(0),
        m_timestamp(),
        m_len(0),
        m_name(name),
        m_encoding_le(isLittleEndian()),
        m_compression(compression_none),
        m_enabled(true),
        m_callback(0),
        m_meta_modulo(1),
        m_meta_offset(0)
{}

size_t bsread::BSDataChannel::set_data(void *data, size_t len){
    m_data=data;
    m_len=len;

    size_t n_bytes = get_len();

    // TODO: Lock the buffer.
    if ( data_buffer.get() == nullptr || n_bytes > data_buffer_length ) {
        data_buffer.reset(new char[n_bytes]);
        data_buffer_length = n_bytes;
    }

    return n_bytes;
}