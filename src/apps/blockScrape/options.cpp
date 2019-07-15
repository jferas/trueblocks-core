/*-------------------------------------------------------------------------
 * This source code is confidential proprietary information which is
 * Copyright (c) 2017 by Great Hill Corporation.
 * All Rights Reserved
 *------------------------------------------------------------------------*/
#include "options.h"

//---------------------------------------------------------------------------------------------------
static const COption params[] = {
    COption("-nBlocks:<num>",     "maximum number of blocks to process (defaults to 5000)"),
    COption("@nBlockProcs:<num>", "number of block channels for blaze"),
    COption("@nAddrProcs:<num>",  "number of address channels for blaze"),
    COption("",                   "Decentralized blockchain scraper and block cache.\n"),
};
static const size_t nParams = sizeof(params) / sizeof(COption);

#define indexFolder_sorted (getCachePath("addr_index/sorted/"))
//---------------------------------------------------------------------------------------------------
bool COptions::parseArguments(string_q& command) {

    if (!standardOptions(command))
        return false;

    Init();
    explode(arguments, command, ' ');
    for (auto arg : arguments) {
        if (startsWith(arg, "-n:") || startsWith(arg, "--nBlocks:")) {
            arg = substitute(substitute(arg, "-n:", ""), "--nBlocks:", "");
            if (!isUnsigned(arg))
                return usage("--nBlocks must be a non-negative number. Quitting...");
            nBlocks = str_2_Uint(arg);

        } else if (startsWith(arg, "--nBlockProcs:")) {
            arg = substitute(arg, "--nBlockProcs:", "");
            if (!isUnsigned(arg))
                return usage("--nBlockProcs must be a non-negative number. Quitting...");
            nBlockProcs = str_2_Uint(arg);

        } else if (startsWith(arg, "--nAddrProcs:")) {
            arg = substitute(arg, "--nAddrProcs:", "");
            if (!isUnsigned(arg))
                return usage("--nAddrProcs must be a non-negative number. Quitting...");
            nAddrProcs = str_2_Uint(arg);

        } else if (startsWith(arg, '-')) {  // do not collapse
            if (!builtInCmd(arg)) {
                return usage("Invalid option: " + arg);
            }
        }
    }

    // Establish the folders that hold the data...
    establishFolder(indexFolder_sorted);
    establishFolder(indexFolder_finalized);
    establishFolder(indexFolder_staging);
    establishFolder(indexFolder_unripe);
    establishFolder(indexFolder_ripe);
    establishFolder(configPath("cache/tmp/"));

    CBlock latest;
    getBlock_light(latest, "latest");
    latestBlockTs = latest.timestamp;
    latestBlockNum = latest.blockNumber;

    if (!isParity() || !nodeHasTraces()) {
        if (latestBlockNum < firstTraceBlock) {
            cerr << "Waiting for first trace block..." << endl;
            return false;
        }
        else
            return usage("This tool will only run if it is running against a Parity node that has tracing enabled. Quitting...");
    }

    string_q zeroBin = indexFolder_finalized + padNum9(0) + "-" + padNum9(0) + ".bin";
    if (!fileExists(zeroBin)) {
        ASSERT(prefunds.size() == 8893);  // This is a known value
        CStringArray appearances;
        for (auto prefund : prefunds) {
            CStringArray parts;
            explode(parts, prefund, '\t');
            // The prefund transactions have a zero for thier block numbers and an index
            // into thier location in the list of presale addresses. We need to do this so we
            // can distringuish them when they are exported.
            ostringstream os;
            os << parts[0] << "\t" << padNum9(0) << "\t" << padNum5((uint32_t)appearances.size()) << endl;
            appearances.push_back(os.str());
        }
        writeIndexAsBinary(zeroBin, appearances); // also writes the bloom file
    }

    return true;
}

//---------------------------------------------------------------------------------------------------
void COptions::Init(void) {
    registerOptions(nParams, params);
    optionOn(OPT_RUNONCE | OPT_PREFUND);

    nBlockProcs = 30;
    nAddrProcs  = 90;
    nBlocks     = 2000;
    minArgs     = 0;
}

//---------------------------------------------------------------------------------------------------
COptions::COptions(void) {
    Init();
}

//--------------------------------------------------------------------------------
COptions::~COptions(void) {
}
