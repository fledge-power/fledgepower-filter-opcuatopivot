#ifndef INCLUDE_PLUGIN_DEFAULT_CONF_H_
#define INCLUDE_PLUGIN_DEFAULT_CONF_H_
/*
 * Fledge "Log" filter plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Jeremie Chabod
 */

// System headers
#include <string>

// Fledge includes
#include "plugin_api.h"

#define FILTER_NAME "pivottoopcua"   // NOLINT //NOSONAR

// PLUGIN DEFAULT EXCHANGED DATA CONF
#define EXCHANGED_DATA_DEF                   \
    QUOTE({                                  \
        "exchanged_data" : {                 \
            "name" : "opcua",         \
            "version" : "1.0",               \
            "datapoints" : [                  \
                {\
                   "label":"labelSPS", \
                   "pivot_id":"pivotSPS", \
                   "pivot_type":"SpsTyp", \
                   "protocols":[\
                      {\
                         "name":"iec104", \
                         "address":"18325-6468171", \
                         "typeid":"M_SP_TB_1", \
                         "gi_groups":"station"\
                      }, \
                      {\
                         "name":"opcua", \
                         "address":"sps", \
                         "typeid":"opcua_sps"\
                      }\
                   ]\
                }\
                , \
                {\
                   "label":"labelDPS", \
                   "pivot_id":"pivotDPS", \
                   "pivot_type":"DpsTyp", \
                   "protocols":[\
                      {\
                         "name":"opcua", \
                         "address":"dps", \
                         "typeid":"opcua_dps"\
                      }\
                   ]\
                }\
                , \
                {\
                   "label":"labelMVI", \
                   "pivot_id":"pivotMVI", \
                   "pivot_type":"MvTyp", \
                   "protocols":[\
                      {\
                         "name":"opcua", \
                         "address":"mvi", \
                         "typeid":"opcua_mvi"\
                      }\
                   ]\
                }\
                , \
                {\
                   "label":"labelMVF", \
                   "pivot_id":"pivotMVF", \
                   "pivot_type":"MvTyp", \
                   "protocols":[\
                      {\
                         "name":"opcua", \
                         "address":"mvf", \
                         "typeid":"opcua_mvf"\
                      }\
                   ]\
                }\
                , \
                {\
                   "label":"labelDPC", \
                   "pivot_id":"pivotDPC", \
                   "pivot_type":"DpcTyp", \
                   "protocols":[\
                      {\
                         "name":"opcua", \
                         "address":"dpc", \
                         "typeid":"opcua_dpc"\
                      }\
                   ]\
                }\
            ]  \
        }  \
    })

#endif  //INCLUDE_PLUGIN_DEFAULT_CONF_H_
