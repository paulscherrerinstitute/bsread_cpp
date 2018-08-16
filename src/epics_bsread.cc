#include "epicsStdlib.h"
#include "epicsStdio.h"
#include "epicsString.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "errlog.h"
#include "ellLib.h"
#include "epicsGuard.h"
#include "epicsMutex.h"
#include "epicsStdioRedirect.h"
#include "ellLib.h"
#include "dbBase.h"
#include "dbStaticLib.h"
#include "link.h"
#include "dbFldTypes.h"
#include "dbAddr.h"
#include "dbLock.h"
#include "recSup.h"
#include "devSup.h"
#include "drvSup.h"
#include "dbCommon.h"
#include "special.h"
#include "db_field_log.h"
#include "epicsString.h"
#include "iocsh.h"

#include "bsread.h"

#include "epicsExport.h"

#include "epics_bsread.h"

#include <fstream>
#include <sstream>



/** Static class member instances **/

epicsMutex epicsBSRead::g_bsread_mtx;
std::map<std::string, bsread::BSRead *> epicsBSRead::g_bsread_inst;


/**
 * Callback function invoked by bsdata used to lock the record and apply the
 * timestamp
 */
void lock_record(bsread::BSDataChannel* chan, bool acquire, void* pvt){
    struct dbAddr* rec_address = static_cast<dbAddr*>(pvt);
    struct dbCommon* precord = rec_address->precord;

    if(acquire){
        dbScanLock(precord);

        chan->set_timestamp(precord->time.secPastEpoch + 631152000u, precord->time.nsec);

        /* EPICS strings are a bit silly since they are statich char[40]
         * allocations, so we always need to check the actual length
         * of the string first... Its a bit of a performance hit, but what can you do...*/
        if(rec_address->field_type == DBR_STRING){
            chan->set_data(rec_address->pfield,strlen( static_cast<char*>(rec_address->pfield) ));
        }
    }
    else{
        dbScanUnlock(precord);
    }
}




bsread::BSRead *epicsBSRead::get_instance(const char *name)
{
    epicsGuard<epicsMutex> guard(g_bsread_mtx);
    if(g_bsread_inst.count(name)){
        return g_bsread_inst[name];
    }

    else{
        throw std::runtime_error("BSREAD instance does not exist!");
    }
}

void epicsBSRead::register_instance(const char *name, bsread::BSRead* inst){
    bsread_debug(1,"Creating new BSREAD instance...");
    epicsGuard<epicsMutex> guard(g_bsread_mtx);
    if(g_bsread_inst.count(name)){
        throw std::runtime_error("BSREAD instance already exist");
    }

    g_bsread_inst[name]=inst;
}

