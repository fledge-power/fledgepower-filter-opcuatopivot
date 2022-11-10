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

#define FILTER_NAME "pivot2opcua"   // NOLINT //NOSONAR

// PLUGIN DEFAULT EXCHANGED DATA CONF
#define EXCHANGED_DATA_DEF                   \
    QUOTE({                                  \
        "exchanged_data" : {                 \
            "name" : "opcua",         \
            "version" : "1.0",               \
            "datapoints" : [                  \
                {                            \
                    "label":"LabelSpsTyp",   \
                    "pivot_id" : "S114562128", \
                    "pivot_type" : "SpsTyp",        \
                    "protocols" :[ \
                      {\
                         "name":"iec104", \
                         "address":"18325-6468171", \
                         "typeid":"M_SP_TB_1", \
                         "gi_groups":"station"\
                      }, \
                      {\
                         "name":"opcua", \
                         "address":"S_1145_6_21_28", \
                         "typeid":"opcua_sps"\
                      }\
                    ] \
                },  \
                {                            \
                    "label":"LabelDpsTyp",   \
                    "pivot_id" : "S114562127", \
                    "pivot_type" : "SpsTyp",        \
                    "protocols" :[ \
                      {\
                         "name":"opcua", \
                         "address":"S_1145_6_21_27", \
                         "typeid":"opcua_dps"\
                      }\
                    ] \
                },  \
                {                            \
                    "label":"LabelMvaTyp",   \
                    "pivot_id" : "S114562129", \
                    "pivot_type" : "MvTyp",        \
                    "protocols" :[ \
                      {\
                         "name":"opcua", \
                         "address":"S_1145_6_21_29", \
                         "typeid":"opcua_mva"\
                      }\
                    ] \
                },  \
                {                            \
                    "label":"LabelMvfTyp",   \
                    "pivot_id" : "S114562130", \
                    "pivot_type" : "MvTyp",        \
                    "protocols" :[ \
                      {\
                         "name":"opcua", \
                         "address":"S_1145_6_21_30", \
                         "typeid":"opcua_mvf"\
                      }\
                    ] \
                },  \
                {                            \
                    "label":"LabelMvfTyp2",   \
                    "pivot_id" : "S114562131", \
                    "pivot_type" : "MvTyp",        \
                    "protocols" :[ \
                      {\
                         "name":"opcua", \
                         "address":"S_1145_6_21_31", \
                         "typeid":"opcua_mvf"\
                         "mapping_rule":"PtoMVF_round"\
                      }\
                    ] \
                }  \
            ]  \
        }  \
    })

#endif  //INCLUDE_PLUGIN_DEFAULT_CONF_H_
