#ifndef INCLUDE_PIVOT2OPCUA_FILTER_H_
#define INCLUDE_PIVOT2OPCUA_FILTER_H_
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
#include <math.h>
#include <string>
#include <mutex>
#include <map>
#include <regex>
#include <memory>
#include <vector>
#include <exception>

// Fledge includes
#include "reading_set.h"
#include "config_category.h"
#include "filter.h"
#include "logger.h"

// Project headers
#include "pivot2opcua_common.h"
#include "pivot2opcua_data.h"

using std::string;
using std::vector;

class InvalidPivotContent : public std::exception {     // //NOSONAR
 public:
    explicit InvalidPivotContent(const std::string& context):
    mContext(context) {}
    inline const char* context(void)const {return mContext.c_str();}

 private:
    const string mContext;
};

/**
 * Convert the incoming data to use a logarithmic scale
 */
class Pivot2OpcuaFilter : public FledgeFilter {
 public:
    Pivot2OpcuaFilter(const string& filterName,
            ConfigCategory& filterConfig,
            OUTPUT_HANDLE *outHandle,
            OUTPUT_STREAM output);
    virtual ~Pivot2OpcuaFilter(void) = default;
    void ingest(READINGSET *readingSet);
    void reconfigure(const string& newConfig);

 private:
    using Readings = vector<Reading*>;
    using Datapoints = vector<Datapoint*>;

    class PivotQuality {
     public:
        explicit PivotQuality(const Datapoints* dict);
        uint32_t toDetails(void)const;
        string getSource(void)const {return m_Source;}
        const string& validity(void)const {return m_Validity;}

     private:
        static const uint32_t Mask_badReference      = 0x0001u;
        static const uint32_t Mask_failure           = 0x0002u;
        static const uint32_t Mask_inconsistent      = 0x0004u;
        static const uint32_t Mask_innaccurate       = 0x0008u;
        static const uint32_t Mask_oldData           = 0x0010u;
        static const uint32_t Mask_oscillatory       = 0x0020u;
        static const uint32_t Mask_outOfRange        = 0x0040u;
        static const uint32_t Mask_overflow          = 0x0080u;
        static const uint32_t Mask_test              = 0x1000u;
        static const uint32_t Mask_operator_blocked  = 0x2000u;

        bool m_test;
        bool m_operatorBlocked;
        string m_Validity;
        string m_Source;
        bool m_Detail_badRef;
        bool m_Detail_failure;
        bool m_Detail_inconsistent;
        bool m_Detail_innacurate;
        bool m_Detail_oldData;
        bool m_Detail_oscillatory;
        bool m_Detail_outOfRange;
        bool m_Detail_overflow;
    };

    class PivotTimestamp {
     public:
        explicit PivotTimestamp(const Datapoints* dict);
        uint32_t toDetails(void)const;
        inline int64_t nbSec(void)const {return time_nbSec;}

     private:
        static const uint32_t Mask_clockFailure      = 0x0001u;
        static const uint32_t Mask_clockNotSynch     = 0x0002u;
        static const uint32_t Mask_leapSecondKnown   = 0x0004u;

        int64_t time_frac;
        int64_t time_nbSec;
        bool clockFailure;
        bool clockNotSynch;
        bool leapSecondKnown;
        int64_t timeAccuracy;
    };

    /** Attributes for Qualified value (quality + timestamp) */
    class Qualified {
     public:
        explicit Qualified(Datapoints* dict);
        virtual ~Qualified(void) = default;

        virtual Datapoint* createData(const std::string& typeName) = 0;

        const PivotTimestamp ts;
        const PivotQuality quality;
    };

    /*** A qualified Mag value */
    class QualifiedMagVal : public Qualified {
     public:
        explicit QualifiedMagVal(Datapoints* dict);
        ~QualifiedMagVal(void) override = default;
        Datapoint* createData(const std::string& typeName) override;

     private:
        Datapoints* m_mag;
        bool hasF;
        bool hasI;
        double fVal;
        int64_t iVal;
    };
    using QualifiedMagValPtr = std::unique_ptr<QualifiedMagVal>;

    /*** A qualified Boolean value */
    class QualifiedBoolStVal : public Qualified {
     public:
        explicit QualifiedBoolStVal(Datapoints* dict);
        ~QualifiedBoolStVal(void) override = default;
        Datapoint* createData(const std::string& typeName) override;

