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
#include <glob.h>
#include <libgen.h>
#include <algorithm>
#include "basetypes.h"
#include "sfos.h"
#include "sftime.h"
#include "sfstring.h"
#include "database.h"
#include "filenames.h"

namespace qblocks {

    #define remove unlink

    //------------------------------------------------------------------
    static string_q escapePath(const string_q& nameIn) {
        string_q name = nameIn;
        replaceAll(name, " ", "\\ ");
        replaceAll(name, "&", "\\&");
        replaceAll(name, "(", "\\(");
        replaceAll(name, ")", "\\)");
        replaceAll(name, "'", "\\'");
        return name;
    }

    //------------------------------------------------------------------
    static int globErrFunc(const char *epath, int eerrno) {
        //  perror(epath);
        return 0;
    }


    //--------------------------------------------------------------------------------
    int cleanFolder(const string_q& path, bool recurse, bool interactive) {
        CStringArray files;
        listFilesInFolder(files, path, true);
        for (auto file : files)
            ::remove(file.c_str());
        return (int)files.size();
    }

    //------------------------------------------------------------------
    int moveFile(const string_q& from, const string_q& to) {
        if (from % to)
            return true;
        if (copyFile(from, to)) {
            int ret = ::remove(from.c_str()); // remove file returns '0' on success
            return !ret;
        }
        return false;
    }

    //------------------------------------------------------------------
    int copyFile(const string_q& fromIn, const string_q& toIn) {
        string_q from = escapePath(fromIn);
        string_q to   = escapePath(toIn);

        string_q command = "cp -f " + from + " " + to;
        if (system(command.c_str())) { }  // do not remove. The test just silences compiler warnings
        return static_cast<int>(fileExists(to));
    }

    //------------------------------------------------------------------
    // Returns a list of either files or folders, but not both.
    //------------------------------------------------------------------
    void doGlob(size_t& nStrs, string_q *strs, const string_q& maskIn, int wantFiles ) {
        ASSERT(!strs || nStrs);

        glob_t globBuf;

        string_q mask = maskIn;

        // should check return code
        glob(mask.c_str(), GLOB_MARK, globErrFunc, &globBuf);

        size_t n = globBuf.gl_pathc;
        size_t mx = nStrs;
        nStrs = 0;
        char c;

        for (size_t i = 0 ; i < n ; i++) {
            // get path
            char *tmp = globBuf.gl_pathv[i];
            // last char
            c   = tmp[strlen(tmp) - 1];

            // if path ends in '/' then this is directory, filter accordingly

            bool isDir = ('/' == c);
            bool listEm = ((isDir) ? !wantFiles : wantFiles);
            if (wantFiles == ANY_FILETYPE)
                listEm = true;

            if (listEm) {
                if (NULL != strs) {
                    string_q path = globBuf.gl_pathv[i];

                    // filter specified directories and remove trailing '/'
                    if (endsWith(path, '/'))
                        path = extract(path, 0, path.length() - 1);

                    // trim path to last directory / file
                    path = basename((char*)path.c_str());  // NOLINT
                    if (startsWith(path, '/'))
                         path = extract(path, 1);
                    // The path we return is always just the name of the folder or file
                    // without any leading (or even existing) '/'
                    ASSERT(path.length() && path[0] != '/');

                    if (wantFiles == ANY_FILETYPE) {
                        if (isDir)
                            path = "d-" + path;
                        else
                            path = "f-" + path;
                    }

                    strs[nStrs] = path;
                }

                nStrs++;
                if (NULL != strs && nStrs >= mx) {
                    break;
                }
            }
        }

        globfree(&globBuf);
    }

    //-------------------------------------------------------------------------------------------------------------
    inline bool waitForCreate(const string_q& filename) {
        size_t mx = 1000;
        size_t cnt = 0;
        while (cnt < mx && !fileExists(filename))
            cnt++;

        return fileExists(filename);
    }

