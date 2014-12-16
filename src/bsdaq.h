/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** bsdaq.h
** Beam synchronous data acquisition, orginally developed for PSI SwissFEL project
**
** Author: Tom Slejko [tom.slejko@cosylab.com]
** -------------------------------------------------------------------------*/
#ifndef BSDAQ_H
#define BSDAQ_H

//std
#include <string>
#include <vector>
#include <tr1/memory>

//EPICS includes
#include <dbAccess.h>       //dbAddr
#include <epicsThread.h>

//External includes
#include "contrib/zmq.hpp"


/**
 * @brief The BSDAQRecordConfig struct
 *
 * Structure contains a per-record configuration for bsdaq.
 */
struct BSDAQRecordConfig {

    BSDAQRecordConfig(): frequency(0), offset(0) {}

    std::string name; //Name of the record
    dbAddr addr; //Pointer to record instance.
    unsigned int frequency; //Frequency of snapshots
    int offset; //Offset
};

/**
 * @brief The BSDAQ class
 *
 * All of the BeamSynchronous data acquistion buisiness logic resides within this class.
 *
 * Interaction from EPICS DB is currently implemented using asubs calling BSDAQ methods.
 * aSubs are just a wrappers for bsdaq methods.
 *
 * BSDAQ should never be manually instantianted, use static singleton get() method insted.
 *
 */
class BSDAQ
{

public:

    /**
     * @brief Constructs a zmq context and a zmq socket.
     */
    BSDAQ();


    /**
     * @brief configureBSDAQ passing it json string. Function will throw runtime error in case json is invalid.
     */
    void configureBSDAQ(const std::string & json);


    /**
     * @brief snapshot
     * Take a snapshot of currently configured values;
     */
    void snapshot(long bunchId);


    /**
     * @brief get returns a singleton instance.
     * @return
     */
    static BSDAQ* get();

private:

    //zmq context
    std::tr1::shared_ptr<zmq::context_t> _zmqCtx;

    //zmq push socket
    std::tr1::shared_ptr<zmq::socket_t>  _zmqSockExtern;

    //making BSDAQ thread safe
    epicsMutex _mutex;

    /**
     * @brief configuration_ contains BSDAQ per-record configuration
     */
    std::vector<BSDAQRecordConfig> _configuration;
};

extern int bsdaqDebug;
#define bsdaqDebug(args...) if(bsdaqDebug) epicsPrintf(args)

#endif // BSDAQ_H
