/*
 * PROJECT:     ReactOS Display Control Panel
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Desktop customization property page
 * COPYRIGHT:   Copyright 2018 Stanislav Motylkov (x86corez@gmail.com)
 */

#include "desk.h"

VOID
DesktopOnInitDialog(IN HWND hwndDlg)
{
    return;
}

/* Property page dialog callback */
INT_PTR CALLBACK
DesktopPageProc(IN HWND hwndDlg, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            DesktopOnInitDialog(hwndDlg);
            break;
        }
    }
    return FALSE;
}
