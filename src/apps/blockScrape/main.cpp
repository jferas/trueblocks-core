/*-------------------------------------------------------------------------
 * This source code is confidential proprietary information which is
 * Copyright (c) 2017 by Great Hill Corporation.
 * All Rights Reserved
 *------------------------------------------------------------------------*/
#include "options.h"

//----------------------------------------------------------------------------------
int main(int argc, const char *argv[]) {

    acctlib_init(defaultQuitHandler);

    COptions options;
    if (!options.prepareArguments(argc, argv))
        return 0;

    for (auto command : options.commandLines) {
        if (options.parseArguments(command)) {
            options.handle_scrape();
        }
    }

    acctlib_cleanup();

    return 0;
}

//--------------------------------------------------------------------------
bool COptions::handle_scrape(void) {

    // Do not run if someone is searching the index
    if (isRunning("acctScrape", false))
        return false;

    // Remove any blocks that were not 'ripe' the last time we ran
    cleanFolder(indexFolder_unripe);

    // Find the last visited block (it's either in staging or finalized)
    blknum_t unused1, ripe, staging, finalized, client;
    getLastBlocks(unused1, ripe, staging, finalized, client);
    blknum_t lastVisit = max(ripe, max(staging, finalized));
    blknum_t startBlock = lastVisit + 1;

    // In some cases (if the user is re-syncing his/her node from scratch for example) the index may be
    // ahead of tip of the chain. In this case, we return without processign anything.
    if (startBlock > client) {
        cerr << cTeal << "INFO: " << cOff;
        cerr << "The index (" << startBlock << ") is caught up to the tip of the chain (" << client << ").";
        cerr << endl;
        return false;
    }

    if (startBlock < 150000) //firstTransactionBlock)
        nBlocks = max((blknum_t)5000, nBlocks);
    else if (ddosRange(startBlock))
        nBlocks = 200;
    if (getEnvStr("DOCKER_MODE") == "true") {
        if (ddosRange(startBlock)) {
            nBlocks = 40;
            nBlockProcs = 2;
            nAddrProcs = 4;
        }
    }
#if 0
    //2287592
    //2288192
    //2356992
#endif

    // At some point, we need to stop re-visiting blocks. We call this point the 'ripe' block. Here,
    // the 'ripe' block is 28 blocks before the current head (a bit more than five minutes under normal
    // operation). Note that, if the index is caught up to the head and the difficulty is high (we're
    // in the time bomb), we will receed from the front of the chain than normal. (28 blocks, when blocks
    // take longer, is a longer amount of time.)
    blknum_t ripeBlock = client - 28;

    //    nBlocks = 150;
//    nBlocks = 15;

    ostringstream os;
    os << "blaze scrape";
    os << " --startBlock " << startBlock;
    os << " --nBlocks " << nBlocks;
    if (nBlockProcs != 20)
        os << " --nBlockProcs " << nBlockProcs;
    if (nAddrProcs != 60)
        os << " --nAddrProcs " << nAddrProcs;

    cerr << endl << cTeal << "\t" << string_q(10,'-') << Now().Format(FMT_EXPORT) << string(40, '-') << cOff << endl;
    cerr << cGreen << "\t" << os.str() << cOff << " (" << (startBlock + nBlocks) << ")" << endl;

    // We don't need to show ripeBlock...
    os << " --ripeBlock " << ripeBlock;
    if (system(os.str().c_str()) != 0) {
        cerr << cRed << "\tBlaze quit without finishing. Reprocessing..." << cOff << endl;
        return false;
    }
    // If blaze returned '0', it sucessfully ran through all the blocks in the range. The ripe
    // folder holds finalized, individual blocks. The unripe folder holds blocks (if any) that
    // are less than 28 blocks from the head of the chain. We don't process them any further.

    // If the user starts using the account scraper or hit's control+c, we want to quit the processing.
    // We use this variable. Note - the following code must handle control+c properly without disturbing
    // the data. This means it has to process each block atomically.
    bool mustQuit = isRunning("acctScrape", false);
    if (mustQuit) {
        cerr << cRed << "\tacctScrape is running. blockScrape will re-run in a moment." << cOff << endl;
        return false;
    }

    // Processes one file at a time by copying the data from the ripe file into the staging file.
    // It flushes the data and then removes the ripe file. If it doesn't complete, it removes cleans up
    // the ripe folder and the temp staging file
    CConsolidator cons(indexFolder_staging + "000000000-temp.txt");
    cons.prevBlock = (staging == NOPOS ? (finalized == NOPOS ? 0 : finalized) : staging);
    if (!forEveryFileInFolder(indexFolder_ripe, copyRipeToStage, &cons)) {
        // Something went wrong (either control+c or a non-sequential list of files). Clean up everything and start over.
        cleanFolder(indexFolder_ripe);
        ::remove((indexFolder_staging + "000000000-temp.txt").c_str());
        return false;
    }

    // The stage now contains all non-consolidated records. Next, we pick off chunks if we can, consolidate them,
    // and re-write any unfinalized records back to the stage (naturally sorted by block number)
    finalize_chunks(&cons);

    return true;
}

