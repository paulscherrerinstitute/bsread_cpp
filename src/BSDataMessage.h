#ifndef LIB_BSREAD_BSDATAMESSAGE_H
#define LIB_BSREAD_BSDATAMESSAGE_H


#include <cstdint>
#include <cstddef>
#include <string>

#include "json.h"
#include "constants.h"
#include "BSDataChannel.h"


namespace bsread {

    class BSDataMessage{
        //Message metadata
        uint64_t  m_pulseid;
        timestamp m_globaltimestamp;
        size_t m_datasize;     //raw size of data
        std::string m_datahash;
        std::string m_dataheader;    //Copy of dataheader JSON string (to avoid reconstructing JSON on every iteration)
        std::string m_mainheader;
        bsdata_compression_type m_dh_compression; //Data header compression

        //Actual members
        std::vector<BSDataChannel*> m_channels;

        Json::FastWriter m_writer;   //Json writer instance used for generating data headers


    public:

        BSDataMessage(){};
        /**
         * @brief add_channel add bsdata channel to the message. Note that the channels are not
         * deleted when message is cleared. It's users application responsibility to deallocate
         * channels.
         * @param c
         */
        void add_channel(BSDataChannel* c);

        void clear_channels();

        /**
         * sets the message pulseid and global timestamp metadata.if set_enable = true
         * than BSDataChannel enable flag will be modified according to offset and modulo.
         * @brief set
         * @param pulseid
         * @param timestamp
         * @param calc_enable
         * @return true if at least one channel was enabled, false otherwise. If
         * set_enable is false than false is always returned
         */
        bool set(uint64_t pulseid, bsread::timestamp tst, bool set_enable=true);

        const std::string* get_main_header();

        const std::string* get_data_header(bool force_build_header = false);

        const std::vector<BSDataChannel*>* get_channels(){
            return &m_channels;
        }


        /**
         * @brief is_empty checks if any of the channels within this message are enabled
         * @return true if no channels are enabled, false otherwise
         */
        bool is_empty();

        BSDataChannel* find_channel(const std::string& name);

        size_t get_datasize();

        void set_dh_compression(bsdata_compression_type type){
            this->m_dh_compression = type;
        }
    };
}


#endif //LIB_BSREAD_BSDATAMESSAGE_H
