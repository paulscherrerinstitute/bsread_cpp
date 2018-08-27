#include "BSDataMessage.h"
#include "md5.h"

using namespace std;


void bsread::BSDataMessage::add_channel(bsread::BSDataChannel *c){
    m_channels.push_back(c);
    //Update MD5 datahash since data header has changed
    m_datahash.clear();
}

void bsread::BSDataMessage::clear_channels(){
    m_channels.clear();
}

bool bsread::BSDataMessage::set(uint64_t pulseid, bsread::timestamp timestamp, bool set_enable){
    m_pulseid = pulseid;
    m_globaltimestamp = timestamp;

    bool any_channels_enabled = false;

    if(set_enable){
        for(size_t i=0;i<m_channels.size();i++){
            BSDataChannel* c = m_channels[i];

            int modulo = c->m_meta_modulo;

            int offset = c->m_meta_offset;

            //Calculate modulo if set
            if (modulo > 0) {
                if ( ((pulseid-offset) % modulo ) == 0) {
                    c->set_enabled(true);
                    any_channels_enabled = true;
                }
                else{
                    c->set_enabled(false);
                }
            }
        }
    }

    return any_channels_enabled;
}

const string *bsread::BSDataMessage::get_main_header(){
    Json::Value root;
    root["htype"] = BSREAD_MAIN_HEADER_VERSION;
    root["pulse_id"] = static_cast<Json::Int64>(m_pulseid);
    root["global_timestamp"]["sec"] = static_cast<Json::Int64>(m_globaltimestamp.sec);
    root["global_timestamp"]["ns"] = static_cast<Json::Int64>(m_globaltimestamp.nsec);

    string compression = "none";
    if(m_dh_compression == compression_lz4) compression = "lz4";
    if(m_dh_compression == compression_bslz4) compression = "bitshuffle_lz4";


    root["dh_compression"] = compression;

    /* Empty datahash indicates that the data_header was not yet constructed,
     * which is needed to calculate datahash. m_datahash is updated whenever a
     * new dataheader needs to be constructed. Here we simply overcome the lazy loading...
     */
    if(m_datahash.empty()){
        this->get_data_header();
    }

    root["hash"]=m_datahash;
    m_mainheader = m_writer.write(root);

    return &m_mainheader;
}

const string* bsread::BSDataMessage::get_data_header(bool force_build_header){
    if(m_datahash.empty() || force_build_header){

        m_datasize = 0;
        Json::Value root;
        root["htype"] = BSREAD_DATA_HEADER_VERSION;

        for(size_t i=0;i<m_channels.size();i++){
            root["channels"][(int)i]=m_channels[i]->get_data_header();
            m_datasize += m_channels[i]->get_len() + sizeof(uint64_t[2]); //Size of data + timestamp
        }

        m_dataheader = m_writer.write(root);

        // Compress data header with LZ4
        if(m_dh_compression == compression_lz4){

            char* compressed=0;
            size_t compressed_buf_size=0;
            size_t compressed_len = compress_lz4(m_dataheader.c_str(),m_dataheader.length(),compressed,compressed_buf_size,true);
            m_dataheader = string(compressed,compressed_len);

            delete compressed;
        }

        // Compress data header with LZ4 bitshuffle
        if(m_dh_compression == compression_bslz4){

            char* compressed=0;
            size_t compressed_buf_size=0;
            size_t compressed_len = compress_bitshuffle(m_dataheader.c_str(),m_dataheader.length(),sizeof(char),compressed,compressed_buf_size);
            m_dataheader = string(compressed,compressed_len);

            delete compressed;
        }



        m_datahash = md5(m_dataheader);
    }

    return &m_dataheader;

}

bool bsread::BSDataMessage::is_empty(){
    const vector<BSDataChannel*>* channels = this->get_channels();
    vector<BSDataChannel*>::const_iterator iter;

    for(iter=channels->begin();iter != channels->end();iter++){
        if((*iter)->get_enabled()){
            return false;
        }
    }
    return true;

}

bsread::BSDataChannel *bsread::BSDataMessage::find_channel(const string &name){
    for(size_t i=0;i<m_channels.size();i++){
        if(m_channels[i]->get_name() == name) return m_channels[i];
    }

    return NULL;
}

size_t bsread::BSDataMessage::get_datasize(){
    get_data_header();
    return m_datasize;
}