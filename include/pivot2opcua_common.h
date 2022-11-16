#ifndef  INCLUDE_PIVOT2OPCUA_COMMON_H_
#define  INCLUDE_PIVOT2OPCUA_COMMON_H_
/*
 * Fledge north service plugin
 *
 * Copyright (c) 2021 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Jeremie Chabod
 */

// System includes
#include <stdint.h>
#include <stdlib.h>
#include <map>
#include <utility>
#include <string>
#include <vector>

// Fledge includes
#include "logger.h"
#include "rapidjson/document.h"

// Project includes

/**************************************************************************/
/*                DEFINITIONS                                             */
/**************************************************************************/
static constexpr const char*const JSON_EXCHANGED_DATA = "exchanged_data";
static constexpr const char*const JSON_DATAPOINTS = "datapoints";
static constexpr const char*const JSON_PROTOCOLS = "protocols";
static constexpr const char*const JSON_LABEL = "label";
static constexpr const char*const JSON_PIVOT_ID = "pivot_id";
static constexpr const char*const JSON_PIVOT_TYPE = "pivot_type";
static constexpr const char*const JSON_VERSION = "version";

static constexpr const char*const PROTOCOL_S2OPC = "opcua";
static constexpr const char*const JSON_PROT_NAME = "name";
static constexpr const char*const JSON_PROT_ADDR = "address";
static constexpr const char*const JSON_PROT_TYPEID = "typeid";

static constexpr const char*const JSON_PROT_MAPPING = "mapping_rule";

static constexpr const char*const EXCH_DATA_VERSION = "1.0";
static constexpr const char*const PROTO_TRANSLATION_VERSION = "1.0";

/**************************************************************************/
/*                HELPER MACROS                                           */
/**************************************************************************/
using StringVect_t = std::vector<std::string>;
using StrStrMap_t = std::map<std::string, std::string>;
using Str2Vect_map_t = std::map<std::string, const StringVect_t>;

/* HELPER MACROS*/
static Logger* const logger(Logger::getLogger());  //NOSONAR FLEDGE API
#define LOG_DEBUG logger->debug
#define LOG_INFO logger->info
#define LOG_WARNING logger->warn
#define LOG_ERROR logger->error
#define LOG_FATAL logger->fatal

#define _PP_XSTR(x) #x
#define _PP_STR(x) _PP_XSTR(x)
#define ASSERT_CONTEXT __FILE__ ":" _PP_STR(__LINE__) ":"

#define ASSERT(c,  ...) do { \
    if (!(c)) {\
        LOG_FATAL("ASSERT FAILED in " ASSERT_CONTEXT __VA_ARGS__);\
        throw std::exception();\
    }\
} while (0)

#define ASSERT_NOT_NULL(c) ASSERT((c) != NULL, "NULL pointer:'" #c "'")

#endif  //   INCLUDE_PIVOT2OPCUA_COMMON_H_