    //---------------------------------------------------------------------------------------
    static const char* CHR_VALID_NAME  = "\t\n\r()<>[]{}`\\|; " "'!$^*~@" "?&#+%" ",:/=\"";
    //---------------------------------------------------------------------------------------
    string_q makeValidName(const string_q& inOut) {
        string_q ret = inOut;
        replaceAny(ret, CHR_VALID_NAME, "_");
        if (!ret.empty() && isdigit(ret[0]))
            ret = "_" + ret;
        return ret;
    }

    //------------------------------------------------------------------------------------------
    string_q doCommand(const string_q& cmd) {

        time_q now = Now();
        string_q tmpPath = "/tmp/";
        string_q filename = tmpPath + makeValidName("qb_" + now.Format("%Y%m%d%H%M%S"));
        string_q theCommand = (cmd + " >" + filename);
        if (system(theCommand.c_str())) { }  // Don't remove. Silences compiler warnings

        // Check twice for existance since the previous command creates the file but may take some time
        waitForCreate(filename);
        string_q ret;
        asciiFileToString(filename, ret);
        remove(filename.c_str());
        return trim(ret, '\n');
    }

    //------------------------------------------------------------------
    string_q getCWD(const string_q& filename) {
        char buffer[kMaxPathSize];
        if (::getcwd(buffer, kMaxPathSize)) { }  // do not remove. The test just silences compiler warnings
        string_q folder = buffer;
        if (!endsWith(folder, '/'))
            folder += "/";
        return folder + filename;  // may be empty
    }

    //------------------------------------------------------------------
    bool fileExists(const string_q& file) {
        struct stat statBuf;
        return !file.empty() && stat(file.c_str(), &statBuf) == 0;
    }

    //------------------------------------------------------------------
    bool folderExists(const string_q& folderName) {
        if (folderName.empty())
            return false;

        string_q folder = folderName;
        if (!endsWith(folder, '/'))
            folder += '/';

        size_t nFiles = 0;
        string_q mask = folder + "*.*";
        doGlob(nFiles, NULL, mask, true);

        // check to see if it is just folders
        if (!nFiles)
            doGlob(nFiles, NULL, mask, false);
        if (!nFiles) {
            mask = folder + ".";
            doGlob(nFiles, NULL, mask, false);
        }

        return (nFiles > 0);
    }

    //------------------------------------------------------------------
    uint64_t fileSize(const string_q& filename) {
        if (!fileExists(filename))
            return 0;

        struct stat statBuf;
        stat(filename.c_str(), &statBuf);
        return (uint64_t)statBuf.st_size;
    }

    //----------------------------------------------------------------------------
    bool establishFolder(const string_q& path, string_q& created) {
        if (fileExists(path) || folderExists(path))
            return true;

        CFilename fullPath(path);
        string_q targetFolder = fullPath.getFullPath();
        size_t find = targetFolder.rfind('/');
        targetFolder = extract(targetFolder, 0, find) + "/";
        string_q folder = targetFolder;
        string_q curFolder = "/";
        while (!folder.empty()) {
            curFolder += nextTokenClear(folder, '/') + "/";
            if (!folderExists(curFolder)) {
                mkdir(curFolder.c_str(), (mode_t)0755);
                if (created.empty())
                    created = curFolder;
            }
        }
        return folderExists(targetFolder);
    }

    //----------------------------------------------------------------------------
    bool isRunning(const string_q& progName, bool excludeSelf) {
        string_q cmd = "ps -ef | grep -i \"" + progName + "\" | grep -v grep | grep -v \"sh -c \" | wc -l";
        string_q result = doCommand(cmd);
        uint64_t cnt = str_2_Uint(result);
        if (!cnt || !contains(result, getEffectiveUserName())) // not running or not running by this user
            return false;
        return (!excludeSelf || cnt > 1);
    }

    //----------------------------------------------------------------------------
    size_t nRunning(const string_q& progName) {
        string_q cmd = "ps -ef | grep -i \"" + progName + "\" | grep -v grep | grep -v \"sh -c \" | wc -l";
        string_q result = doCommand(cmd);
        return str_2_Uint(result);
    }
}  // namespace qblocks
