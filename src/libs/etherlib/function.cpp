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
/*
 * This file was generated with makeClass. Edit only those parts of the code inside
 * of 'EXISTING_CODE' tags.
 */
#include <algorithm>
#include "function.h"

namespace qblocks {

//---------------------------------------------------------------------------
IMPLEMENT_NODE(CFunction, CBaseNode);

//---------------------------------------------------------------------------
static string_q nextFunctionChunk(const string_q& fieldIn, const void *dataPtr);
static string_q nextFunctionChunk_custom(const string_q& fieldIn, const void *dataPtr);

//---------------------------------------------------------------------------
void CFunction::Format(ostream& ctx, const string_q& fmtIn, void *dataPtr) const {
    if (!m_showing)
        return;

    string_q fmt = (fmtIn.empty() ? expContext().fmtMap["function_fmt"] : fmtIn);
    if (fmt.empty()) {
        ctx << toJson();
        return;
    }

    // EXISTING_CODE
    // EXISTING_CODE

    while (!fmt.empty())
        ctx << getNextChunk(fmt, nextFunctionChunk, this);
}

//---------------------------------------------------------------------------
string_q nextFunctionChunk(const string_q& fieldIn, const void *dataPtr) {
    if (dataPtr)
        return reinterpret_cast<const CFunction *>(dataPtr)->getValueByName(fieldIn);

    // EXISTING_CODE
    // EXISTING_CODE

    return fldNotFound(fieldIn);
}

//---------------------------------------------------------------------------------------------------
bool CFunction::setValueByName(const string_q& fieldNameIn, const string_q& fieldValueIn) {
    string_q fieldName = fieldNameIn;
    string_q fieldValue = fieldValueIn;

    // EXISTING_CODE
    if ( fieldName % "signature" ) {
        signature = getSignature(SIG_CANONICAL);
        return true;
    } else if (fieldName % "name") {
        name = fieldValue;
        return true;
    }
    // EXISTING_CODE

    switch (tolower(fieldName[0])) {
        case 'a':
            if ( fieldName % "anonymous" ) { anonymous = str_2_Bool(fieldValue); return true; }
            break;
        case 'c':
            if ( fieldName % "constant" ) { constant = str_2_Bool(fieldValue); return true; }
            break;
        case 'e':
            if ( fieldName % "encoding" ) { encoding = fieldValue; return true; }
            break;
        case 'i':
            if ( fieldName % "inputs" ) {
                uint64_t cnt = 0;
                CParameter item;
                string_q str = fieldValue;
                while (item.parseJson3(str)) {
                    if (item.name.empty())
                        item.name = "val_" + uint_2_Str(cnt++);
                    inputs.push_back(item);
                    item = CParameter();  // reset
                }
                return true;
            }
            break;
        case 'm':
            if ( fieldName % "message" ) { message = fieldValue; return true; }
            break;
        case 'n':
            if ( fieldName % "name" ) { name = fieldValue; return true; }
            break;
        case 'o':
            if ( fieldName % "outputs" ) {
                uint64_t cnt = 0;
                CParameter item;
                string_q str = fieldValue;
                while (item.parseJson3(str)) {
                    if (item.name.empty())
                        item.name = "val_" + uint_2_Str(cnt++);
                    outputs.push_back(item);
                    item = CParameter();  // reset
                }
                return true;
            }
            break;
        case 'p':
            if ( fieldName % "payable" ) { payable = str_2_Bool(fieldValue); return true; }
            break;
        case 's':
            if ( fieldName % "signature" ) { signature = fieldValue; return true; }
            break;
        case 't':
            if ( fieldName % "type" ) { type = fieldValue; return true; }
            break;
        default:
            break;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------
void CFunction::finishParse() {
    // EXISTING_CODE
    signature = getSignature(SIG_CANONICAL);
    encoding = encodeItem();
    // The input parameters need to have a name. If not, we provide one
    int cnt = 0;
    for (size_t i = 0 ; i < inputs.size() ; i++) {
        if (inputs[i].name.empty())
            inputs.at(i).name = "val_" + int_2_Str(cnt++);  // the non-const reference already exists
    }
    // EXISTING_CODE
}

//---------------------------------------------------------------------------------------------------
bool CFunction::Serialize(CArchive& archive) {

    if (archive.isWriting())
        return SerializeC(archive);

    // Always read the base class (it will handle its own backLevels if any, then
    // read this object's back level (if any) or the current version.
    CBaseNode::Serialize(archive);
    if (readBackLevel(archive))
        return true;

    // EXISTING_CODE
    // EXISTING_CODE
    archive >> name;
    archive >> type;
    archive >> anonymous;
    archive >> constant;
    archive >> payable;
    archive >> signature;
    archive >> encoding;
//    archive >> message;
    archive >> inputs;
    archive >> outputs;
    finishParse();
    return true;
}

//---------------------------------------------------------------------------------------------------
bool CFunction::SerializeC(CArchive& archive) const {

    // Writing always write the latest version of the data
    CBaseNode::SerializeC(archive);

    // EXISTING_CODE
    // EXISTING_CODE
    archive << name;
    archive << type;
    archive << anonymous;
    archive << constant;
    archive << payable;
    archive << signature;
    archive << encoding;
//    archive << message;
    archive << inputs;
    archive << outputs;

    return true;
}

//---------------------------------------------------------------------------
CArchive& operator>>(CArchive& archive, CFunctionArray& array) {
    uint64_t count;
    archive >> count;
    array.resize(count);
    for (size_t i = 0 ; i < count ; i++) {
        ASSERT(i < array.capacity());
        array.at(i).Serialize(archive);
    }
    return archive;
}

//---------------------------------------------------------------------------
CArchive& operator<<(CArchive& archive, const CFunctionArray& array) {
    uint64_t count = array.size();
    archive << count;
    for (size_t i = 0 ; i < array.size() ; i++)
        array[i].SerializeC(archive);
    return archive;
}

//---------------------------------------------------------------------------
void CFunction::registerClass(void) {
    // only do this once
    if (HAS_FIELD(CFunction, "schema")) return;

    size_t fieldNum = 1000;
    ADD_FIELD(CFunction, "schema",  T_NUMBER, ++fieldNum);
    ADD_FIELD(CFunction, "deleted", T_BOOL,  ++fieldNum);
    ADD_FIELD(CFunction, "showing", T_BOOL,  ++fieldNum);
    ADD_FIELD(CFunction, "cname", T_TEXT,  ++fieldNum);
    ADD_FIELD(CFunction, "name", T_TEXT, ++fieldNum);
    ADD_FIELD(CFunction, "type", T_TEXT, ++fieldNum);
    ADD_FIELD(CFunction, "anonymous", T_BOOL, ++fieldNum);
    ADD_FIELD(CFunction, "constant", T_BOOL, ++fieldNum);
    ADD_FIELD(CFunction, "payable", T_BOOL, ++fieldNum);
    ADD_FIELD(CFunction, "signature", T_TEXT, ++fieldNum);
    ADD_FIELD(CFunction, "encoding", T_TEXT, ++fieldNum);
    ADD_FIELD(CFunction, "message", T_TEXT, ++fieldNum);
    HIDE_FIELD(CFunction, "message");
    ADD_FIELD(CFunction, "inputs", T_OBJECT|TS_ARRAY, ++fieldNum);
    ADD_FIELD(CFunction, "outputs", T_OBJECT|TS_ARRAY, ++fieldNum);

    // Hide our internal fields, user can turn them on if they like
    HIDE_FIELD(CFunction, "schema");
    HIDE_FIELD(CFunction, "deleted");
    HIDE_FIELD(CFunction, "showing");
    HIDE_FIELD(CFunction, "cname");

    builtIns.push_back(_biCFunction);

    // EXISTING_CODE
    ADD_FIELD(CFunction, "input_names", T_TEXT, ++fieldNum);
    HIDE_FIELD(CFunction, "input_names");
    HIDE_FIELD(CFunction, "indexed");
    HIDE_FIELD(CFunction, "anonymous");
    // EXISTING_CODE
}

//---------------------------------------------------------------------------
string_q nextFunctionChunk_custom(const string_q& fieldIn, const void *dataPtr) {
    const CFunction *fun = reinterpret_cast<const CFunction *>(dataPtr);
    if (fun) {
        switch (tolower(fieldIn[0])) {
            // EXISTING_CODE
            case 'h':
                if ( fieldIn % "hex" ) {
                    string_q ret = fun->name + "(";
                    for (size_t i = 0 ; i < fun->inputs.size() ; i++) {
                        ret += fun->inputs[i].type;
                        if (i < fun->inputs.size())
                            ret += ",";
                    }
                    ret += ")";
                    replace(ret, ",)", ")");
                    return (ret + "\t" + chr_2_HexStr(ret));
                }
                break;
            case 'i':
                if ( fieldIn % "isBuiltIn" ) {
                    return int_2_Str(fun->isBuiltIn);

                } else if ( fieldIn % "input_names") {
                    string_q ret;
                    for (size_t i = 0 ; i < fun->inputs.size() ; i++) {
                        ret += fun->inputs[i].name;
                        if (i < fun->inputs.size())
                            ret += ",";
                    }
                    return ret;
                }
                break;

            case 'o':
                if ( fieldIn % "origName" ) {
                    return fun->origName;
                }
                break;
            // EXISTING_CODE
            case 'p':
                // Display only the fields of this node, not it's parent type
                if ( fieldIn % "parsed" )
                    return nextBasenodeChunk(fieldIn, fun);
                // EXISTING_CODE
                // EXISTING_CODE
                break;

            default:
                break;
        }
    }

    return "";
}

//---------------------------------------------------------------------------
bool CFunction::readBackLevel(CArchive& archive) {

    bool done = false;
    // EXISTING_CODE
    // EXISTING_CODE
    return done;
}

//---------------------------------------------------------------------------
CArchive& operator<<(CArchive& archive, const CFunction& fun) {
    fun.SerializeC(archive);
    return archive;
}

//---------------------------------------------------------------------------
CArchive& operator>>(CArchive& archive, CFunction& fun) {
    fun.Serialize(archive);
    return archive;
}

//---------------------------------------------------------------------------
string_q CFunction::getValueByName(const string_q& fieldName) const {

    // Give customized code a chance to override first
    string_q ret = nextFunctionChunk_custom(fieldName, this);
    if (!ret.empty())
        return ret;

    // Return field values
    switch (tolower(fieldName[0])) {
        case 'a':
            if ( fieldName % "anonymous" ) return bool_2_Str_t(anonymous);
            break;
        case 'c':
            if ( fieldName % "constant" ) return bool_2_Str_t(constant);
            break;
        case 'e':
            if ( fieldName % "encoding" ) return encoding;
            break;
        case 'i':
            if ( fieldName % "inputs" || fieldName % "inputsCnt" ) {
                size_t cnt = inputs.size();
                if (endsWith(toLower(fieldName), "cnt"))
                    return uint_2_Str(cnt);
                if (!cnt) return "";
                string_q retS;
                for (size_t i = 0 ; i < cnt ; i++) {
                    retS += inputs[i].Format();
                    retS += ((i < cnt - 1) ? ",\n" : "\n");
                }
                return retS;
            }
            break;
        case 'm':
            if ( fieldName % "message" ) return message;
            break;
        case 'n':
            if ( fieldName % "name" ) return name;
            break;
        case 'o':
            if ( fieldName % "outputs" || fieldName % "outputsCnt" ) {
                size_t cnt = outputs.size();
                if (endsWith(toLower(fieldName), "cnt"))
                    return uint_2_Str(cnt);
                if (!cnt) return "";
                string_q retS;
                for (size_t i = 0 ; i < cnt ; i++) {
                    retS += outputs[i].Format();
                    retS += ((i < cnt - 1) ? ",\n" : "\n");
                }
                return retS;
            }
            break;
        case 'p':
            if ( fieldName % "payable" ) return bool_2_Str_t(payable);
            break;
        case 's':
            if ( fieldName % "signature" ) return signature;
            break;
        case 't':
            if ( fieldName % "type" ) return type;
            break;
    }

    // EXISTING_CODE
    // EXISTING_CODE

    // Finally, give the parent class a chance
    return CBaseNode::getValueByName(fieldName);
}

//-------------------------------------------------------------------------
ostream& operator<<(ostream& os, const CFunction& item) {
    // EXISTING_CODE
    // EXISTING_CODE

    item.Format(os, "", nullptr);
    return os;
}

//---------------------------------------------------------------------------
const CBaseNode *CFunction::getObjectAt(const string_q& fieldName, size_t index) const {
    if ( fieldName % "inputs" && index < inputs.size() )
        return &inputs[index];
    if ( fieldName % "outputs" && index < outputs.size() )
        return &outputs[index];
    return NULL;
}

//---------------------------------------------------------------------------
// EXISTING_CODE
//---------------------------------------------------------------------------
string_q CFunction::getSignature(uint64_t parts) const {
    size_t cnt = inputs.size();

    string_q nm = (origName.empty() ? name : origName);

    size_t tl = type.length();
    size_t nl = nm.length();
    size_t v1 = 35 - tl;
    size_t ll = (nl > v1 ? 0 : v1 - nl);

    ostringstream os;
    os << (parts & SIG_FTYPE  ? "\t"+type+" " : "");
    os << (parts & SIG_FNAME  ? nm            : "");
    os << (parts & SIG_FSPACE ? string_q(ll, ' ') : "");
    os << (parts & SIG_FTYPE || parts & SIG_FNAME  ? "("    : "");
    for (size_t j = 0 ; j < cnt ; j++) {
        os << (parts & SIG_ITYPE    ? inputs[j].type : "");
        os << (parts & SIG_IINDEXED ? (inputs[j].indexed ? " indexed" : "") : "");
        os << (parts & SIG_INAME    ? " "+inputs[j].name : "");
        os << (parts & SIG_ITYPE    ? (j < cnt-1 ? "," : "") : "");
    }
    os << (parts & SIG_FTYPE || parts & SIG_FNAME  ? ")" : "");
    if (parts == SIG_ENCODE)
        os << (parts & SIG_ENCODE ? (parts & SIG_FNAME ? " " + encoding : encoding + " ") : "");
    else
        os << (parts & SIG_ENCODE ? " [" + encoding + "]" : "");
    if (verbose && parts != SIG_CANONICAL) {
        os << (anonymous ? " anonymous" : "");
        os << (constant  ? " constant" : "");
        os << (payable   ? " payable" : "");
    }

    string_q ret = os.str().c_str();
    replaceAll(ret, " )", ")");
    return trim(ret);
}

//-----------------------------------------------------------------------
bool CFunction::checkTypes(void) const {
    for (auto input : inputs) {
        if (!input.isValid())
            return false;
    }
    return true;
}

//-----------------------------------------------------------------------
string_q CFunction::encodeItem(void) const {
    if (encoding == "()")  // optimization
        return "0x861731d5";
//    if (!encoding.empty())
//        return encoding;
extern string_q getSha3(const string_q& hexIn);
    string_q ret = getSha3(chr_2_HexStr(signature));
    return (type == "event" ? ret : extract(ret, 0, 10));
}

//-----------------------------------------------------------------------
bool CFunction::fromDefinition(const string_q& lineIn) {

    if (lineIn.empty())
        return false;

    uint64_t iCnt = 0, oCnt = 0;

    string_q line = lineIn;
    line = substitute(line,"(","|");  // clean up
    line = substitute(line,")","|");  // clean up
    line = substitute(line,", ",","); // clean up
    line = substitute(line,"\t","");  // clean up
    while (contains(line, " [")) replace(line, " [", "[");  // clean up
    while (contains(line, " ]")) replace(line, " ]", "]");  // clean up
    this->constant = (contains(line, "constant") || contains(line, "view"));
    this->type = trim(nextTokenClear(line,' '));
    this->name = trim(nextTokenClear(line,'|'));

    string_q inputStr = trim(nextTokenClear(line, '|'));
    CStringArray inputArray;
    explode(inputArray, inputStr, ',');
    for (auto input : inputArray) {
        CParameter param;
        param.fromDefinition(input);
        if (param.name.empty())
            param.name = "val_" + uint_2_Str(iCnt++);
        this->inputs.push_back(param);
    }

    CStringArray parts;
    explode(parts, line, '|');
    if (parts.size()>1 && contains(parts[0], "returns")) {
        string_q outputStr = trim(nextTokenClear(parts[1], '|'));
        CStringArray outputArray;
        explode(outputArray, outputStr, ',');
        for (auto output : outputArray) {
            CParameter param;
            param.fromDefinition(output);
            if (param.name.empty())
                param.name = "ret_" + uint_2_Str(oCnt++);
            outputs.push_back(param);
        }
    }
    finishParse();
    return true;
}

//-----------------------------------------------------------------------
string_q CFunction::compressed(void) const {
    if (name.empty())
        return "";
    string_q ret = name + " ( ";
    for (auto input : inputs)
        ret += (input.name + ": " + input.value + ", ");
    ret = trim(trim(ret, ' '), ',');
    ret += " )";
    return ret;
}
// EXISTING_CODE
}  // namespace qblocks

