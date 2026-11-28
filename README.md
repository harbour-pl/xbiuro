# xBiuro

xBiuro to otwartoźródłowy program do rozliczeń podatkowych dla pojedynczych firm i biur rachunkowych,
napisany w C na bibliotece [XWGUI](https://github.com/harbour-pl/xwgui). Baza danych to własna implementacja tabel DBF, co zapewnia wygodniejszy dostęp do danych.

## Moduły główne

| Stan | Nazwa modułu         | Skrót | Zakres                                  |
|------|----------------------|-------|-----------------------------------------|
| ✅   | Faktury              | FV    | Fakturowanie                            |
| ⬜   | Księgi rachunkowe    | KR    | Pełna księgowość                        |
| ⬜   | PKPiR                | PKPiR | Podatkowa księga przychodów i rozchodów |
| ⬜   | Ewidencja przychodów | EP    | Ryczałt od przychodów ewidencjonowanych |
| ⬜   | Wynagrodzenia        | WY    | Naliczanie wynagrodzeń                  |

## Platformy

- [x] Windows
- [ ] Linux
- [ ] macOS

## Budowanie

W MSYS2 clang64, z [XWGUI](https://github.com/harbour-pl/xwgui) w katalogu obok (`../xwgui`) i z libharu
(`pacman -S mingw-w64-clang-x86_64-libharu`):

```sh
make
```

## Licencja

GPL-3.0, zobacz `LICENSE`.
