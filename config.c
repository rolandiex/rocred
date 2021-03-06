// -----------------------------------------------------------------
// RO Credentials (ROCred)
// (c) 2012+ Ai4rei/AN
//
// -----------------------------------------------------------------

#include <stdio.h>

#include <windows.h>

#include <btypes.h>
#include <kvdb.h>
#include <memtaf.h>
#include <rsrcio.h>
#include <w32ex.h>

#include "config.h"
#include "rocred.h"

struct LPFOREACHSECTIONCONTEXT
{
    LPFNFOREACHSECTION Func;
    void* lpContext;
};

static char l_szIniFile[MAX_PATH] = { 0 };
static KVDB l_ConfigDB = { 0 };

#define CONFIG_MAIN_SECTION "ROCred"

static bool __stdcall Config_P_ForEachSection(LPKVDB DB, LPKVDBSECTION Section, const char* const lpszSection, void* lpContext)
{
    struct LPFOREACHSECTIONCONTEXT* lpCtx = (struct LPFOREACHSECTIONCONTEXT*)lpContext;

    return lpCtx->Func(lpszSection, lpCtx->lpContext);
}

void __stdcall ConfigForEachSectionMatch(const char* lpszMatch, LPFNFOREACHSECTION Func, void* lpContext)
{
    struct LPFOREACHSECTIONCONTEXT Ctx = { Func, lpContext };

    KvForEachSectionMatch(&l_ConfigDB, lpszMatch, &Config_P_ForEachSection, &Ctx);
}

void __stdcall ConfigSetStr(const char* lpszKey, const char* lpszValue)
{
    if(lpszValue)
    {
        KvKeySetStrValue(&l_ConfigDB, NULL, CONFIG_MAIN_SECTION, NULL, lpszKey, lpszValue);
    }
    else
    {
        KvKeyDelete(&l_ConfigDB, NULL, CONFIG_MAIN_SECTION, NULL, lpszKey);
    }

    KvSave(&l_ConfigDB, l_szIniFile);
}

void __stdcall ConfigSetInt(const char* lpszKey, int nValue)
{
    char szBuffer[16];

    snprintf(szBuffer, __ARRAYSIZE(szBuffer), "%d", nValue);
    ConfigSetStr(lpszKey, szBuffer);
}

void __stdcall ConfigSetIntU(const char* lpszKey, unsigned int uValue)
{
    char szBuffer[16];

    snprintf(szBuffer, __ARRAYSIZE(szBuffer), "%u", uValue);
    ConfigSetStr(lpszKey, szBuffer);
}

const char* __stdcall ConfigGetStrFromSection(const char* lpszSection, const char* lpszKey)
{
    return KvKeyGetStrValue(&l_ConfigDB, NULL, lpszSection, NULL, lpszKey);
}

int __stdcall ConfigGetIntFromSection(const char* lpszSection, const char* lpszKey)
{
    return atoi(ConfigGetStrFromSection(lpszSection, lpszKey));
}

unsigned int __stdcall ConfigGetIntUFromSection(const char* lpszSection, const char* lpszKey)
{
    return ConfigGetIntFromSection(lpszSection, lpszKey);
}

const char* __stdcall ConfigGetStr(const char* lpszKey)
{
    return ConfigGetStrFromSection(CONFIG_MAIN_SECTION, lpszKey);
}

int __stdcall ConfigGetInt(const char* lpszKey)
{
    return ConfigGetIntFromSection(CONFIG_MAIN_SECTION, lpszKey);
}

unsigned int __stdcall ConfigGetIntU(const char* lpszKey)
{
    return ConfigGetIntUFromSection(CONFIG_MAIN_SECTION, lpszKey);
}

