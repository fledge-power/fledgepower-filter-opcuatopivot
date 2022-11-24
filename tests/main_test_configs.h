/*
 * Fledge north service plugin (TESTS)
 *
 * Copyright (c) 2021 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Jeremie Chabod
 */

#ifndef INCLUDE_FLEDGE_FILTER_PIVOT2OPCUA_TESTS_MAIN_CONFIGS_H_
#define INCLUDE_FLEDGE_FILTER_PIVOT2OPCUA_TESTS_MAIN_CONFIGS_H_

// System includes
#include <sys/wait.h>
#include <fstream>
#include <string>
#include <regex>

// Fledge / tools  includes
#include <plugin_api.h>
#include <datapoint.h>
#include <gtest/gtest.h>

#include "pivot2opcua_common.h"

///////////////////////
// helpful test macros

#define TRACE(...) LOG_WARNING("[TEST]: " __VA_ARGS__)
#define TITLE(...) do { LOG_WARNING("-------------------------------"); \
    LOG_WARNING("[TEST]: " __VA_ARGS__);} while (0)

#define ASSERT_STR_CONTAINS(s1,s2) ASSERT_NE(s1.find(s2), string::npos);
#define ASSERT_STR_NOT_CONTAINS(s1,s2) ASSERT_EQ(s1.find(s2), string::npos);

#define WAIT_UNTIL(c, mtimeoutMs) do {\
        int maxwaitMs(mtimeoutMs);\
        do {\
            this_thread::sleep_for(chrono::milliseconds(10));\
            maxwaitMs -= 10;\
        } while (!(c) && maxwaitMs > 0);\
    } while(0)


//////////////////////////////////////
// TEST HELPER CLASS

//////////////////////////////////////
// TEST HELPER FUNCTIONS
using Readings = std::vector<Reading *>;
using Datapoints = std::vector<Datapoint *>;

namespace {
void debugJSON(const std::string& json) {
    LOG_INFO("DEBUG JSON :'%s'", json.c_str());
    Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError()) {
        LOG_ERROR("Parse error: Code : %d, at offset %u near '%.15s'",
                doc.GetParseError(),
                doc.GetErrorOffset(),
                json.c_str() + doc.GetErrorOffset()
        );
        throw new ReadingSetException("Unable to parse results json document");
    }
}

inline std::string replace_in_string(
        const std::string& ref, const std::string& old, const std::string& New) {
    return std::regex_replace(ref, std::regex(old), New);
}

void Debug_Dump (Datapoint* datapoint, const string& depth= "  ") {
    if (datapoint ==nullptr) {
        LOG_DEBUG("     ==> %s- !NULL!", depth.c_str());
        return;
    }
    string repr("?Unknown?");
    const string & name(datapoint->getName());
    DatapointValue & value(datapoint->getData());
    int idx(0);
    switch (value.getType()) {
    case DatapointValue::T_STRING: repr = value.toStringValue(); break;
    case DatapointValue::T_INTEGER: repr = std::to_string(value.toInt()); break;
    case DatapointValue::T_FLOAT: repr = std::to_string(value.toDouble()); break;
    case DatapointValue::T_DP_DICT: repr = "{}"; break;
    case DatapointValue::T_DP_LIST: repr = "[]"; break;
    default:break;
    }
    LOG_DEBUG("     ==> %s- %s = %s", depth.c_str(), name.c_str(), repr.c_str());

    switch (value.getType()) {
    case DatapointValue::T_DP_LIST:
    case DatapointValue::T_DP_DICT:
        for (Datapoint* item : *value.getDpVec()) {
            Debug_Dump(item, depth+"|");
        }
        break;
    default:break;
    }
}

