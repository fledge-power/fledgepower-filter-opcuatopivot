/*
 * Fledge "Log" filter plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */

#include "pivot2opcua_data.h"

// System headers
#include <vector>
#include <algorithm>

// Fledge headers
#include "logger.h"
#include "reading_set.h"
#include "rapidjson/document.h"

// Project headers
#include "pivot2opcua_common.h"
#include "pivot2opcua_rules.h"

using rapidjson::Value;
using std::string;
using std::vector;

namespace {

/**************************************************************************/
string getString(const Value& value,
        const char* section, const string& context) {
    ASSERT(value.HasMember(section), "Missing STRING '%s' in '%s'",
            section, context.c_str());
    const Value& object(value[section]);
    ASSERT(object.IsString(), "Error :'%s' in '%s' must be an STRING",
            section, context.c_str());
    return object.GetString();
}

/**************************************************************************/
const Value& getObject(const Value& value,
        const char* section, const string& context) {
    ASSERT(value.HasMember(section), "Missing OBJECT '%s' in '%s'",
            section, context.c_str());
    const Value& object(value[section]);
    ASSERT(object.IsObject(), "Error :'%s' in '%s' must be an OBJECT",
            section, context.c_str());
    return object;
}

/**************************************************************************/
Value::ConstArray getArray(const Value& value,
        const char* section, const string& context) {
    ASSERT(value.HasMember(section), "Missing ARRAY '%s' in '%s'",
            section, context.c_str());
    const Value& object(value[section]);
    ASSERT(object.IsArray(), "Error :'%s' in '%s' must be an ARRAY",
            section, context.c_str());
    return object.GetArray();
}

}   // namespace


/**************************************************************************/
ExchangedDataC::
ExchangedDataC(const rapidjson::Value& json):
mPreCheck(internalChecks(json)),
address(json[JSON_PROT_ADDR].GetString()),
typeId(json[JSON_PROT_TYPEID].GetString()) {
}

bool
ExchangedDataC::internalChecks(const rapidjson::Value& json)const {
    ASSERT(json.IsObject(), "datapoint protocol description must be JSON");
    ASSERT(json.HasMember(JSON_PROT_NAME) && json[JSON_PROT_NAME].IsString()
            , "datapoint protocol description must have a 'name' key defining a STRING");
    const string protocolName(json[JSON_PROT_NAME].GetString());
    if (protocolName != PROTOCOL_S2OPC) {
        throw NotAnS2opcInstance();
    }
    ASSERT(json.HasMember(JSON_PROT_ADDR) && json[JSON_PROT_ADDR].IsString()
            , "datapoint protocol description must have a '%s' key defining a STRING",
            JSON_PROT_ADDR);
    ASSERT(json.HasMember(JSON_PROT_TYPEID) && json[JSON_PROT_TYPEID].IsString()
            , "datapoint protocol description must have a '%s' key defining a STRING",
            JSON_PROT_TYPEID);
    return true;
}

/**************************************************************************/
DataDictionnary::
DataDictionnary(const string& jsonExData) {
    rapidjson::Document doc;
    doc.Parse(jsonExData.c_str());
    ASSERT(!doc.HasParseError(),
            "Malformed JSON (section '%s', index = %u, near %50s)",
            JSON_EXCHANGED_DATA, doc.GetErrorOffset(),
            jsonExData.c_str() + doc.GetErrorOffset());

    // Parse "exchanged_data" section
    const Value& exData(::getObject(doc, JSON_EXCHANGED_DATA, JSON_EXCHANGED_DATA));
    const Value::ConstArray datapoints(::getArray(exData, JSON_DATAPOINTS, JSON_EXCHANGED_DATA));

    // Check version compatibility
    const string version(::getString(exData, JSON_VERSION, JSON_EXCHANGED_DATA));
    ASSERT(version == EXCH_DATA_VERSION,
            "Incompatible '%s' version. Found '%s', expected '%s'",
            JSON_EXCHANGED_DATA, version.c_str(), EXCH_DATA_VERSION);

    // Parse "datapoints" section
    for (const Value& datapoint : datapoints) {
        const string label(::getString(datapoint, JSON_LABEL, JSON_DATAPOINTS));
        LOG_DEBUG("Parsing DATAPOINT(%s)", label.c_str());
        const string pivot_id(::getString(datapoint, JSON_PIVOT_ID, JSON_DATAPOINTS));
        const string pivot_type(::getString(datapoint, JSON_PIVOT_TYPE, JSON_DATAPOINTS));
        const Value::ConstArray& protocols(getArray(datapoint, JSON_PROTOCOLS, JSON_DATAPOINTS));

        for (const Value& protocol : protocols) {
            try {
                // Check for matching protocol ("s2opcua")
                // ExchangedDataC will throw if protocol does not match
                const ExchangedDataC data(protocol);

                // Insert element
                LOG_INFO("Add Pivot id '%s' : {'%s', '%s', '%s'}",
                        pivot_id.c_str(),
                        pivot_type.c_str(), data.address.c_str(), data.typeId.c_str());
                m_map.emplace(std::make_pair(pivot_id, PivotElement(pivot_type, data.typeId)));
            }
            catch (const ExchangedDataC::NotAnS2opcInstance&) {     // //NOSONAR
                // Just ignore other protocols
            }
        }
    }
}
