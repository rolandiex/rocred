// -----------------------------------------------------------------
// RO Credentials (ROCred)
// (c) 2012+ Ai4rei/AN
//
// -----------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <btypes.h>
#include <bvcstr.h>
#include <bvllst.h>
#include <bvpars.h>
#include <memtaf.h>
#include <regionui.h>
#include <w32ui.h>
#include <w32uxt.h>

#include "bgskin.h"
#include "button.h"
#include "config.h"
#include "rocred.h"

BEGINSTRUCT(BUTTONSKININFO)
{
    BVLLISTNODE Node;
    UINT uID;
    HBITMAP hbmLook;
}
CLOSESTRUCT(BUTTONSKININFO);

static HBITMAP l_hbmBackground = NULL;
static HBRUSH l_hbrEditBack = NULL;
static BVLLIST l_SkinList = { 0 };  // id -> LPBGSKININFO

static const char* __stdcall BgSkin_P_ButtonId2Name(UINT uId)
{
    switch(uId)
    {
        C2N(IDOK);
        C2N(IDCANCEL);
        C2N(IDS_USERNAME);
        C2N(IDC_USERNAME);
        C2N(IDS_PASSWORD);
        C2N(IDC_PASSWORD);
        C2N(IDC_CHECKSAVE);
    }

    return NULL;
}

static unsigned char __stdcall BgSkin_P_ButtonState2Index(UINT uState)
{
    unsigned char ucIndex = 0;

    if(uState&ODS_SELECTED)
    {
        ucIndex = 2;
    }
    else if(uState&ODS_FOCUS)
    {
        ucIndex = 1;
    }

    return ucIndex;
}

static HBITMAP __stdcall BgSkin_P_GetSkin(UINT uID)
{
    LPBVLLISTNODE Item;

    for(Item = l_SkinList.Head; Item!=NULL; Item = Item->Next)
    {
        CONTEXTCAST(LPCBUTTONSKININFO,lpBsi,Item);

        if(lpBsi->uID==uID)
        {
            return lpBsi->hbmLook;
        }
    }

    return NULL;
}

static HBITMAP __stdcall BgSkin_P_LoadBitmap(const char* lpszFileName, const char* lpszImageName)
{
    HBITMAP hBitmap;

    // try local file
    if((hBitmap = LoadImage(NULL, lpszFileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE))==NULL)
    {
        // fall back to embedded, but do not actually care about the result
        hBitmap = LoadImage(GetModuleHandle(NULL), lpszImageName, IMAGE_BITMAP, 0, 0, 0);
    }

    return hBitmap;
}

static bool __stdcall BgSkin_P_IsActive(void)
{
    return (bool)(l_hbmBackground!=NULL);
}

bool __stdcall BgSkinOnEraseBkGnd(HWND hWnd, HDC hDC)
{
    RECT rcWnd;
    HDC hdcBitmap;

    if(BgSkin_P_IsActive())
    {
        if(GetClientRect(hWnd, &rcWnd))
        {
            if((hdcBitmap = CreateCompatibleDC(hDC))!=NULL)
            {
                HGDIOBJ hGdiObj = SelectObject(hdcBitmap, l_hbmBackground);

                BitBlt(hDC, 0, 0, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, hdcBitmap, 0, 0, SRCCOPY);

                SelectObject(hdcBitmap, hGdiObj);
                DeleteDC(hdcBitmap);
                return true;
            }
        }
    }

    return false;
}

bool __stdcall BgSkinOnLButtonDown(HWND hWnd)
{
    if(BgSkin_P_IsActive())
    {
        // enable moving everywhere, since we no longer have a caption
        SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return true;
    }

    return false;
}

HBRUSH __stdcall BgSkinOnCtlColorStatic(HDC hDC, HWND hWnd)
{
    if(BgSkin_P_IsActive())
    {
        SetBkMode(hDC, TRANSPARENT);

        return GetStockObject(NULL_BRUSH);
    }

    return NULL;
}

