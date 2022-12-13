#include <gtest/gtest.h>
#include <plugin_api.h>
#include <string.h>
#include <exception>
#include <vector>
#include <algorithm>
#include <string>
#include <rapidjson/document.h>

// Tested files
#include "pivot2opcua_common.h"
#include "pivot2opcua_filter.h"
#include "pivot2opcua_data.h"
#include "pivot2opcua_rules.h"

// Fledge / tools  includes
#include "config_category.h"
#include "main_test_configs.h"
#include "plugin_api.h"
#include "filter.h"
#include "asset_tracking.h"

using namespace std;
using namespace rapidjson;

extern "C" {
PLUGIN_HANDLE plugin_init(ConfigCategory* config,
                          OUTPUT_HANDLE *outHandle,
                          OUTPUT_STREAM output);
void plugin_ingest(PLUGIN_HANDLE handle,
        READINGSET *readingSet);
void plugin_reconfigure(PLUGIN_HANDLE handle, const std::string& newConfig);
void plugin_shutdown(PLUGIN_HANDLE handle);
}

namespace {
void* stubOutH(&stubOutH);
READINGSET* called_output_reading;
OUTPUT_HANDLE* called_output_handle;
void f_reset_outputs(void) {
    called_output_handle = nullptr;
    called_output_reading = nullptr;
}
void f_output_stream(OUTPUT_HANDLE * out, READINGSET *set) {
    called_output_handle = out;
    called_output_reading = set;
}
}

// Test with Disabled filter
TEST(Pivot2Opcua_Filter, Pivot2OpcuaFilterDisabled) {
    TITLE("*** TEST FILTER Pivot2OpcuaFilterDisabled");

    ConfigCategory config;
    config.addItem(string("exchanged_data"),
            string("exchanged data list"),
            string("string"),
            Json_ExDataOK, Json_ExDataOK);
    Pivot2OpcuaFilter filter("A great filter", config,
            stubOutH, &f_output_stream);

    // Filter is disabled !
    Readings readings;
    DatapointValue dpv("string");
    Datapoint* dpBadbaseType = new Datapoint("PIVOT_Unknown", dpv);
    Reading* reading = new Reading("Title", dpBadbaseType);
    readings.push_back(reading);

    READINGSET rSet(&readings);

    f_reset_outputs();
    ASSERT_EQ(called_output_handle, nullptr);
    ASSERT_EQ(called_output_reading, nullptr);
    filter.ingest(&rSet);
    ASSERT_EQ(called_output_handle, &stubOutH);
    ASSERT_EQ(called_output_reading, &rSet);
    // Filter disabled : content unchanged
    ASSERT_EQ(rSet.getAllReadings().size(), 1);
    Readings& outReadings(*rSet.getAllReadingsPtr());
    ASSERT_EQ(outReadings.size(), 1);
    ASSERT_EQ(outReadings.front()->getAssetName(), "Title");
    ASSERT_EQ(outReadings.front()->getDatapointCount(), 1);
    Datapoints& outDp(outReadings.front()->getReadingData());
    ASSERT_EQ(outDp.size(), 1);
    ASSERT_EQ(outDp.front()->getName(), "PIVOT_Unknown");
    DatapointValue& outDPV(outDp.front()->getData());
    ASSERT_EQ(outDPV.getType(), DatapointValue::T_STRING);
    ASSERT_EQ(outDPV.toStringValue(), "string");
}

namespace {
ConfigCategory getEnabledConfig(const std::string& json) {
    ConfigCategory config;
    config.addItem(string("exchanged_data"),
            string("exchanged data list"),
            string("string"),
            json, json);
    config.addItem(string("enable"),
            string("enable"),
            string("boolean"),
            "true", "true");
    return config;
}
}
ConfigCategory configEnabled(::getEnabledConfig(Json_ExDataOK));


#define FILTER_PARAMS \
    "A great filter", ::configEnabled, stubOutH, &f_output_stream
#define FILTER_PARAMS2 \
    "A second filter", ::getEnabledConfig_2(), stubOutH, &f_output_stream

