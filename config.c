// -----------------------------------------------------------------
// RO Credentials (ROCred)
// (c) 2012-2013 Ai4rei/AN
//
// -----------------------------------------------------------------

#include <windows.h>

#include <btypes.h>
#include <kvdb.h>
#include <kvdb/win32ini.h>

#include "config.h"
#include "rocred.h"
#include "rsrcio.h"

static char l_szIniFile[MAX_PATH] = { 0 };
static KVDB l_ConfigDB = { 0 };

#define CONFIG_MAIN_SECTION "ROCred"

void __stdcall ConfigSetStr(const char* lpszKey, const char* lpszValue)
{
    KvKeySetStrValue(&l_ConfigDB, NULL, CONFIG_MAIN_SECTION, NULL, lpszKey, lpszValue);
    KvSave(&l_ConfigDB, l_szIniFile);
}

void __stdcall ConfigSetInt(const char* lpszKey, int nValue)
{
    char szBuffer[16];

    wsprintfA(szBuffer, "%d", nValue);
    ConfigSetStr(lpszKey, szBuffer);
}

void __stdcall ConfigSetIntU(const char* lpszKey, unsigned int uValue)
{
    char szBuffer[16];

    wsprintfA(szBuffer, "%u", uValue);
    ConfigSetStr(lpszKey, szBuffer);
}

const char* __stdcall ConfigGetStr(const char* lpszKey)
{
    return KvKeyGetStrValue(&l_ConfigDB, NULL, CONFIG_MAIN_SECTION, NULL, lpszKey);
}

int __stdcall ConfigGetInt(const char* lpszKey)
{
    return atoi(ConfigGetStr(lpszKey));
}

unsigned int __stdcall ConfigGetIntU(const char* lpszKey)
{
    return ConfigGetInt(lpszKey);
}

bool __stdcall ConfigSave(void)
{
    bool bSuccess = false;
    unsigned long luLen, luRead;
    void* lpData;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    // load configuration
    if((hFile = CreateFileA(l_szIniFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL))!=INVALID_HANDLE_VALUE)
    {
        luLen = GetFileSize(hFile, NULL);

        if((lpData = LocalAlloc(0, luLen))!=NULL)
        {
            if(ReadFile(hFile, lpData, luLen, &luRead, NULL) && luLen==luRead)
            {
                char szSrcName[MAX_PATH];
                char szDstName[MAX_PATH];
                HANDLE hUpdate;

                GetModuleFileNameA(NULL, szSrcName, __ARRAYSIZE(szSrcName));
                wsprintfA(szDstName, "%s.embed.exe", szSrcName);

                if(CopyFileA(szSrcName, szDstName, FALSE))
                {
                    // persist as resource
                    if((hUpdate = BeginUpdateResource(szDstName, FALSE))!=NULL)
                    {
                        if(UpdateResource(hUpdate, MAKEINTRESOURCE(RT_RCDATA), "CONFIG", 0, lpData, luLen))
                        {
                            if(EndUpdateResource(hUpdate, FALSE))
                            {
                                bSuccess = true;
                            }
                        }
                        else
                        {
                            EndUpdateResource(hUpdate, TRUE);
                        }
                    }

                    if(!bSuccess)
                    {
                        DeleteFileA(szDstName);
                    }
                }
            }

            LocalFree(lpData);
        }

        CloseHandle(hFile);
    }

    return bSuccess;
}

static bool __stdcall Config_P_FoilEachKey(LPKVDB DB, const char* lpszSection, LPKVDBKEY Key, const char* lpszKey, void* lpContext)
{
    if(lpszKey[0]=='_')
    {
        KvKeyDelete(NULL, NULL, NULL, Key, NULL);
    }

    return true;
}

static bool __stdcall Config_P_FoilEachSection(LPKVDB DB, LPKVDBSECTION Section, const char* lpszSection, void* lpContext)
{
    KvForEachKey(NULL, Section, NULL, &Config_P_FoilEachKey, NULL);
    return true;
}

bool __stdcall ConfigInit(void)
{
    bool bSuccess = true;
    char szMbdFile[MAX_PATH];
    const void* lpData;
    unsigned long luLen, luWritten;
    HANDLE hFile;

    // set defaults
    KvInit(&l_ConfigDB, &g_Win32PrivateProfileAdapter);
    KvKeySetStrValue(&l_ConfigDB, NULL, CONFIG_MAIN_SECTION, NULL, "ExeType", "1rag");
    KvKeySetStrValue(&l_ConfigDB, NULL, CONFIG_MAIN_SECTION, NULL, "FontSize", "9");

    // load embedded/admin configuration
    if(ResourceFetch(NULL, MAKEINTRESOURCE(RT_RCDATA), "CONFIG", &lpData, &luLen))
    {
        char szTmpPath[MAX_PATH];
        unsigned int uUniq = 0;

        GetTempPathA(__ARRAYSIZE(szTmpPath), szTmpPath);

        do
        {
            wsprintfA(szMbdFile, "%s\\~rcd%04x.ini", szTmpPath, uUniq++);

            hFile = CreateFileA(szMbdFile, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_WRITE_THROUGH, NULL);
        }
        while(hFile==INVALID_HANDLE_VALUE && GetLastError()==ERROR_FILE_EXISTS && uUniq<=0xFFFF);

        if(hFile!=INVALID_HANDLE_VALUE)
        {
            if(WriteFile(hFile, lpData, luLen, &luWritten, NULL))
            {
                CloseHandle(hFile);

                KvLoad(&l_ConfigDB, szMbdFile);

                // foil attempts to override protected defaults
                KvForEachSection(&l_ConfigDB, &Config_P_FoilEachSection, NULL);
            }
            else
            {
                bSuccess = false;
                CloseHandle(hFile);
            }

            // clean up the evidence
            DeleteFileA(szMbdFile);
        }
        else
        {
            bSuccess = false;
        }

        if(!bSuccess)
        {
            // something bad happened
            return false;
        }
    }

    // external/user configuration
    luLen = GetModuleFileNameA(NULL, l_szIniFile, __ARRAYSIZE(l_szIniFile));
    lstrcpyA((luLen>4 && l_szIniFile[luLen-4]=='.') ? &l_szIniFile[luLen-4] : &l_szIniFile[luLen], ".ini");
    KvLoad(&l_ConfigDB, l_szIniFile);

    return true;
}

void __stdcall ConfigQuit(void)
{
    KvFree(&l_ConfigDB);
}