bool __stdcall ConfigSave(void)
{
    bool bSuccess = false;
    HANDLE hFile;

    // load configuration
    hFile = CreateFileA(l_szIniFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if(hFile!=INVALID_HANDLE_VALUE)
    {
        ubyte_t* lpucBuffer = NULL;
        DWORD dwFileSize = GetFileSize(hFile, NULL);  // if your config is over 4GB, you are doing something wrong

        if(dwFileSize && MemTAllocEx(&lpucBuffer, dwFileSize))
        {
            DWORD dwRead;

            if(ReadFile(hFile, lpucBuffer, dwFileSize, &dwRead, NULL))
            {
                char szSrcName[MAX_PATH];

                if(GetModuleFileNameSpecificPathA(NULL, szSrcName, __ARRAYSIZE(szSrcName), NULL, NULL))
                {
                    char szDstName[MAX_PATH];

                    if(GetModuleFileNameSpecificPathA(NULL, szDstName, __ARRAYSIZE(szDstName), NULL, "embed.exe"))
                    {
                        if(CopyFileA(szSrcName, szDstName, FALSE))
                        {
                            // persist as resource
                            if(ResourceStoreA(szDstName, MAKEINTRESOURCE(RT_RCDATA), "CONFIG", lpucBuffer, dwFileSize))
                            {
                                bSuccess = true;
                            }
                            else
                            {
                                DeleteFileA(szDstName);
                            }
                        }
                    }
                }
            }

            MemTFree(&lpucBuffer);
        }

        CloseFile(&hFile);
    }

    return bSuccess;
}

static bool __stdcall Config_P_FoilEachKey(LPKVDB DB, const char* const lpszSection, LPKVDBKEY Key, const char* const lpszKey, void* lpContext)
{
    if(lpszKey[0]=='_')
    {
        KvKeyDelete(NULL, NULL, NULL, Key, NULL);
    }

    return true;
}

static bool __stdcall Config_P_FoilEachSection(LPKVDB DB, LPKVDBSECTION Section, const char* const lpszSection, void* lpContext)
{
    KvForEachKey(NULL, Section, NULL, &Config_P_FoilEachKey, NULL);
    return true;
}

bool __stdcall ConfigInit(void)
{
    bool bSuccess = false;
    const void* lpData;
    unsigned long luLen, luWritten;

    // set defaults
    KvInit(&l_ConfigDB, &g_Win32PrivateProfileAdapter);
    KvKeySetStrValue(&l_ConfigDB, NULL, CONFIG_MAIN_SECTION, NULL, "ExeType", "1rag");
    KvKeySetStrValue(&l_ConfigDB, NULL, CONFIG_MAIN_SECTION, NULL, "FontSize", "9");
    KvDirty(&l_ConfigDB, false);

    // load embedded/admin configuration
    if(ResourceFetchA(NULL, MAKEINTRESOURCE(RT_RCDATA), "CONFIG", &lpData, &luLen))
    {
        for(;;)
        {
            char szTmpPath[MAX_PATH];
            char szMbdFile[MAX_PATH];
            HANDLE hFile;

            if(!GetTempPathExA(szTmpPath, __ARRAYSIZE(szTmpPath)))
            {
                break;
            }

            if(!GetTempFileNameExA(szTmpPath, "~rcd", 0, szMbdFile, __ARRAYSIZE(szMbdFile)) || GetLastError()!=ERROR_SUCCESS)
            {
                break;
            }

            hFile = CreateFileA(szMbdFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_NONE, NULL, OPEN_EXISTING /* GetTempFileNameEx created it */, 0, NULL);

            if(hFile!=INVALID_HANDLE_VALUE)
            {
                if(WriteFile(hFile, lpData, luLen, &luWritten, NULL))
                {
                    CloseFile(&hFile);

                    if(KvLoad(&l_ConfigDB, szMbdFile))
                    {
                        // foil attempts to override protected defaults
                        KvForEachSection(&l_ConfigDB, &Config_P_FoilEachSection, NULL);

                        bSuccess = true;
                    }
                }
                else
                {
                    CloseFile(&hFile);
                }
            }

            // clean up the evidence
            DeleteFileA(szMbdFile);
            break;
        }

        if(!bSuccess)
        {
            // something bad happened
            return false;
        }
    }

    // external/user configuration
    if(GetModuleFileNameSpecificPathA(NULL, l_szIniFile, __ARRAYSIZE(l_szIniFile), NULL, "ini"))
    {
        KvLoad(&l_ConfigDB, l_szIniFile);
    }

    return true;
}

void __stdcall ConfigQuit(void)
{
    KvFree(&l_ConfigDB);
}
