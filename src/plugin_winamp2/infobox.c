#include <windows.h>
#include <stdio.h>
#include "FLAC/all.h"
#include "plugin_common/all.h"
#include "infobox.h"
#include "resource.h"


typedef struct
{
    char filename[MAX_PATH];
} LOCALDATA;

static char buffer[256];
static char *genres = NULL;
static int  genresSize = 0, genresCount = 0, genresChanged = 0;

static const char infoTitle[] = "FLAC File Info";

//fixme int64
static __inline DWORD FileSize(const char *file)
{
    HANDLE hFile = CreateFile(file, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD res;

    if (hFile == INVALID_HANDLE_VALUE) return 0;
    res = GetFileSize(hFile, 0);
    CloseHandle(hFile);
    return res;
}

static __inline int GetGenresFileName(char *buffer, int size)
{
    char *c;

    if (!GetModuleFileName(NULL, buffer, size))
        return 0;
    c = strrchr(buffer, '\\');
    if (!c) return 0;
    strcpy(c+1, "genres.txt");

    return 1;
}

static void LoadGenres()
{
    HANDLE hFile;
    DWORD  spam;
    char  *c;

    if (!GetGenresFileName(buffer, sizeof(buffer))) return;
    // load file
    hFile = CreateFile(buffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;
    genresSize = GetFileSize(hFile, 0);
    if (!genresSize) return;
    genres = (char*)malloc(genresSize+2);
    if (!genres) return;
    if (!ReadFile(hFile, genres, genresSize, &spam, NULL))
    {
        free(genres);
        genres = NULL;
        return;
    }
    genres[genresSize] = 0;
    genres[genresSize+1] = 0;
    // replace newlines
    genresChanged = 0;
    genresCount = 1;

    for (c=genres; *c; c++)
    {
        if (*c == 10)
        {
            *c = 0;
            if (*(c+1))
                genresCount++;
            else genresSize--;
        }
    }

    CloseHandle(hFile);
}

static void SaveGenres(HWND hlist)
{
    HANDLE hFile;
    DWORD  spam;
    int i, count, len;

    if (!GetGenresFileName(buffer, sizeof(buffer))) return;
    // write file
    hFile = CreateFile(buffer, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    count = SendMessage(hlist, CB_GETCOUNT, 0, 0);
    for (i=0; i<count; i++)
    {
        SendMessage(hlist, CB_GETLBTEXT, i, (LPARAM)buffer);
        len = strlen(buffer);
        if (i != count-1)
        {
            buffer[len] = 10;
            len++;
        }
        WriteFile(hFile, buffer, len, &spam, NULL);
    }

    CloseHandle(hFile);
}


#define SetText(x,y)            SetDlgItemText(hwnd, x, y)
#define GetText(x,y)            (GetDlgItemText(hwnd, x, buffer, sizeof(buffer)), y = buffer[0] ? strdup(buffer) : 0)

static BOOL InitInfobox(HWND hwnd, const char *file)
{
    LOCALDATA *data = LocalAlloc(LPTR, sizeof(LOCALDATA));
	FLAC__StreamMetadata streaminfo;
    FLAC_Plugin__CanonicalTag tag;
    DWORD filesize, length, bps, ratio;

    SetWindowLong(hwnd, GWL_USERDATA, (LONG)data);
    // file name
    strncpy(data->filename, file, sizeof(data->filename));
    SetDlgItemText(hwnd, IDC_NAME, file);
    // stream data
    filesize = FileSize(file);
    if (!filesize) return FALSE;
	if (!FLAC__metadata_get_streaminfo(file, &streaminfo))
        return FALSE;

    length = (DWORD)(streaminfo.data.stream_info.total_samples / streaminfo.data.stream_info.sample_rate);
    bps = (DWORD)(filesize / (125*streaminfo.data.stream_info.total_samples/streaminfo.data.stream_info.sample_rate));
    ratio = bps*1000000 / (streaminfo.data.stream_info.sample_rate*streaminfo.data.stream_info.channels*streaminfo.data.stream_info.bits_per_sample);

    sprintf(buffer, "Sample rate: %d Hz\nChannels: %d\nBits per sample: %d\nMin block size: %d\nMax block size: %d\n"
                    "File size: %d bytes\nTotal samples: %I64d\nLength: %d:%02d\nAvg. bitrate: %d\nCompression ratio: %d.%d%%\n",
        streaminfo.data.stream_info.sample_rate, streaminfo.data.stream_info.channels, streaminfo.data.stream_info.bits_per_sample,
        streaminfo.data.stream_info.min_blocksize, streaminfo.data.stream_info.max_blocksize, filesize, streaminfo.data.stream_info.total_samples,
        length/60, length%60, bps, ratio/10, ratio%10);
    //todo: replaygain

    SetDlgItemText(hwnd, IDC_INFO, buffer);
    // tag
	FLAC_plugin__canonical_tag_init(&tag);
	FLAC_plugin__canonical_tag_get_combined(file, &tag);

    SetText(IDC_TITLE,   tag.title);
    SetText(IDC_ARTIST,  tag.performer ? tag.performer : tag.composer);
    SetText(IDC_ALBUM,   tag.album);
    SetText(IDC_COMMENT, tag.comment);
    SetText(IDC_YEAR,    tag.year_recorded ? tag.year_recorded : tag.year_performed);
    SetText(IDC_TRACK,   tag.track_number);
    SetText(IDC_GENRE,   tag.genre);

	FLAC_plugin__canonical_tag_clear(&tag);

    return TRUE;
}

static void __inline SetTag(HWND hwnd, const char *filename, FLAC_Plugin__CanonicalTag *tag)
{
    strcpy(buffer, infoTitle);

	if (FLAC_plugin__vorbiscomment_set(filename, tag))
        strcat(buffer, " [Updated]");
    else strcat(buffer, " [Failed]");

    SetWindowText(hwnd, buffer);
}

static void UpdateTag(HWND hwnd)
{
    LOCALDATA *data = (LOCALDATA*)GetWindowLong(hwnd, GWL_USERDATA);
    FLAC_Plugin__CanonicalTag tag;
	FLAC_plugin__canonical_tag_init(&tag);

    // get fields
    GetText(IDC_TITLE,   tag.title);
    GetText(IDC_ARTIST,  tag.composer);
    GetText(IDC_ALBUM,   tag.album);
    GetText(IDC_COMMENT, tag.comment);
    GetText(IDC_YEAR,    tag.year_recorded);
    GetText(IDC_TRACK,   tag.track_number);
    GetText(IDC_GENRE,   tag.genre);

    // update genres list
    if (tag.genre)
    {
        HWND hgen = GetDlgItem(hwnd, IDC_GENRE);

        if (SendMessage(hgen, CB_FINDSTRINGEXACT, -1, (LPARAM)tag.genre) == CB_ERR)
        {
            genresChanged = 1;
            SendMessage(hgen, CB_ADDSTRING, 0, (LPARAM)tag.genre);
        }
    }

    // write tag
    SetTag(hwnd, data->filename, &tag);
	FLAC_plugin__canonical_tag_clear(&tag);
}

static void RemoveTag(HWND hwnd)
{
    LOCALDATA *data = (LOCALDATA*)GetWindowLong(hwnd, GWL_USERDATA);
    FLAC_Plugin__CanonicalTag tag;
	FLAC_plugin__canonical_tag_init(&tag);

    SetText(IDC_TITLE,   "");
    SetText(IDC_ARTIST,  "");
    SetText(IDC_ALBUM,   "");
    SetText(IDC_COMMENT, "");
    SetText(IDC_YEAR,    "");
    SetText(IDC_TRACK,   "");
    SetText(IDC_GENRE,   "");

    SetTag(hwnd, data->filename, &tag);
}


static INT_PTR CALLBACK InfoProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    // init
    case WM_INITDIALOG:
        SetWindowText(hwnd, infoTitle);
        // init genres list
        {
            HWND hgen = GetDlgItem(hwnd, IDC_GENRE);
            char *c;

            // set text length limit to 64 chars
            SendMessage(hgen, CB_LIMITTEXT, 64, 0);
            // try to load genres
            if (!genres) LoadGenres(hgen);
            // add the to list
            if (genres)
            {
                SendMessage(hgen, CB_INITSTORAGE, genresCount, genresSize);

                for (c = genres; *c; c += strlen(c)+1)
                    SendMessage(hgen, CB_ADDSTRING, 0, (LPARAM)c);
            }
        }
        // init fields
        if (!InitInfobox(hwnd, (const char*)lParam))
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        return TRUE;
    // destroy
    case WM_DESTROY:
        if (genresChanged)
        {
            SaveGenres(GetDlgItem(hwnd, IDC_GENRE));
            free(genres);
            genres = 0;
        }
        LocalFree((LOCALDATA*)GetWindowLong(hwnd, GWL_USERDATA));
        break;
    // commands
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        // ok/cancel
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd, LOWORD(wParam));
            return TRUE;
        // save
        case IDC_UPDATE:
            UpdateTag(hwnd);
            break;
        // remove
        case IDC_REMOVE:
            RemoveTag(hwnd);
            break;
        }
        break;
    }

    return 0;
}


void DoInfoBox(HINSTANCE inst, HWND hwnd, const char *filename)
{
    DialogBoxParam(inst, MAKEINTRESOURCE(IDD_INFOBOX), hwnd, InfoProc, (LONG)filename);
}
