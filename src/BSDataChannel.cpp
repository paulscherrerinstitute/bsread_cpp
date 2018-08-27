#include "BSDataChannel.h"

#include "compression.h"

using namespace std;

//Simple code to allow runtime detection of system endianess
bool isLittleEndian()
{
    short int number = 0x1;
    char *numPtr = (char*)&number;
    return (numPtr[0] == 1);
}

Json::Value bsread::BSDataChannel::get_data_header(bool config_only){
    Json::Value root;
    root["name"]=m_name;
    root["offset"] = this->m_meta_offset;
    root["modulo"] = this->m_meta_modulo;

    if(!config_only){

        root["type"]= bsdata_type_name[m_type];
        root["encoding"]= m_encoding_le ? "little" : "big";

        if(m_shape.size()){
            for(unsigned int i=0;i<m_shape.size();i++){
                root["shape"][i]=m_shape[i];
            }
        }
        else{
            root["shape"][0]=static_cast<int>(m_len); //shape is array of dimensions, scalar = [1]
        }

        string compression = "none";

        if(m_compression == compression_lz4) compression = "lz4";
        if(m_compression == compression_bslz4) compression = "bitshuffle_lz4";


        root["compression"] = compression;

    }

    return root;
}

size_t bsread::BSDataChannel::acquire_compressed(char*& buffer, size_t& buffer_size){
    if(m_compression==compression_none) return 0;

    const void* uncompressed_data = acquire(); //Do not forget to release before exiting this function
    int32_t uncompressed_data_len = get_len();
    size_t compressed_size=0;

    if(m_compression==compression_lz4){
        compressed_size=compress_lz4((const char*)uncompressed_data,uncompressed_data_len,buffer,buffer_size,false);
    }
    if(m_compression==compression_bslz4){
        compressed_size=compress_bitshuffle((const char*)uncompressed_data,get_nelm(),get_elem_size(),buffer,buffer_size);
    }

    return compressed_size;
}

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

