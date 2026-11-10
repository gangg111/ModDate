# ModDate

Mała aplikacja okienkowa dla Windows (C++ / WinAPI) do zmiany daty i godziny modyfikacji pliku.

![ModDate](docs/screenshot.png)

**Gotowy plik:** [`release/ModDate.exe`](release/ModDate.exe), jeden plik bez instalacji i bez dodatkowych DLL.

## Obsługa

1. Kliknij pole pliku lub przycisk z ikoną folderu i wybierz plik. Możesz też przeciągnąć plik do okna albo na ikonę `ModDate.exe`.
2. W karcie **Data modyfikacji pliku** widać obecną datę modyfikacji (oraz datę utworzenia i ostatniego dostępu).
3. W polu obok przycisku **Aktualizuj** wpisz nową datę i godzinę (`dd.MM.rrrr  GG:mm:ss`) albo wybierz ją z kalendarza (strzałka).
   Przycisk **Aktualny** wstawia do pola bieżącą datę i godzinę.
4. Kliknij **Aktualizuj**, a data modyfikacji pliku zostanie zmieniona.

Data jest podawana w czasie lokalnym (z uwzględnieniem czasu letniego i zimowego).

## Kompilacja

- **Visual Studio:** uruchom `build.bat` w *Developer Command Prompt*.
- **MinGW-w64 (Linux/MSYS2):** `./build.sh`
- **GitHub Actions:** każdy push buduje `ModDate.exe`, który można pobrać z zakładki *Actions* jako artefakt.
