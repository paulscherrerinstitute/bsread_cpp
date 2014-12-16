/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** bsdaq.h
** Beam synchronous data acquisition, originally developed for PSI SwissFEL project
**
** -------------------------------------------------------------------------*/
#include "bsdaq.h"

#include <stdexcept>
#include <iostream>
#include <sstream>

#include <registryFunction.h>
#include <dbStaticLib.h>
#include <dbAccess.h>
#include <epicsString.h>
#include <epicsGuard.h>
#include <errlog.h>
#include <recSup.h>
//#include <google/protobuf/text_format.h>


#include "contrib/json/json.h"  //jsoncpp, external dep.
#include "bunchData.pb.h"       //protocol buffer serialization.


using namespace std;
using namespace zmq;


/**
 * Create a zmq context, create and connect a zmq push socket
 * If a zmq context or zmq socket creation failed, an zmq exception will be thrown
 */
BSDAQ::BSDAQ():_zmqCtx(new zmq::context_t(1)),_zmqSockExtern(new socket_t(*_zmqCtx, ZMQ_PUSH)), _mutex(), _configuration()
{
    //TODO should be part of the configuration
    const char * const addr = "tcp://10.5.1.215:9999";
    int hwm = 100;
    _zmqSockExtern->setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
    _zmqSockExtern->connect(addr);
}


/**
 * @brief configureBSDAQ passing it json string. Function will throw runtime error in case json is invalid.
 * Parsed json is divided into per record configuration that is contained in a vector of BSDAQRecordConfig structs.
 * This per-record configurations are later used this->snapshot() method that records appropriate values.
 */
void BSDAQ::configureBSDAQ(const string & json)
{

    Json::Value root;
    Json::Reader reader;

    // Try to parse the incoming json
    bool parsingSuccessful = reader.parse( json, root );
    if ( !parsingSuccessful ){
       string msg = "Could not parse JSON:" + reader.getFormatedErrorMessages();
       errlogPrintf(msg.c_str());
       throw runtime_error(msg);
    }

    const Json::Value records = root["records"];
    if (records.empty()) {
        string msg = "Configuration is missing the mandatory records array";
        errlogPrintf(msg.c_str());
        throw runtime_error(msg);
    } else {
        //Parsing was successful so we can drop existing configuration

        epicsGuard < epicsMutex > guard(_mutex);

        _configuration.clear();

        // Parsing success, iterate over per-record configuration
        for (Json::Value::const_iterator it = records.begin(); it != records.end(); ++it)  {

            BSDAQRecordConfig config;
            const Json::Value currentRecord = *it;
            config.name = currentRecord["name"].asString(); //TODO: Add grep support.

            if (currentRecord["offset"] != Json::Value::null) {
              int offset = 0;
              if (!(istringstream(currentRecord["offset"].asString()) >> offset)) {
                errlogPrintf("offset is invalid for a record name: %s . Should be an int \n", config.name.c_str());
              }
              else {
                config.offset = offset;
              }
            }

            if (currentRecord["freq"] != Json::Value::null) {
              int freq = 0;
              if (!(istringstream(currentRecord["freq"].asString()) >> freq)) {
                 errlogPrintf("freq is invalid for a record name: %s . Should be an unsigned int \n", config.name.c_str());
              }
              else {
                if (freq > 0) {
                  config.frequency = freq;
                }
                else {
                  errlogPrintf("freq is invalid for a record name: %s . Should be greater then 0 \n", config.name.c_str());
                }
              }
            }

            //Find address
            if(dbNameToAddr(config.name.c_str(),&(config.addr))){
                //Could not find desired record
                errlogPrintf("Record %s does not exist!", config.name.c_str());
                continue;
            }

            _configuration.push_back(config);
            cout << "Added record " << config.name << " offset:" << config.offset << " freq:" << config.frequency << endl;

        }//end of configuration iteration
    }
}


void BSDAQ::snapshot(long bunchId)
{
    epicsGuard < epicsMutex > guard(_mutex);

    if (_configuration.empty()) {
      return;
    }

    bsdaqPB::BunchData bunchData;

    bunchData.set_pulse_id(bunchId);

    //Iterate over all records in configuration
    for(vector<BSDAQRecordConfig>::iterator it = _configuration.begin(); it != _configuration.end(); ++it){

        BSDAQRecordConfig *recordConfig = &(*it);

        unsigned freq = recordConfig->frequency;
        int offset = recordConfig->offset;

        //Check if record should be snapshoted.
        if (freq > 0) {
            freq = 100/freq;
            if ( ((bunchId-offset) % freq ) != 0) {
              continue;
            }
        }

        //Preapre protoBuf entry
        bsdaqPB::BunchData_Record* recordData = bunchData.add_record();
        recordData->set_record_name(recordConfig->name);

        //Fill protoBuf
        if(recordConfig->addr.dbr_field_type == DBR_DOUBLE){
            epicsFloat64 val;
            dbGetField(&(recordConfig->addr), DBR_DOUBLE, &val, NULL, NULL, NULL);
            recordData->add_double_val(val);
        }
        else if(recordConfig->addr.dbr_field_type == DBR_STRING){
            char c_val[255];
            dbGetField(&(recordConfig->addr), DBR_STRING, &c_val, NULL, NULL, NULL);
            recordData->add_string_val()->append(c_val);
        }
    }

    string output;

    bunchData.SerializeToString(&output);

    //Deserialize the data back to human readble format, this is used for diagnostic purposes only
//    google::protobuf::TextFormat::PrintToString(bunchData, &output); //Comment out this line if you would like to have an actual PB on as output

    try {

      size_t sendBytes =_zmqSockExtern->send(output.c_str(), output.size(), ZMQ_NOBLOCK);

      if (sendBytes == 0) {
        printf("zmq socket queue full. Message NOT send.\n");
      }
    }catch(zmq::error_t &e ){
      printf("zmq send failed: %s  \n", e.what());
    }
}


/**
 * @brief BSDAQ::get singleton interface. ensures that there is exactly one instance of BSDAQ class in a proccess
 * Not thread safe at the moment! TODO: Add scoped lock!
 * @return pointer to BSDAQ instance...
 */
BSDAQ *BSDAQ::get()
{
    static BSDAQ* instance;

    if(!instance){
      instance = new BSDAQ();
    } 

    return instance;
}

int bsdaqDebug=1;
