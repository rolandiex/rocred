// -----------------------------------------------------------------
// RO Credentials (ROCred)
// (c) 2012-2013 Ai4rei/AN
//
// -----------------------------------------------------------------

#ifndef _CONFIG_H_
#define _CONFIG_H_

void __stdcall ConfigSetStr(const char* lpszKey, const char* lpszValue);
void __stdcall ConfigSetInt(const char* lpszKey, int nValue);
void __stdcall ConfigSetIntU(const char* lpszKey, unsigned int uValue);
const char* __stdcall ConfigGetStr(const char* lpszKey);
int __stdcall ConfigGetInt(const char* lpszKey);
unsigned int __stdcall ConfigGetIntU(const char* lpszKey);
bool __stdcall ConfigSave(void);
bool __stdcall ConfigInit(void);
void __stdcall ConfigQuit(void);

#endif  /* _CONFIG_H_ */