long epicsBSRead::bsread_add_epics_records(bsread::BSRead *instance)
{
    bsread_debug(3,"bsread_add_epics_records: adding records to bsread");
    DBENTRY dbentry;
    DBENTRY *pdbentry = &dbentry;
    long status;


    if (!pdbbase) {
        printf("No database loaded");
        return 0;
    }

    dbInitEntry(pdbbase, pdbentry);
    status = dbFirstRecordType(pdbentry);
    while (!status) {
        status = dbFirstRecord(pdbentry);
        while (!status) {
            char *pname = dbGetRecordName(pdbentry);
            struct dbAddr rec_address;
            bsread::bsdata_type type;

            dbNameToAddr(pname, &(rec_address));

            size_t n_elements = rec_address.no_elements;

            // determine if the DBR type is supported
            if(rec_address.dbr_field_type == DBR_DOUBLE){
                type=bsread::BSDATA_FLOAT64;
            }
            else if(rec_address.dbr_field_type == DBR_FLOAT){
                type=bsread::BSDATA_FLOAT32;
            }
            else if(rec_address.dbr_field_type == DBR_STRING){
                type=bsread::BSDATA_STRING;
                // Epics strings are fixed, 40 character arrays.
                n_elements = 40;
            }
            else if(rec_address.dbr_field_type == DBR_LONG){
                type=bsread::BSDATA_INT32;
            }
            else if(rec_address.dbr_field_type == DBR_ULONG){
                type=bsread::BSDATA_UINT32;
            }
            else if(rec_address.dbr_field_type == DBR_SHORT){
                type=bsread::BSDATA_INT16;
            }
            else if(rec_address.dbr_field_type == DBR_USHORT){
                type=bsread::BSDATA_UINT16;
            }
            else if(rec_address.dbr_field_type == DBR_CHAR){
                type=bsread::BSDATA_INT8;
            }
            else if(rec_address.dbr_field_type == DBR_UCHAR){
                type=bsread::BSDATA_UINT8;
            }
            else if(rec_address.dbr_field_type == DBR_ENUM){
                type=bsread::BSDATA_UINT16;
            }
            else{
                errlogPrintf("BSREAD: Channel %s has unsuporrted type: %d\n",pname, rec_address.dbr_field_type); // TODO Need to throw exception
                status = dbNextRecord(pdbentry);
                continue;
            }

            //Add a record to bsread instance
            bsread::BSDataChannel* chan = new bsread::BSDataChannel(pname,type);
            chan->set_data(rec_address.pfield, n_elements);
            chan->set_callback(lock_record,new struct dbAddr(rec_address));

            bsread_debug(4,"bsread_add_epics_records: adding channel %s with %ld no_elements",pname,rec_address.no_elements);
            instance->add_channel(chan);


            status = dbNextRecord(pdbentry);
        }
        status = dbNextRecordType(pdbentry);
    }

    dbFinishEntry(pdbentry);
    return 0;
}

void epicsBSRead::initHook(initHookState state)
{

    if(state==initHookAfterIocBuilt){
        epicsGuard<epicsMutex> guard(g_bsread_mtx);

        for(std::map<std::string,bsread::BSRead*>::iterator it = g_bsread_inst.begin(); it != g_bsread_inst.end();++it){
            bsread_debug(1,"Registering records to %s",it->first.c_str());
            epicsBSRead::bsread_add_epics_records(it->second);
        }

    }
}



extern "C"{
    epicsExportAddress(int,bsread_debug);
}


//#### bsreadConfigure #####
static const iocshArg bsreadConfigureArg0 = { "name",iocshArgString};
static const iocshArg bsreadConfigureArg1 = { "port",iocshArgInt};
static const iocshArg bsreadConfigureArg2 = { "type",iocshArgString};
static const iocshArg bsreadConfigureArg3 = { "hwm",iocshArgInt};

static const iocshArg *const bsreadConfigureArgs[4] =
    {&bsreadConfigureArg0,&bsreadConfigureArg1,&bsreadConfigureArg2,&bsreadConfigureArg3};

static const iocshFuncDef bsreadConfigureFuncDef =
    {"bsreadConfigure",4,bsreadConfigureArgs};

static void bsreadConfigFunc(const iocshArgBuf *args)
{

    if(!args[0].sval | !args[1].sval | !args[2].sval){
        printf("not enough arguments\n");
        return;
    }

    char* name = args[0].sval;

    int socket_type=-1;
    if(!strcmp(args[2].sval,"PUSH")) socket_type = ZMQ_PUSH;
    if(!strcmp(args[2].sval,"PUB")) socket_type = ZMQ_PUB;

    if(socket_type == -1){
        errlogPrintf("Invalid type arguemnt, avialbale socket types: PUSH, PUB\n");
        return;
    }

    char zmq_addr_buff[255];
    bsread::BSRead* bsread_inst=0;
    try{

        bsread_inst = new bsread::BSRead();
//        epicsBSRead::bsread_add_epics_records(bsread_inst);

        epicsSnprintf(zmq_addr_buff,255,"tcp://*:%d",args[1].ival);
        epicsPrintf("BSREAD[%s]: Configuring ZMQ to %s\n",name,zmq_addr_buff);

        int hwm = args[3].ival;
        if (hwm <= 0) hwm = 10;
        bsread_inst->confiugre_zmq(zmq_addr_buff,socket_type,hwm);


        epicsSnprintf(zmq_addr_buff,255,"tcp://*:%d",args[1].ival+1);
        epicsPrintf("BSREAD[%s]: Configuring ZMQ RPC to %s\n",name,zmq_addr_buff);
        bsread_inst->confiugre_zmq_config(zmq_addr_buff);

        epicsBSRead::register_instance(name,bsread_inst);
    }
    catch(std::runtime_error& e){
        delete bsread_inst;
        errlogPrintf("BSREAD: Could not configure BSREAD: %s due to %s\nBSREAD: Aborting!\n",name,e.what());
    }    

//    bsread->enable_all_channels();

}

