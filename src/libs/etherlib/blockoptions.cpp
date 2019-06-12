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
#include "etherlib.h"
#include "blockoptions.h"

static uint64_t findBlockNumByHash(const hash_t& hash, void *data);
//--------------------------------------------------------------------------------
CBlockOptions::CBlockOptions(void) {
    Init();
}

//--------------------------------------------------------------------------------
void CBlockOptions::Init(void) {
    blocks.hashFind = findBlockNumByHash;
}

//--------------------------------------------------------------------------------
uint64_t findBlockNumByHash(const hash_t& hash, void *data) {
    ASSERT(isHash(hash));
    CBlock block;
    getBlock(block, hash);  // getBlock returns true if it has transactions and false otherwise
    if (block.hash != hash) {
        cerr << "Block hash '" << hash << "' does not appear to be a valid block hash. Quitting...";
        exit(0);
    }
    return block.blockNumber;
}

//--------------------------------------------------------------------------------
bool blockNumToString(uint64_t num, void *data) {
    if (num != NOPOS) {
        string_q *str = (string_q*)data;  // NOLINT
        *str += (uint_2_Str(num) + "|");
    }
    return true;
}

//--------------------------------------------------------------------------------
string_q CBlockOptions::getBlockNumList(void) {
    string_q ret;
    blocks.hashFind = findBlockNumByHash;
    blocks.forEveryBlockNumber(blockNumToString, &ret);
    return ret;
}

//--------------------------------------------------------------------------------
bool CHistoryOptions::hasHistory(void) const {
    if (isTestMode()) return false; // we don't report this error during testing
    blknum_t n_blocks = getGlobalConfig()->getConfigInt("history", "n_blocks", 250);
    return (newestBlock - oldestBlock) >= n_blocks;
}

//--------------------------------------------------------------------------------
string_q getDispBal(blknum_t bn, biguint_t bal) {
    // Makes finding the dollar value quicker (if we call into this more than once)
    static map<blknum_t, timestamp_t> timestampMap;
    if (expContext().asDollars && (timestampMap[bn] == (timestamp_t)0)) {
        CBlock blk;
        getBlock(blk, bn);
        timestampMap[bn] = blk.timestamp;
    }
    ostringstream os;
    if (expContext().asEther) {
        os << wei_2_Ether(bal);
    } else if (expContext().asDollars) {
        os << padLeft("$" + displayDollars(timestampMap[bn], bal), 14);
    } else {
        os << bal;
    }
    return os.str();
}

//--------------------------------------------------------------------------------
bool wrangleTxId(string_q& argOut, string_q& errorMsg) {

    if (contains(argOut, "0x"))
        return true;

    // valid args are 'latest', 'bn.txid', 'bn.txid.next', or 'bn.txid.prev'
    CStringArray parts;
    explode(parts, argOut, '.');

    //txnum_t txid;
    if (parts.size() == 0 ||  // there are not enough
        (parts.size() == 1 && parts[0] != "latest") ||  // there's only one and it's not 'latest'
        ((parts.size() == 2 || parts.size() == 3) && (!isNumeral(parts[0]) || !isNumeral(parts[1]))) ||  // two or three, first two are not numbers
        parts.size() > 3) { // too many
        errorMsg = argOut + " does not appear to be a valid transaction index.";
        return false;
    }

    if (parts.size() == 2)
        return true;

    // it's directional
    if (parts[0] == "latest") {
        CBlock block;
        getBlock_light(block, "latest");
        if (block.transactions.size() > 0) {
            ostringstream os;
            os << block.blockNumber << "." << (block.transactions.size() - 1);
            argOut = os.str();
            return true;
        }
        parts[0] = uint_2_Str(block.blockNumber);
        parts[1] = "0";
        parts[2] = "prev";
    }
    ASSERT(parts[2] == "prev" || parts[2] == "next");
    return getDirectionalTxId(str_2_Uint(parts[0]), str_2_Uint(parts[1]), parts[2], argOut, errorMsg);
}

//--------------------------------------------------------------------------------
bool getDirectionalTxId(blknum_t bn, txnum_t txid, const string_q& dir, string_q& argOut, string_q& errorMsg) {

    blknum_t lastBlock = getLastBlock_client();

    if (bn < firstTransactionBlock) {
        argOut = uint_2_Str(firstTransactionBlock) + ".0";
        return true;
    }

    CBlock block;
    getBlock(block, bn);

    argOut = "";
    txnum_t nextid = txid + 1;
    while (argOut.empty() && bn >= firstTransactionBlock && bn <= lastBlock) {
        if (dir == "next") {
            if (nextid < block.transactions.size()) {
                argOut = uint_2_Str(block.blockNumber) + "." + uint_2_Str(nextid);
                return true;
            }
            block = CBlock();
            getBlock(block, ++bn);
            nextid = 0;
        } else if (dir == "prev") {
            if (txid > 0 && block.transactions.size() > 0) {
                argOut = uint_2_Str(block.blockNumber) + "." + uint_2_Str(txid - 1);
                return true;
            }
            if (bn == 0)
                return true;
            block = CBlock();
            getBlock(block, --bn);
            txid = block.transactions.size();
        }
    }
    errorMsg = "Could not find " + dir + " transaction to " + uint_2_Str(bn) + "." + uint_2_Str(txid);
    return false;
}
