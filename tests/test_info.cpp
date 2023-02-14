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


extern "C" {
	PLUGIN_INFORMATION *plugin_info();
};

TEST(Pivot2Opcua_Plugin, PluginInfo) {
    TITLE("*** TEST Info PluginInfo");

    PLUGIN_INFORMATION *info = plugin_info();
    ASSERT_STREQ(info->name, "pivottoopcua");
    ASSERT_STREQ(info->type, PLUGIN_TYPE_FILTER);
}