        const bool bVal;
    };
    using QualifiedBoolStValPtr = std::unique_ptr<QualifiedBoolStVal>;

    /*** A qualified String value */
    class QualifiedStringStVal : public Qualified {
     public:
        explicit QualifiedStringStVal(Datapoints* dict);
        virtual ~QualifiedStringStVal(void) = default;
        virtual Datapoint* createData(const std::string& typeName);

        const string sVal;
    };
    using QualifiedStringStValPtr = std::unique_ptr<QualifiedStringStVal>;

    /** Common behavior for PIVOT measurements*/
    class CommonMeasurePivot {
     public:
        explicit CommonMeasurePivot(const Datapoints* dict);
        const std::string& pivotId(void)const {return m_Identifier;}
        virtual ~CommonMeasurePivot(void) = default;
        void updateReading(const DataDictionnary* dictPtr, Reading* orig)const;

     private:
        using FieldDecoder = void (*) (CommonMeasurePivot*, DatapointValue&, const string& name);
        using decoder_map_t = std::map<std::string, FieldDecoder>;
        static void ignoreField(CommonMeasurePivot* pivot, DatapointValue& data, const string& name);
        static void decodeConfirmation(CommonMeasurePivot* pivot, DatapointValue& data, const string& name);
        static void decodeCause(CommonMeasurePivot* pivot, DatapointValue& data, const string& name);
        static void decodeComingFrom(CommonMeasurePivot* pivot, DatapointValue& data, const string& name);
        static void decodeIdentifier(CommonMeasurePivot* pivot, DatapointValue& data, const string& name);
        static void decodeTmOrg(CommonMeasurePivot* pivot, DatapointValue& data, const string& name);
        static void decodeTmValidity(CommonMeasurePivot* pivot, DatapointValue& data, const string& name);
        static void decodeMagVal(CommonMeasurePivot* pivot, DatapointValue& data, const string& name);
        static void decodeSpsVal(CommonMeasurePivot* pivot, DatapointValue& data, const string& name);
        static void decodeDpsVal(CommonMeasurePivot* pivot, DatapointValue& data, const string& name);

        static const decoder_map_t decoder_map;

        static const uint32_t FieldMask_cot = 0x0001;   // COT
        static const uint32_t FieldMask_pty = 0x0002;   // Pivot Type
        static const uint32_t FieldMask_cnf = 0x0004;   // Confirmation
        static const uint32_t FieldMask_cmf = 0x0010;   // Coming From
        static const uint32_t FieldMask_idt = 0x0020;   // Identifier
        static const uint32_t FieldMask_tmo = 0x0040;   // TmOrg
        static const uint32_t FieldMask_tmv = 0x0080;   // TmValidity
        static const uint32_t FieldMask_typ = 0x0100;   // type
        static const uint32_t FieldMask_val = 0x0200;   // value
        static const uint32_t FieldMask_vqu = 0x0400;   // value quality
        static const uint32_t Mandatory_fields =
                FieldMask_cot | FieldMask_cmf | FieldMask_idt | FieldMask_typ |
                FieldMask_val | FieldMask_vqu;

        static void logMissingMandatoryFields(const std::string& pivotName, uint32_t fields);

        uint32_t        m_readFields;   // A mask to  FieldMask_XXX
        string          m_pivotType;
        bool            m_Confirmation;
        int             m_Cause;
        string          m_ComingFrom;
        string          m_Identifier;
        string          m_TmOrg;
        string          m_TmValidity;
        QualifiedMagValPtr m_MagVal;
        QualifiedBoolStValPtr m_BoolVal;
        QualifiedStringStValPtr m_StrVal;
        Qualified*      m_Qualified; /// Pointer to a QualifiedxxPtr above
    };

    static Datapoints* findDictElement(Datapoints* dict, const string& key);
    static string getStringStVal(Datapoints* dict, const string& context);
    static int64_t getIntStVal(Datapoints* dict, const string& context);
    void pivot2opcua(Reading* readDp);
    void                         handleConfig(const ConfigCategory& config);
    DataDictionnary_Ptr          m_dictionnary;
    std::mutex                   m_configMutex;
};


#endif  //INCLUDE_PIVOT2OPCUA_FILTER_H_