// Test with unknown PIVOT data type
TEST(Pivot2Opcua_Filter, Pivot2OpcuaFilterBadData) {
    TRACE("*** TEST FILTER Pivot2OpcuaFilterBadData");

    Pivot2OpcuaFilter filter(FILTER_PARAMS);

    // Send a reading
    Readings readings;
    DatapointValue dpv("stringxx");
    Datapoint* dpBadbaseType = new Datapoint("PIVOT_Unknown", dpv);
    Reading* reading = new Reading("Title", dpBadbaseType);
    readings.push_back(reading);

    READINGSET rSet(&readings);

    f_reset_outputs();
    ASSERT_EQ(called_output_handle, nullptr);
    ASSERT_EQ(called_output_reading, nullptr);
    filter.ingest(&rSet);
    ASSERT_EQ(called_output_handle, &stubOutH);
    ASSERT_EQ(called_output_reading, &rSet);

    // Type unknown: no data modified
    ASSERT_EQ(rSet.getAllReadings().size(), 1);
    Readings& outReadings(*rSet.getAllReadingsPtr());
    ASSERT_EQ(outReadings.size(), 1);
    {
        // - Reading 1 : Asset = "Title"
        Reading* reading (outReadings.front());
        ASSERT_NOT_NULL(reading);
        ASSERT_EQ(reading->getAssetName(), "Title");
        ASSERT_EQ(reading->getDatapointCount(), 1);
        Datapoints& outDp(reading->getReadingData());

        ASSERT_EQ(outDp.size(), 1);
        ASSERT_EQ(outDp.front()->getName(), "PIVOT_Unknown");
        DatapointValue& do_dpv(outDp.front()->getData());
        ASSERT_EQ(do_dpv.getType(), DatapointValue::T_STRING);
        ASSERT_EQ(do_dpv.toStringValue(), "stringxx");
    }
}