// Recursive function
Datapoint* DocToDataPoint(const string& name, Value & value, const string& depth= "  ") {
    std::unique_ptr<DatapointValue> dpv(nullptr);
    Datapoints* v(nullptr);
    int idx(0);
    // LOG_DEBUG("     ==> %s- %s", depth.c_str(), name.c_str());
    switch (value.GetType()) {
    case kFalseType:
        dpv.reset(new DatapointValue(0l));
        break;
    case kTrueType:
        dpv.reset(new DatapointValue(1l));
        break;
    case kObjectType:
        v = new Datapoints;
        for (Value::MemberIterator subIt = value.MemberBegin(); subIt != value.MemberEnd(); subIt++) {
            Datapoint* item(DocToDataPoint(subIt->name.GetString(), subIt->value, depth+"|"));
            v->push_back(item);
        }
        dpv.reset(new DatapointValue(v, true));
        break;
    case kArrayType:
        v = new Datapoints;
        idx = 0;
        for (Value::ValueIterator subIt = value.Begin(); subIt != value.End(); subIt++) {
            idx++;
            Value& value (*subIt);
            string idxName(string ("#") + std::to_string(idx));
            Datapoint* item(DocToDataPoint(idxName, value, depth+"|"));
            v->push_back(item);
        }
        dpv.reset(new DatapointValue(v, false));
        break;
    case kStringType:
        dpv.reset(new DatapointValue(value.GetString()));
        break;
    case kNumberType:
        if (value.IsInt()) dpv.reset(new DatapointValue((long)value.GetInt()));
        else if (value.IsUint()) dpv.reset(new DatapointValue((long)value.GetUint()));
        else if (value.IsInt64()) dpv.reset(new DatapointValue((long)value.GetInt64()));
        else if (value.IsFloat()) dpv.reset(new DatapointValue(value.GetFloat()));
        else {}
        break;
    default:break;
    }
    if (nullptr == dpv) return nullptr;
    return new Datapoint(name, *dpv);
}

Readings JsonToReading(const std::string& json, const string& asset_name) {
    Readings result;
    Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError()) {
        LOG_ERROR("Parse error: Code : %d, at offset %u near '%.20s'",
                doc.GetParseError(),
                doc.GetErrorOffset(),
                json.c_str() + doc.GetErrorOffset()
        );
        throw std::exception();
    }
    Value::MemberIterator it;

    for (it = doc.MemberBegin(); it != doc.MemberEnd(); it++) {
        const string name = it->name.GetString();
        Value& value (it->value);
        Datapoint* newDp(DocToDataPoint(name, value));
        result.push_back(new Reading(asset_name, newDp));
    }
    return result;
}

DatapointValue* get_datapoint_by_key(Datapoints* dps, const string& s) {
    Datapoints::const_iterator it = find_if(dps->begin(), dps->end(),
            [s] (const Datapoint* dp) { return dp->getName() == s; } );
    if (it == dps->end()) {return nullptr;}
    Datapoint* res(*it);
    return &res->getData();
}

inline DatapointValue* getFieldResult(ReadingSet& rSet, const string& key) {
    Datapoints& dp(rSet.getAllReadingsPtr()->back()->getReadingData());
    Datapoints* do_dp(dp.front()->getData().getDpVec());
    return get_datapoint_by_key(do_dp, key);
}

inline DatapointValue* getDoResult(ReadingSet& rSet) {
    Datapoints& dp(rSet.getAllReadingsPtr()->back()->getReadingData());
    return get_datapoint_by_key(&dp, "data_object");
}


}   // namespace


