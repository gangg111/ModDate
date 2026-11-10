// ModDate - zmiana daty i godziny modyfikacji pliku (Win32, C++)
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <objbase.h>
#include <gdiplus.h>
#include <string>

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#endif

using namespace Gdiplus;

// ---------------------------------------------------------------- stałe ----
enum { IDC_BROWSE = 101, IDC_UPDATE, IDC_DATETIME, IDC_NOW };

// Wymiary w jednostkach niezależnych od DPI (96 DPI)
static const int WIN_W = 600, WIN_H = 474, MARGIN = 20, PAD = 18;

// Paleta (styl Windows 11)
static const COLORREF CLR_BG          = RGB(243, 243, 243);
static const COLORREF CLR_CARD        = RGB(255, 255, 255);
static const COLORREF CLR_CARD_BORDER = RGB(229, 229, 229);
static const COLORREF CLR_TEXT        = RGB(27, 27, 27);
static const COLORREF CLR_TEXT2       = RGB(96, 96, 96);
static const COLORREF CLR_LABEL       = RGB(112, 112, 112);
static const COLORREF CLR_ACCENT      = RGB(0, 103, 192);
static const COLORREF CLR_ACCENT_HOT  = RGB(25, 117, 197);
static const COLORREF CLR_ACCENT_DOWN = RGB(0, 84, 158);
static const COLORREF CLR_ACCENT_SOFT = RGB(232, 241, 251);
static const COLORREF CLR_ACCENT_SOFT2= RGB(214, 231, 248);
static const COLORREF CLR_FIELD       = RGB(250, 250, 250);
static const COLORREF CLR_FIELD_HOT   = RGB(243, 248, 253);
static const COLORREF CLR_FIELD_BORDER= RGB(209, 209, 209);
static const COLORREF CLR_OK_BG       = RGB(223, 246, 221);
static const COLORREF CLR_OK          = RGB(16, 124, 16);
static const COLORREF CLR_ERR_BG      = RGB(253, 231, 233);
static const COLORREF CLR_ERR         = RGB(196, 43, 28);
static const COLORREF CLR_INFO_BG     = RGB(234, 234, 234);
static const COLORREF CLR_INFO        = RGB(96, 96, 96);

// ---------------------------------------------------------------- stan -----
static HINSTANCE g_inst;
static HWND g_hwnd, g_browse, g_update, g_dt, g_now;
static int g_dpi = 96;
static HFONT fTitle, fSub, fLabel, fBody, fBodyBold, fBig, fBtn, fSmall;
static HICON g_appIcon, g_fileIcon;
static std::wstring g_file;
static bool g_hasTimes;
static SYSTEMTIME g_mod, g_created, g_access;
static ULONGLONG g_size;
static enum { ST_INFO, ST_OK, ST_ERR } g_statusKind = ST_INFO;
static std::wstring g_status = L"Wybierz plik, aby rozpocząć.";
static bool g_zoneHover;
static RECT rcCard1, rcCard2, rcCard3, rcZone, rcStatus;

static int S(int v) { return MulDiv(v, g_dpi, 96); }

// --------------------------------------------------------- formatowanie ----
static const wchar_t* DAYS[] = {L"niedziela", L"poniedziałek", L"wtorek", L"środa",
                                L"czwartek", L"piątek", L"sobota"};
static const wchar_t* MONTHS[] = {L"stycznia", L"lutego", L"marca", L"kwietnia", L"maja", L"czerwca",
                                  L"lipca", L"sierpnia", L"września", L"października", L"listopada", L"grudnia"};

