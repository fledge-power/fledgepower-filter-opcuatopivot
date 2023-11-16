/*
 * Fledge "Log" filter plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */

#include "pivot2opcua_filter.h"

// System headers
#include <time.h>
#include <memory>
#include <regex>
#include <mutex>

// Fledge headers
#include "asset_tracking.h"
#include "datapoint.h"

// Project headers
#include "pivot2opcua_common.h"
#include "pivot2opcua_rules.h"
#include "pivot2opcua_data.h"

//SOP2C headers
extern "C" {
#include "opcua_statuscodes.h"
}
using std::regex;
using std::mutex;
using std::unique_ptr;

namespace {
using Datapoints = vector<Datapoint*>;

static const string stValName = "stVal";   // //NOLINT

/* HELPER FUNCTIONS*/

static Datapoint*
createDatapoint(const string& name)
{
    Datapoints* datapoints = new Datapoints;
    DatapointValue dpv(datapoints, true);
    return new Datapoint(name, dpv);
}

static Datapoint*
datapointAddElement(Datapoint* dp, const string& name)
{
    DatapointValue& dpv = dp->getData();
    Datapoints* subDatapoints = dpv.getDpVec();
    Datapoint* element = createDatapoint(name);

    if (element) {
       subDatapoints->push_back(element);
    }

    return element;
}

static Datapoint*
createDpWithValue(const string& name, const long value)
{
    DatapointValue dpv(value);
    Datapoint* dp = new Datapoint(name, dpv);
    return dp;
}

static Datapoint*
createDpWithValue(const string& name, const string& value)
{
    DatapointValue dpv(value);
    Datapoint* dp = new Datapoint(name, dpv);
    return dp;
}

static Datapoint*
createDpWithValue(const string& name, const DatapointValue& value)
{
    DatapointValue dpv(value);
    Datapoint* dp = new Datapoint(name, dpv);
    return dp;
}

template <class T>
static Datapoint*
datapointAddElementWithValue(Datapoint* dp, const string& name, const T value)
{
    DatapointValue& dpv = dp->getData();
    Datapoints* subDatapoints = dpv.getDpVec();
    Datapoint* element = createDpWithValue(name, value);
    if (element) {
       subDatapoints->push_back(element);
    }

    return element;
}

}  // namespace

/**
 * Constructor for the Pivot2OpcuaFilter.
 *
 * We call the constructor of the base class and handle the initial
 * configuration of the filter.
 *
 * @param    filterName      The name of the filter
 * @param    filterConfig    The configuration category for this filter
 * @param    outHandle       The handle of the next filter in the chain
 * @param    output          A function pointer to call to output data to the next filter
 */
Pivot2OpcuaFilter::Pivot2OpcuaFilter(const string& filterName,
        ConfigCategory& filterConfig,
        OUTPUT_HANDLE *outHandle,    // //NOSONAR (Use of Fledge API)
        OUTPUT_STREAM output) :      // //NOSONAR (Use of Fledge API)
                FledgeFilter(filterName, filterConfig, outHandle, output) {
    handleConfig(filterConfig);
}

Pivot2OpcuaFilter::Datapoints*
Pivot2OpcuaFilter::findDictElement(Datapoints* dict, const string& key) {
    for (Datapoint* dp : *dict) {
        if (dp->getName() == key) {
            DatapointValue& data = dp->getData();
            if (data.getType() == DatapointValue::T_DP_DICT) {
                return data.getDpVec();
            }
        }
    }
    const std::string errMsg(string("Missing Obj value for ") + key);
    throw InvalidPivotContent(errMsg);
}

string
Pivot2OpcuaFilter::getStringStVal(Datapoints* dict, const string& context) {
    for (Datapoint* dp : *dict) {
        if (dp->getName() == stValName) {
            DatapointValue& data = dp->getData();
            const DatapointValue::dataTagType dType(data.getType());
            if (dType == DatapointValue::T_STRING) {
                return data.toStringValue();
            }
        }
    }

    const std::string errMsg(string("Missing 'stVal' of type T_STRING value for ") + context);
    throw InvalidPivotContent(errMsg);
}

