#ifndef INCLUDE_PIVOT2OPCUA_DATA_H_
#define INCLUDE_PIVOT2OPCUA_DATA_H_
/*
 * Fledge "Log" filter plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Jeremie Chabod
 */

// System headers
#include <string>
#include <unordered_map>
#include <memory>

// Fledge includes
#include "config_category.h"
#include "logger.h"

/**************************************************************************/
/**
 * This class parses the configuration of a single OPC UA variable (as Datapoint)
 */
class ExchangedDataC {
 public:
    class NotAnS2opcInstance : public std::exception{};
    /** Expected format:
      {
          "name":"s2opcua",
          "address":"<nodeid>",
          "typeid":"<opcua_sps|...>",
       }
     */
    explicit ExchangedDataC(const rapidjson::Value& json);
    virtual ~ExchangedDataC(void) = default;

 private:
    const bool mPreCheck;
    /**
     * @return true if all checks are OK:
     * - json is an object
     * - contains keys "name", "address" and "typeid" with string values
     * Throws exception() otherwise.
     * NotAnS2opcInstance is thrown if json["name"] is not opcua
     */
    bool internalChecks(const rapidjson::Value& json)const;

 public:
    const std::string address;
    const std::string typeId;
};  // class ExchangedDataC


/**
 * This structure contains all required information for a given Pivot data
 */
struct PivotElement {
    explicit PivotElement(const std::string& pivot_type,
            const std::string& address,
            const std::string& opcType):
        m_pivot_type(pivot_type),
        m_opcType(opcType) {}
    const std::string m_pivot_type;
    const std::string m_opcType;
};

using PivotElementMap_t = std::unordered_map<std::string, PivotElement>;

/**************************************************************************/
/**
 * Object used to extract data from "exchanged_data" json
 */
class DataDictionnary {
 public:
    /**
     * @param jsonExData the full "exchanged_data" configuration category
     */
    explicit DataDictionnary(const std::string& jsonExData);

    /**
     * @param key the Pivot Id to search in the dictionnary
     * @return a const_iterator to the element (::isEmpty can be used to check result)
     */
    inline PivotElementMap_t::const_iterator find(const std::string& key)const;
    /**
     * @param it A result of call to ::find
     * @return false if it contains an element. True otherwise
     */
    inline bool isEmpty(const PivotElementMap_t::const_iterator& it)const;

 private:
    PivotElementMap_t m_map;
};

using DataDictionnary_Ptr = std::unique_ptr<DataDictionnary>;

inline bool
DataDictionnary::isEmpty(const PivotElementMap_t::const_iterator& it)const {
    return it == m_map.end();
}

inline PivotElementMap_t::const_iterator
DataDictionnary::find(const std::string& key)const {
    return m_map.find(key);
}

#endif  //INCLUDE_PIVOT2OPCUA_DATA_H_
