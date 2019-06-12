#pragma once
/*-------------------------------------------------------------------------------------------
 * qblocks - fast, easily-accessible, fully-decentralized data from blockchains
 * copyright (c) 2018 Great Hill Corporation (http://greathill.com)
 *
 * This program is free software: you may redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version. This program is
 * distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details. You should have received a copy of the GNU General
 * Public License along with this program. If not, see http://www.gnu.org/licenses/.
 *-------------------------------------------------------------------------------------------*/
#include "version.h"
#include "accountname.h"
#include "filenames.h"
#include "toml.h"

// #define PROVING 1
// Bit flags to enable / disable various options
#define OPT_HELP    (1<<1)
#define OPT_VERBOSE (1<<2)
#define OPT_DOLLARS (1<<3)
#define OPT_WEI     (1<<4)
#define OPT_ETHER   (1<<5)
#define OPT_DENOM   (OPT_DOLLARS|OPT_WEI|OPT_ETHER)
#define OPT_PARITY  (1<<6)
#ifndef PROVING
#define OPT_DEFAULT (OPT_HELP|OPT_VERBOSE|OPT_DENOM|OPT_PARITY)
#else
#define OPT_PROVE   (1<<7)
#define OPT_VERIFY  (1<<8)
#define OPT_TRUEDATA (OPT_PROVE|OPT_VERIFY)
#define OPT_DEFAULT (OPT_HELP|OPT_VERBOSE|OPT_DENOM|OPT_PARITY|OPT_TRUEDATA)
#endif
#define OPT_RUNONCE (1<<9)
#define OPT_RAW     (1<<10)
#define OPT_PREFUND (1<<11)
#define OPT_OUTPUT  (1<<12)

//-----------------------------------------------------------------------------
enum format_t { NONE1 = 0, JSON1 = (1<<1), TXT1 = (1<<2), CSV1 = (1<<3), API1 = (1<<4) };
namespace qblocks {
    class COption;
    class COptionsBase {
    public:
        CStringArray prefunds;
        CStringArray arguments;
        //TODO(tjayrush): global data
        uint32_t enableBits;
        bool needsOption;
        bool isReadme;
        bool isRaw;
        bool isVeryRaw;
        bool api_mode;
        format_t exportFmt;
        blkrange_t scanRange;

        // redirecting cout for --output option
        streambuf *origCout;
        ostringstream strCout;
        string_q outputFn;
        void closeRedirect(void);

        CStringArray commandLines;
        uint64_t minArgs;
        CRuntimeClass *sorts[5];

        COptionsBase(void);
        virtual ~COptionsBase(void) { closeRedirect(); }

        //--------------------------------------------------------------------------------
        static string_q g_progName;
        void setProgName(const string_q& name);
        string_q getProgName(void) const;
        bool prepareArguments(int argc, const char *argv[]);
        virtual bool parseArguments(string_q& command) = 0;
        bool builtInCmd(const string_q& arg);
        bool standardOptions(string_q& cmdLine);

        // supporting special block names
        typedef bool (*NAMEVALFUNC)(CNameValue& pair, void *data);
        static CNameValueArray specials;
        static void loadSpecials(void);
        static bool findSpecial(CNameValue& pair, const string_q& arg);
        static bool forEverySpecialBlock(NAMEVALFUNC func, void *data);

        // supporting named accounts
        CAccountNameArray namedAccounts;
        CFilename namesFile;
        bool loadNames(void);
        bool getNamedAccount(CAccountName& acct, const string_q& addr) const;
        string_q getNamedAccount(const string_q& addr) const;

        // enabling options
        bool isEnabled(uint32_t q) const;
        void optionOff(uint32_t q);
        void optionOn (uint32_t q);

        string_q expandOption(string_q& arg);
        bool     usage(const string_q& errMsg = "") const;
        string_q usageStr(const string_q& errMsg = "") const;
        string_q purpose(void) const;
        string_q options(void) const;
        string_q descriptions(void) const;
        string_q oneDescription(const string_q& sN, const string_q& lN, const string_q& d, bool isMode, bool required) const;
        string_q notes(void) const;
        virtual string_q postProcess(const string_q& which, const string_q& str) const { return str; }

    protected:
        void registerOptions(size_t nP, COption const *pP);
        virtual void Init(void) = 0;
        const COption *pParams;
        size_t cntParams;
        string_q hiUp1;
        string_q hiUp2;
        string_q hiDown;
    };

    //--------------------------------------------------------------------------------
    class CDefaultOptions : public COptionsBase {
    public:
        CDefaultOptions() {}
        bool parseArguments(string_q& command) { return true; }
        void Init(void) {}
    };

    //--------------------------------------------------------------------------------
    class COption {
    public:
        string_q  shortName;
        string_q  longName;
        string_q  description;
        string_q  permitted;
        bool      hidden;
        bool      mode;
        bool      optional;
        COption(const string_q& name, const string_q& descr);
    };

    //--------------------------------------------------------------------------------
    extern int sortParams(const void *c1, const void *c2);

    //--------------------------------------------------------------------------------
    extern uint64_t verbose;

    //--------------------------------------------------------------------------------
    extern void     editFile  (const string_q& fileName);
    extern string_q configPath(const string_q& part);

    //--------------------------------------------------------------------------------
    class CToml;
    extern const CToml *getGlobalConfig(const string_q& name = "");

    typedef bool (*UINT64VISITFUNC)(uint64_t num, void *data);
    typedef uint64_t (*HASHFINDFUNC)(const hash_t& hash, void *data);
    class COptionsBlockList {
    public:
        CBlockNumArray numList;
        CStringArray hashList;
        HASHFINDFUNC hashFind;
        blknum_t start;
        blknum_t stop;
        blknum_t skip;
        blknum_t latest;
        void Init(void);
        string_q parseBlockList(const string_q& arg, blknum_t latest);
        COptionsBlockList(void);
        bool forEveryBlockNumber(UINT64VISITFUNC func, void *) const;
        bool hasBlocks(void) const { return (hashList.size() || numList.size() || (start != stop)); }
        bool isInRange(blknum_t bn) const;
        blknum_t parseBlockOption(string_q& msg, blknum_t lastBlock, direction_t offset) const;
    };

    class COptionsTransList {
    public:
        string_q queries;
        void Init(void);
        string_q parseTransList(const string_q& arg);
        COptionsTransList(void);
        string_q int_2_Str(void) const;
        bool hasTrans(void) const { return !queries.empty(); }
    };

    extern const char *STR_DEFAULT_WHENBLOCKS;
    extern bool prepareEnv(int argc, const char *argv[]);
    extern string_q cleanFmt(const string_q& str, format_t fmt);

}  // namespace qblocks
