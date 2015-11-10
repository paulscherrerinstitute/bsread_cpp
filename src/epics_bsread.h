#ifndef EPICS_BSREAD_H
#define EPICS_BSREAD_H

#include "bsread.h"
#include "initHooks.h"


/**
 * @brief The epicsBSRead class Glue code needed to bind
 * tech. agnostic bsread to EPICS IOC. The class handles
 * instances of bsread (creation and retrival, delition
 * is irrelevant within IOC scope).
 *
 * Most important method is bsread_add_epics_records that
 * registers all currently available records into bsread.
 *
 * note that epics_bsread.cc also contains iocsh functions
 */
class epicsBSRead{
public:
    static epicsMutex g_bsread_mtx;
    static std::map<std::string, bsread::BSRead *> g_bsread_inst;

    /**
     * @brief get_instance retrive existing bsread instance
     * @param name
     * @return
     */
    static bsread::BSRead* get_instance(const char *name);


    /** Create BSREAD instance **/
    static void register_instance(const char* name, bsread::BSRead *inst);


    /**
     * @brief bsread_add_epics_records intializes a bsread instance with all available epics channels
     * @param instance
     * @return
     */
    static long bsread_add_epics_records(bsread::BSRead* instance);

    static void initHook(initHookState state);

};

#endif // EPICS_BSREAD_H