static std::wstring FmtDateTime(const SYSTEMTIME& st, bool twoSpaces = true) {
    wchar_t b[64];
    wsprintfW(b, twoSpaces ? L"%02d.%02d.%04d   %02d:%02d:%02d" : L"%02d.%02d.%04d  %02d:%02d:%02d",
              st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
    return b;
}

static std::wstring FmtShort(const SYSTEMTIME& st) {
    wchar_t b[32];
    wsprintfW(b, L"%02d.%02d.%04d  %02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
    return b;
}

static std::wstring FmtLong(SYSTEMTIME st) {
    FILETIME ft;
    if (SystemTimeToFileTime(&st, &ft)) FileTimeToSystemTime(&ft, &st);  // uzupełnia dzień tygodnia
    wchar_t b[96];
    wsprintfW(b, L"%s, %d %s %d", DAYS[st.wDayOfWeek % 7], st.wDay, MONTHS[(st.wMonth + 11) % 12], st.wYear);
    return b;
}

static std::wstring FmtSize(ULONGLONG n) {
    const wchar_t* u[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
    double v = (double)n;
    int i = 0;
    while (v >= 1024 && i < 4) { v /= 1024; ++i; }
    wchar_t b[32];
    if (i == 0) wsprintfW(b, L"%u B", (unsigned)n);
    else {
        int t = (int)(v * 10 + 0.5);
        wsprintfW(b, L"%d,%d %s", t / 10, t % 10, u[i]);
    }
    return b;
}

static std::wstring ErrText(DWORD err) {
    wchar_t* msg = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, 0, (LPWSTR)&msg, 0, nullptr);
    std::wstring r = msg ? msg : L"nieznany błąd";
    if (msg) LocalFree(msg);
    while (!r.empty() && (r.back() == L'\n' || r.back() == L'\r' || r.back() == L' ' || r.back() == L'.')) r.pop_back();
    return r;
}

// ------------------------------------------------------------ rysowanie ----
static Color GC(COLORREF c, BYTE a = 255) { return Color(a, GetRValue(c), GetGValue(c), GetBValue(c)); }

static void RoundPath(GraphicsPath& p, RectF r, REAL rad) {
    REAL d = rad * 2;
    p.AddArc(r.X, r.Y, d, d, 180, 90);
    p.AddArc(r.X + r.Width - d, r.Y, d, d, 270, 90);
    p.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0, 90);
    p.AddArc(r.X, r.Y + r.Height - d, d, d, 90, 90);
    p.CloseFigure();
}

// Zaokrąglony prostokąt z wypełnieniem i (opcjonalnie) obramowaniem
static void FillRR(HDC dc, const RECT& rc, int radius, COLORREF fill, COLORREF border = CLR_NONE,
                   REAL borderW = 1.0f, bool dashed = false) {
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    RectF r((REAL)rc.left - 0.5f, (REAL)rc.top - 0.5f, (REAL)(rc.right - rc.left), (REAL)(rc.bottom - rc.top));
    if (border != CLR_NONE) r.Inflate(-borderW / 2, -borderW / 2);
    GraphicsPath p;
    RoundPath(p, r, (REAL)radius);
    SolidBrush b(GC(fill));
    g.FillPath(&b, &p);
    if (border != CLR_NONE) {
        Pen pen(GC(border), borderW);
        if (dashed) pen.SetDashStyle(DashStyleDash);
        g.DrawPath(&pen, &p);
    }
}

static void FillCircle(HDC dc, int cx, int cy, int r, COLORREF c) {
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    SolidBrush b(GC(c));
    g.FillEllipse(&b, (REAL)(cx - r), (REAL)(cy - r), (REAL)(2 * r), (REAL)(2 * r));
}

static void Text(HDC dc, const std::wstring& s, RECT r, HFONT f, COLORREF c, UINT flags) {
    SelectObject(dc, f);
    SetTextColor(dc, c);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, s.c_str(), (int)s.size(), &r, flags | DT_NOPREFIX);
}

static int TextWidth(HDC dc, const std::wstring& s, HFONT f) {
    SelectObject(dc, f);
    SIZE sz;
    GetTextExtentPoint32W(dc, s.c_str(), (int)s.size(), &sz);
    return sz.cx;
}

