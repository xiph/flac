#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "winamp2/in2.h"
#include "winamp2/frontend.h"
#include "config.h"
#include "resource.h"


static char buffer[256];

//
//  read/write
//

#define RI(x, def)          (x = GetPrivateProfileInt("FLAC", #x, def, ini_name))
#define WI(x)               WritePrivateProfileString("FLAC", #x, itoa(x, buffer, 10), ini_name)


void ReadConfig()
{
    RI(flac_cfg.output.replaygain.enable, 1);
    RI(flac_cfg.output.replaygain.album_mode, 0);
    RI(flac_cfg.output.replaygain.hard_limit, 0);
    RI(flac_cfg.output.replaygain.preamp, 0);
    RI(flac_cfg.output.resolution.normal.dither_24_to_16, 0);
    RI(flac_cfg.output.resolution.replaygain.dither, 0);
    RI(flac_cfg.output.resolution.replaygain.noise_shaping, 1);
    RI(flac_cfg.output.resolution.replaygain.bps_out, 16);
}

void WriteConfig()
{
    WI(flac_cfg.output.replaygain.enable);
    WI(flac_cfg.output.replaygain.album_mode);
    WI(flac_cfg.output.replaygain.hard_limit);
    WI(flac_cfg.output.replaygain.preamp);
    WI(flac_cfg.output.resolution.normal.dither_24_to_16);
    WI(flac_cfg.output.resolution.replaygain.dither);
    WI(flac_cfg.output.resolution.replaygain.noise_shaping);
    WI(flac_cfg.output.resolution.replaygain.bps_out);
}

//
//  dialog
//

#define Check(x,y)              CheckDlgButton(hwnd, x, y ? BST_CHECKED : BST_UNCHECKED)
#define GetCheck(x)             (IsDlgButtonChecked(hwnd, x)==BST_CHECKED)
#define GetSel(x)               SendDlgItemMessage(hwnd, x, CB_GETCURSEL, 0, 0)
#define GetPos(x)               SendDlgItemMessage(hwnd, x, TBM_GETPOS, 0, 0)

#define PREAMP_RANGE            24


static void UpdatePreamp(HWND hwnd, HWND hamp)
{
    int pos = SendMessage(hamp, TBM_GETPOS, 0, 0) - PREAMP_RANGE;
    sprintf(buffer, "%d dB", pos);
    SetDlgItemText(hwnd, IDC_PA, buffer);
}

static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    // init
    case WM_INITDIALOG:
        Check(IDC_ENABLE, flac_cfg.output.replaygain.enable);
        Check(IDC_ALBUM, flac_cfg.output.replaygain.album_mode);
        Check(IDC_LIMITER, flac_cfg.output.replaygain.hard_limit);
        Check(IDC_DITHER, flac_cfg.output.resolution.normal.dither_24_to_16);
        Check(IDC_DITHERRG, flac_cfg.output.resolution.replaygain.dither);
        {
            HWND hamp = GetDlgItem(hwnd, IDC_PREAMP);
            SendMessage(hamp, TBM_SETRANGE, 1, MAKELONG(0, PREAMP_RANGE*2));
            SendMessage(hamp, TBM_SETPOS, 1, flac_cfg.output.replaygain.preamp+PREAMP_RANGE);
            UpdatePreamp(hwnd, hamp);
        }
        {
            HWND hlist = GetDlgItem(hwnd, IDC_TO);
            SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"16 bps");
            SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"24 bps");
            SendMessage(hlist, CB_SETCURSEL, flac_cfg.output.resolution.replaygain.bps_out/8 - 2, 0);
  
            hlist = GetDlgItem(hwnd, IDC_SHAPE);
            SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"None");
            SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"Low");
            SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"Medium");
            SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"High");
            SendMessage(hlist, CB_SETCURSEL, flac_cfg.output.resolution.replaygain.noise_shaping, 0);
        }
        return TRUE;
    // commands
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        // ok/cancel
        case IDOK:
            if (thread_handle != INVALID_HANDLE_VALUE)
                SendMessage(mod_.hMainWindow, WM_COMMAND, WINAMP_BUTTON4, 0);

            flac_cfg.output.replaygain.enable = GetCheck(IDC_ENABLE);
            flac_cfg.output.replaygain.album_mode = GetCheck(IDC_ALBUM);
            flac_cfg.output.replaygain.hard_limit = GetCheck(IDC_LIMITER);
            flac_cfg.output.replaygain.preamp = GetPos(IDC_PREAMP) - PREAMP_RANGE;
            flac_cfg.output.resolution.normal.dither_24_to_16 = GetCheck(IDC_DITHER);
            flac_cfg.output.resolution.replaygain.dither = GetCheck(IDC_DITHERRG);
            flac_cfg.output.resolution.replaygain.noise_shaping = GetSel(IDC_SHAPE);
            flac_cfg.output.resolution.replaygain.bps_out = (GetSel(IDC_TO)+2)*8;
            /* fall through */
        case IDCANCEL:
            EndDialog(hwnd, LOWORD(wParam));
            return TRUE;
        case IDC_RESET:
            Check(IDC_ENABLE, 1);
            Check(IDC_ALBUM, 0);
            Check(IDC_LIMITER, 0);
            Check(IDC_DITHER, 0);
            Check(IDC_DITHERRG, 0);

            SendDlgItemMessage(hwnd, IDC_PREAMP, TBM_SETPOS, 1, PREAMP_RANGE);
            UpdatePreamp(hwnd, GetDlgItem(hwnd, IDC_PREAMP));

            SendDlgItemMessage(hwnd, IDC_TO, CB_SETCURSEL, 0, 0);
            SendDlgItemMessage(hwnd, IDC_SHAPE, CB_SETCURSEL, 1, 0);
            break;
        }
        break;
    // scroller
    case WM_HSCROLL:
        if (GetDlgCtrlID((HWND)lParam)==IDC_PREAMP)
            UpdatePreamp(hwnd, (HWND)lParam);
        return 0;
    }

    return 0;
}


int DoConfig(HWND parent)
{
    return DialogBox(mod_.hDllInstance, MAKEINTRESOURCE(IDD_CONFIG), parent, DialogProc) == IDOK;
}
