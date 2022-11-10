#include <gtest/gtest.h>
#include <plugin_api.h>
#include <string.h>
#include <exception>
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

using namespace std;
using namespace rapidjson;

/** Test pivot2opcua_data APIs */

/* ExchangedDataC */
TEST(Pivot2Opcua_Data, ExchangedDataC) {
    TITLE("*** TEST DATA ExchangedDataC");
    rapidjson::Document doc;

    static const string JsonTests = QUOTE({
        "testNotObj" : "a string",
        "testIec" : {"name":"iec104","address":"18325-6441925", "typeid":"C_DC_TA_1"},
        "testNoName" : {"address":"1234", "typeid":"nothing realistic"},
        "testBadName" : {"name":true,"address":"1234", "typeid":"nothing realistic"},
        "testNoAddr" : {"name":"opcua","typeid":"opcua_dps"},
        "testBadAddr1" : {"name":"opcua","address":14, "typeid":"opcua_dps"},
        "testNoType" : {"name":"opcua","address":"1234"},
        "testBadType1" : {"name":"opcua","address":"1234", "typeid":1.4},
        "testPreCheckOk" : {"name":"opcua","address":"1234", "typeid":"nothing realistic"}});

    doc.Parse(JsonTests.c_str());
    ASSERT_TRUE(!doc.HasParseError());
    Value& testV(doc["testIec"]);

    ASSERT_THROW(ExchangedDataC x1(doc["testIec"]), ExchangedDataC::NotAnS2opcInstance);
    ASSERT_THROW(ExchangedDataC x2(doc["testNotObj"]), std::exception);
    ASSERT_THROW(ExchangedDataC x2(doc["testNoName"]), std::exception);
    ASSERT_THROW(ExchangedDataC x3(doc["testBadName"]), std::exception);
    ASSERT_THROW(ExchangedDataC x2(doc["testNoAddr"]), std::exception);
    ASSERT_THROW(ExchangedDataC x3(doc["testBadAddr1"]), std::exception);
    ASSERT_THROW(ExchangedDataC x3(doc["testNoType"]), std::exception);
    ASSERT_THROW(ExchangedDataC x3(doc["testBadType1"]), std::exception);
    ASSERT_NO_THROW(ExchangedDataC exDa(doc["testPreCheckOk"]));

    // ASSERT_EQ(1, 2);
}

/* PivotElement */
TEST(Pivot2Opcua_Data, PivotElement) {
    TITLE("*** TEST DATA PivotElement");
    const PivotElement elt1("type", "opcType");
    ASSERT_EQ(elt1.m_pivot_type, string("type"));
    ASSERT_EQ(elt1.m_opcType, string("opcType"));
}   // test PivotElement

/* DataDictionnary */
TEST(Pivot2Opcua_Data, DataDictionnary) {
    TITLE("*** TEST DATA DataDictionnary");
    const DataDictionnary dic1(Json_ExDataOK);

    PivotElementMap_t::const_iterator it;
    it = dic1.find("pivot42");
    ASSERT_TRUE(dic1.isEmpty(it));

    it = dic1.find("pivot1");
    ASSERT_FALSE(dic1.isEmpty(it));
    const PivotElement& pivot1(it->second);
    ASSERT_EQ(pivot1.m_pivot_type, "type1");
    ASSERT_EQ(pivot1.m_opcType, "opcua_dps");

    // Test invalid configs
    string Json_Broken = QUOTE({"k": "v" : "s"});
    ASSERT_THROW(DataDictionnary dicNOk(Json_Broken), std::exception);

    string Json_BadPivot_id_type = replace_in_string(Json_ExDataOK, QUOTE("pivotMVF"), QUOTE(false));
    ASSERT_THROW(DataDictionnary dicNOk(Json_BadPivot_id_type), std::exception);

    string Json_BadJson = replace_in_string(Json_ExDataOK, QUOTE("pivotMVF"), QUOTE(false));
    ASSERT_THROW(DataDictionnary dicNOk(Json_BadJson), std::exception);

    string Json_BadSection = replace_in_string(Json_ExDataOK, QUOTE("exchanged_data"), QUOTE("exchanged_datas"));
    ASSERT_THROW(DataDictionnary dicNOk(Json_BadSection), std::exception);

    string Json_BadVersion = replace_in_string(Json_ExDataOK, QUOTE("version" : "1.0"), QUOTE("version" : "2.0"));
    ASSERT_THROW(DataDictionnary dicNOk(Json_BadVersion), std::exception);

    string Json_NoVersion = replace_in_string(Json_ExDataOK, QUOTE("version" : "1.0"), QUOTE("versio" : "1.0"));
    ASSERT_THROW(DataDictionnary dicNOk(Json_NoVersion), std::exception);

    string Json_NoDataPoints = replace_in_string(Json_ExDataOK, QUOTE("datapoints"), QUOTE("datapoint"));
    ASSERT_THROW(DataDictionnary dicNOk(Json_NoDataPoints), std::exception);

    string Json_BadDataPoints = QUOTE({"exchanged_data" : {
        "name" : "data1",
        "version" : "1.0",
        "datapoints" : 32 }});
    ASSERT_THROW(DataDictionnary dicNOk(Json_BadDataPoints), std::exception);

    string Json_ExData = QUOTE({"exchanged_data" : ["name" , "data1"]});
    ASSERT_THROW(DataDictionnary dicNOk(Json_ExData), std::exception);


}   // test DataDictionnary