// Ikona folderu rysowana wektorowo
static void DrawFolderGlyph(HDC dc, RECT rc, COLORREF c) {
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    REAL w = (REAL)S(22), h = (REAL)S(17);
    REAL x = (rc.left + rc.right - w) / 2, y = (rc.top + rc.bottom - h) / 2;
    SolidBrush b(GC(c));
    GraphicsPath tab, body;
    RoundPath(tab, RectF(x, y, w * 0.45f, h * 0.4f), (REAL)S(2));
    RoundPath(body, RectF(x, y + h * 0.2f, w, h * 0.8f), (REAL)S(2));
    g.FillPath(&b, &tab);
    g.FillPath(&b, &body);
    SolidBrush light(Color(70, 255, 255, 255));
    g.FillRectangle(&light, x + (REAL)S(2), y + h * 0.2f + (REAL)S(3), w - (REAL)S(4), (REAL)S(1));
}

// Zastępcza ikona dokumentu (gdy system nie zwróci ikony pliku)
static void DrawDocGlyph(HDC dc, RECT rc) {
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    RECT page = {rc.left + w / 6, rc.top + h / 16, rc.right - w / 6, rc.bottom - h / 16};
    FillRR(dc, page, S(3), RGB(255, 255, 255), RGB(150, 160, 175), (REAL)S(1));
    for (int i = 0; i < 3; ++i) {
        RECT ln = {page.left + S(5), page.top + S(8) + i * S(6), page.right - S(5), page.top + S(10) + i * S(6)};
        FillRR(dc, ln, S(1), CLR_ACCENT_SOFT2);
    }
}