// Test with TM PIVOT data type
// Send several Readings each containing 1 element
TEST(Pivot2Opcua_Filter, Pivot2OpcuaFilterTM) {
    TITLE("*** TEST FILTER Pivot2OpcuaFilterTM");

    Pivot2OpcuaFilter filter(FILTER_PARAMS);

    // Send a reading (PIVOT/GTIM)
    TRACE("  * Create reading()");
    ReadingSet rSet;
    rSet.append(JsonToReading(JsonPivotMvf, "code1"));
    rSet.append(JsonToReading(JsonPivotMvi, "code2"));

    f_reset_outputs();
    ASSERT_EQ(called_output_handle, nullptr);
    ASSERT_EQ(called_output_reading, nullptr);
    TRACE("  * INGEST()");
    filter.ingest(&rSet);
    ASSERT_EQ(called_output_handle, &stubOutH);
    ASSERT_EQ(called_output_reading, &rSet);

    // Readings = 2 reading
    Readings* outReadings(rSet.getAllReadingsPtr());
    ASSERT_EQ(outReadings->size(), 2);

    // - Reading 1 : Asset = "code1"
    {
        Reading* reading (outReadings->front());
        ASSERT_NOT_NULL(reading);
        ASSERT_EQ(reading->getAssetName(), "code1");
        ASSERT_EQ(reading->getDatapointCount(), 1);
        Datapoints& outDp(reading->getReadingData());

        ASSERT_EQ(outDp.size(), 1);
        ASSERT_EQ(outDp.front()->getName(), "data_object");
        DatapointValue& do_dpv(outDp.front()->getData());
        ASSERT_EQ(do_dpv.getType(), DatapointValue::T_DP_DICT);

        Datapoints* do_dp(do_dpv.getDpVec());
//        for (Datapoint* dp : *do_dp) {
//            TRACE("  * data_object1 contains %s", dp->getName().c_str());
//        }

        // Check Id
        DatapointValue* do_dv = get_datapoint_by_key(do_dp, "do_id");
        ASSERT_NOT_NULL(do_dv);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_STRING);
        ASSERT_EQ(do_dv->toStringValue(), "pivotMVF");

        // Check Cause
        do_dv = get_datapoint_by_key(do_dp, "do_cot");
        ASSERT_NOT_NULL(do_dv);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_INTEGER);
        ASSERT_EQ(do_dv->toInt(), 1);

        // Check Confirmation
        do_dv = get_datapoint_by_key(do_dp, "do_confirmation");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_INTEGER);
        ASSERT_EQ(do_dv->toInt(), 1);

        // Check do_comingfrom
        do_dv = get_datapoint_by_key(do_dp, "do_comingfrom");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_STRING);
        ASSERT_EQ(do_dv->toStringValue(), "iec104");

        // Check do_type
        do_dv = get_datapoint_by_key(do_dp, "do_type");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_STRING);
        ASSERT_EQ(do_dv->toStringValue(), "opcua_mvf");

        // Check do_quality
        do_dv = get_datapoint_by_key(do_dp, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_INTEGER);
        ASSERT_EQ(do_dv->toInt(), 0x0);

        // Check do_ts_quality
        do_dv = get_datapoint_by_key(do_dp, "do_ts_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_INTEGER);
        ASSERT_EQ(do_dv->toInt(), 0x0);

        // Check do_source
        do_dv = get_datapoint_by_key(do_dp, "do_source");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_STRING);
        ASSERT_EQ(do_dv->toStringValue(), "process");

        // Check do_ts
        do_dv = get_datapoint_by_key(do_dp, "do_ts");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_INTEGER);
        ASSERT_EQ(do_dv->toInt(), 12345678);

        // Check do_ts_org
        do_dv = get_datapoint_by_key(do_dp, "do_ts_org");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_STRING);
        ASSERT_EQ(do_dv->toStringValue(), "genuine");

        // Check do_ts_validity
        do_dv = get_datapoint_by_key(do_dp, "do_ts_validity");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_STRING);
        ASSERT_EQ(do_dv->toStringValue(), "questionable");

        // Check do_value
        do_dv = get_datapoint_by_key(do_dp, "do_value");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_FLOAT);
        EXPECT_NEAR(do_dv->toDouble(), 3.14, 0.00001);

        // Check do_value
        do_dv = get_datapoint_by_key(do_dp, "do_value_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_STRING);
        ASSERT_EQ(do_dv->toStringValue(), "good");
    }

    // - Reading 2 : Asset = "code2"
    {
        Reading* reading (outReadings->back());
        ASSERT_NE(reading, nullptr);
        ASSERT_EQ(reading->getAssetName(), "code2");
        ASSERT_EQ(reading->getDatapointCount(), 1);
        Datapoints& outDp(reading->getReadingData());

        ASSERT_EQ(outDp.size(), 1);
        ASSERT_EQ(outDp.front()->getName(), "data_object");
        DatapointValue& do_dpv(outDp.front()->getData());
        ASSERT_EQ(do_dpv.getType(), DatapointValue::T_DP_DICT);

        Datapoints* do_dp(do_dpv.getDpVec());
//        for (Datapoint* dp : *do_dp) {
//            TRACE("  * data_object2 contains %s", dp->getName().c_str());
//        }

        // Check Id
        DatapointValue* do_dv = get_datapoint_by_key(do_dp, "do_id");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_STRING);
        ASSERT_EQ(do_dv->toStringValue(), "pivotMVI");

        // Check Cause
        do_dv = get_datapoint_by_key(do_dp, "do_cot");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_INTEGER);
        ASSERT_EQ(do_dv->toInt(), 4);

        // Check Confirmation
        do_dv = get_datapoint_by_key(do_dp, "do_confirmation");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_INTEGER);
        ASSERT_EQ(do_dv->toInt(), 0);

        // Check do_value
        do_dv = get_datapoint_by_key(do_dp, "do_value");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_INTEGER);
        ASSERT_EQ(do_dv->toInt(), 10);
    }
}