#define Json_ExDataOK QUOTE(\
        {"exchanged_data" : {\
            "name" : "data1",\
            "version" : "1.0",\
            "datapoints" : [\
            {\
               "label" : "label1",\
               "pivot_id" : "pivot1",\
               "pivot_type": "type1",\
               "protocols":[\
                  {\
                   "name":"iec104",\
                   "address":"18325-6441925",\
                   "typeid":"C_DC_TA_1"\
                  },\
                  {\
                    "name":"opcua",\
                    "address":"addr1",\
                    "typeid":"opcua_dps"\
                   }\
                 ]\
               },\
              {\
                 "label" : "labelSPS",\
                 "pivot_id" : "pivotSPS",\
                 "pivot_type": "typeSPS",\
                 "protocols":[\
                    {\
                      "name":"opcua",\
                      "address":"sps",\
                      "typeid":"opcua_sps"\
                     }\
                  ]\
               }, \
               {\
                  "label" : "labelDPS",\
                  "pivot_id" : "pivotDPS",\
                  "pivot_type": "typeDPS",\
                  "protocols":[\
                     {\
                       "name":"opcua",\
                       "address":"dps",\
                       "typeid":"opcua_dps"\
                      }\
                  ]\
               }, \
               {\
                  "label" : "labelMVF",\
                  "pivot_id" : "pivotMVF",\
                  "pivot_type": "typeMVF",\
                  "protocols":[\
                     {\
                       "name":"opcua",\
                       "address":"mvf",\
                       "typeid":"opcua_mvf"\
                      }\
                    ]\
                }, \
                {\
                   "label" : "labelMVI",\
                   "pivot_id" : "pivotMVI",\
                   "pivot_type": "typeMVI",\
                   "protocols":[\
                      {\
                        "name":"opcua",\
                        "address":"mvi",\
                        "typeid":"opcua_mvi"\
                       }\
                     ]\
                   },\
                {\
                 "label" : "label2",\
                 "pivot_id" : "pivot2",\
                 "pivot_type": "type2",\
                 "protocols":[\
                   {\
                     "name":"opcua",\
                     "address":"addr2",\
                     "typeid":"opcua_sps"\
                    }\
                   ]\
                 },\
                 {\
                  "label" : "labelSPC",\
                  "pivot_id" : "pivotSPC",\
                  "pivot_type": "typeSPC",\
                  "protocols":[\
                    {\
                      "name":"opcua",\
                      "address":"spc",\
                      "typeid":"opcua_spc"\
                     }\
                    ]\
                  },\
                  {\
                   "label" : "labelAPC",\
                   "pivot_id" : "pivotAPC",\
                   "pivot_type": "typeAPC",\
                   "protocols":[\
                     {\
                       "name":"opcua",\
                       "address":"apc",\
                       "typeid":"opcua_apc"\
                      }\
                     ]\
                   },\
                   {\
                    "label" : "labelINC",\
                    "pivot_id" : "pivotINC",\
                    "pivot_type": "typeINC",\
                    "protocols":[\
                      {\
                        "name":"opcua",\
                        "address":"inc",\
                        "typeid":"opcua_inc"\
                       }\
                      ]\
                    },\
                   {\
                    "label" : "labelDPC",\
                    "pivot_id" : "pivotDPC",\
                    "pivot_type": "typeDPC",\
                    "protocols":[\
                      {\
                        "name":"opcua",\
                        "address":"dpc",\
                        "typeid":"opcua_dpc"\
                       }\
                      ]\
                    }\
             ]\
        }})

//////////////////////////////////////
// TEST CONFIGURATIONS
static const char * const JsonPlugConfDisabled =
    QUOTE({
        "plugin" : {
                "description" : "PIVOT to OPCUA filter plugin",
                "type" : "string",
                "default" : "pivot2opcua",
                "readonly": "true"
                },
        "enable": {
               "description": "A switch that can be used to enable or disable execution of the log filter.",
               "type": "boolean",
               "displayName": "Enabled",
               "default": "false"
               },
        "exchanged_data": {
                "description" : "exchanged data list",
                "type" : "string",
                "displayName" : "Exchanged data list",
                "order" : "3",
                "default" : Json_ExDataOK
               },
        "match" : {
                "description" : "An optional regular expression to match in the asset name.",
                "type": "string",
                "default": "",
                "order": "1",
                "displayName": "Asset filter"}
        });

static const char * const JsonPlugConfOK =
    QUOTE({
        "plugin" : {
                "description" : "PIVOT to OPCUA filter plugin",
                "type" : "string",
                "default" : "pivot2opcua",
                "readonly": "true"
                },
        "enable": {
               "description": "A switch that can be used to enable or disable execution of the log filter.",
               "type": "boolean",
               "displayName": "Enabled",
               "default": "true"
               },
        "exchanged_data": {
                "description" : "exchanged data list",
                "type" : "string",
                "displayName" : "Exchanged data list",
                "order" : "3",
                "default" : Json_ExDataOK
               },
        "match" : {
                "description" : "An optional regular expression to match in the asset name.",
                "type": "string",
                "default": "",
                "order": "1",
                "displayName": "Asset filter"}
        });