//--------------------------------------------------------------------------
void COptions::finalize_chunks(CConsolidator *cons) {

    string_q stagedFile = indexFolder_staging + "000000000-temp.txt";
    string_q prevFile = getLastFileInFolder(indexFolder_staging, false);

    CStringArray stage;
    if (prevFile != stagedFile && fileExists(prevFile))
        asciiFileToLines(prevFile, stage);
    asciiFileToLines(stagedFile, stage);

    lockSection(true);
    string_q newFile = indexFolder_staging + padNum9(cons->prevBlock) + ".txt";
    linesToAsciiFile(newFile, stage, true);
    if (prevFile != stagedFile && fileExists(prevFile))
        ::remove(prevFile.c_str());
    if (fileExists(stagedFile))
        ::remove(stagedFile.c_str());
    lockSection(false);

    // newFile contains all un-consolidated, but staged records
    size_t curSize = fileSize(newFile) / 59;

    blknum_t maxIndexRows = 500000;
    if (curSize > maxIndexRows) {

        // We need to continue to process until curSize is less than maxIndexRows. This may mean more than once
        size_t pass = 0;
        while (curSize > maxIndexRows && !shouldQuit()) {
            lockSection(true);
            cerr << bBlue << "\tConsolodate (pass " + uint_2_Str(pass++) + ")" << endl;
            CStringArray lines;
            lines.reserve(curSize + 100);
            asciiFileToLines(newFile, lines);
            cerr << "\tnewFile: " << newFile << ": " << lines.size() << endl;

            string_q prev;
            cerr << "\tSearching from " << (maxIndexRows-1) << " to " << lines.size() << endl;
            cerr << cGreen << "\t\t" << (maxIndexRows-1) << ": " << lines[maxIndexRows-1] << cOff << endl;
            cerr << cGreen << "\t\t" << (maxIndexRows) << ": " << lines[maxIndexRows] << cOff << endl;
            size_t where = 0;
            for (uint64_t record = (maxIndexRows-1) ; record < lines.size() && where == 0; record++) {
                CStringArray pParts;
                explode(pParts, lines[record], '\t');
                cerr << bBlue << "\t\t" << record << ": " << pParts[0] << " -- " << pParts[1] << " -- " << pParts[2] << cOff << "\r";
                cerr.flush();
                if (record == lines.size() - 2)
                    cerr << endl;
                if (prev != pParts[1]) {
                    if (!prev.empty())
                        where = record - 1;
                    prev = pParts[1];
                }
            }
            if (where == 0) {
                cerr << "The weird edge case";
                //                getchar();
                where = lines.size() - 1;
            }
            //            if (where == NOPOS)
            //                where = lines.size() - 1;
            cerr << endl;
            cerr << "\tFound break at " << where << ": " << bBlue << lines[where] << cOff << endl;

            CStringArray consolidatedLines;
            consolidatedLines.reserve(lines.size());
            cerr << "\tExtract records 0 to " << where << " (inclusive) of " << lines.size() << endl;
            cerr << cGreen << "\t\t" << 0 << ": " << lines[0] << cOff << endl;
            cerr << cGreen << "\t\t" << 1 << ": " << lines[1] << cOff << endl;
            for (uint64_t record = 0 ; record <= where ; record++) {
                if (verbose > 2) {
                    cerr << bBlue << "\t\t" << record << ": " << lines[record] << cOff << "\r"; cerr.flush();
                    if (record == where - 1)
                        cerr << endl;
                }
                consolidatedLines.push_back(lines[record]);
            }
            if (verbose > 2) {
                cerr << endl;
                cerr << bTeal << "\t\t" << (where+1) << ": " << lines[where+1] << cOff << endl;
            } else {
                cerr << bBlue << "\t\t" << (where) << ": " << lines[where] << cOff << endl;
                cerr << bTeal << "\t\t" << (where+1) << ": " << lines[where+1] << cOff << endl;
            }
            CStringArray p1;
            explode(p1, consolidatedLines[0], '\t');
            CStringArray p2;
            explode(p2, consolidatedLines[consolidatedLines.size()-1], '\t');

            sort(consolidatedLines.begin(), consolidatedLines.end());
            string_q binFile = indexFolder_finalized + p1[1] + "-" + p2[1] + ".bin";
            writeIndexAsBinary(binFile, consolidatedLines);
            cerr << "\tWrote " << bYellow << consolidatedLines.size() << cOff << " records to " << binFile << endl;

            where += 1;
            CStringArray remainingLines;
            remainingLines.reserve(maxIndexRows+100);
            cerr << "\tExtract records " << where << " to " << lines.size() << " of " << lines.size() << endl;
            cerr << cGreen << "\t\t" << where << ": " << lines[where] << cOff << endl;
            cerr << cGreen << "\t\t" << where+1 << ": " << lines[where+1] << cOff << endl;
            for (uint64_t record = where ; record < lines.size() ; record++) {
                if (verbose > 2) {
                    cerr << bBlue << "\t\t" << record << ": " << lines[record] << cOff << "\r"; cerr.flush();
                    if (record == lines.size() - 2)
                        cerr << endl;
                }
                remainingLines.push_back(lines[record]);
            }
            if (verbose > 2)
                cerr << endl;
            else {
                cerr << bBlue << "\t\t" << (where-1) << ": " << lines[where-1] << cOff << endl;
                cerr << bTeal << "\t\t" << (where) << ": " << lines[where] << cOff << endl;
            }
            ::remove(newFile.c_str());
            linesToAsciiFile(newFile, remainingLines, true);
            cerr << "\tWrote " << bMagenta << remainingLines.size() << cOff << " records to " << newFile << endl;

            curSize = fileSize(newFile) / 59;
            lockSection(false);
        }

    } else {
        cerr << bTeal << "\t  Will consolidate in " << (maxIndexRows - curSize) << " rows (" << curSize << " of " << maxIndexRows << ")" << cOff << endl;
    }
}

//--------------------------------------------------------------------------
bool copyRipeToStage(const string_q& path, void *data) {

    if (endsWith(path, '/'))
        return forEveryFileInFolder(path + "*", copyRipeToStage, data);

    else {
        blknum_t bn = bnFromPath(path);
        CConsolidator *con = (CConsolidator *)data;
        if ((con->prevBlock + 1) != bn) {
            // For some reason, we're missing a file. Quit and try again next time
            cerr << cRed << "Current file (" << path << ") does not sequentially follow previous file " << con->prevBlock << "." << cOff << endl;
            return false;
        }
        ifstream inputStream(path, ios::in);
        if (!inputStream.is_open()) {
            // Something went wrong, try again next time
            cerr << cRed << "Could not open input stream " << path << cOff << endl;
            return false;
        }
        lockSection(true);
        con->output << inputStream.rdbuf();
        con->output.flush();
        ::remove(path.c_str());
        con->prevBlock = bn;
        lockSection(false);
        inputStream.close();
    }

    return !shouldQuit();
}