// Test with Sps PIVOT data type
TEST(Pivot2Opcua_Filter, Pivot2OpcuaFilterSps) {
    TITLE("*** TEST FILTER Pivot2OpcuaFilterSps");

    Pivot2OpcuaFilter filter(FILTER_PARAMS);

    // Send a reading (PIVOT/GTIS)
    TRACE("  * Create reading()");
    ReadingSet rSet;
    rSet.append(JsonToReading(JsonPivotSps, "testSps"));

    f_reset_outputs();
    ASSERT_EQ(called_output_handle, nullptr);
    ASSERT_EQ(called_output_reading, nullptr);
    TRACE("  * INGEST()");
    filter.ingest(&rSet);
    ASSERT_EQ(called_output_handle, &stubOutH);
    ASSERT_EQ(called_output_reading, &rSet);

    // Readings = 1 reading
    Readings* outReadings(rSet.getAllReadingsPtr());
    ASSERT_EQ(outReadings->size(), 1);

    {
        Reading* reading (outReadings->back());
        ASSERT_NE(reading, nullptr);
        ASSERT_EQ(reading->getAssetName(), "testSps");
        ASSERT_EQ(reading->getDatapointCount(), 1);
        Datapoints& outDp(reading->getReadingData());

        ASSERT_EQ(outDp.size(), 1);
        ASSERT_EQ(outDp.front()->getName(), "data_object");
        DatapointValue& do_dpv(outDp.front()->getData());
        ASSERT_EQ(do_dpv.getType(), DatapointValue::T_DP_DICT);

        Datapoints* do_dp(do_dpv.getDpVec());

        // Check do_value
        DatapointValue* do_dv = get_datapoint_by_key(do_dp, "do_value");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_INTEGER);
        ASSERT_EQ(do_dv->toInt(), 1);
    }
}

// Test with Sps PIVOT data type
TEST(Pivot2Opcua_Filter, Pivot2OpcuaFilterDps) {
    TITLE("*** TEST FILTER Pivot2OpcuaFilterDps");

    Pivot2OpcuaFilter filter(FILTER_PARAMS);

    // Send a reading (PIVOT/GTIS)
    TRACE("  * Create reading()");

    ReadingSet rSet;
    rSet.append(JsonToReading(JsonPivotDps, "testDps"));

    f_reset_outputs();
    ASSERT_EQ(called_output_handle, nullptr);
    ASSERT_EQ(called_output_reading, nullptr);
    TRACE("  * INGEST()");
    filter.ingest(&rSet);
    ASSERT_EQ(called_output_handle, &stubOutH);
    ASSERT_EQ(called_output_reading, &rSet);

    // Readings = 1 reading
    Readings* outReadings(rSet.getAllReadingsPtr());
    ASSERT_EQ(outReadings->size(), 1);

    {
        Reading* reading (outReadings->back());
        ASSERT_NE(reading, nullptr);
        ASSERT_EQ(reading->getAssetName(), "testDps");
        ASSERT_EQ(reading->getDatapointCount(), 1);
        Datapoints& outDp(reading->getReadingData());

        ASSERT_EQ(outDp.size(), 1);
        ASSERT_EQ(outDp.front()->getName(), "data_object");
        DatapointValue& do_dpv(outDp.front()->getData());
        ASSERT_EQ(do_dpv.getType(), DatapointValue::T_DP_DICT);

        Datapoints* do_dp(do_dpv.getDpVec());

        // Check do_value
        DatapointValue* do_dv = get_datapoint_by_key(do_dp, "do_value");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->getType(), DatapointValue::T_STRING);
        ASSERT_EQ(do_dv->toStringValue(), "on");
    }
}

#define INGEST_DEBUG(rSet, json, _type)  \
    ReadingSet rSet;                                    \
    rSet.append(JsonToReading(json, _type));            \
    DEBUG_DATA;\
    f_reset_outputs();                                  \
    ASSERT_EQ(called_output_handle, nullptr);           \
    ASSERT_EQ(called_output_reading, nullptr);          \
    TRACE("  * INGEST() /" _type);                      \
    filter.ingest(&rSet);                               \
    DEBUG_DATA;\
    ASSERT_EQ(called_output_handle, &stubOutH);         \
    ASSERT_EQ(called_output_reading, &rSet);

