// ModDate - zmiana daty i godziny modyfikacji pliku (Win32, C++)
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <string>

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#endif

enum {
    IDC_PATH = 101,
    IDC_BROWSE,
    IDC_CURDATE,
    IDC_UPDATE,
    IDC_DATETIME,
    IDC_NOW,
    IDC_STATUS
};

static HINSTANCE g_inst;
static HFONT g_font, g_fontBold;
static HICON g_folderIcon;
static HWND g_path, g_browse, g_curDate, g_update, g_dt, g_now, g_status;
static std::wstring g_file;
static int g_dpi = 96;

static int S(int v) { return MulDiv(v, g_dpi, 96); }

static std::wstring FormatLocal(const SYSTEMTIME& st) {
    wchar_t buf[64];
    wsprintfW(buf, L"%02d.%02d.%04d  %02d:%02d:%02d",
              st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

static void SetStatus(const std::wstring& s) { SetWindowTextW(g_status, s.c_str()); }

static std::wstring LastErrorText(DWORD err) {
    wchar_t* msg = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, 0, (LPWSTR)&msg, 0, nullptr);
    std::wstring r = msg ? msg : L"Nieznany błąd";
    if (msg) LocalFree(msg);
    while (!r.empty() && (r.back() == L'\n' || r.back() == L'\r')) r.pop_back();
    return r;
}

// Odczyt daty modyfikacji pliku (czas lokalny). Zwraca false przy błędzie.
static bool ReadModTime(const std::wstring& file, SYSTEMTIME& local) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(file.c_str(), GetFileExInfoStandard, &fad)) return false;
    SYSTEMTIME utc;
    FileTimeToSystemTime(&fad.ftLastWriteTime, &utc);
    SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local);
    return true;
}

static void RefreshFileInfo(bool loadIntoPicker) {
    BOOL hasFile = !g_file.empty();
    EnableWindow(g_update, hasFile);
    if (!hasFile) {
        SetWindowTextW(g_curDate, L"—");
        return;
    }
    SYSTEMTIME local;
    if (!ReadModTime(g_file, local)) {
        SetWindowTextW(g_curDate, L"(nie można odczytać)");
        SetStatus(L"Błąd odczytu: " + LastErrorText(GetLastError()));
        EnableWindow(g_update, FALSE);
        return;
    }
    SetWindowTextW(g_curDate, FormatLocal(local).c_str());
    if (loadIntoPicker) DateTime_SetSystemtime(g_dt, GDT_VALID, &local);
}

static void SelectFile(const std::wstring& file) {
    g_file = file;
    SetWindowTextW(g_path, file.c_str());
    SetStatus(L"");
    RefreshFileInfo(true);
}

static void Browse(HWND hwnd) {
    wchar_t buf[32768] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Wszystkie pliki (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = ARRAYSIZE(buf);
    ofn.lpstrTitle = L"Wybierz plik";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;
    if (GetOpenFileNameW(&ofn)) SelectFile(buf);
}

static void SetNow() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    st.wMilliseconds = 0;
    DateTime_SetSystemtime(g_dt, GDT_VALID, &st);
}

static void DoUpdate(HWND hwnd) {
    if (g_file.empty()) return;
    SYSTEMTIME local = {};
    if (DateTime_GetSystemtime(g_dt, &local) != GDT_VALID) {
        SetStatus(L"Nieprawidłowa data.");
        return;
    }
    local.wMilliseconds = 0;
    SYSTEMTIME utc;
    FILETIME ft;
    if (!TzSpecificLocalTimeToSystemTime(nullptr, &local, &utc) || !SystemTimeToFileTime(&utc, &ft)) {
        SetStatus(L"Nieprawidłowa data.");
        return;
    }
    HANDLE h = CreateFileW(g_file.c_str(), FILE_WRITE_ATTRIBUTES,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        SetStatus(L"Błąd: " + LastErrorText(e));
        MessageBoxW(hwnd, (L"Nie można otworzyć pliku:\n" + LastErrorText(e)).c_str(), L"ModDate", MB_ICONERROR);
        return;
    }
    BOOL ok = SetFileTime(h, nullptr, nullptr, &ft);
    DWORD e = GetLastError();
    CloseHandle(h);
    if (!ok) {
        SetStatus(L"Błąd: " + LastErrorText(e));
        MessageBoxW(hwnd, (L"Nie można zmienić daty:\n" + LastErrorText(e)).c_str(), L"ModDate", MB_ICONERROR);
        return;
    }
    RefreshFileInfo(false);
    SetStatus(L"Zaktualizowano datę modyfikacji pliku.");
}

