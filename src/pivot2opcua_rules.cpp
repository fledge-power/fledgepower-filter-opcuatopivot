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

//SOP2C headers

namespace Rules {
/**************************************************************************/
/*             TRANSLATION RULES                                          */
/**************************************************************************/
const StrStrMap_t pivotTx2GTIx {
    {"PIVOTTM", "GTIM"},
    {"PIVOTTS", "GTIS"}
};  // pivotTx2GTIx

const Str2Vect_map_t gtix2pivotTypeMap {
    {"GTIM", {"MvTyp"}},
    {"GTIS", {"SpsTyp", "DpsTyp"}}
};  // gtix2pivotTypeMap

}   // namespace Rules