#define INGEST(rSet, json, _type)  \
    ReadingSet rSet;                                    \
    rSet.append(JsonToReading(json, _type));            \
                                                        \
    f_reset_outputs();                                  \
    ASSERT_EQ(called_output_handle, nullptr);           \
    ASSERT_EQ(called_output_reading, nullptr);          \
    TRACE("  * INGEST() /" _type);                      \
    filter.ingest(&rSet);                               \
    ASSERT_EQ(called_output_handle, &stubOutH);         \
    ASSERT_EQ(called_output_reading, &rSet);

#define DATA_PASSES DatapointValue* do_dv = getDoResult(rSet);\
        ASSERT_NE(do_dv, nullptr)
#define DATA_FAILS DatapointValue* do_dv = getDoResult(rSet);\
        ASSERT_EQ(do_dv, nullptr)

#define DEBUG_DATA Debug_Dump(rSet.getAllReadingsPtr()->back()->getReadingData().back())

// Test Different qualities
TEST(Pivot2Opcua_Filter, Pivot2OpcuaFilterQualities) {
    TITLE("*** TEST FILTER Pivot2OpcuaFilterQualities");

    Pivot2OpcuaFilter filter(FILTER_PARAMS);

    // badReference
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"badReference\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "testbadref");

        DatapointValue* do_dv = getFieldResult(rSet, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x1);
    }

    // badReference
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"failure\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "testfailure");

        DatapointValue* do_dv = getFieldResult(rSet, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x2);
    }

    // inconsistent
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"inconsistent\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "inconsistent");

        DatapointValue* do_dv = getFieldResult(rSet, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x4);
    }

    // innacurate
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"innacurate\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "innacurate");

        DatapointValue* do_dv = getFieldResult(rSet, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x8);
    }

    // oldData
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"oldData\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "oldData");

        DatapointValue* do_dv = getFieldResult(rSet, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x10);
    }
    // oscillatory
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"oscillatory\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "oscillatory");

        DatapointValue* do_dv = getFieldResult(rSet, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x20);
    }

    // outOfRange
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"outOfRange\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "outOfRange");

        DatapointValue* do_dv = getFieldResult(rSet, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x40);
    }

    // overflow
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"overflow\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "overflow");

        DatapointValue* do_dv = getFieldResult(rSet, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x80);
    }

    // test
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"test\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "test");

        DatapointValue* do_dv = getFieldResult(rSet, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x1000);
    }

    // operatorBlocked
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"operatorBlocked\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "operatorBlocked");

        DatapointValue* do_dv = getFieldResult(rSet, "do_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x2000);
    }

    // clockFailure
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"clockFailure\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "clockFailure");

        Debug_Dump(rSet.getAllReadingsPtr()->back()->getReadingData().back());
        DatapointValue* do_dv = getFieldResult(rSet, "do_ts_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x01);
    }

    // clockNotSynchronized
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"clockNotSynchronized\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "clockNotSynchronized");

        DatapointValue* do_dv = getFieldResult(rSet, "do_ts_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x02);
    }

    // leapSecondKnown
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"leapSecondKnown\") *: *false", "$1: true"));

        INGEST(rSet, testStr, "leapSecondKnown");

        DatapointValue* do_dv = getFieldResult(rSet, "do_ts_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toInt(), 0x04);
    }

    // Validity
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"Validity\") *: *\"good\"", "$1: \"invalid\""));
        ASSERT_NE(testStr, JsonPivotDps);

        INGEST(rSet, testStr, "Validity");

        DatapointValue* do_dv = getFieldResult(rSet, "do_value_quality");
        ASSERT_NE(do_dv, nullptr);
        ASSERT_EQ(do_dv->toStringValue(), "invalid");
    }
}