//#### bsreadApplyConfig #####
static const iocshArg bsreadApplyConfigArg0 = { "name",iocshArgString};
static const iocshArg bsreadApplyConfigArg1 = { "filename",iocshArgString};

static const iocshArg *const bsreadApplyConfigArgs[2] =
    {&bsreadApplyConfigArg0,&bsreadApplyConfigArg1};

static const iocshFuncDef bbsreadApplyConfigFuncDef =
    {"bsreadApply",2,bsreadApplyConfigArgs};

static void bsreadApplyConfigFunc(const iocshArgBuf *args)
{

    if(!args[0].sval | !args[1].sval){
        printf("not enough arguments\n");
        return;
    }

    try{

        string instance_name = std::string(args[0].sval);
        string filename = string(args[1].sval);

        //Fetch instance
        bsread::BSRead* inst = epicsBSRead::get_instance(instance_name.c_str());

        //Open file
        epicsPrintf("BSREAD: Opening file %s\n",filename.c_str());
        std::ifstream t(filename.c_str());
        std::stringstream buffer;
        buffer << t.rdbuf();

        //Parse file
        inst->configure(buffer.str());
    }
     catch(std::runtime_error& e){
        errlogPrintf("BSREAD: Could not configure BSREAD due to:\n\t %s\n",e.what());
    }

    epicsPrintf("Successfully applied configuration\n");

}



//#### bsreadInfo #####
static const iocshArg bsreadInfoArg0 = { "name",iocshArgString};


static const iocshArg *const bsreadInfoArgs[1] =
    {&bsreadApplyConfigArg0};

static const iocshFuncDef bsreadInfoFuncDef =
    {"bsreadInfo",1,bsreadInfoArgs};

static void bsreadInfoFunc(const iocshArgBuf *args)
{

    if(!args[0].sval){
        printf("not enough arguments\n");
        return;
    }

    try{

        string instance_name = std::string(args[0].sval);
        //Fetch instance
        bsread::BSRead* inst = epicsBSRead::get_instance(instance_name.c_str());

        Json::Value config = inst->generate_json_config();

        epicsPrintf("Current config:\n");
        epicsPrintf("%s",config.toStyledString().c_str());

        epicsPrintf("\nCurrent params:\n");
        epicsPrintf("\tinhibit: %d\n",inst->get_inhibit());

        epicsPrintf("\nCurrent status:\n");
        epicsPrintf("\tZMQ overflows: %u\n",inst->zmq_overflows());





    }
     catch(std::runtime_error& e){
        errlogPrintf("BSREAD: Could not configure BSREAD due to:\n\t %s\n",e.what());
    }

}



//BSREAD zmq configure
static int bsreadconfigureRegister(void)
{
    initHookRegister(&epicsBSRead::initHook);
    iocshRegister(&bsreadConfigureFuncDef,bsreadConfigFunc);
    iocshRegister(&bbsreadApplyConfigFuncDef,bsreadApplyConfigFunc);
    iocshRegister(&bsreadInfoFuncDef,bsreadInfoFunc);

    return 1;
}

extern "C" {
	epicsExportRegistrar(bsreadconfigureRegister);
}