static void DrawCard(HDC dc, const RECT& r, const wchar_t* label) {
    FillRR(dc, r, S(8), CLR_CARD, CLR_CARD_BORDER);
    RECT lr = {r.left + S(PAD), r.top + S(14), r.right - S(PAD), r.top + S(30)};
    Text(dc, label, lr, fLabel, CLR_LABEL, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
}

static void Paint(HDC target) {
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    HDC dc = CreateCompatibleDC(target);
    HBITMAP bmp = CreateCompatibleBitmap(target, rc.right, rc.bottom);
    HGDIOBJ oldBmp = SelectObject(dc, bmp);
    HGDIOBJ oldFont = SelectObject(dc, fBody);

    HBRUSH bg = CreateSolidBrush(CLR_BG);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    // --- nagłówek
    int m = S(MARGIN);
    DrawIconEx(dc, m, S(18), g_appIcon, S(40), S(40), 0, nullptr, DI_NORMAL);
    RECT tr = {m + S(54), S(16), rc.right - m, S(42)};
    Text(dc, L"ModDate", tr, fTitle, CLR_TEXT, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    tr = {m + S(54), S(41), rc.right - m, S(60)};
    Text(dc, L"Zmiana daty i godziny modyfikacji pliku", tr, fSub, CLR_TEXT2, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    // --- karta 1: plik
    DrawCard(dc, rcCard1, L"PLIK");
    bool has = !g_file.empty();
    FillRR(dc, rcZone, S(6), g_zoneHover ? CLR_FIELD_HOT : CLR_FIELD,
           g_zoneHover ? CLR_ACCENT : CLR_FIELD_BORDER, (REAL)S(1), !has);
    if (has) {
        int iconSz = S(32);
        int ix = rcZone.left + S(10), iy = (rcZone.top + rcZone.bottom - iconSz) / 2;
        if (g_fileIcon) DrawIconEx(dc, ix, iy, g_fileIcon, iconSz, iconSz, 0, nullptr, DI_NORMAL);
        else DrawDocGlyph(dc, {ix, iy, ix + iconSz, iy + iconSz});
        size_t slash = g_file.find_last_of(L"\\/");
        std::wstring name = slash == std::wstring::npos ? g_file : g_file.substr(slash + 1);
        std::wstring dir = slash == std::wstring::npos ? L"" : g_file.substr(0, slash);
        int tx = ix + iconSz + S(10), mid = (rcZone.top + rcZone.bottom) / 2;
        RECT r1 = {tx, rcZone.top + S(5), rcZone.right - S(10), mid + S(1)};
        Text(dc, name, r1, fBodyBold, CLR_TEXT, DT_LEFT | DT_SINGLELINE | DT_BOTTOM | DT_END_ELLIPSIS);
        std::wstring line2 = dir + (g_hasTimes ? L"   •   " + FmtSize(g_size) : L"");
        RECT r2 = {tx, mid + S(1), rcZone.right - S(10), rcZone.bottom - S(5)};
        Text(dc, line2, r2, fSmall, CLR_TEXT2, DT_LEFT | DT_SINGLELINE | DT_TOP | DT_PATH_ELLIPSIS);
    } else {
        Text(dc, L"Kliknij, aby wybrać plik, lub przeciągnij go tutaj", rcZone, fBody,
             g_zoneHover ? CLR_ACCENT : CLR_TEXT2, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }

    // --- karta 2: obecna data modyfikacji
    DrawCard(dc, rcCard2, L"DATA MODYFIKACJI PLIKU");
    int cx = rcCard2.left + S(PAD), cr = rcCard2.right - S(PAD);
    RECT big = {cx, rcCard2.top + S(34), cr, rcCard2.top + S(74)};
    RECT sub = {cx, rcCard2.top + S(74), cr, rcCard2.top + S(96)};
    if (g_hasTimes) {
        Text(dc, FmtDateTime(g_mod, false), big, fBig, CLR_ACCENT, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        Text(dc, FmtLong(g_mod), sub, fSmall, CLR_TEXT2, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        // prawa kolumna: utworzono / ostatni dostęp
        int colL = cr - S(196);
        RECT a = {colL, rcCard2.top + S(40), cr, rcCard2.top + S(58)};
        RECT b = {colL, rcCard2.top + S(66), cr, rcCard2.top + S(84)};
        Text(dc, L"Utworzono", a, fSmall, CLR_LABEL, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        Text(dc, FmtShort(g_created), a, fSmall, CLR_TEXT, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
        Text(dc, L"Dostęp", b, fSmall, CLR_LABEL, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        Text(dc, FmtShort(g_access), b, fSmall, CLR_TEXT, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
        // pionowy separator
        RECT sep = {colL - S(18), rcCard2.top + S(40), colL - S(18) + 1, rcCard2.top + S(84)};
        HBRUSH sb = CreateSolidBrush(CLR_CARD_BORDER);
        FillRect(dc, &sep, sb);
        DeleteObject(sb);
    } else {
        Text(dc, L"— —", big, fBig, CLR_FIELD_BORDER, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        Text(dc, L"Nie wybrano pliku", sub, fSmall, CLR_TEXT2, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    }

    // --- karta 3: nowa data (kontrolki są oknami potomnymi)
    DrawCard(dc, rcCard3, L"NOWA DATA I GODZINA");

    // --- pasek statusu
    if (!g_status.empty()) {
        COLORREF bgc = g_statusKind == ST_OK ? CLR_OK_BG : g_statusKind == ST_ERR ? CLR_ERR_BG : CLR_INFO_BG;
        COLORREF fg = g_statusKind == ST_OK ? CLR_OK : g_statusKind == ST_ERR ? CLR_ERR : CLR_INFO;
        int tw = TextWidth(dc, g_status, fSmall);
        int maxW = rcStatus.right - rcStatus.left;
        int pw = tw + S(44);
        if (pw > maxW) pw = maxW;
        RECT pill = {rcStatus.left, rcStatus.top, rcStatus.left + pw, rcStatus.bottom};
        FillRR(dc, pill, (pill.bottom - pill.top) / 2, bgc);
        int cy = (pill.top + pill.bottom) / 2;
        FillCircle(dc, pill.left + S(15), cy, S(4), fg);
        RECT t = {pill.left + S(27), pill.top, pill.right - S(12), pill.bottom};
        Text(dc, g_status, t, fSmall, fg, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    }

    BitBlt(target, 0, 0, rc.right, rc.bottom, dc, 0, 0, SRCCOPY);
    SelectObject(dc, oldFont);
    SelectObject(dc, oldBmp);
    DeleteObject(bmp);
    DeleteDC(dc);
}

// Przyciski rysowane samodzielnie
static void DrawButton(const DRAWITEMSTRUCT* d) {
    RECT r = d->rcItem;
    int w = r.right - r.left, h = r.bottom - r.top;
    HDC dc = CreateCompatibleDC(d->hDC);
    HBITMAP bmp = CreateCompatibleBitmap(d->hDC, w, h);
    HGDIOBJ ob = SelectObject(dc, bmp);
    RECT lr = {0, 0, w, h};
    HBRUSH bg = CreateSolidBrush(CLR_CARD);
    FillRect(dc, &lr, bg);
    DeleteObject(bg);

    bool dis = d->itemState & ODS_DISABLED;
    bool down = d->itemState & ODS_SELECTED;
    bool focus = (d->itemState & ODS_FOCUS) && !(d->itemState & ODS_NOFOCUSRECT);
    bool hot = GetPropW(d->hwndItem, L"hot") != nullptr;
    int rad = S(6);

    if (d->CtlID == IDC_UPDATE) {  // przycisk główny – akcent
        COLORREF fill = dis ? RGB(235, 235, 235) : down ? CLR_ACCENT_DOWN : hot ? CLR_ACCENT_HOT : CLR_ACCENT;
        FillRR(dc, lr, rad, fill, dis ? RGB(225, 225, 225) : CLR_NONE);
        if (focus) {
            RECT fr = {S(2), S(2), w - S(2), h - S(2)};
            FillRR(dc, fr, rad - S(1), fill, RGB(255, 255, 255), (REAL)S(1));
        }
        Text(dc, L"Aktualizuj", lr, fBtn, dis ? RGB(160, 160, 160) : RGB(255, 255, 255),
             DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    } else if (d->CtlID == IDC_BROWSE) {  // przycisk z ikoną folderu
        COLORREF fill = down ? CLR_ACCENT_SOFT2 : hot ? CLR_ACCENT_SOFT2 : CLR_ACCENT_SOFT;
        FillRR(dc, lr, rad, fill, focus ? CLR_ACCENT : (hot ? RGB(190, 214, 240) : CLR_NONE),
               (REAL)(focus ? S(2) : 1));
        DrawFolderGlyph(dc, lr, down ? CLR_ACCENT_DOWN : CLR_ACCENT);
    } else {  // przycisk drugorzędny
        COLORREF fill = down ? RGB(240, 240, 240) : hot ? RGB(246, 246, 246) : RGB(255, 255, 255);
        FillRR(dc, lr, rad, fill, focus ? CLR_ACCENT : CLR_FIELD_BORDER, (REAL)(focus ? S(2) : 1));
        wchar_t txt[64];
        GetWindowTextW(d->hwndItem, txt, 64);
        Text(dc, txt, lr, fBtn, down ? CLR_TEXT2 : CLR_TEXT, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }
    BitBlt(d->hDC, r.left, r.top, w, h, dc, 0, 0, SRCCOPY);
    SelectObject(dc, ob);
    DeleteObject(bmp);
    DeleteDC(dc);
}

static LRESULT CALLBACK ButtonProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR) {
    switch (m) {
    case WM_MOUSEMOVE:
        if (!GetPropW(h, L"hot")) {
            SetPropW(h, L"hot", (HANDLE)1);
            TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, h, 0};
            TrackMouseEvent(&t);
            InvalidateRect(h, nullptr, FALSE);
        }
        break;
    case WM_MOUSELEAVE:
        RemovePropW(h, L"hot");
        InvalidateRect(h, nullptr, FALSE);
        break;
    case WM_LBUTTONDBLCLK:  // szybkie klikanie = kolejne kliknięcia
        m = WM_LBUTTONDOWN;
        break;
    case WM_SETCURSOR:
        SetCursor(LoadCursor(nullptr, IDC_HAND));
        return TRUE;
    case WM_NCDESTROY:
        RemovePropW(h, L"hot");
        RemoveWindowSubclass(h, ButtonProc, id);
        break;
    }
    return DefSubclassProc(h, m, w, l);
}

// ------------------------------------------------------------- logika ------
static void SetStatus(int kind, const std::wstring& s) {
    g_statusKind = (decltype(g_statusKind))kind;
    g_status = s;
    InvalidateRect(g_hwnd, &rcStatus, FALSE);
}

static bool LoadTimes() {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(g_file.c_str(), GetFileExInfoStandard, &fad)) return false;
    auto conv = [](const FILETIME& ft, SYSTEMTIME& out) {
        SYSTEMTIME utc;
        FileTimeToSystemTime(&ft, &utc);
        SystemTimeToTzSpecificLocalTime(nullptr, &utc, &out);
    };
    conv(fad.ftLastWriteTime, g_mod);
    conv(fad.ftCreationTime, g_created);
    conv(fad.ftLastAccessTime, g_access);
    g_size = ((ULONGLONG)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
    return true;
}

static void RefreshFileInfo(bool loadIntoPicker) {
    g_hasTimes = !g_file.empty() && LoadTimes();
    if (!g_file.empty() && !g_hasTimes)
        SetStatus(ST_ERR, L"Nie można odczytać pliku: " + ErrText(GetLastError()));
    EnableWindow(g_update, g_hasTimes);
    if (g_hasTimes && loadIntoPicker) DateTime_SetSystemtime(g_dt, GDT_VALID, &g_mod);
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

static void SelectFile(const std::wstring& file) {
    g_file = file;
    if (g_fileIcon) DestroyIcon(g_fileIcon);
    g_fileIcon = nullptr;
    SHFILEINFOW sfi = {};
    if (SHGetFileInfoW(file.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) g_fileIcon = sfi.hIcon;
    SetStatus(ST_INFO, L"Ustaw nową datę i godzinę, a następnie kliknij „Aktualizuj”.");
    RefreshFileInfo(true);
}

static void Browse() {
    wchar_t buf[32768] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
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

static void DoUpdate() {
    if (g_file.empty()) return;
    SYSTEMTIME local = {}, utc;
    FILETIME ft;
    if (DateTime_GetSystemtime(g_dt, &local) != GDT_VALID) {
        SetStatus(ST_ERR, L"Nieprawidłowa data.");
        return;
    }
    local.wMilliseconds = 0;
    if (!TzSpecificLocalTimeToSystemTime(nullptr, &local, &utc) || !SystemTimeToFileTime(&utc, &ft)) {
        SetStatus(ST_ERR, L"Nieprawidłowa data.");
        return;
    }
    HANDLE h = CreateFileW(g_file.c_str(), FILE_WRITE_ATTRIBUTES,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        SetStatus(ST_ERR, L"Nie można otworzyć pliku: " + ErrText(GetLastError()));
        return;
    }
    BOOL ok = SetFileTime(h, nullptr, nullptr, &ft);
    DWORD e = GetLastError();
    CloseHandle(h);
    if (!ok) {
        SetStatus(ST_ERR, L"Nie można zmienić daty: " + ErrText(e));
        return;
    }
    RefreshFileInfo(false);
    SetStatus(ST_OK, L"Gotowe! Nowa data modyfikacji: " + FmtDateTime(g_mod, false));
}

// -------------------------------------------------------------- układ ------
static HFONT MkFont(int px, int weight, const wchar_t* face = L"Segoe UI") {
    return CreateFontW(-S(px), 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, face);
}

static void CreateResources() {
    HFONT* all[] = {&fTitle, &fSub, &fLabel, &fBody, &fBodyBold, &fBig, &fBtn, &fSmall};
    for (HFONT* f : all) if (*f) DeleteObject(*f);
    fTitle = MkFont(22, FW_SEMIBOLD);
    fSub = MkFont(14, FW_NORMAL);
    fLabel = MkFont(11, FW_BOLD);
    fBody = MkFont(15, FW_NORMAL);
    fBodyBold = MkFont(15, FW_SEMIBOLD);
    fBig = MkFont(28, FW_SEMIBOLD);
    fBtn = MkFont(15, FW_SEMIBOLD);
    fSmall = MkFont(13, FW_NORMAL);
    if (g_appIcon) DestroyIcon(g_appIcon);
    g_appIcon = (HICON)LoadImageW(g_inst, MAKEINTRESOURCEW(1), IMAGE_ICON, S(40), S(40), 0);
}

static void Layout() {
    int m = S(MARGIN), p = S(PAD), W = S(WIN_W);
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    if (rc.right > 0) W = rc.right;

    rcCard1 = {m, S(78), W - m, S(78 + 102)};
    int btn = S(52);
    rcZone = {rcCard1.left + p, rcCard1.top + S(36), rcCard1.right - p - btn - S(10), rcCard1.top + S(36 + 50)};
    MoveWindow(g_browse, rcZone.right + S(10), rcZone.top, btn, rcZone.bottom - rcZone.top, TRUE);

    rcCard2 = {m, rcCard1.bottom + S(12), W - m, rcCard1.bottom + S(12 + 108)};

    rcCard3 = {m, rcCard2.bottom + S(12), W - m, rcCard2.bottom + S(12 + 96)};
    int y = rcCard3.top + S(38), h = S(40);
    int bw1 = S(138), bw2 = S(120), gap = S(10);
    int x = rcCard3.left + p, xr = rcCard3.right - p;
    MoveWindow(g_update, x, y, bw1, h, TRUE);
    MoveWindow(g_now, xr - bw2, y, bw2, h, TRUE);
    MoveWindow(g_dt, x + bw1 + gap, y + S(2), xr - bw2 - gap - (x + bw1 + gap), h - S(4), TRUE);

    rcStatus = {m, rcCard3.bottom + S(14), W - m, rcCard3.bottom + S(14 + 30)};
}

static void ApplyFonts() {
    SendMessageW(g_dt, WM_SETFONT, (WPARAM)fBody, TRUE);
    SendMessageW(g_dt, DTM_SETMCFONT, (WPARAM)fBody, TRUE);
}

static UINT GetDpi(HWND hwnd) {
    typedef UINT(WINAPI * Fn)(HWND);
    static Fn fn = (Fn)(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
    if (fn) return fn(hwnd);
    HDC dc = GetDC(hwnd);
    UINT d = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(hwnd, dc);
    return d;
}

// Kolor paska tytułu dopasowany do tła (Windows 11)
static void StyleTitleBar(HWND hwnd) {
    typedef HRESULT(WINAPI * Fn)(HWND, DWORD, LPCVOID, DWORD);
    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) return;
    if (Fn fn = (Fn)(void*)GetProcAddress(dwm, "DwmSetWindowAttribute")) {
        COLORREF cap = CLR_BG, txt = CLR_TEXT;
        fn(hwnd, 35 /*DWMWA_CAPTION_COLOR*/, &cap, sizeof(cap));
        fn(hwnd, 36 /*DWMWA_TEXT_COLOR*/, &txt, sizeof(txt));
    }
    FreeLibrary(dwm);
}

static HWND MakeBtn(const wchar_t* text, int id) {
    HWND h = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                             0, 0, 0, 0, g_hwnd, (HMENU)(INT_PTR)id, g_inst, nullptr);
    SetWindowSubclass(h, ButtonProc, id, 0);
    return h;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_hwnd = hwnd;
        g_dpi = GetDpi(hwnd);
        CreateResources();
        StyleTitleBar(hwnd);
        g_browse = MakeBtn(L"Wybierz plik", IDC_BROWSE);
        g_update = MakeBtn(L"Aktualizuj", IDC_UPDATE);
        g_dt = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                               0, 0, 0, 0, hwnd, (HMENU)IDC_DATETIME, g_inst, nullptr);
        DateTime_SetFormat(g_dt, L"  dd'.'MM'.'yyyy'     'HH':'mm':'ss");
        g_now = MakeBtn(L"Aktualny", IDC_NOW);
        ApplyFonts();
        SetNow();
        EnableWindow(g_update, FALSE);
        DragAcceptFiles(hwnd, TRUE);
        Layout();

        int argc;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv) {
            if (argc > 1 && GetFileAttributesW(argv[1]) != INVALID_FILE_ATTRIBUTES) SelectFile(argv[1]);
            LocalFree(argv);
        }
        return 0;
    }
    case WM_SIZE:
        Layout();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_DPICHANGED: {
        g_dpi = HIWORD(wp);
        CreateResources();
        ApplyFonts();
        RECT* r = (RECT*)lp;
        SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        Layout();
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        Paint(dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DRAWITEM:
        DrawButton((DRAWITEMSTRUCT*)lp);
        return TRUE;
    case WM_MOUSEMOVE: {
        POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        bool hot = PtInRect(&rcZone, pt) != 0;
        if (hot != g_zoneHover) {
            g_zoneHover = hot;
            InvalidateRect(hwnd, &rcZone, FALSE);
        }
        TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&t);
        return 0;
    }
    case WM_MOUSELEAVE:
        if (g_zoneHover) {
            g_zoneHover = false;
            InvalidateRect(hwnd, &rcZone, FALSE);
        }
        return 0;
    case WM_LBUTTONUP: {
        POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        if (PtInRect(&rcZone, pt)) Browse();
        return 0;
    }
    case WM_SETCURSOR:
        if ((HWND)wp == hwnd && LOWORD(lp) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            SetCursor(LoadCursor(nullptr, PtInRect(&rcZone, pt) ? IDC_HAND : IDC_ARROW));
            return TRUE;
        }
        break;
    case WM_DROPFILES: {
        HDROP drop = (HDROP)wp;
        wchar_t buf[32768];
        if (DragQueryFileW(drop, 0, buf, ARRAYSIZE(buf))) SelectFile(buf);
        DragFinish(drop);
        return 0;
    }
    case WM_COMMAND:
        if (HIWORD(wp) == BN_CLICKED || HIWORD(wp) == BN_DOUBLECLICKED) {
            switch (LOWORD(wp)) {
            case IDC_BROWSE: Browse(); return 0;
            case IDC_UPDATE: DoUpdate(); return 0;
            case IDC_NOW: SetNow(); return 0;
            }
        }
        break;
    case WM_ACTIVATE:
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
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    GdiplusStartupInput gsi;
    ULONG_PTR gdipToken;
    GdiplusStartup(&gdipToken, &gsi, nullptr);
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_DATE_CLASSES | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ModDateWnd";
    wc.hIcon = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                                 GetSystemMetrics(SM_CYICON), 0);
    wc.hIconSm = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                   GetSystemMetrics(SM_CYSMICON), 0);
    RegisterClassExW(&wc);

    HDC sdc = GetDC(nullptr);
    g_dpi = GetDeviceCaps(sdc, LOGPIXELSX);
    ReleaseDC(nullptr, sdc);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    RECT wr = {0, 0, S(WIN_W), S(WIN_H)};
    AdjustWindowRectEx(&wr, style, FALSE, 0);
    int ww = wr.right - wr.left, wh = wr.bottom - wr.top;
    RECT work;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"ModDate", style,
                                work.left + (work.right - work.left - ww) / 2,
                                work.top + (work.bottom - work.top - wh) / 2, ww, wh,
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
    GdiplusShutdown(gdipToken);
    CoUninitialize();
    return (int)m.wParam;
}