// Test Different qualities
TEST(Pivot2Opcua_Filter, Pivot2OpcuaFilterMissFields) {
    TITLE("*** TEST FILTER Pivot2OpcuaFilterMissFields");

    Pivot2OpcuaFilter filter(FILTER_PARAMS);

    // reference test
    {
        const std::string testStr(JsonPivotDps);
        INGEST(rSet, testStr, "Ref test");

        DEBUG_DATA;
        DATA_PASSES;
    }

    // COT
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "\"Cause\"", "\"__Cause__\""));
        INGEST(rSet, testStr, "testMissCot");

        DATA_FAILS;
    }

    // Confirmation (optional)
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "\"Confirmation\"", "\"__Confirmation__\""));
        INGEST(rSet, testStr, "Confirmation");

        DATA_PASSES;
    }

    // do_comingfrom
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "\"ComingFrom\"", "\"__ComingFrom__\""));
        INGEST(rSet, testStr, "testMiss do_comingfrom");

        DATA_FAILS;
    }

    // Identifier
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "\"Identifier\"", "\"__Identifier__\""));
        INGEST(rSet, testStr, "testMiss Identifier");

        DATA_FAILS;
    }

    // "Validity"
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "\"Validity\"", "\"__Validity__\""));
        INGEST(rSet, testStr, "testMiss Validity");

        DATA_FAILS;
    }

    // "TmOrg"
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "\"TmOrg\"", "\"__TmOrg__\""));
        INGEST(rSet, testStr, "testMiss TmOrg");

        DATA_FAILS;
    }

    // "TmValidity"
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "\"TmValidity\"", "\"__TmValidity__\""));
        INGEST(rSet, testStr, "testMiss TmValidity");

        DATA_FAILS;
    }

    // "DpsTyp"
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "\"DpsTyp\"", "\"__DpsTyp__\""));

        INGEST(rSet, testStr, "testMiss DpsTyp");

        DATA_FAILS;
    }

    // Ignored field test
    {
        const std::string testStr(JsonPivotDps2);
        INGEST(rSet, testStr, "Ref Ignored field");

        DATA_PASSES;
    }

    // Mismatch type
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "\"DpsTyp\"", "\"SpsTyp\""));
        INGEST(rSet, testStr, "testMiss Mismatch");

        DATA_FAILS;
    }

    // Invalid subObject
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"Identifier\" *: *)\"[^\"]*\"", "$1 34"));
        INGEST(rSet, testStr, "testMiss Invalid subObject");

        DATA_FAILS;
    }

    // COT-Invalid
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"Cause\") *: *[{][^}]*[}]", "$1 : 3"));
        INGEST(rSet, testStr, "COT-Invalid");

        DATA_FAILS;
    }

    // Q-Int-Invalid
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"inconsistent\") *: *[a-zA-Z0-9_-]*", "$1 : \"STR\""));
        INGEST(rSet, testStr, "Q-Int-Invalid");

        DATA_FAILS;
    }

    // Q-Str-Invalid
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"Source\") *: \"*[a-zA-Z0-9_-]*\"", "$1 : 33"));
        INGEST(rSet, testStr, "Q-Str-Invalid");

        DATA_FAILS;
    }

    // Q-Obj-Invalid
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"DetailQuality\") *: [{]*[^}]*[}]", "$1 : 33"));
        INGEST(rSet, testStr, "Q-Obj-Invalid");

        DATA_FAILS;
    }

    // do_comingfrom-Invalid
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"ComingFrom\") *: [{]*[^}]*[}]", "$1 : 33"));
        INGEST(rSet, testStr, "do_comingfrom-Invalid");

        DATA_FAILS;
    }

    // Q-Unknown-field (not an error)
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"FractionOfSecond\") *:", "\"UnknownField\" :"));
        INGEST(rSet, testStr, "Q-Unknown-field");

        DATA_PASSES;
    }

    // MVF-Invalid-Pivot-type
    {
        const std::string testStr(replace_in_string(JsonPivotMvf,
                "(\"Identifier\") *: \"*pivotMVF\"", "$1 : \"pivotDPS\""));
        INGEST(rSet, testStr, "MVF-Invalid-Pivot-type");

        DATA_FAILS;
    }

    // Unknown-Pivot-Id
    {
        const std::string testStr(replace_in_string(JsonPivotMvf,
                "(\"Identifier\") *: \"*pivotMVF\"", "$1 : \"pivotMVX\""));
        INGEST(rSet, testStr, "Unknown-Pivot-Id");

        DATA_FAILS;
    }

    // No TS
    {
        const std::string testStr(replace_in_string(JsonPivotMvf,
                "\"t\" *:", "\"_t_\" :"));
        INGEST(rSet, testStr, "No TS");

        DATA_FAILS;
    }

    // No Str-StVal
    {
        const std::string testStr(replace_in_string(JsonPivotMvf,
                "(\"stVal\") *: *\"[^\"]*\"", "$1 :33"));
        INGEST(rSet, testStr, "No Str-StVal");

        DATA_FAILS;
    }

    // Bad Confirmation (optional)
    {
        const std::string testStr(replace_in_string(JsonPivotMvf,
                "(\"Confirmation\") *: *[{]*[^}]*[}]", "$1 : 33"));
        INGEST(rSet, testStr, "Bad Confirmation");

        DATA_PASSES;
    }

    // Bad TmOrg
    {
        const std::string testStr(replace_in_string(JsonPivotMvf,
                "(\"TmOrg\") *: *[{]*[^}]*[}]", "$1 : 33"));
        INGEST(rSet, testStr, "Bad TmOrg");

        DATA_FAILS;
    }

    // Bad TmValidity
    {
        const std::string testStr(replace_in_string(JsonPivotMvf,
                "(\"TmValidity\") *: *[{]*[^}]*[}]", "$1 : 33"));
        INGEST(rSet, testStr, "Bad TmValidity");

        DATA_FAILS;
    }

    // Bad MvTyp
    {
        const std::string testStr =
                QUOTE({
                "PIVOT" :{
                    "GTIM": {
                        "Cause": {"stVal": 1},
                        "Confirmation": {"stVal": true},
                        "ComingFrom":  {"stVal": "iec104"},
                        "Identifier":  "pivotMVF",
                        "TmOrg" :  {"stVal":"genuine"},
                        "TmValidity" :  {"stVal":"questionable"},
                        "MvTyp" : 33
                    }
                }
            });
        INGEST(rSet, testStr, "Bad MvTyp");

        DATA_FAILS;
    }

    // Bad SpsTyp
    {
        const std::string testStr =
                QUOTE({
            "PIVOT" :{
                "GTIS": {
                    "Cause": {"stVal": 4},
                    "Confirmation": {"stVal": false},
                    "ComingFrom":  {"stVal": "opcua"},
                    "Identifier":  "pivotSPS",
                    "TmOrg" :  {"stVal":"substituted"},
                    "TmValidity" :  {"stVal":"good"},
                    "SpsTyp" : 33
                }
            }
        });
        INGEST(rSet, testStr, "SpsTyp");

        DATA_FAILS;
    }

    // Bad DpsTyp
    {
        const std::string testStr =
                QUOTE({
            "PIVOT" :{
                "GTIS": {
                    "Cause": {"stVal": 4},
                    "Confirmation": {"stVal": false},
                    "ComingFrom":  {"stVal": "opcua"},
                    "Identifier":  "pivotSPS",
                    "TmOrg" :  {"stVal":"substituted"},
                    "TmValidity" :  {"stVal":"good"},
                    "DpsTyp" : 33
                }
            }
        });
        INGEST(rSet, testStr, "DpsTyp");

        DATA_FAILS;
    }

    // Bad FloatVal
    {
        const std::string testStr(replace_in_string(JsonPivotMvf,
                "(\"mag\") *: *[{][^}]*[}]", "$1 : {\"f\" : 32}"));
        INGEST(rSet, testStr, "Bad FloatVal");

        DATA_FAILS;
    }

    // Miss FloatVal
    {
        const std::string testStr(replace_in_string(JsonPivotMvf,
                "(\"mag\") *: *[{][^}]*[}]", "$1 : {\"i\" : 32}"));
        INGEST(rSet, testStr, "Miss FloatVal");

        DATA_FAILS;
    }

    // Miss IntVal
    {
        const std::string testStr(replace_in_string(JsonPivotMvi,
                "(\"mag\") *: *[{][^}]*[}]", "$1 : {\"f\" : 3.2}"));
        INGEST(rSet, testStr, "Miss IntVal");

        DATA_FAILS;
    }

    // SPS-Invalid-Pivot-type
    {
        const std::string testStr(replace_in_string(JsonPivotSps,
                "(\"Identifier\") *: \"*pivotSPS\"", "$1 : \"pivotMVF\""));
        INGEST(rSet, testStr, "SPS-Invalid-Pivot-type");

        DATA_FAILS;
    }

    // DPS-Invalid-Pivot-type
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"Identifier\") *: \"*pivotDPS\"", "$1 : \"pivotMVF\""));
        INGEST(rSet, testStr, "DPS-Invalid-Pivot-type");

        DATA_FAILS;
    }

    // SPS-Invalid-PIVOT
    {
        const std::string testStr = QUOTE({ "PIVOT" :33 });
        INGEST(rSet, testStr, "SPS-Invalid-PIVOT");

        DATA_FAILS;
    }

    // SPS-Invalid-GTIS
    {
        const std::string testStr = QUOTE({ "PIVOT" :{ "GTIS": 33 }});
        INGEST(rSet, testStr, "SPS-Invalid-GTIS");

        DATA_FAILS;
    }

    // SPS-Mismatch-GTIS
    {
        const std::string testStr(replace_in_string(JsonPivotDps,
                "(\"GTIS\")", "\"GTISM\""));
        INGEST(rSet, testStr, "SPS-Mismatch-GTIS");

        DATA_FAILS;
    }
}

