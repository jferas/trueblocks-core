//TODO: This used to work: watch->nodeBal = getNodeBal(watch->stateHistory, watch->address, watch->firstBlock-1);
/*-------------------------------------------------------------------------
 * This source code is confidential proprietary information which is
 * Copyright (c) 2017 by Great Hill Corporation.
 * All Rights Reserved
 *------------------------------------------------------------------------*/
#include "options.h"

//---------------------------------------------------------------------------------------------------
static const COption params[] = {
    COption("~address_list",      "one or more addresses (0x...) to export"),
    COption("-fmt:<fmt>",         "export format (one of [json|txt|csv])"),
    COption("-articulate",        "articulate transactions, traces, logs, and outputs"),
    COption("@blocks:<on/off>",   "write blocks to the binary cache ('off' by default)"),
    COption("@txs:<on/off>",      "write transactions to the binary cache ('on' by default)"),
    COption("@t(r)aces:<on/off>", "write traces to the binary cache ('off' by default)"),
    COption("@ddos:<on/off>",     "skip over dDos transactions in export ('on' by default)"),
    COption("@maxTraces:<num>",   "if --ddos:on, the number of traces defining a dDos (default = 250)"),
    COption("@start:<num>",       "first block to export (inclusive)"),
    COption("@end:<num>",         "last block to export (inclusive)"),
    COption("",                   "Export full detail of transactions for one or more Ethereum addresses.\n"),
};
static const size_t nParams = sizeof(params) / sizeof(COption);