int64_t
Pivot2OpcuaFilter::getIntStVal(Datapoints* dict, const string& context) {
    for (Datapoint* dp : *dict) {
        if (dp->getName() == stValName) {
            DatapointValue& data = dp->getData();
            const DatapointValue::dataTagType dType(data.getType());
            if (dType == DatapointValue::T_INTEGER) {
                return static_cast<int64_t>(data.toInt());
            }
        }
    }
    const std::string errMsg(string("Missing 'stVal' of type T_INTEGER value for ") + context);
    throw InvalidPivotContent(errMsg);
}

/***
 * @param dict The object under "PIVOT.GTxx"
 */
Pivot2OpcuaFilter::CommonMeasurePivot::
CommonMeasurePivot(const Datapoints* dict) :
m_readFields(0),            // //NOSONAR (FP)
m_pivotType("unknown"),     // //NOSONAR (FP)
m_Confirmation(false),      // //NOSONAR (FP)
m_Cause(0),                 // //NOSONAR (FP)
m_ComingFrom(""),           // //NOSONAR (FP)
m_Identifier(""),           // //NOSONAR (FP)
m_TmOrg("genuine"),         // //NOSONAR (FP)
m_TmValidity("good"),       // //NOSONAR (FP)
m_MagVal(nullptr),          // //NOSONAR (FP)
m_Qualified(nullptr) {      // //NOSONAR (FP)
    static const FieldDecoder notFound(nullptr);

    for (Datapoint* dp : *dict) {
        const string name(dp->getName());
        DatapointValue& data = dp->getData();
        FieldDecoder decoder(Rules::find_T(decoder_map, name, notFound));
        if (notFound != decoder) {
            try {
                (*decoder)(this, data, name);
            } catch (const InvalidPivotContent& e) {
                LOG_WARNING("Invalid/incomplete PIVOT content: '%s'", e.context());
            }
        } else {
            LOG_WARNING("Unknown PIVOT field '%s'", name.c_str());
        }
    }

    if (m_Qualified != nullptr && m_Qualified->quality.validity() != "") {
        m_readFields |= FieldMask_vqu;
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
ignoreField(CommonMeasurePivot* pivot, DatapointValue& data, const string& name) {
    (void)pivot;
    (void)data;
    LOG_DEBUG("Ignoring PIVOT field '%s'", name.c_str());
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
decodeConfirmation(CommonMeasurePivot* pivot, DatapointValue& data, const string& name) {
    (void)name;
    if (data.getType() == DatapointValue::T_DP_DICT) {
        Datapoints* gtElems(data.getDpVec());
        pivot->m_Confirmation = getIntStVal(gtElems, "confirmation");
        pivot->m_readFields |= FieldMask_cnf;
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
decodeCause(CommonMeasurePivot* pivot, DatapointValue& data, const string& name) {
    if (data.getType() == DatapointValue::T_DP_DICT) {
        Datapoints* gtElems(data.getDpVec());
        pivot->m_Cause = static_cast<int>(getIntStVal(gtElems, "cause"));
        pivot->m_readFields |= FieldMask_cot;
    } else {
        throw InvalidPivotContent(string("Missing Cause.stVal in ") + name.c_str());
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
decodeComingFrom(CommonMeasurePivot* pivot, DatapointValue& data, const string& name) {
    if (data.getType() == DatapointValue::T_STRING) {
        pivot->m_ComingFrom = data.toStringValue();
        pivot->m_readFields |= FieldMask_cmf;
    } else {
        throw InvalidPivotContent(string("Missing or invalid ComingFrom in ") + name.c_str());
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
decodeTmOrg(CommonMeasurePivot* pivot, DatapointValue& data, const string& name) {
    if (data.getType() == DatapointValue::T_DP_DICT) {
        Datapoints* gtElems(data.getDpVec());
        pivot->m_TmOrg = getStringStVal(gtElems, "TmOrg");
        pivot->m_readFields |= FieldMask_tmo;
    } else {
        throw InvalidPivotContent(string("Missing or invalid TmOrg.stVal in ") + name.c_str());
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
decodeTmValidity(CommonMeasurePivot* pivot, DatapointValue& data, const string& name) {
    if (data.getType() == DatapointValue::T_DP_DICT) {
        Datapoints* gtElems(data.getDpVec());
        pivot->m_TmValidity = getStringStVal(gtElems, "TmValidity");
        pivot->m_readFields |= FieldMask_tmv;
    } else {
        throw InvalidPivotContent(string("Missing or invalid TmValidity.stVal in ") + name.c_str());
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
decodeIdentifier(CommonMeasurePivot* pivot, DatapointValue& data, const string& name) {
    if (data.getType() == DatapointValue::T_STRING) {
        pivot->m_Identifier =  data.toStringValue();
        pivot->m_readFields |= FieldMask_idt | FieldMask_typ;
    } else {
        throw InvalidPivotContent(string("Missing or invalid 'Identifier' in ") + name.c_str());
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
decodeMagVal(CommonMeasurePivot* pivot, DatapointValue& data, const string& name) {
    if (data.getType() != DatapointValue::T_DP_DICT) return;

    Datapoints* dp(data.getDpVec());

    try {
        pivot->m_MagVal.reset(new QualifiedMagVal(dp));
        pivot->m_Qualified = pivot->m_MagVal.get();
        pivot->m_pivotType = name;
        pivot->m_readFields |= FieldMask_val;
    } catch (const InvalidPivotContent& e) {
        LOG_WARNING("Failed to decode MvTyp. Cause : %s", e.what());
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
decodeSpsVal(CommonMeasurePivot* pivot, DatapointValue& data, const string& name) {
    if (data.getType() != DatapointValue::T_DP_DICT) return;

    Datapoints* dp(data.getDpVec());

    try {
        pivot->m_BoolVal.reset(new QualifiedBoolStVal(dp));
        pivot->m_Qualified = pivot->m_BoolVal.get();
        pivot->m_pivotType = name;
        pivot->m_readFields |= FieldMask_val;
    } catch (const InvalidPivotContent& e) {
        LOG_WARNING("Failed to decode SpsTyp for '%s'. Cause : %s",
                name.c_str(), e.what());
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
decodeDpsVal(CommonMeasurePivot* pivot, DatapointValue& data, const string& name) {
    if (data.getType() != DatapointValue::T_DP_DICT) return;

    Datapoints* dp(data.getDpVec());

    try {
        pivot->m_StrVal.reset(new QualifiedStringStVal(dp));
        pivot->m_Qualified = pivot->m_StrVal.get();
        pivot->m_pivotType = name;
        pivot->m_readFields |= FieldMask_val;
    } catch (const InvalidPivotContent& e) {
        LOG_WARNING("Failed to decode DpsTyp for '%s'. Reason : %s",
                name.c_str(),  e.context());
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
logMissingMandatoryFields(const std::string& pivotName, uint32_t fields) {
    const uint32_t missing(Mandatory_fields & (~fields));
    if (missing & FieldMask_cot) {
        LOG_WARNING("Mandatory field 'Cause' missing for PIVOT ID='%s'", pivotName.c_str());
    }
    if (missing & FieldMask_cmf) {
        LOG_WARNING("Mandatory field 'ComingFrom' missing for PIVOT ID='%s'", pivotName.c_str());
    }
    if (missing & FieldMask_vqu) {
        LOG_WARNING("Mandatory field 'XxTyp.q.Validity' missing for PIVOT ID='%s'", pivotName.c_str());
    }
}

void
Pivot2OpcuaFilter::CommonMeasurePivot::
updateReading(const DataDictionnary* dictPtr, Reading* reading)const {
    // Search for initial data in "exchanged_data" section
    if (dictPtr == nullptr || m_Qualified == nullptr) return;
    if (m_Identifier.empty()) {
        LOG_WARNING("Mandatory field 'Identifier' from PIVOT is missing ");
        return;
    }

    if ((m_readFields & Mandatory_fields) != Mandatory_fields) {
        LOG_WARNING("Mandatory fields from PIVOT are missing for PIVOT ID='%s' (Missing mask = 0x%04X)",
                m_Identifier.c_str(), Mandatory_fields & (~m_readFields));
        logMissingMandatoryFields(m_Identifier.c_str(), m_readFields);
        return;
    }

    const DataDictionnary& dict(*dictPtr);

    const PivotElementMap_t::const_iterator search = dict.find(m_Identifier);
    if (dict.isEmpty(search)) {
        LOG_WARNING("Could not identify PIVOT ID='%s'", m_Identifier.c_str());
        return;
    }

    // Elment found in dictionary
    const PivotElement& element(search->second);

    // Ensure all elements can be created before deleting previous item
    Datapoint* dp_value(nullptr);
    try {
        dp_value = m_Qualified->createData(element.m_opcType);
    } catch (const InvalidPivotContent& e) {
        LOG_WARNING("Failed to extract PIVOT content for '%s'",
                m_Identifier.c_str());
        LOG_WARNING("... Reason : %s", e.context());
        throw;
    }

    reading->removeAllDatapoints();

    // Build content of "data_object" Object
    Datapoints* dp_vect = new Datapoints;    // //NOSONAR (Use of Fledge API)

    dp_vect->push_back(createDpWithValue("do_cot", m_Cause));
    dp_vect->push_back(createDpWithValue("do_confirmation", m_Confirmation));
    dp_vect->push_back(createDpWithValue("do_comingfrom", m_ComingFrom));
    dp_vect->push_back(createDpWithValue("do_id", m_Identifier));
    dp_vect->push_back(createDpWithValue("do_type", element.m_opcType));
    dp_vect->push_back(createDpWithValue("do_quality", m_Qualified->quality.toDetails()));
    dp_vect->push_back(createDpWithValue("do_ts_quality", m_Qualified->ts.toDetails()));
    dp_vect->push_back(createDpWithValue("do_value_quality", m_Qualified->quality.validity()));
    dp_vect->push_back(createDpWithValue("do_source", m_Qualified->quality.getSource()));
    dp_vect->push_back(createDpWithValue("do_ts", m_Qualified->ts.nbSec()));
    dp_vect->push_back(createDpWithValue("do_ts_org", m_TmOrg));
    dp_vect->push_back(createDpWithValue("do_ts_validity", m_TmValidity));
    dp_vect->push_back(dp_value);
    DatapointValue dpVal(dp_vect, true);
    Datapoint* dp(new Datapoint("data_object", dpVal));  // //NOSONAR (Use of Fledge API)
    LOG_INFO("Successfully converted PIVOT ID='%s' from type '%s' to OPCUA '%s'",
            m_Identifier.c_str(), m_pivotType.c_str(), element.m_opcType.c_str());
    reading->addDatapoint(dp);
}

namespace {
using Datapoints = std::vector<Datapoint *>;
inline Datapoints* getDatapointValueObjVal(DatapointValue& data, const string& context) {
    const DatapointValue::dataTagType qType(data.getType());
    if (qType != DatapointValue::T_DP_DICT)
        throw InvalidPivotContent(string("Bad type for '") + context + "' (expecting T_DP_DICT)");
    return data.getDpVec();
}

inline int64_t getDatapointValueIntVal(const DatapointValue& data, const string& context) {
    const DatapointValue::dataTagType qType(data.getType());
    if (qType != DatapointValue::T_INTEGER)
        throw InvalidPivotContent(string("Bad type for '") + context + "' (expecting T_INTEGER)");
    return data.toInt();
}

inline double getDatapointValueFloatVal(const DatapointValue& data, const string& context) {
    const DatapointValue::dataTagType qType(data.getType());
    if (qType != DatapointValue::T_FLOAT)
        throw InvalidPivotContent(string("Bad type for '") + context + "' (expecting T_FLOAT)");
    return data.toDouble();
}

inline string getDatapointValueStrVal(const DatapointValue& data, const string& context) {
    const DatapointValue::dataTagType qType(data.getType());
    if (qType != DatapointValue::T_STRING)
        throw InvalidPivotContent(string("Bad type for '") + context + "' (expecting T_STRING)");
    return data.toStringValue();
}

}   // namespace

Pivot2OpcuaFilter::PivotQuality::
PivotQuality(const Datapoints* dict):
m_test(false),                         // //NOSONAR (FP)
m_operatorBlocked(false),              // //NOSONAR (FP)
m_Validity(""),                        // //NOSONAR (FP)
m_Source(""),                          // //NOSONAR (FP)
m_Detail_badRef(false),                // //NOSONAR (FP)
m_Detail_failure(false),               // //NOSONAR (FP)
m_Detail_inconsistent(false),          // //NOSONAR (FP)
m_Detail_innacurate(false),            // //NOSONAR (FP)
m_Detail_oldData(false),               // //NOSONAR (FP)
m_Detail_oscillatory(false),           // //NOSONAR (FP)
m_Detail_outOfRange(false),            // //NOSONAR (FP)
m_Detail_overflow(false) {             // //NOSONAR (FP)
    for (Datapoint* dp : *dict) {
        const string name(dp->getName());
        DatapointValue& data = dp->getData();

        if (name == "DetailQuality") {
            Datapoints* quality(getDatapointValueObjVal(data, "DetailQuality"));
            for (Datapoint* qDp : *quality) {
                const string qName(qDp->getName());
                DatapointValue& qData = qDp->getData();

                if (qName == "badReference") {  // //NOSONAR
                    m_Detail_badRef = getDatapointValueIntVal(qData, "badReference");
                } else if (qName == "failure") {
                    m_Detail_failure = getDatapointValueIntVal(qData, "failure");
                } else if (qName == "inconsistent") {
                    m_Detail_inconsistent = getDatapointValueIntVal(qData, "inconsistent");
                } else if (qName == "innacurate") {
                    m_Detail_innacurate = getDatapointValueIntVal(qData, "innacurate");
                } else if (qName == "oldData") {
                    m_Detail_oldData = getDatapointValueIntVal(qData, "oldData");
                } else if (qName == "oscillatory") {
                    m_Detail_oscillatory = getDatapointValueIntVal(qData, "oscillatory");
                } else if (qName == "outOfRange") {
                    m_Detail_outOfRange = getDatapointValueIntVal(qData, "outOfRange");
                } else if (qName == "overflow") {
                    m_Detail_overflow = getDatapointValueIntVal(qData, "overflow");
                }
            }
        } else if (name == "Source") {
            m_Source = getDatapointValueStrVal(data, "Source");
        } else if (name == "Validity") {
            m_Validity = getDatapointValueStrVal(data, "Validity");
        } else if (name == "operatorBlocked") {
            m_operatorBlocked = getDatapointValueIntVal(data, "operatorBlocked");
        } else if (name == "test") {
            m_test = getDatapointValueIntVal(data, "test");
        } else {
            LOG_WARNING("Unknown field '%s' in PivotQuality 'q'", name.c_str());
        }
    }
}

uint32_t
Pivot2OpcuaFilter::PivotQuality::toDetails(void)const {
    uint32_t result(0);

    if (m_Detail_badRef) result |= Mask_badReference;
    if (m_Detail_failure) result |= Mask_failure;
    if (m_Detail_inconsistent) result |= Mask_inconsistent;
    if (m_Detail_innacurate) result |= Mask_innaccurate;
    if (m_Detail_oldData) result |= Mask_oldData;
    if (m_Detail_oscillatory) result |= Mask_oscillatory;
    if (m_Detail_outOfRange) result |= Mask_outOfRange;
    if (m_Detail_overflow) result |= Mask_overflow;
    if (m_operatorBlocked) result |= Mask_operator_blocked;
    if (m_test) result |= Mask_test;
    return result;
}

Pivot2OpcuaFilter::PivotTimestamp::
PivotTimestamp(const Datapoints* dict) :
time_frac(0),                   // //NOSONAR (FP)
time_nbSec(0),                  // //NOSONAR (FP)
clockFailure(0),                // //NOSONAR (FP)
clockNotSynch(0),               // //NOSONAR (FP)
leapSecondKnown(0),             // //NOSONAR (FP)
timeAccuracy(0) {               // //NOSONAR (FP)
    for (Datapoint* dp : *dict) {
        const string name(dp->getName());
        DatapointValue& data = dp->getData();

        if (name == "TimeQuality") {
            Datapoints* quality(getDatapointValueObjVal(data, "TimeQuality"));
            for (Datapoint* qDp : *quality) {
                const string qName(qDp->getName());
                DatapointValue& qData = qDp->getData();
                const DatapointValue::dataTagType qType(qData.getType());

                if (qName == "clockFailure") {  // //NOSONAR
                    clockFailure = getDatapointValueIntVal(qData, "clockFailure");
                } else if (qName == "clockNotSynchronized") {
                    clockNotSynch = getDatapointValueIntVal(qData, "clockNotSynchronized");
                } else if (qName == "leapSecondKnown") {
                    leapSecondKnown = getDatapointValueIntVal(qData, "leapSecondKnown");
                } else if (qName == "timeAccuracy") {
                    timeAccuracy = getDatapointValueIntVal(qData, "timeAccuracy");
                }
            }
        } else if (name == "SecondSinceEpoch") {
            time_nbSec = getDatapointValueIntVal(data, "SecondSinceEpoch");
        } else if (name == "FractionOfSecond") {
            time_frac = getDatapointValueIntVal(data, "FractionOfSecond");
        } else {
            LOG_WARNING("Unknown field '%s' in PivotTimestamp 't'", name.c_str());
        }
    }
}

uint32_t
Pivot2OpcuaFilter::PivotTimestamp::
toDetails(void)const {
    uint32_t result(0);
    if (clockFailure) result |= Mask_clockFailure;
    if (clockNotSynch) result |= Mask_clockNotSynch;
    if (leapSecondKnown) result |= Mask_leapSecondKnown;
    return result;
}

Pivot2OpcuaFilter::Qualified::
Qualified(Datapoints* dict) :
    ts(findDictElement(dict, "t")),
    quality(findDictElement(dict, "q")) {}

Pivot2OpcuaFilter::QualifiedMagVal::
QualifiedMagVal(Datapoints* dict) :
    Qualified(dict),
    m_mag(findDictElement(dict, "mag")),
    hasF(false),
    hasI(false),
    fVal(0.0),
    iVal(0) {
    for (Datapoint* dp : *m_mag) {
        const string name(dp->getName());
        DatapointValue& data = dp->getData();
        if (name == "f") {
            fVal = getDatapointValueFloatVal(data, "mag.f");
            hasF = true;
        }
        if (name == "i") {
            iVal = getDatapointValueIntVal(data, "mag.i");
            hasI = true;
        }
    }
}

Datapoint*
Pivot2OpcuaFilter::QualifiedMagVal::
createData(const std::string& typeName) {
    if (typeName == "opcua_mvi" && hasI)
        return createDpWithValue("do_value", iVal);
    if  (typeName == "opcua_mvf" && hasF)
        return createDpWithValue("do_value", (float)fVal);
    const std::string errMsg(
            string("'do_value' encoding error:'") +
            typeName + "' cannot be used with content of type magVal, or missing val I/F");
    throw InvalidPivotContent(errMsg);
}

Pivot2OpcuaFilter::QualifiedBoolStVal::
QualifiedBoolStVal(Datapoints* dict) :
    Qualified(dict),
    bVal(getIntStVal(dict, "stVal")) {}

Datapoint*
Pivot2OpcuaFilter::QualifiedBoolStVal::
createData(const std::string& typeName) {
    if (typeName == "opcua_sps")
        return createDpWithValue("do_value", bVal);

    const std::string errMsg(
            string("'do_value' encoding error:'") +
            typeName + "' cannot be used with stVal of type BOOL");
    throw InvalidPivotContent(errMsg);
}

Pivot2OpcuaFilter::QualifiedStringStVal::
QualifiedStringStVal(Datapoints* dict) :
    Qualified(dict),
    sVal(getStringStVal(dict, "DpsTyp")) {}

Datapoint*
Pivot2OpcuaFilter::QualifiedStringStVal::
createData(const std::string& typeName) {
    if (typeName == "opcua_dps")
        return createDpWithValue("do_value", sVal);
    const std::string errMsg(
            string("'do_value' encoding error:'") +
            typeName + "' cannot be used with stVal of type String");
    throw InvalidPivotContent(errMsg);
}

/**
 * Convert a Reading from PIVOT to OPCUA.
 * @param readingRef The Reading.
 *      It is expected as an array of 1 Datapoint, eah of them containing:
 *        - PIVOT.GTIx....
 *      If the reading content is compatible, it is replaced by the equivalent OPC data
 *      ("data_object")
 *      Only One datapoint is translated.
 */
void
Pivot2OpcuaFilter::pivot2opcua(Reading* readingRef) {

    Datapoints& readDp(readingRef->getReadingData());
    for (Datapoint* dp : readDp) {
        // Expecting "PIVOT" in first level
        const string name(dp->getName());
        if (name != "PIVOT") {
            LOG_WARNING("Unknown Reading Type '%s'", name.c_str());
            continue;
        }
        // Check sub content is an object
        DatapointValue& data = dp->getData();
        if (data.getType() != DatapointValue::T_DP_DICT) {
            LOG_WARNING("invalid content for Reading Type '%s'. expecting T_DP_DICT", name.c_str());
            continue;
        }

        Datapoints* gtElems(data.getDpVec());
        for (Datapoint* gtElem : *gtElems) {
            const string gtName(gtElem->getName());
            // Check if the GTxx is known

            Str2Vect_map_t::const_iterator gtIter(Rules::gtix2pivotTypeMap.find(gtName));

            if (gtIter == Rules::gtix2pivotTypeMap.end()) {
                LOG_WARNING("invalid content for Datapoint '%s'. found unexpected '%s'",
                        name.c_str(), gtName.c_str());
                continue;
            }

            // Found matching (e.g. "PIVOT.GTIM")
            DatapointValue& gtData = gtElem->getData();
            if (gtData.getType() != DatapointValue::T_DP_DICT) {
                LOG_WARNING("invalid content for Datapoint '%s'. expecting T_DP_DICT",
                        gtName.c_str());
                continue;
            }

            try {
                CommonMeasurePivot pivot(gtData.getDpVec());
                pivot.updateReading(m_dictionnary.get(), readingRef);
                return;
            } catch (const InvalidPivotContent& e) {
                LOG_WARNING("Failed to extract PIVOT content from '%s.%s'",
                        name.c_str(), gtName.c_str());
                LOG_WARNING("... Reason : %s", e.context());
                continue;
            }
        }
    }
    LOG_DEBUG("Received 'Reading' with no known content.");
    return;
}

/**
 * Convert a Reading from OPCUA to PIVOT.
 * @param readingRef The Reading.
 *      If the reading content is compatible, it is replaced by the equivalent PIVOT data
 */
void
Pivot2OpcuaFilter::opcua2pivot(Reading* reading) {
    if (reading == nullptr) return;

    Datapoints& readDp(reading->getReadingData());
    LOG_INFO("Pivot2OpcuaFilter::opcua2pivot('%s' :%d elem.)",
            reading->getAssetName().c_str(),
            readDp.size());

    // Values to read from the reading
    string co_id;
    string co_type;
    DatapointValue* co_value(nullptr);
    long co_test(-1);
    long co_neg(-1);
    long co_qu(-1);
    long co_se(-1);
    long co_ts(-1);

    for (Datapoint* dp : readDp) {
        /* An "opcua_operation" is expected to be an array with:
         *  "co_id", "co_type", "co_value", "co_test", "co_negative", "co_qu", "co_se", "co_ts"
         */
        const string name(dp->getName());
        string* targetStr(nullptr);
        long* targetInt(nullptr);
        DatapointValue& data = dp->getData();

        if (name == "co_id") {
            targetStr = &co_id;
        } else if (name == "co_type") {
            targetStr = &co_type;
        } else if (name == "co_value") {
            co_value = &data;
        } else if (name == "co_test") {
            targetInt = &co_test;
        } else if (name == "co_negative") {
            targetInt = &co_neg;
        } else if (name == "co_qu") {
            targetInt = &co_qu;
        } else if (name == "co_se") {
            targetInt = &co_se;
        } else if (name == "co_ts") {
            targetInt = &co_ts;
        } else {
            LOG_WARNING("Unknown Reading element '%s' (Ignored)", name.c_str());
        }

        // Do actual reading
        if (nullptr != targetInt) {
            if (data.getType() != DatapointValue::T_INTEGER) {
                LOG_WARNING("Reading element '%s' has an invalid type (expected INTEGER). Control ignored.", name.c_str());
                return;
            }
            *targetInt = data.toInt();
        }
        if (nullptr != targetStr) {
            if (data.getType() != DatapointValue::T_STRING) {
                LOG_WARNING("Reading element '%s' has an invalid type (expected STRING). Control ignored.", name.c_str());
                return;
            }
            *targetStr = data.toString();
        }
    }

    // Check if all mandatory fields are OK
    if (co_value != nullptr && co_id.size() > 0 && co_type.size() > 0 &&
            co_test >= 0 && co_neg >= 0 && co_qu >= 0 && co_se >= 0 && co_ts >= 0) {
        /* The expected output is:
         {"PIVOT": { "GTIC": {
                        "Cause": {"stVal": 0},
                        "Select": {"stVal": <co_se>},
                        "ComingFrom": "opcua",
                        "Identifier": "<co_id>",
                        "<co_type>" : {
                            "t": { "SecondSinceEpoch": <co_ts> },
                            "q": { "test": <co_test> },
                            "ctlVal" : <co_value>
                            }
                    }
                 }
            }
          Note : co_type is in ["SpcTyp", "DpcTyp", "IncType" or "ApcTyp"]
         */
        try {
            // The Value must be read before cleaning old datapoints!

            Datapoint* pivot = createDatapoint("PIVOT");
            Datapoint* gtic = datapointAddElement(pivot, "GTIC");
            datapointAddElementWithValue(gtic, "Cause", 0);
            datapointAddElementWithValue(gtic, "Select", co_se);
            datapointAddElementWithValue(gtic, "ComingFrom", "opcua");
            datapointAddElementWithValue(gtic, "Identifier", co_id);
            Datapoint* dvType = datapointAddElement(gtic, co_type);
            Datapoint* dvTypeT = datapointAddElement(dvType, "t");
            datapointAddElementWithValue(dvTypeT, "SecondSinceEpoch", co_ts);
            Datapoint* dvTypeQ = datapointAddElement(dvType, "q");
            datapointAddElementWithValue(dvTypeQ, "test", co_test);
            datapointAddElementWithValue(dvType, "ctlVal", *co_value);

            reading->removeAllDatapoints();

            LOG_INFO("Successfully created PIVOT ID='%s' with type '%s'",
                    co_id.c_str(), co_type.c_str());
            reading->addDatapoint(pivot);
        } catch (const InvalidPivotContent& e) {
            LOG_WARNING("Failed to build PIVOT content from '%s(%s)'",
                    co_id.c_str(), co_type.c_str());
            LOG_WARNING("... Reason : %s", e.context());
        }
    } else {
        LOG_WARNING("'opcua_operation' ignored since some mandatory fields were not provided.");
    }
    return;
}

/**
 * The actual filtering code
 *
 * @param readingSet The reading data to filter
 */
void
Pivot2OpcuaFilter::ingest(ReadingSet *readingSet) {
    std::lock_guard<mutex> guard(m_configMutex);

    // Filter enable, process the readings
    if (isEnabled()) {
        Readings* readings(readingSet->getAllReadingsPtr());
        LOG_DEBUG("Pivot2OpcuaFilter::ingest(%d readings)", readings->size());
        for (Reading* reading : *readings) {

            std::string assetName = reading->getAssetName();
            AssetTracker* tracker(AssetTracker::getAssetTracker());
            if (nullptr != tracker) {
                // We are modifying this asset so put an entry in the asset tracker
                AssetTracker::getAssetTracker()->addAssetTrackingTuple(
                        getName(), reading->getAssetName(), string("Filter"));
            }
            // proceed to conversion
            if (assetName == "opcua_operation") {
                opcua2pivot(reading);
            } else {
                // Default case convert PIVOT to OPCUA
                pivot2opcua(reading);
            }
        }
    }
    // Pass on all readings
    (*m_func)(m_data, readingSet);
}

/**
 * Reconfiguration entry point to the filter.
 *
 * This method runs holding the configMutex to prevent concurrent access
 * with ingest
 *
 * Pass the configuration to the base FilterPlugin class and
 * then call the private method to handle the filter specific
 * configuration.
 *
 * @param newConfig  The JSON of the new configuration
 */
void
Pivot2OpcuaFilter::reconfigure(const string& newConfig) {
    std::lock_guard<mutex> guard(m_configMutex);
    setConfig(newConfig);    // Pass the configuration to the base class
    handleConfig(m_config);
}

/**
 * Handle the filter specific configuration. In this case
 * it is just the single item "match" that is a regex
 * expression
 *
 * @param config     The configuration category
 */
void
Pivot2OpcuaFilter::handleConfig(const ConfigCategory& config) {
    // Parse exchanged_data section and create a fast-search dictionnary
    LOG_INFO("Receiving new configuration '%s'", config.getDisplayName().c_str());
    if (config.itemExists(JSON_EXCHANGED_DATA)) {
        LOG_INFO("Updating Exchanged data section...");
        m_dictionnary.reset(new DataDictionnary(config.getValue(JSON_EXCHANGED_DATA)));
    }
}