static HWND MakeCtl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int id, DWORD ex = 0) {
    HWND h = CreateWindowExW(ex, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0,
                             parent, (HMENU)(INT_PTR)id, g_inst, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)g_font, TRUE);
    return h;
}

static HWND g_lblFile, g_lblDate, g_lblNew;

static void CreateFonts() {
    if (g_font) DeleteObject(g_font);
    if (g_fontBold) DeleteObject(g_fontBold);
    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    LOGFONTW lf = ncm.lfMessageFont;
    lf.lfHeight = -S(15);
    g_font = CreateFontIndirectW(&lf);
    lf.lfWeight = FW_BOLD;
    lf.lfHeight = -S(18);
    g_fontBold = CreateFontIndirectW(&lf);
}

static void LoadFolderIcon() {
    if (g_folderIcon) DestroyIcon(g_folderIcon);
    g_folderIcon = nullptr;
    int sz = S(20);
    HMODULE shell = LoadLibraryW(L"shell32.dll");
    if (shell) {
        // 4 = ikona otwartego folderu w shell32.dll
        g_folderIcon = (HICON)LoadImageW(shell, MAKEINTRESOURCEW(4), IMAGE_ICON, sz, sz, LR_DEFAULTCOLOR);
        FreeLibrary(shell);
    }
}

static void ApplyFonts(HWND hwnd) {
    HWND all[] = {g_lblFile, g_path, g_lblDate, g_lblNew, g_update, g_dt, g_now, g_status};
    for (HWND h : all) SendMessageW(h, WM_SETFONT, (WPARAM)g_font, TRUE);
    SendMessageW(g_curDate, WM_SETFONT, (WPARAM)g_fontBold, TRUE);
    LoadFolderIcon();
    SendMessageW(g_browse, BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_folderIcon);
    InvalidateRect(hwnd, nullptr, TRUE);
}

static void Layout(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int m = S(16), w = rc.right - 2 * m, y = m;
    int h = S(28), btn = S(110);

    MoveWindow(g_lblFile, m, y, w, S(20), TRUE);
    y += S(22);
    MoveWindow(g_path, m, y, w - h - S(6), h, TRUE);
    MoveWindow(g_browse, m + w - h, y, h, h, TRUE);
    y += h + S(16);

    MoveWindow(g_lblDate, m, y, w, S(20), TRUE);
    y += S(22);
    MoveWindow(g_curDate, m, y, w, S(26), TRUE);
    y += S(26) + S(16);

    MoveWindow(g_lblNew, m, y, w, S(20), TRUE);
    y += S(22);
    int dtw = w - 2 * btn - 2 * S(8);
    MoveWindow(g_update, m, y, btn, h + S(2), TRUE);
    MoveWindow(g_dt, m + btn + S(8), y, dtw, h + S(2), TRUE);
    MoveWindow(g_now, m + btn + S(8) + dtw + S(8), y, btn, h + S(2), TRUE);
    y += h + S(16);

    MoveWindow(g_status, m, y, w, S(20), TRUE);
}

