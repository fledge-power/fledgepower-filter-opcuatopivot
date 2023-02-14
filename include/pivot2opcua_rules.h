/*
 * Fledge north service plugin
 *
 * Copyright (c) 2021 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Jeremie Chabod
 */

#ifndef  INCLUDE_PIVOT2OPCUA_RULES_H_
#define  INCLUDE_PIVOT2OPCUA_RULES_H_

// System includes
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <utility>
#include <string>

// Fledge includes
#include "reading_set.h"

// Project includes
#include "pivot2opcua_common.h"

namespace Rules {
using std::string;

/**************************************************************************/
/*             TRANSLATION RULES                                          */
/**************************************************************************/
/// In pivot object, name of the sub-section, depending on GTIx type
extern const Str2Vect_map_t gtix2pivotTypeMap;

template <typename Tin, typename Tout, typename Tmap = std::map<const Tin&, const Tout&>>
static const Tout&
find_T(const Tmap& map, const Tin& in, const Tout& defaultValue) {
    typename Tmap::const_iterator it(map.find(in));

    if (it != map.end()) {
        return (*it).second;
    }
    return defaultValue;
}

}   // namespace Rules

#endif  //   INCLUDE_PIVOT2OPCUA_RULES_H_
