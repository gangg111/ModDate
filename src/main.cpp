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

// Paleta – ciemny motyw (styl Windows 11)
static const COLORREF CLR_BG          = RGB(32, 32, 32);
static const COLORREF CLR_CARD        = RGB(43, 43, 43);
static const COLORREF CLR_CARD_BORDER = RGB(56, 56, 56);
static const COLORREF CLR_TEXT        = RGB(255, 255, 255);
static const COLORREF CLR_TEXT2       = RGB(176, 176, 176);
static const COLORREF CLR_LABEL       = RGB(150, 150, 150);
static const COLORREF CLR_ACCENT      = RGB(96, 205, 255);   // tekst / ikony akcentu
static const COLORREF CLR_BTN         = RGB(76, 194, 255);   // przycisk główny
static const COLORREF CLR_BTN_HOT     = RGB(71, 177, 232);
static const COLORREF CLR_BTN_DOWN    = RGB(66, 161, 210);
static const COLORREF CLR_BTN_TEXT    = RGB(0, 0, 0);
static const COLORREF CLR_SEC         = RGB(55, 55, 55);     // przycisk drugorzędny
static const COLORREF CLR_SEC_HOT     = RGB(61, 61, 61);
static const COLORREF CLR_SEC_DOWN    = RGB(50, 50, 50);
static const COLORREF CLR_DIS_BG      = RGB(58, 58, 58);
static const COLORREF CLR_DIS_TEXT    = RGB(120, 120, 120);
static const COLORREF CLR_ACCENT_SOFT = RGB(42, 56, 70);
static const COLORREF CLR_ACCENT_SOFT2= RGB(50, 68, 88);
static const COLORREF CLR_FIELD       = RGB(50, 50, 50);
static const COLORREF CLR_FIELD_HOT   = RGB(56, 56, 56);
static const COLORREF CLR_FIELD_FOCUS = RGB(31, 31, 31);
static const COLORREF CLR_FIELD_BORDER= RGB(72, 72, 72);
static const COLORREF CLR_OK          = RGB(108, 203, 95);
static const COLORREF CLR_ERR         = RGB(255, 153, 164);
static const COLORREF CLR_INFO        = RGB(197, 197, 197);

// ---------------------------------------------------------------- stan -----
static HINSTANCE g_inst;
static HWND g_hwnd, g_browse, g_update, g_dt, g_now;
static int g_dpi = 96;
static HFONT fTitle, fSub, fLabel, fBody, fBodyBold, fBig, fBtn, fSmall, fField;
static HICON g_appIcon, g_fileIcon;
static std::wstring g_file;
static bool g_hasTimes;
static SYSTEMTIME g_mod, g_created, g_access;
static ULONGLONG g_size;
static enum { ST_INFO, ST_OK, ST_ERR } g_statusKind = ST_INFO;
static std::wstring g_status = L"Wybierz plik, aby rozpocząć.";
static bool g_zoneHover, g_zonePress;
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
    SolidBrush light(Color(60, 0, 0, 0));
    g.FillRectangle(&light, x + (REAL)S(2), y + h * 0.2f + (REAL)S(3), w - (REAL)S(4), (REAL)S(1));
}

