/*
 * Fledge "log" filter plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Jeremie CHABOD
 */

// System headers

// Fledge includes
#include "logger.h"
#include "config_category.h"
#include "plugin_api.h"
#include "version.h"

// Project includes
#include "plugin_default_conf.h"
#include "pivot2opcua_filter.h"

static const char * const default_config = QUOTE({
                "plugin" : {
                        "description" : "PIVOT to OPCUA filter plugin",
                        "type" : "string",
                        "default" : FILTER_NAME,
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
                        "default" : EXCHANGED_DATA_DEF
                       },
                "match" : {
                        "description" : "An optional regular expression to match in the asset name.",
                        "type": "string",
                        "default": "",
                        "order": "1",
                        "displayName": "Asset filter"}
                });

using std::string;
/**
 * The Filter plugin interface
 */
extern "C" {

/**
 * The plugin information structure
 */
static PLUGIN_INFORMATION info = {
        FILTER_NAME,              // Name
        FLEDGE_FILTER_PIVOTTOOPCUA_VERSION,                  // Version
        0,                        // Flags
        PLUGIN_TYPE_FILTER,       // Type
        "1.0.0",                  // Interface version
        default_config                 // Default plugin configuration
};  // //NOSONAR (Fledge API)

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info() {
        return &info;
}

/**
 * Initialise the plugin, called to get the plugin handle.
 * We merely create an instance of our LogFilter class
 *
 * @param config     The configuration category for the filter
 * @param outHandle  A handle that will be passed to the output stream
 * @param output     The output stream (function pointer) to which data is passed
 * @return           An opaque handle that is used in all subsequent calls to the plugin
 */
PLUGIN_HANDLE plugin_init(ConfigCategory* config,
                          OUTPUT_HANDLE *outHandle,
                          OUTPUT_STREAM output) {
    Pivot2OpcuaFilter *filter = new Pivot2OpcuaFilter(FILTER_NAME,  // //NOSONAR (Fledge API)
            *config,
            outHandle,
            output);

        return static_cast<PLUGIN_HANDLE>(filter);
}

/**
 * Ingest a set of readings into the plugin for processing
 *
 * @param handle     The plugin handle returned from plugin_init
 * @param readingSet The readings to process
 */
void plugin_ingest(PLUGIN_HANDLE handle,
        READINGSET *readingSet) {
    Pivot2OpcuaFilter *filter = static_cast<Pivot2OpcuaFilter*>(handle);
    filter->ingest(readingSet);
}

/**
 * Plugin reconfiguration method
 *
 * @param handle     The plugin handle
 * @param newConfig  The updated configuration
 */
void plugin_reconfigure(PLUGIN_HANDLE handle, const std::string& newConfig) {
    Pivot2OpcuaFilter *filter = static_cast<Pivot2OpcuaFilter*>(handle);
    filter->reconfigure(newConfig);
}

/**
 * Call the shutdown method in the plugin
 */
void plugin_shutdown(PLUGIN_HANDLE handle) {
    Pivot2OpcuaFilter *filter = static_cast<Pivot2OpcuaFilter*>(handle);
    delete filter;  // //NOSONAR (Fledge API)
}

// End of extern "C"
};
