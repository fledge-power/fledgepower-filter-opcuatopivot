/*
 * Fledge "Log" filter plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */

#include "pivot2opcua_rules.h"

// System headers

// Fledge headers

// Project headers
#include "pivot2opcua_common.h"
#include "pivot2opcua_filter.h"

/** Note
 * This file contains all maps used in the filter code. They are separated from the
 * This remaining of the code because GCOV and SONAR tools see uncovered lines to these declarations
 * This file only contains those maps and is excluded from coverage measures
 */

const Pivot2OpcuaFilter::CommonMeasurePivot::decoder_map_t
Pivot2OpcuaFilter::CommonMeasurePivot::decoder_map = {
        {"Confirmation", &decodeConfirmation},
        {"Cause", &decodeCause},
        {"ComingFrom", &decodeComingFrom},
        {"Identifier", &decodeIdentifier},
        {"TmOrg", &decodeTmOrg},
        {"TmValidity", &decodeTmValidity},
        {"MvTyp", &decodeMagVal},
        {"SpsTyp", &decodeSpsVal},
        {"DpsTyp", &decodeDpsVal},
        {"Beh", &ignoreField},
        {"ChgValCnt", &ignoreField},
        {"NormalSrc", &ignoreField},
        {"NormalVal", &ignoreField},
        {"Origin", &ignoreField}
};

namespace Rules {
/**************************************************************************/
/*             TRANSLATION RULES                                          */
/**************************************************************************/

const Str2Vect_map_t gtix2pivotTypeMap {
    {"GTIM", {"MvTyp"}},
    {"GTIS", {"SpsTyp", "DpsTyp"}}
};  // gtix2pivotTypeMap

}   // namespace Rules