extern const char* STR_DISPLAY;
//---------------------------------------------------------------------------------------------------
bool COptions::parseArguments(string_q& command) {

    CAccountWatch::registerClass();

    if (!standardOptions(command))
        return false;

    Init();
    explode(arguments, command, ' ');
    for (auto arg : arguments) {
        if (startsWith(arg, "-b") || startsWith(arg, "--blocks")) {
            arg = substitute(substitute(arg, "-b:", ""), "--blocks:", "");
            if (arg != "on" && arg != "off")
                return usage("Please provide either 'on' or 'off' for the --blocks options. Quitting...");
            writeBlocks = (arg == "on" ? true : false);

        } else if (startsWith(arg, "-t") || startsWith(arg, "--txs")) {
            arg = substitute(substitute(arg, "-t:", ""), "--txs:", "");
            if (arg != "on" && arg != "off")
                return usage("Please provide either 'on' or 'off' for the --txs options. Quitting...");
            writeTrxs = (arg == "on" ? true : false);

        } else if (startsWith(arg, "-r") || startsWith(arg, "--traces")) {
            arg = substitute(substitute(arg, "-r:", ""), "--traces:", "");
            if (arg != "on" && arg != "off")
                return usage("Please provide either 'on' or 'off' for the --trace options. Quitting...");
            writeTraces = (arg == "on" ? true : false);

        } else if (startsWith(arg, "-d") || startsWith(arg, "--ddos")) {
            arg = substitute(substitute(arg, "-d:", ""), "--ddos:", "");
            if (arg != "on" && arg != "off")
                return usage("Please provide either 'on' or 'off' for the --ddos options. Quitting...");
            skipDdos = (arg == "on" ? true : false);

        } else if (startsWith(arg, "-m") || startsWith(arg, "--maxTraces")) {
            arg = substitute(substitute(arg, "-m:", ""), "--maxTraces:", "");
            if (!isNumeral(arg))
                return usage("Please provide a number (you provided " + arg + ") for --maxTraces. Quitting...");
            maxTraces = str_2_Uint(arg);

        } else if (arg == "-a" || arg == "--articulate") {
            articulate = true;

        } else if (startsWith(arg, "-s") || startsWith(arg, "--start")) {
            arg = substitute(substitute(arg, "-s:", ""), "--start:", "");
            if (!isNumeral(arg))
                return usage("Not a number for --startBlock: " + arg + ". Quitting.");
            scanRange.first = str_2_Uint(arg);

        } else if (startsWith(arg, "-e") || startsWith(arg, "--end")) {
            arg = substitute(substitute(arg, "-e:", ""), "--end:", "");
            if (!isNumeral(arg))
                return usage("Not a number for --endBlock: " + arg + ". Quitting.");
            scanRange.second = str_2_Uint(arg);

        } else if (startsWith(arg, "0x")) {

            arg = toLower(arg);

            if (!isAddress(arg))
                return usage(arg + " does not appear to be a valid address. Quitting...");

            string_q fn = getMonitorPath(arg);
            if (!fileExists(fn)) {
                fn = (isTestMode() ? substitute(fn, getMonitorPath(""), "./") : fn);
                return usage("File not found '" + fn + ". Quitting...");
            }

            if (fileExists(fn + ".lck"))
                return usage("The cache lock file is present. The program is either already "
                             "running or it did not end cleanly the\n\tlast time it ran. "
                             "Quit the already running program or, if it is not running, "
                             "remove the lock\n\tfile: " + fn + ".lck'. Quitting...");

            //if (fileSize(fn) == 0)
            //    return usage("Nothing to export. Quitting...");

            CAccountWatch watch;
            // below - don't change, sets bloom value also
            watch.setValueByName("address", toLower(arg));
            // above - don't change, sets bloom value also
            watch.setValueByName("name", toLower(arg));
            watch.extra_data = getVersionStr() + "/" + watch.address;
            watch.color = cTeal;
            watch.finishParse();
            monitors.push_back(watch);

        } else if (startsWith(arg, '-')) {  // do not collapse
            if (!builtInCmd(arg)) {
                return usage("Invalid option: " + arg);
            }
        }
    }

    SHOW_FIELD(CTransaction, "traces");

    if (monitors.size() == 0)
        return usage("You must provide at least one Ethereum address. Quitting...");

    // show certain fields and hide others
    SEP4("default field hiding: " + defHide);
    manageFields(defHide, false);
    SEP4("default field showing: " + defShow);
    manageFields(defShow, true);

    CToml toml(getMonitorPath(monitors[0].address + ".toml"));
    SEP4("field hiding: " + toml.getConfigStr("fields", "hide", ""));
    manageFields(toml.getConfigStr("fields", "hide", ""), false);
    SEP4("field showing: " + toml.getConfigStr("fields", "show", ""));
    manageFields(toml.getConfigStr("fields", "show", ""), true );

    // Try to articulate the watched addresses
    for (size_t i = 0 ; i < monitors.size() ; i++) {
        CAccountWatch *watch = &monitors[i];
        watch->abi_spec.loadAbiByAddress(watch->address);
        watch->abi_spec.loadAbiKnown("all");
        string_q path = getMonitorPath(watch->address + ".toml");
        if (fileExists(path)) { // if there's a config file, let's use it
                                // user can tell us the names of other addresses
            CToml thisToml(path);
            string_q str = substitute(substitute(thisToml.getConfigJson("named", "list", ""),"[",""),"=",":");
            CAccountWatch item;
            while (item.parseJson3(str)) {
                item.address   = str_2_Addr(toLower(item.address));
                item.color     = convertColor(item.color);
                item.extra_data = getVersionStr() + "/" + item.address;
                item.finishParse();
                named.push_back(item);
                watch->abi_spec.loadAbiByAddress(item.address);
                item = CAccountWatch();
            }
        }
    }

    if (api_mode)
        exportFmt = TXT1;

    writeBlocks = getGlobalConfig("acctExport")->getConfigBool("settings", "writeBlocks", writeBlocks);;
    writeTrxs   = getGlobalConfig("acctExport")->getConfigBool("settings", "writeTrxs", writeTrxs);;
    writeTraces = getGlobalConfig("acctExport")->getConfigBool("settings", "writeTraces", writeTraces);;
    skipDdos    = getGlobalConfig("acctExport")->getConfigBool("settings", "skipDdos", skipDdos);;
    maxTraces   = getGlobalConfig("acctExport")->getConfigBool("settings", "maxTraces", maxTraces);;

    if (exportFmt != JSON1) {
        string_q deflt, format;

        deflt = getGlobalConfig("acctExport")->getConfigStr("display", "format", STR_DISPLAY);
        format = toml.getConfigStr("formats", "trans_fmt", deflt);
        expContext().fmtMap["transaction_fmt"] = cleanFmt(format, exportFmt);

        if (format.empty())
            return usage("For non-json export a 'trans_fmt' string is required. Check your config file. Quitting...");
        if (!contains(toLower(format), "trace"))
            HIDE_FIELD(CTransaction, "traces");

        deflt = getGlobalConfig("acctExport")->getConfigStr("display", "trace", "{TRACES}");
        format = toml.getConfigStr("formats", "trace_fmt", deflt);
        expContext().fmtMap["trace_fmt"] = cleanFmt(format, exportFmt);

        deflt = getGlobalConfig("acctExport")->getConfigStr("display", "log", "{LOGS}");
        format = toml.getConfigStr("formats", "logentry_fmt", deflt);
        expContext().fmtMap["logentry_fmt"] = cleanFmt(format, exportFmt);
    }

    return true;
}

//---------------------------------------------------------------------------------------------------
void COptions::Init(void) {
    registerOptions(nParams, params);
    optionOn(OPT_PREFUND | OPT_OUTPUT);

    monitors.clear();

    writeBlocks = false;
    writeTrxs = true;
    writeTraces = false;
    skipDdos = true;
    maxTraces = 250;
    articulate = false;

    minArgs = 0;
}

//---------------------------------------------------------------------------------------------------
COptions::COptions(void) {
    ts_array = NULL;
    ts_cnt = 0;
    exportFmt = JSON1;
    Init();
}

//--------------------------------------------------------------------------------
COptions::~COptions(void) {
}

//--------------------------------------------------------------------------------
string_q COptions::postProcess(const string_q& which, const string_q& str) const {

    if (which == "options") {
        return substitute(str, "address_list", "<address> [address...]");

    } else if (which == "notes" && (verbose || COptions::isReadme)) {

        string_q ret;
        ret += "[{addresses}] must start with '0x' and be forty two characters long.\n";
        return ret;
    }
    return str;
}