#undef INGEST
#define INGEST(rSet, json, _type)  \
    ReadingSet rSet;                                    \
    rSet.append(JsonToReading(json, _type));            \
                                                        \
    f_reset_outputs();                                  \
    ASSERT_EQ(called_output_handle, nullptr);           \
    ASSERT_EQ(called_output_reading, nullptr);          \
    TRACE("  * INGEST() /" _type);                      \
    plugin_ingest(plugin, &rSet);                       \
    ASSERT_EQ(called_output_handle, &stubOutH);         \
    ASSERT_EQ(called_output_reading, &rSet);

// Test lugin APIS
TEST(Pivot2Opcua_Filter, Pivot2OpcuaFilterApi) {
    TITLE("*** TEST FILTER Pivot2OpcuaFilterApi");

    ConfigCategory* pConf(&::configEnabled);
    void(*fOut)(OUTPUT_HANDLE * out, READINGSET *set) = &::f_output_stream;
    PLUGIN_HANDLE plugin = plugin_init(pConf, stubOutH, fOut);

    // reference test
    {
        const std::string testStr(JsonPivotMvf);
        INGEST(rSet, testStr, "Ref test");

        DATA_PASSES;
    }

    // Disable filter
    {
        string conf(QUOTE({ "enable" : { "description" : "", "value" : "false", "type" : "string"}}));
        plugin_reconfigure(plugin, conf);
        const std::string testStr(JsonPivotMvf);
        INGEST(rSet, testStr, "Ref test");

        DATA_FAILS;
    }

    // Re-Enable filter
    {
        string conf(QUOTE({ "enable" : { "description" : "", "value" : "true", "type" : "string"}}));
        plugin_reconfigure(plugin, conf);
        const std::string testStr(JsonPivotMvf);
        INGEST(rSet, testStr, "Ref test");

        DATA_PASSES;
    }

    // Cover Asset tracking
    {
        ManagementClient client("stub", 3333);
        AssetTracker tracker(&client, "FILTER OPC");
        string conf(QUOTE({ "enable" : { "description" : "", "value" : "true", "type" : "string"}}));
        plugin_reconfigure(plugin, conf);
        const std::string testStr(JsonPivotMvf);
        INGEST(rSet, testStr, "Ref test");

        DATA_PASSES;
    }
    plugin_shutdown(plugin);
    plugin_shutdown(nullptr);
}