// Zastępcza ikona dokumentu (gdy system nie zwróci ikony pliku)
static void DrawDocGlyph(HDC dc, RECT rc) {
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    RECT page = {rc.left + w / 6, rc.top + h / 16, rc.right - w / 6, rc.bottom - h / 16};
    FillRR(dc, page, S(3), RGB(62, 62, 62), RGB(110, 110, 110), (REAL)S(1));
    for (int i = 0; i < 3; ++i) {
        RECT ln = {page.left + S(5), page.top + S(8) + i * S(6), page.right - S(5), page.top + S(10) + i * S(6)};
        FillRR(dc, ln, S(1), RGB(90, 130, 160));
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
        Text(dc, L"— —", big, fBig, RGB(85, 85, 85), DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        Text(dc, L"Nie wybrano pliku", sub, fSmall, CLR_TEXT2, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    }

    // --- karta 3: nowa data (kontrolki są oknami potomnymi)
    DrawCard(dc, rcCard3, L"NOWA DATA I GODZINA");

    // --- pasek statusu (sam kolorowy tekst, bez tła)
    if (!g_status.empty()) {
        COLORREF fg = g_statusKind == ST_OK ? CLR_OK : g_statusKind == ST_ERR ? CLR_ERR : CLR_INFO;
        int cy = (rcStatus.top + rcStatus.bottom) / 2;
        FillCircle(dc, rcStatus.left + S(6), cy, S(4), fg);
        RECT t = {rcStatus.left + S(18), rcStatus.top, rcStatus.right, rcStatus.bottom};
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
        COLORREF fill = dis ? CLR_DIS_BG : down ? CLR_BTN_DOWN : hot ? CLR_BTN_HOT : CLR_BTN;
        FillRR(dc, lr, rad, fill);
        if (focus) {
            RECT fr = {S(2), S(2), w - S(2), h - S(2)};
            FillRR(dc, fr, rad - S(1), fill, CLR_CARD, (REAL)S(1));
        }
        Text(dc, L"Aktualizuj", lr, fBtn, dis ? CLR_DIS_TEXT : CLR_BTN_TEXT,
             DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    } else if (d->CtlID == IDC_BROWSE) {  // przycisk z ikoną folderu
        COLORREF fill = down ? CLR_ACCENT_SOFT2 : hot ? CLR_ACCENT_SOFT2 : CLR_ACCENT_SOFT;
        FillRR(dc, lr, rad, fill, focus ? CLR_ACCENT : (hot ? RGB(70, 100, 130) : CLR_NONE),
               (REAL)(focus ? S(2) : 1));
        DrawFolderGlyph(dc, lr, down ? CLR_BTN_DOWN : CLR_ACCENT);
    } else {  // przycisk drugorzędny
        COLORREF fill = down ? CLR_SEC_DOWN : hot ? CLR_SEC_HOT : CLR_SEC;
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

// ------------------------------------------------ pole daty i godziny ------
// Własna kontrolka (ciemna), segmenty: 0 dzień, 1 miesiąc, 2 rok, 3 godz., 4 min., 5 sek.
static SYSTEMTIME g_val;
static int g_seg = 0;
static std::wstring g_typed;
static bool g_fieldHot, g_calHot;
static RECT g_segRc[6], g_calRc;
static HWND g_cal;
static const UINT WM_APP_CLOSECAL = WM_APP + 1;

static int DaysIn(int y, int m) {
    static const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
    return d[(m + 11) % 12];
}
static WORD* SegPtr(int s) {
    switch (s) {
    case 0: return &g_val.wDay;
    case 1: return &g_val.wMonth;
    case 2: return &g_val.wYear;
    case 3: return &g_val.wHour;
    case 4: return &g_val.wMinute;
    default: return &g_val.wSecond;
    }
}
static void SegRange(int s, int& lo, int& hi) {
    switch (s) {
    case 0: lo = 1; hi = DaysIn(g_val.wYear, g_val.wMonth); break;
    case 1: lo = 1; hi = 12; break;
    case 2: lo = 1601; hi = 9999; break;
    case 3: lo = 0; hi = 23; break;
    default: lo = 0; hi = 59;
    }
}
static void FixDay() {
    int dm = DaysIn(g_val.wYear, g_val.wMonth);
    if (g_val.wDay > dm) g_val.wDay = (WORD)dm;
    if (g_val.wDay < 1) g_val.wDay = 1;
}
static void FieldChanged() {
    g_val.wMilliseconds = 0;
    InvalidateRect(g_dt, nullptr, FALSE);
}
static void FieldSet(const SYSTEMTIME& st) {
    g_val = st;
    g_typed.clear();
    FixDay();
    FieldChanged();
}
static void CommitTyped() {
    if (g_typed.empty()) return;
    int v = _wtoi(g_typed.c_str()), lo, hi;
    g_typed.clear();
    SegRange(g_seg, lo, hi);
    if (!(g_seg == 2 && v < lo)) {  // niepełny rok – zostaw poprzedni
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        *SegPtr(g_seg) = (WORD)v;
        FixDay();
    }
    FieldChanged();
}
static void SelectSeg(int s) {
    CommitTyped();
    g_seg = s < 0 ? 0 : s > 5 ? 5 : s;
    FieldChanged();
}
static void Step(int d) {
    CommitTyped();
    int lo, hi;
    SegRange(g_seg, lo, hi);
    int v = *SegPtr(g_seg) + d;
    if (g_seg == 2) v = v < lo ? lo : v > hi ? hi : v;
    else {
        int n = hi - lo + 1;
        v = lo + ((v - lo) % n + n) % n;
    }
    *SegPtr(g_seg) = (WORD)v;
    FixDay();
    FieldChanged();
}
static void TypeDigit(wchar_t c) {
    g_typed += c;
    int v = _wtoi(g_typed.c_str()), lo, hi;
    SegRange(g_seg, lo, hi);
    size_t maxLen = g_seg == 2 ? 4 : 2;
    if (g_typed.size() >= maxLen || (g_seg != 2 && v * 10 > hi)) {
        CommitTyped();
        if (g_seg < 5) g_seg++;
    }
    FieldChanged();
}

static void DrawCalGlyph(HDC dc, RECT rc, COLORREF c) {
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    REAL w = (REAL)S(16), h = (REAL)S(15);
    REAL x = (rc.left + rc.right - w) / 2, y = (rc.top + rc.bottom - h) / 2;
    GraphicsPath p;
    RoundPath(p, RectF(x, y, w, h), (REAL)S(2));
    Pen pen(GC(c), (REAL)S(1) * 1.3f);
    g.DrawPath(&pen, &p);
    SolidBrush b(GC(c));
    g.FillRectangle(&b, x, y, w, h * 0.27f);
    REAL ds = (REAL)S(2) + 0.5f;
    for (int r = 0; r < 2; ++r)
        for (int k = 0; k < 3; ++k)
            g.FillRectangle(&b, x + w * (0.2f + k * 0.25f), y + h * (0.45f + r * 0.25f), ds, ds);
}

static void PaintField(HWND h, HDC target) {
    RECT rc;
    GetClientRect(h, &rc);
    HDC dc = CreateCompatibleDC(target);
    HBITMAP bmp = CreateCompatibleBitmap(target, rc.right, rc.bottom);
    HGDIOBJ ob = SelectObject(dc, bmp);
    HBRUSH bg = CreateSolidBrush(CLR_CARD);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    bool focus = GetFocus() == h;
    FillRR(dc, rc, S(6), focus ? CLR_FIELD_FOCUS : g_fieldHot ? CLR_FIELD_HOT : CLR_FIELD,
           focus ? CLR_ACCENT : CLR_FIELD_BORDER);

    g_calRc = {rc.right - S(40), S(5), rc.right - S(5), rc.bottom - S(5)};
    if (g_calHot || g_cal) FillRR(dc, g_calRc, S(4), RGB(70, 70, 70));
    DrawCalGlyph(dc, g_calRc, g_cal ? CLR_ACCENT : CLR_TEXT2);

    static const wchar_t* sep[] = {L".", L".", L"     ", L":", L":"};
    int x = S(14);
    for (int i = 0; i < 6; ++i) {
        wchar_t b[8];
        if (focus && i == g_seg && !g_typed.empty()) lstrcpynW(b, g_typed.c_str(), 8);
        else wsprintfW(b, i == 2 ? L"%04d" : L"%02d", *SegPtr(i));
        int w = TextWidth(dc, i == 2 ? L"0000" : L"00", fField);
        RECT sr = {x - S(3), rc.top + S(7), x + w + S(3), rc.bottom - S(7)};
        bool sel = focus && i == g_seg;
        if (sel) FillRR(dc, sr, S(4), CLR_BTN);
        Text(dc, b, sr, fField, sel ? CLR_BTN_TEXT : CLR_TEXT, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        g_segRc[i] = sr;
        x += w;
        if (i < 5) {
            int sw = TextWidth(dc, sep[i], fField);
            RECT tr = {x, rc.top, x + sw + S(2), rc.bottom};
            Text(dc, sep[i], tr, fField, CLR_TEXT2, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            x += sw + S(2);
        }
    }
    BitBlt(target, 0, 0, rc.right, rc.bottom, dc, 0, 0, SRCCOPY);
    SelectObject(dc, ob);
    DeleteObject(bmp);
    DeleteDC(dc);
}

static void CloseCalendar(bool refocus) {
    if (!g_cal) return;
    HWND c = g_cal;
    g_cal = nullptr;
    DestroyWindow(c);
    if (refocus) SetFocus(g_dt);
    InvalidateRect(g_dt, nullptr, FALSE);
}

// ------------------------------------------------- kalendarz (po polsku) ---
static const wchar_t* MONTHS_NOM[] = {L"Styczeń", L"Luty", L"Marzec", L"Kwiecień", L"Maj", L"Czerwiec",
                                      L"Lipiec", L"Sierpień", L"Wrzesień", L"Październik", L"Listopad", L"Grudzień"};
static const wchar_t* WD_SHORT[] = {L"Pn", L"Wt", L"Śr", L"Cz", L"Pt", L"So", L"Nd"};
static const int CAL_PAD = 12, CELL = 36, CAL_HEAD = 48, CAL_WD = 26;
static const int CAL_W = 2 * CAL_PAD + 7 * CELL;
static const int CAL_GRID = CAL_HEAD + CAL_WD + 4;
static const int CAL_H = CAL_GRID + 6 * CELL + 6 + 34 + CAL_PAD;
enum { HIT_NONE = -1, HIT_PREV = 100, HIT_NEXT, HIT_TODAY };
static int g_cvY, g_cvM;          // widoczny miesiąc
static SYSTEMTIME g_csel;         // zaznaczony dzień
static int g_cHover = HIT_NONE, g_cPress = HIT_NONE;

static int WeekdayMon0(int y, int m, int d) {  // 0 = poniedziałek
    SYSTEMTIME st = {};
    st.wYear = (WORD)y; st.wMonth = (WORD)m; st.wDay = (WORD)d;
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    FileTimeToSystemTime(&ft, &st);
    return (st.wDayOfWeek + 6) % 7;
}
static void CellDate(int i, int& y, int& m, int& d) {
    y = g_cvY; m = g_cvM;
    d = i - WeekdayMon0(g_cvY, g_cvM, 1) + 1;
    if (d < 1) {
        if (--m < 1) { m = 12; --y; }
        d += DaysIn(y, m);
    } else if (d > DaysIn(y, m)) {
        d -= DaysIn(y, m);
        if (++m > 12) { m = 1; ++y; }
    }
}
static bool ValidYM(int y, int m) { return (y > 1601 || (y == 1601 && m >= 1)) && y <= 9999 && m >= 1 && m <= 12; }
static void ShiftMonth(int d) {
    int y = g_cvY, m = g_cvM + d;
    while (m < 1) { m += 12; --y; }
    while (m > 12) { m -= 12; ++y; }
    if (ValidYM(y, m)) { g_cvY = y; g_cvM = m; }
}
static void SelAddDays(int days) {
    FILETIME ft;
    SystemTimeToFileTime(&g_csel, &ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime; u.HighPart = ft.dwHighDateTime;
    u.QuadPart += (LONGLONG)days * 864000000000LL;
    ft.dwLowDateTime = u.LowPart; ft.dwHighDateTime = u.HighPart;
    SYSTEMTIME n;
    if (FileTimeToSystemTime(&ft, &n) && n.wYear >= 1601 && n.wYear <= 9999) g_csel = n;
    g_cvY = g_csel.wYear; g_cvM = g_csel.wMonth;
}
static void CalCommit(int y, int m, int d) {
    SYSTEMTIME v = g_val;
    v.wYear = (WORD)y; v.wMonth = (WORD)m; v.wDay = (WORD)d;
    FieldSet(v);
    PostMessageW(g_hwnd, WM_APP_CLOSECAL, 1, 0);
}
static RECT CalBtnRect(int which) {
    if (which == HIT_PREV) return {S(CAL_PAD), S(CAL_PAD), S(CAL_PAD + 32), S(CAL_PAD + 32)};
    if (which == HIT_NEXT) return {S(CAL_W - CAL_PAD - 32), S(CAL_PAD), S(CAL_W - CAL_PAD), S(CAL_PAD + 32)};
    int y = CAL_GRID + 6 * CELL + 6;
    return {S(CAL_W / 2 - 50), S(y), S(CAL_W / 2 + 50), S(y + 32)};
}
static RECT CellRect(int i) {
    int x = CAL_PAD + (i % 7) * CELL, y = CAL_GRID + (i / 7) * CELL;
    return {S(x), S(y), S(x + CELL), S(y + CELL)};
}
static int CalHit(POINT pt) {
    for (int w : {HIT_PREV, HIT_NEXT, HIT_TODAY}) {
        RECT r = CalBtnRect(w);
        if (PtInRect(&r, pt)) return w;
    }
    for (int i = 0; i < 42; ++i) {
        RECT r = CellRect(i);
        if (PtInRect(&r, pt)) return i;
    }
    return HIT_NONE;
}

static void DrawChevron(HDC dc, RECT rc, bool left, COLORREF c) {
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    REAL cx = (rc.left + rc.right) / 2.0f, cy = (rc.top + rc.bottom) / 2.0f, k = (REAL)S(4);
    Pen pen(GC(c), (REAL)S(1) * 1.6f);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    PointF pts[3] = {{cx + (left ? k / 2 : -k / 2), cy - k}, {cx + (left ? -k / 2 : k / 2), cy},
                     {cx + (left ? k / 2 : -k / 2), cy + k}};
    g.DrawLines(&pen, pts, 3);
}

static void PaintCal(HWND h, HDC target) {
    RECT rc;
    GetClientRect(h, &rc);
    HDC dc = CreateCompatibleDC(target);
    HBITMAP bmp = CreateCompatibleBitmap(target, rc.right, rc.bottom);
    HGDIOBJ ob = SelectObject(dc, bmp);
    HBRUSH bg = CreateSolidBrush(CLR_CARD);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);
    HBRUSH br = CreateSolidBrush(CLR_FIELD_BORDER);
    FrameRect(dc, &rc, br);
    DeleteObject(br);

    // nagłówek: miesiąc i rok, strzałki
    for (int w : {HIT_PREV, HIT_NEXT}) {
        RECT r = CalBtnRect(w);
        if (g_cHover == w) FillRR(dc, r, S(5), RGB(62, 62, 62));
        DrawChevron(dc, r, w == HIT_PREV, CLR_TEXT);
    }
    wchar_t title[48];
    wsprintfW(title, L"%s %d", MONTHS_NOM[g_cvM - 1], g_cvY);
    RECT tr = {S(CAL_PAD + 36), S(CAL_PAD), S(CAL_W - CAL_PAD - 36), S(CAL_PAD + 32)};
    Text(dc, title, tr, fBodyBold, CLR_TEXT, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    // dni tygodnia
    for (int i = 0; i < 7; ++i) {
        RECT r = {S(CAL_PAD + i * CELL), S(CAL_HEAD), S(CAL_PAD + (i + 1) * CELL), S(CAL_HEAD + CAL_WD)};
        Text(dc, WD_SHORT[i], r, fSmall, i >= 5 ? RGB(130, 130, 130) : CLR_LABEL, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }

    // siatka dni
    SYSTEMTIME today;
    GetLocalTime(&today);
    for (int i = 0; i < 42; ++i) {
        int y, m, d;
        CellDate(i, y, m, d);
        RECT r = CellRect(i);
        int cx = (r.left + r.right) / 2, cy = (r.top + r.bottom) / 2, rad = S(CELL / 2 - 2);
        bool inMonth = m == g_cvM;
        bool sel = y == g_csel.wYear && m == g_csel.wMonth && d == g_csel.wDay;
        bool isToday = y == today.wYear && m == today.wMonth && d == today.wDay;
        if (sel) FillCircle(dc, cx, cy, rad, g_cHover == i ? CLR_BTN_HOT : CLR_BTN);
        else {
            if (isToday) {
                FillCircle(dc, cx, cy, rad, CLR_ACCENT);
                FillCircle(dc, cx, cy, rad - S(1) - 1, g_cHover == i ? RGB(62, 62, 62) : CLR_CARD);
            } else if (g_cHover == i) FillCircle(dc, cx, cy, rad, RGB(62, 62, 62));
        }
        wchar_t num[4];
        wsprintfW(num, L"%d", d);
        COLORREF c = sel ? CLR_BTN_TEXT : isToday ? CLR_ACCENT : inMonth ? CLR_TEXT : RGB(100, 100, 100);
        Text(dc, num, r, sel ? fBodyBold : fBody, c, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }

    // przycisk "Dziś"
    RECT tb = CalBtnRect(HIT_TODAY);
    if (g_cHover == HIT_TODAY) FillRR(dc, tb, S(5), RGB(62, 62, 62));
    Text(dc, L"Dziś", tb, fBtn, CLR_ACCENT, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    BitBlt(target, 0, 0, rc.right, rc.bottom, dc, 0, 0, SRCCOPY);
    SelectObject(dc, ob);
    DeleteObject(bmp);
    DeleteDC(dc);
}

static LRESULT CALLBACK CalWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        PaintCal(h, dc);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_MOUSEACTIVATE:
        return MA_ACTIVATE;
    case WM_MOUSEMOVE: {
        POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
        int hit = CalHit(pt);
        if (hit != g_cHover) {
            g_cHover = hit;
            InvalidateRect(h, nullptr, FALSE);
        }
        TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, h, 0};
        TrackMouseEvent(&t);
        return 0;
    }
    case WM_MOUSELEAVE:
        g_cHover = HIT_NONE;
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    case WM_SETCURSOR:
        SetCursor(LoadCursor(nullptr, g_cHover != HIT_NONE ? IDC_HAND : IDC_ARROW));
        return TRUE;
    case WM_LBUTTONDOWN: {
        POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
        int hit = CalHit(pt);
        if (hit == HIT_PREV || hit == HIT_NEXT) {
            ShiftMonth(hit == HIT_PREV ? -1 : 1);
            InvalidateRect(h, nullptr, FALSE);
        }
        g_cPress = hit;
        return 0;
    }
    case WM_LBUTTONUP: {  // wybór dnia po puszczeniu przycisku (żeby kliknięcie nie "przeszło" do okna pod spodem)
        POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
        int hit = CalHit(pt);
        if (hit != g_cPress) return 0;
        g_cPress = HIT_NONE;
        if (hit == HIT_TODAY) {
            SYSTEMTIME t;
            GetLocalTime(&t);
            CalCommit(t.wYear, t.wMonth, t.wDay);
        } else if (hit >= 0 && hit < 42) {
            int y, mo, d;
            CellDate(hit, y, mo, d);
            if (y >= 1601 && y <= 9999) CalCommit(y, mo, d);
        }
        return 0;
    }
    case WM_MOUSEWHEEL:
        ShiftMonth(GET_WHEEL_DELTA_WPARAM(w) > 0 ? -1 : 1);
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    case WM_KEYDOWN:
        switch (w) {
        case VK_LEFT: SelAddDays(-1); break;
        case VK_RIGHT: SelAddDays(1); break;
        case VK_UP: SelAddDays(-7); break;
        case VK_DOWN: SelAddDays(7); break;
        case VK_PRIOR: ShiftMonth(-1); break;
        case VK_NEXT: ShiftMonth(1); break;
        case VK_RETURN: case VK_SPACE: CalCommit(g_csel.wYear, g_csel.wMonth, g_csel.wDay); return 0;
        case VK_ESCAPE: case VK_F4: PostMessageW(g_hwnd, WM_APP_CLOSECAL, 1, 0); return 0;
        }
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    case WM_KILLFOCUS:
        PostMessageW(g_hwnd, WM_APP_CLOSECAL, 0, 0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static void ToggleCalendar() {
    if (g_cal) { CloseCalendar(true); return; }
    CommitTyped();
    g_csel = g_val;
    g_cvY = g_val.wYear;
    g_cvM = g_val.wMonth;
    g_cHover = g_cPress = HIT_NONE;
    int cw = S(CAL_W), ch = S(CAL_H);
    RECT fr;
    GetWindowRect(g_dt, &fr);
    int x = fr.right - cw, y = fr.bottom + S(4);
    MONITORINFO mi = {sizeof(mi)};
    if (GetMonitorInfoW(MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST), &mi)) {
        if (y + ch > mi.rcWork.bottom) y = fr.top - S(4) - ch;
        if (x < mi.rcWork.left) x = mi.rcWork.left;
    }
    g_cal = CreateWindowExW(WS_EX_TOOLWINDOW, L"ModDateCal", L"", WS_POPUP, 0, 0, 0, 0,
                            g_hwnd, nullptr, g_inst, nullptr);
    if (!g_cal) return;
    typedef HRESULT(WINAPI * Fn)(HWND, DWORD, LPCVOID, DWORD);
    if (HMODULE dwm = LoadLibraryW(L"dwmapi.dll")) {
        if (Fn fn = (Fn)(void*)GetProcAddress(dwm, "DwmSetWindowAttribute")) {
            int round = 3;  // DWMWCP_ROUNDSMALL – zaokrąglone rogi w Windows 11
            fn(g_cal, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &round, sizeof(round));
        }
        FreeLibrary(dwm);
    }
    SetWindowPos(g_cal, HWND_TOP, x, y, cw, ch, SWP_SHOWWINDOW);
    SetFocus(g_cal);
    InvalidateRect(g_dt, nullptr, FALSE);
}

static LRESULT CALLBACK FieldProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        PaintField(h, dc);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_SETFOCUS:
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    case WM_KILLFOCUS:
        CommitTyped();
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN: {
        POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
        bool wasOpen = g_cal != nullptr;
        SetFocus(h);
        if (PtInRect(&g_calRc, pt)) {
            if (!wasOpen) ToggleCalendar();
            return 0;
        }
        int s = 5;
        for (int i = 0; i < 6; ++i)
            if (pt.x < g_segRc[i].right + S(3)) { s = i; break; }
        SelectSeg(s);
        return 0;
    }
    case WM_MOUSEMOVE: {
        POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
        bool ch = PtInRect(&g_calRc, pt) != 0;
        if (!g_fieldHot || ch != g_calHot) {
            g_fieldHot = true;
            g_calHot = ch;
            TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, h, 0};
            TrackMouseEvent(&t);
            InvalidateRect(h, nullptr, FALSE);
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        g_fieldHot = g_calHot = false;
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    case WM_SETCURSOR: {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(h, &pt);
        SetCursor(LoadCursor(nullptr, PtInRect(&g_calRc, pt) ? IDC_HAND : IDC_ARROW));
        return TRUE;
    }
    case WM_MOUSEWHEEL:
        Step(GET_WHEEL_DELTA_WPARAM(w) > 0 ? 1 : -1);
        return 0;
    case WM_SYSKEYDOWN:
        if (w == VK_DOWN) { ToggleCalendar(); return 0; }
        break;
    case WM_KEYDOWN:
        switch (w) {
        case VK_LEFT: SelectSeg(g_seg - 1); break;
        case VK_RIGHT: SelectSeg(g_seg + 1); break;
        case VK_UP: Step(1); break;
        case VK_DOWN: Step(-1); break;
        case VK_HOME: SelectSeg(0); break;
        case VK_END: SelectSeg(5); break;
        case VK_F4: ToggleCalendar(); break;
        case VK_BACK:
            if (!g_typed.empty()) { g_typed.pop_back(); FieldChanged(); }
            else SelectSeg(g_seg - 1);
            break;
        }
        return 0;
    case WM_CHAR:
        if (w >= L'0' && w <= L'9') TypeDigit((wchar_t)w);
        else if (wcschr(L".:/-, ", (wchar_t)w)) SelectSeg(g_seg + 1);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
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
    if (g_hasTimes && loadIntoPicker) FieldSet(g_mod);
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
    FieldSet(st);
}

static void DoUpdate() {
    if (g_file.empty()) return;
    SYSTEMTIME local = {}, utc;
    FILETIME ft;
    CommitTyped();
    local = g_val;
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
    HFONT* all[] = {&fTitle, &fSub, &fLabel, &fBody, &fBodyBold, &fBig, &fBtn, &fSmall, &fField};
    for (HFONT* f : all) if (*f) DeleteObject(*f);
    fTitle = MkFont(22, FW_SEMIBOLD);
    fSub = MkFont(14, FW_NORMAL);
    fLabel = MkFont(11, FW_BOLD);
    fBody = MkFont(15, FW_NORMAL);
    fBodyBold = MkFont(15, FW_SEMIBOLD);
    fBig = MkFont(28, FW_SEMIBOLD);
    fBtn = MkFont(15, FW_SEMIBOLD);
    fSmall = MkFont(13, FW_NORMAL);
    fField = MkFont(16, FW_NORMAL);
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
    MoveWindow(g_dt, x + bw1 + gap, y, xr - bw2 - gap - (x + bw1 + gap), h, TRUE);

    rcStatus = {m, rcCard3.bottom + S(14), W - m, rcCard3.bottom + S(14 + 30)};
}

static void ApplyFonts() { InvalidateRect(g_dt, nullptr, FALSE); }

static UINT GetDpi(HWND hwnd) {
    typedef UINT(WINAPI * Fn)(HWND);
    static Fn fn = (Fn)(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
    if (fn) return fn(hwnd);
    HDC dc = GetDC(hwnd);
    UINT d = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(hwnd, dc);
    return d;
}

// Ciemny pasek tytułu (Windows 10/11) w kolorze tła
static void StyleTitleBar(HWND hwnd) {
    typedef HRESULT(WINAPI * Fn)(HWND, DWORD, LPCVOID, DWORD);
    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) return;
    if (Fn fn = (Fn)(void*)GetProcAddress(dwm, "DwmSetWindowAttribute")) {
        BOOL dark = TRUE;
        if (FAILED(fn(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark))))
            fn(hwnd, 19, &dark, sizeof(dark));  // starsze wersje Windows 10
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
        g_dt = CreateWindowExW(0, L"ModDateField", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                               0, 0, 0, 0, hwnd, (HMENU)IDC_DATETIME, g_inst, nullptr);
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
    case WM_APP_CLOSECAL:
        CloseCalendar(wp != 0);
        return 0;
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
    case WM_LBUTTONDOWN: {
        POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        g_zonePress = PtInRect(&rcZone, pt) != 0;
        return 0;
    }
    case WM_LBUTTONUP: {
        POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        bool press = g_zonePress;
        g_zonePress = false;
        if (press && PtInRect(&rcZone, pt)) Browse();
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

    WNDCLASSEXW fc = {};
    fc.cbSize = sizeof(fc);
    fc.lpfnWndProc = FieldProc;
    fc.hInstance = inst;
    fc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    fc.lpszClassName = L"ModDateField";
    RegisterClassExW(&fc);

    WNDCLASSEXW cc = {};
    cc.cbSize = sizeof(cc);
    cc.style = CS_DROPSHADOW;
    cc.lpfnWndProc = CalWndProc;
    cc.hInstance = inst;
    cc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    cc.lpszClassName = L"ModDateCal";
    RegisterClassExW(&cc);

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
