#ifndef XMPPCLI_CWMP_PARAMETERS_H
#define XMPPCLI_CWMP_PARAMETERS_H
#include "hnt_interface.h"

typedef struct deviceGeneralParameter_s {
    char * paramName;
    int (*getParameterFunc)(char *, char *, int);    
    int (*setParameterFunc)(char *, char *);
}deviceGeneralParameter_t;

typedef struct {
    deviceParameter_t *table;
    int tableSize;
}deviceParameterTable_t;

typedef int (*hnt_xmpp_slavedev_upgrade_process_func)(void);
void 
cwmp_datatype_set_bool(char *value, uint8_t data);
void 
cwmp_datatype_set_uint(char *value, uint32_t data);

#endif
