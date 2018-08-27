#ifndef LIB_BSREAD_BSDATACHANNEL_H
#define LIB_BSREAD_BSDATACHANNEL_H

#include <vector>
#include <string>
#include <memory>

#include "json.h"
#include "constants.h"



namespace bsread {

    class BSDataChannel;
    typedef void (*BSDataCallaback)(BSDataChannel* chan,bool acquire, void* pvt);

    class BSDataChannel {

        bsdata_type     m_type;
        void*           m_data;
        timestamp       m_timestamp;
        size_t          m_len;
        std::string          m_name;
        bool            m_encoding_le;
        bsdata_compression_type    m_compression;
        bool            m_enabled;
        std::vector<unsigned int> m_shape;

        BSDataCallaback m_callback;
        void* m_callback_pvt;

        std::unique_ptr<char> data_buffer;
        size_t data_buffer_length = 0;

    public:

        /* standard meta data variables */
        int             m_meta_modulo;
        int             m_meta_offset;


        /* extra metadata variables */
        Json::Value     m_meta;

        BSDataChannel(const std::string& name,bsdata_type type);

        /**
         * @brief set_data set message data. length is specified in number of
         * elements and not size in bytes. It is considered that data points
         * to an array of elements of type specified in constructor.
         * @param data
         * @param len number of elements of type m_type.
         * @return expected size of data (e.g. len*sizeof(m_type))
         */
        size_t set_data(void* data, size_t len);

        void* get_data(){
            return m_data;
        }

        /**
         * @brief set_callback set channel callback.
         * The callback is invoked when acquire is called and allows user to dynamically set data pointer,
         * data shape, or to preform lock to a data structure. The callback is invoked again after the sender
         * has finished using the data.
         *
         * @param c
         */
        void set_callback(BSDataCallaback c,void* pvt){
            m_callback = c;
            m_callback_pvt = pvt;
        }


        void clear_callback(){
            m_callback = 0;
            m_callback_pvt =0;
        }

        /**
         * @brief acquire function has to be called before attempting to access data,
         */
        const void* acquire(){
            if(m_callback) m_callback(this,true,m_callback_pvt);

            // TODO: Check if bsread does not use the buffer anymore.
            memcpy(data_buffer.get(), m_data, get_len());

            if(m_callback) m_callback(this,false,m_callback_pvt);

            return data_buffer.get();
        }

        void set_compression(bsdata_compression_type type){
            m_compression=type;
        }

        /**
         * @brief acquire function has to be called before attempting to access data
         * @param temp_buffer; temporary buffer to use for compression. If temp_buffer
         * is not nullptr than existing buffer is used and the result of compression is
         * placed in it. Note that if buffer is too small it will be silently resized by
         * realloc.
         *
         * If temp_buffer is nullptr than new buffer is allocated. It is responsiblity of
         * the caller to free this buffer.
         *
         */
        size_t acquire_compressed(char *&buffer, size_t &buffer_size);

        /**
         * @brief get_len
         * @return size of data returned by acquire in bytes
         */
        inline size_t get_len(){
            return get_nelm()*get_elem_size();
        }

        /**
         * @brief get_nelm
         * @return number of elements in the channel
         */
        inline size_t get_nelm(){
            return m_len;
        }

        /**
         * @brief get_elem_size
         * @return size of each element in bytes
         */
        inline size_t get_elem_size(){
            return bsdata_type_size[m_type];
        }

        void set_timestamp(timestamp timestamp);

        void set_timestamp(int64_t sec, int64_t nsec){
            m_timestamp.sec=sec;
            m_timestamp.nsec = nsec;
        }


        timestamp get_timestamp(){
            return m_timestamp;
        }

        void get_timestamp(uint64_t dest[2]){
            dest[0] = m_timestamp.sec;
            dest[1] = m_timestamp.nsec;
        }

        void set_enabled(bool enabled);
        bool get_enabled();

        /**
         * @brief set_shape sets the shape of the channel. If not specified (or if given an empty vector)
         * than the shape is deducted automatically from size of data. Note that total size must remain
         * unchanged or else the clients will not be able to parse the data.
         *
         * @param shape
         */
        void set_shape(const std::vector<unsigned int>& shape){
            m_shape = shape;
        }


        /**
         * @brief set_shape same as set_shape(); but with classic array interface that is more suitable for
         * static configurations
         * @param shape
         * @param ndim
         */
        void set_shape(unsigned int shape[], size_t ndim){
            m_shape.clear();
            for(size_t i=0;i<ndim;i++){
                m_shape.push_back(shape[i]);
            }
        }

        std::string get_name();

        std::string dump_header();

        /**
         * @brief get_data_header
         * @param config_only if set to true the data header returend contains
         *  only configuration fields (name, modulo, offset)
         * @return Json::Value representing data header for this channel
         */
        Json::Value get_data_header(bool config_only=false);

    };
}




#endif //LIB_BSREAD_BSDATACHANNEL_H