//-----------------------------------------------------------------------
bool COptions::loadMonitorData(CAppearanceArray_base& apps, const address_t& addr) {

    ENTER("loadMonitorData");
    string_q fn = getMonitorPath(addr);

    size_t nRecords = (fileSize(fn) / sizeof(CAppearance_base));
    ASSERT(nRecords);

    CAppearance_base *buffer = new CAppearance_base[nRecords];
    if (buffer) {
        bzero(buffer, nRecords * sizeof(CAppearance_base));

        CArchive txCache(READING_ARCHIVE);
        if (txCache.Lock(fn, modeReadOnly, LOCK_NOWAIT)) {
            txCache.Read(buffer, sizeof(CAppearance_base), nRecords);
            txCache.Release();
        } else {
            cerr << "Could not open cache file.";
            return false;
        }

        // Add to the apps which may be non-empty
        apps.reserve(apps.size() + nRecords);
        for (size_t i = 0 ; i < nRecords ; i++) {
            if (buffer[i].blk == 0)
                prefundMap[buffer[i].txid] = addr;
            if (buffer[i].txid == 99999)
                blkRewardMap[buffer[i].blk] = addr;
            apps.push_back(buffer[i]);
        }

        delete [] buffer;

    } else {
        cerr << "Could not allocate memory for address " << addr;
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------
bool COptions::loadData(void) {

    ENTER("loadData");

    CAppearanceArray_base tmp;
    for (auto monitor : monitors) {
        if (!loadMonitorData(tmp, monitor.address))
            EXIT_FAIL("Could not load data.");
    }
    if (tmp.size() == 0)
        EXIT_MSG("Nothing to export.", false);

    // Should be sorted already, so it can't hurt
    sort(tmp.begin(), tmp.end());

    bool hasFuture = false;
    blknum_t lastAtClient = getLastBlock_client();
    items.push_back(tmp[0]);
    for (auto item : tmp) {
        CAppearance_base *prev = &items[items.size() - 1];
        // TODO(tjayrush): I think this removes dups. Is it really necessary?
        if (item.blk != prev->blk || item.txid != prev->txid) {
            if (item.blk > lastAtClient)
                hasFuture = true;
            else
                items.push_back(item);
        }
    }
    LOG1("Items array: " + uint_2_Str(items.size()) + " - " + uint_2_Str(items.size() * sizeof(CAppearance_base)));
    if (hasFuture)
        LOG_WARN("Cache file contains blocks ahead of the chain. Some items will not be exported.");

    if (!freshenTsArray(items[items.size()-1].blk)) {
        EXIT_FAIL("Could not open timestamp file.");
    }

    EXIT_NOMSG(true);
}

//-----------------------------------------------------------------------
bool COptions::freshenTsArray(blknum_t last) {

    string_q zipFile = configPath("ts.bin.gz");
    if (fileExists(zipFile)) {  // is this first run since install?
        string_q cmd = "cd " + configPath("") + " ; gunzip " + zipFile;
        cerr << doCommand(cmd) << endl;
        ASSERT(!fileExists(zipFile));
    }

    // We've been asked to freshen to the latest block in this address. If
    // we're already there, do nothing.
    string_q fn = configPath("ts.bin");
    if (!fileExists(fn))
        return false;

    ts_cnt = fileSize(fn) / sizeof(uint32_t);
    if (last >= ts_cnt) {
        CArchive oo(WRITING_ARCHIVE);
        if (oo.Lock(fn, modeWriteAppend, LOCK_WAIT)) {
            for (blknum_t bl = ts_cnt ; bl < last ; bl++) {
                CBlock block;
                getBlock_light(block, bl);
                oo << (uint32_t)block.timestamp;
                oo.flush();
                cerr << "Updating timestamp for block " << bl << " (" << block.timestamp << ") of " << last << " (" << (last - bl) << ")    \r";
                cerr.flush();
            }
            oo.Release();
        }
    }

    if (ts_array) {
        delete [] ts_array;
        ts_array = NULL;
    }

    ts_cnt = fileSize(fn) / sizeof(uint32_t);
    ts_array = new uint32_t[ts_cnt];
    if (!ts_array)
        return false;

    CArchive in(READING_ARCHIVE);
    if (!in.Lock(fn, modeReadOnly, LOCK_NOWAIT))
        return false;

    in.Read(ts_array, sizeof(uint32_t), ts_cnt);
    in.Release();
    return true;
}

//-----------------------------------------------------------------------
const char* STR_DISPLAY =
"[{HASH}]\t[{TIMESTAMP}]\t[{FROM}]\t[{TO}]\t[{ETHER}]\t[{BLOCKNUMBER}]\t[{TRANSACTIONINDEX}]\t[{ETHERGASCOST}]\t[{GASUSED}]\t[{ISERROR}]\t[{ENCODING}]";