HBRUSH __stdcall BgSkinOnCtlColorEdit(HDC hDC, HWND hWnd)
{
    if(BgSkin_P_IsActive())
    {
        switch(ConfigGetInt("EditBackground"))
        {
            case 1:
                SetBkMode(hDC, TRANSPARENT);

                return GetStockObject(NULL_BRUSH);
            case 2:
                SetBkMode(hDC, TRANSPARENT);

                SetTextColor(hDC, BvStrToRgbA(ConfigGetStr("EditForegroundColor"), NULL));

                if(l_hbrEditBack)
                {
                    DeleteObject(l_hbrEditBack);
                }

                l_hbrEditBack = CreateSolidBrush(BvStrToRgbA(ConfigGetStr("EditBackgroundColor"), NULL));

                return l_hbrEditBack;
        }
    }

    return NULL;
}

bool __stdcall BgSkinOnDrawItem(UINT uID, const DRAWITEMSTRUCT* lpDis)
{
    HBITMAP hbmLook;
    HDC hdcBitmap;

    if((hbmLook = BgSkin_P_GetSkin(uID))!=NULL)
    {
        if((hdcBitmap = CreateCompatibleDC(lpDis->hDC))!=NULL)
        {
            int nWidth = lpDis->rcItem.right-lpDis->rcItem.left;
            int nHeight = lpDis->rcItem.bottom-lpDis->rcItem.top;
            HGDIOBJ hGdiObj = SelectObject(hdcBitmap, hbmLook);

            BitBlt(lpDis->hDC, lpDis->rcItem.left, lpDis->rcItem.top, nWidth, nHeight, hdcBitmap, lpDis->rcItem.left+BgSkin_P_ButtonState2Index(lpDis->itemState)*nWidth, lpDis->rcItem.top, SRCCOPY);

            SelectObject(hdcBitmap, hGdiObj);
            DeleteDC(hdcBitmap);
            return true;
        }
    }

    return false;
}

static void __stdcall BgSkin_P_RegisterButtonSkin(unsigned int uBtnId, const char* lpszName)
{
    char szFileName[MAX_PATH];
    HBITMAP hbmLook;

    wsprintfA(szFileName, "%s.bmp", lpszName);
    BvStrToLowerA(szFileName);

    hbmLook = BgSkin_P_LoadBitmap(szFileName, lpszName);

    if(hbmLook)
    {
        LPBUTTONSKININFO lpBsi = NULL;
        
        if(MemTAlloc(&lpBsi))
        {
            BvLListNodeInit(&lpBsi->Node);

            lpBsi->uID     = uBtnId;
            lpBsi->hbmLook = hbmLook;

            BvLListInsert(&l_SkinList, &lpBsi->Node);
        }
        else
        {
            DeleteBitmap(hbmLook);
        }
    }
}