#define JSON_DEF_Q_QUALITY "q" : {  \
    "DetailQuality": {                  \
        "badReference": false,           \
        "failure": false,                \
        "inconsistent": false,           \
        "innacurate": false,             \
        "oldData": false,                \
        "oscillatory": false,            \
        "outOfRange": false,             \
        "overflow": false                \
    },                                  \
    "Source": "process",                \
    "Validity": "good",                 \
    "operatorBlocked": false,            \
    "test": false                        \
}

#define JSON_DEF_T_QUALITY(x) "t" : {   \
    "FractionOfSecond": 1,                \
    "SecondSinceEpoch": x,         \
    "TimeQuality": {                      \
        "clockFailure": false,            \
        "clockNotSynchronized": false,    \
        "leapSecondKnown": false,         \
        "timeAccuracy": 10                \
    }                                     \
}

static const char * const JsonPivotMvf =
    QUOTE({
    "PIVOT" :{
        "GTIM": {
            "Cause": {"stVal": 1},
            "Confirmation": {"stVal": true},
            "ComingFrom":  {"stVal": "iec104"},
            "Identifier":  "pivotMVF",
            "TmOrg" :  {"stVal":"genuine"},
            "TmValidity" :  {"stVal":"questionable"},
            "MvTyp" : {
                JSON_DEF_Q_QUALITY,
                JSON_DEF_T_QUALITY(12345678),
                "mag" : { "f" : 3.14, "i" : 42}
            }
        }
    }
});

static const char * const JsonPivotMvi =
        QUOTE({
    "PIVOT" :{
        "GTIM": {
            "Cause": {"stVal": 4},
            "Confirmation": {"stVal": false},
            "ComingFrom":  {"stVal": "opcua"},
            "Identifier":  "pivotMVI",
            "TmOrg" :  {"stVal":"substituted"},
            "TmValidity" :  {"stVal":"good"},
            "MvTyp" : {
                JSON_DEF_Q_QUALITY,
                JSON_DEF_T_QUALITY(12345679),
                "mag" : { "f" : 7.42, "i" : 10}
            }
        }
    }
});

static const char * const JsonPivotSps =
        QUOTE({
    "PIVOT" :{
        "GTIS": {
            "Cause": {"stVal": 4},
            "Confirmation": {"stVal": false},
            "ComingFrom":  {"stVal": "opcua"},
            "Identifier":  "pivotSPS",
            "TmOrg" :  {"stVal":"substituted"},
            "TmValidity" :  {"stVal":"good"},
            "SpsTyp" : {
                JSON_DEF_Q_QUALITY,
                JSON_DEF_T_QUALITY(12345678),
                "stVal" : true
            }
        }
    }
});

static const char * const JsonPivotDps =
        QUOTE({
    "PIVOT" :{
        "GTIS": {
            "Cause": {"stVal": 4},
            "Confirmation": {"stVal": false},
            "ComingFrom":  {"stVal": "opcua"},
            "Identifier":  "pivotDPS",
            "TmOrg" :  {"stVal":"substituted"},
            "TmValidity" :  {"stVal":"good"},
            "DpsTyp" : {
                JSON_DEF_Q_QUALITY,
                JSON_DEF_T_QUALITY(12345678),
                "stVal" : "on"
            }
        }
    }
});

static const char * const JsonPivotDps2 =
        QUOTE({
    "PIVOT" :{
        "GTIS": {
            "Origin": {"stVal": 4},
            "ChgValCnt": {"stVal": 4},
            "NormalSrc": {"stVal": 4},
            "NormalVal": {"stVal": 4},
            "Cause": {"stVal": 4},
            "Confirmation": {"stVal": false},
            "ComingFrom":  {"stVal": "opcua"},
            "Identifier":  "pivotDPS",
            "TmOrg" :  {"stVal":"substituted"},
            "TmValidity" :  {"stVal":"good"},
            "DpsTyp" : {
                JSON_DEF_Q_QUALITY,
                JSON_DEF_T_QUALITY(12345678),
                "stVal" : "on"
            }
        }
    }
});


#endif /* INCLUDE_FLEDGE_FILTER_PIVOT2OPCUA_TESTS_MAIN_CONFIGS_H_ */