static UINT GetDpi(HWND hwnd) {
    typedef UINT(WINAPI * Fn)(HWND);
    static Fn fn = (Fn)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
    if (fn) return fn(hwnd);
    HDC dc = GetDC(hwnd);
    UINT d = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(hwnd, dc);
    return d;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_dpi = GetDpi(hwnd);
        CreateFonts();
        g_lblFile = MakeCtl(hwnd, L"STATIC", L"Plik:", 0, 0);
        g_path = MakeCtl(hwnd, L"EDIT", L"", ES_AUTOHSCROLL | ES_READONLY | WS_TABSTOP, IDC_PATH, WS_EX_CLIENTEDGE);
        SendMessageW(g_path, EM_SETCUEBANNER, TRUE, (LPARAM)L"Kliknij ikonę folderu lub przeciągnij plik do okna…");
        g_browse = MakeCtl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | BS_ICON | WS_TABSTOP, IDC_BROWSE);
        g_lblDate = MakeCtl(hwnd, L"STATIC", L"Data modyfikacji pliku:", 0, 0);
        g_curDate = MakeCtl(hwnd, L"STATIC", L"—", SS_LEFT, IDC_CURDATE);
        g_lblNew = MakeCtl(hwnd, L"STATIC", L"Nowa data i godzina:", 0, 0);
        g_update = MakeCtl(hwnd, L"BUTTON", L"Aktualizuj", BS_DEFPUSHBUTTON | WS_TABSTOP, IDC_UPDATE);
        g_dt = MakeCtl(hwnd, DATETIMEPICK_CLASSW, L"", DTS_SHORTDATECENTURYFORMAT | WS_TABSTOP, IDC_DATETIME);
        DateTime_SetFormat(g_dt, L"dd'.'MM'.'yyyy'   'HH':'mm':'ss");
        g_now = MakeCtl(hwnd, L"BUTTON", L"Aktualny", BS_PUSHBUTTON | WS_TABSTOP, IDC_NOW);
        g_status = MakeCtl(hwnd, L"STATIC", L"", SS_LEFT | SS_ENDELLIPSIS, IDC_STATUS);
        ApplyFonts(hwnd);
        SetNow();
        RefreshFileInfo(false);
        DragAcceptFiles(hwnd, TRUE);

        // Plik podany w linii poleceń (np. przeciągnięty na ikonę .exe)
        int argc;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv) {
            if (argc > 1 && GetFileAttributesW(argv[1]) != INVALID_FILE_ATTRIBUTES) SelectFile(argv[1]);
            LocalFree(argv);
        }
        return 0;
    }
    case WM_SIZE:
        Layout(hwnd);
        return 0;
    case WM_GETMINMAXINFO: {
        auto* mm = (MINMAXINFO*)lp;
        mm->ptMinTrackSize.x = S(480);
        mm->ptMinTrackSize.y = S(300);
        return 0;
    }
    case WM_DPICHANGED: {
        g_dpi = HIWORD(wp);
        CreateFonts();
        ApplyFonts(hwnd);
        RECT* r = (RECT*)lp;
        SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wp;
        if ((HWND)lp == g_path) break;  // pole tylko do odczytu – domyślny wygląd
        SetBkMode(dc, TRANSPARENT);
        if ((HWND)lp == g_curDate) SetTextColor(dc, RGB(0, 90, 170));
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }
    case WM_DROPFILES: {
        HDROP drop = (HDROP)wp;
        wchar_t buf[32768];
        if (DragQueryFileW(drop, 0, buf, ARRAYSIZE(buf))) SelectFile(buf);
        DragFinish(drop);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BROWSE: Browse(hwnd); return 0;
        case IDC_UPDATE: DoUpdate(hwnd); return 0;
        case IDC_NOW: SetNow(); return 0;
        }
        break;
    case WM_ACTIVATE:
        // odśwież datę po powrocie do okna (plik mógł się zmienić)
        if (LOWORD(wp) != WA_INACTIVE && !g_file.empty()) RefreshFileInfo(false);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int show) {
    g_inst = inst;
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_DATE_CLASSES | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ModDateWnd";
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    wc.hIconSm = wc.hIcon;
    RegisterClassExW(&wc);

    HDC dc = GetDC(nullptr);
    g_dpi = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(nullptr, dc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"ModDate – zmiana daty modyfikacji pliku",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, S(560), S(320),
                                nullptr, nullptr, inst, nullptr);
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hwnd, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }
    return (int)m.wParam;
}