bool __stdcall BgSkinInit(HWND hWnd)
{
    BITMAP bmBG;
    HRGN hRGN;
    HWND hChildWnd = NULL;
    RECT rcWnd, rcWA;

    l_hbmBackground = BgSkin_P_LoadBitmap("bgskin.bmp", "BGSKIN");

    if(l_hbmBackground)
    {
        if(GetObject(l_hbmBackground, sizeof(bmBG), &bmBG))
        {
            // set window size to bitmap size and get rid of
            // non-client elements while being at it
            SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX);
            SetWindowPos(hWnd, NULL, 0, 0, bmBG.bmWidth, bmBG.bmHeight, SWP_NOZORDER|SWP_NOMOVE);

            // center window
            GetWindowRect(hWnd, &rcWnd);
            SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWA, 0);
            OffsetRect(&rcWA, ((rcWA.right-rcWA.left)-(rcWnd.right-rcWnd.left))/2, ((rcWA.bottom-rcWA.top)-(rcWnd.bottom-rcWnd.top))/2);
            SetWindowPos(hWnd, NULL, rcWA.left, rcWA.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

            // window size for base window region (origin transform)
            GetWindowRect(hWnd, &rcWnd);
            rcWnd.right-= rcWnd.left;
            rcWnd.bottom-= rcWnd.top;
            rcWnd.left = rcWnd.top = 0;

            // set up region
            if((hRGN = CreateRectRgnIndirect(&rcWnd))!=NULL)
            {
                if(MaskRegionFromBitmap(hRGN, l_hbmBackground, RGB(255, 0, 255)))
                {
                    SetWindowRgn(hWnd, hRGN, TRUE);
                    hRGN = NULL;  // system took ownership
                }
                else
                {
                    DeleteObject(hRGN);
                }
            }
        }

        // process all child windows
        while((hChildWnd = FindWindowExA(hWnd, hChildWnd, NULL, NULL))!=NULL)
        {
            char szBuffer[128];
            const char* lpszName;
            unsigned int uBtnId = GetDlgCtrlID(hChildWnd);
            int nX, nY, nW, nH;
            HBITMAP hbmLook;

            lpszName = BgSkin_P_ButtonId2Name(uBtnId);

            // disable visual styles to prevent glitches
            W32UxTheme_DisableVisualStyle(hChildWnd);

            if(lpszName)
            {// fixed control
                char szKeyName[64];

                switch(uBtnId)
                {// button
                    case IDOK:       BgSkin_P_RegisterButtonSkin(uBtnId, "BTNSTART"); break;
                    case IDCANCEL:   BgSkin_P_RegisterButtonSkin(uBtnId, "BTNCLOSE"); break;
                }

                wsprintfA(szKeyName, "%s.X", lpszName); nX = ConfigGetInt(szKeyName);
                wsprintfA(szKeyName, "%s.Y", lpszName); nY = ConfigGetInt(szKeyName);
                wsprintfA(szKeyName, "%s.W", lpszName); nW = ConfigGetInt(szKeyName);
                wsprintfA(szKeyName, "%s.H", lpszName); nH = ConfigGetInt(szKeyName);

                if((hbmLook = BgSkin_P_GetSkin(uBtnId))!=NULL && GetObject(hbmLook, sizeof(bmBG), &bmBG))
                {// take W/H from skin in pixels
                    nW = bmBG.bmWidth/3;
                    nH = bmBG.bmHeight;

                    // switch button to use skins
                    SetWindowLongPtr(hChildWnd, GWL_STYLE, GetWindowLongPtr(hChildWnd, GWL_STYLE)|BS_OWNERDRAW);
                }

                // remove border, if any
                if(GetClassKind(hChildWnd)==CTL_BASE_EDIT && ConfigGetInt("EditFrame"))
                {
                    SetWindowLongPtr(hChildWnd, GWL_STYLE, GetWindowLongPtr(hChildWnd, GWL_STYLE)&~WS_BORDER);
                    SetWindowLongPtr(hChildWnd, GWL_EXSTYLE, GetWindowLongPtr(hChildWnd, GWL_EXSTYLE)&~WS_EX_CLIENTEDGE);
                }

                // set size
                SetWindowPos(hChildWnd, NULL, nX, nY, nW, nH, SWP_NOZORDER);
                continue;
            }

            lpszName = ButtonGetName(uBtnId, szBuffer, __ARRAYSIZE(szBuffer));

            if(lpszName)
            {// custom button
                BgSkin_P_RegisterButtonSkin(uBtnId, lpszName);

                if((hbmLook = BgSkin_P_GetSkin(uBtnId))!=NULL && GetObject(hbmLook, sizeof(bmBG), &bmBG))
                {// custom buttons change if they are skinned
                    char szSectionName[MAX_PATH];

                    wsprintfA(szSectionName, "ROCred.Buttons.%s", lpszName);  // FIXME: only button.c should (have to) know this!

                    nX = ConfigGetIntFromSection(szSectionName, "X");
                    nY = ConfigGetIntFromSection(szSectionName, "Y");
                    nW = bmBG.bmWidth/3;
                    nH = bmBG.bmHeight;

                    // switch button to use skins
                    SetWindowLongPtr(hChildWnd, GWL_STYLE, GetWindowLongPtr(hChildWnd, GWL_STYLE)|BS_OWNERDRAW);

                    // set size
                    SetWindowPos(hChildWnd, NULL, nX, nY, nW, nH, SWP_NOZORDER);
                }
                continue;
            }

            // anything else
        }
    }

    return true;
}

static void __WDECL BgSkin_P_ReleaseSkin(LPBVLLISTNODE Node, void* lpContext)
{
    CONTEXTCAST(LPBUTTONSKININFO,lpBsi,Node);

    if(lpBsi->hbmLook)
    {
        DeleteBitmap(lpBsi->hbmLook);
    }

    MemTFree(&lpBsi);
}

void __stdcall BgSkinFree(void)
{
    BvLListClear(&l_SkinList, BgSkin_P_ReleaseSkin, NULL);

    if(l_hbrEditBack)
    {
        DeleteObject(l_hbrEditBack);
        l_hbrEditBack = NULL;
    }

    if(l_hbmBackground)
    {
        DeleteObject(l_hbmBackground);
        l_hbmBackground = NULL;
    }
}
