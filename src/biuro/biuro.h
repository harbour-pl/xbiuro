/*
 * Program...: xBiuro
 * Plik......: src/biuro/biuro.h
 *
 */

#ifndef BIURO_H
#define BIURO_H

#include "xwgui.h"
#include "xrdd.h"

#include <hpdf.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Stałe katalogi programu */
#define DANE_BAZA "dane-baza/"
#define PROG_BIBL "prog-bibl/"
#define PROG_ZASO "prog-zaso/"
#define PROG_MODU "prog-modu/"

/* Akty prawne w prog-zaso/ustawy/ dla pomocy F1 (zasada 16); nowy tekst jednolity zmienia się tylko tutaj */
#define USTAWA_PIT     PROG_ZASO "ustawy/DU-2026-592_pit-tekst-jednolity-2026.pdf"
#define USTAWA_RYCZALT PROG_ZASO "ustawy/DU-2025-843_ryczalt-tekst-jednolity-2025.pdf"
#define USTAWA_NIP     PROG_ZASO "ustawy/DU-2026-151_nip-ewidencja-tekst-jednolity-2026.pdf"
#define WZORY_NIP      PROG_ZASO "ustawy/DU-2024-1367_wzory-zgloszen-nip-tekst-jednolity.pdf"
#define WZOR_PRZELEWU  PROG_ZASO "ustawy/DU-2026-537_wzor-wplaty-i-przelewu-na-rachunek-organu-podatkowego.pdf"
#define USTAWA_VAT     PROG_ZASO "ustawy/DU-2026-1263_vat-tekst-jednolity-2026-09.pdf"

/* Pierwsze wiersze pomocy F1 firmy i kontrahenta: akty, z których są pola i układ okna podmiotu (podmiot.c) */
#define POMOC_PODMIOT "<a href=\"" WZORY_NIP "\">Rozporządzenie w sprawie wzorów zgłoszeń identyfikacyjnych i aktualizacyjnych</a> (załącznik nr 1 – NIP-2, załącznik nr 4 – NIP-8)\n<a href=\"" USTAWA_NIP "\">Ustawa o zasadach ewidencji i identyfikacji podatników i płatników</a> (art. 3 – NIP)\n\n"

/* Sekundy, przez które NetUse(), NetAppBlank() i NetRLock() próbują, zanim zapytają o następną próbę */
#define NET_WAIT 1

#define SRODOWISKO_TESTOWE     1
#define SRODOWISKO_PRODUKCYJNE 3

/* Tryb poprawiania rekordu, ENUM MODE z xbiuro.ch */
typedef enum
{
   NOWY,
   KOPIA,
   POPRAW
} Tryb;

/* Zakres wydruku, ENUM DRUK z xbiuro.ch */
typedef enum
{
   BIEZACY,
   ZAZNACZONE,
   WSZYSTKIE
} Druk;

/* Forma opodatkowania firmy na rok według art. 9a ustawy o PIT; pole FORMA_OPODATKOWANIA w opodatkowanie.dbf */
typedef enum
{
   ZASADY_OGOLNE = 1,
   PODATEK_LINIOWY,
   RYCZALT
} FormaOpodatkowania;

/* Ewidencja firmy na rok: księga albo księgi rachunkowe (art. 24a ustawy o PIT), ewidencja przychodów przy ryczałcie
   (art. 15 ustawy o zryczałtowanym podatku dochodowym); pole EWIDENCJA w opodatkowanie.dbf */
typedef enum
{
   KSIEGA_PRZYCHODOW_I_ROZCHODOW = 1,
   KSIEGI_RACHUNKOWE,
   EWIDENCJA_PRZYCHODOW
} RodzajEwidencji;

typedef struct Biuro
{
   char       daneYYYY[ 16 ];   /* dane-YYYY/ */
   int        rok;              /* rok danych */
   Win       *win;              /* główne okno programu */
   Sidebar   *side;             /* pasek boczny z listą firm */
   StatusBar *status;           /* pasek stanu z wybraną firmą */
   int        firmy;            /* strona firm na pasku bocznym */
   int        moduly;           /* strona modułów na pasku bocznym */
   Menu      *menu;
   int        modul;
   int       *numery;           /* numery firm z katalogów dane-RRRR/firmaN, rosnąco; wiersz listy i to numer[ i - 1 ] */
   int        liczbaFirm;
   int        firma;            /* wybrana firma od 1 */
   char       tytul[ 2100 ];    /* wybrana firma na środku paska tytułu: „Firma N  nazwa pełna” */
   void     ( *zmianaFirmy )( struct Biuro *biuro );
} Biuro;

typedef void ( *ModulAkcja )( Biuro *biuro );

/* Strona z danymi urzędu skarbowego (typ, kod, nazwa i adres siedziby) i jej kontrolki, nazwane jak pola tabeli us */
typedef struct
{
   Control *TYP;
   Control *KOD_URZEDU;
   Control *NAZWA_URZEDU;
   Control *WOJEWODZTWO;
   Control *ULICA;
   Control *NR_DOMU;
   Control *NR_LOKALU;
   Control *KOD_POCZTOWY;
   Control *MIEJSCOWOSC;
   Control *POCZTA;
} Urzad;

/* Zakładki z danymi podmiotu (firmy albo kontrahenta) i ich kontrolki, nazwane jak pola tabeli; cAlias to obszar tabeli */
typedef struct
{
   const char *cAlias;
   Control    *zakladka;
   Win        *zakladki[ 8 ];
   Control    *NIP;
   Control    *PELNA_NAZWA;
   Control    *SKROCONA_NAZWA;
   Control    *REGON;
   Control    *NUMER_KRS;
   Control    *KOD_KRAJU;
   Control    *WOJEWODZTWO;
   Control    *POWIAT;
   Control    *GMINA;
   Control    *ULICA;
   Control    *NR_DOMU;
   Control    *NR_LOKALU;
   Control    *KOD_POCZTOWY;
   Control    *MIEJSCOWOSC;
   Control    *POCZTA;
   Control    *DATA_ROZPOCZECIA;
   Control    *RODZAJ_DZIALALNOSCI;
   Control    *KOD_PKD;
   Control    *NUMER_BDO;
   Control    *ORGAN_REJESTROWY;
   Control    *NAZWA_REJESTRU;
   Control    *DATA_REJESTRACJI;
   Control    *NUMER_W_REJESTRZE;
   Control    *DRUKUJ_RACHUNEK;
   Control    *KRAJ_BANKU_1;
   Control    *NAZWA_BANKU_1;
   Control    *NUMER_RACHUNKU_1;
   Control    *KOD_SWIFT_1;
   Control    *KRAJ_BANKU_2;
   Control    *NAZWA_BANKU_2;
   Control    *NUMER_RACHUNKU_2;
   Control    *KOD_SWIFT_2;
   Control    *DRUKUJ_1;
   Control    *DRUKUJ_2;
   Control    *TELEFON_1;
   Control    *TELEFON_2;
   Control    *EMAIL;
   Control    *STRONA_INTERNETOWA;
   Urzad       urzad;
   Control    *PODATNIK_VAT;
   Control    *PODSTAWA_ZWOLNIENIA;
   Control    *UWAGI;
} Podmiot;

/* biuro.c */
void  Katalogi( Biuro *biuro );

/* firma.c */
void Firma( Win *win, void *data );
xBool FirmaDbOpen( const char *cBaza );
void FirmaOpis( Biuro *biuro, int firma, char *nazwa, int rozmiarNazwy, char *nip, int rozmiarNip );
void FirmaNazwa( Biuro *biuro, int firma, char *nazwa, int rozmiar );
xBool FirmaNipPoprawny( const char *nip );
void FirmaCyfry( const char *tekst, char *cyfry, int rozmiar );
xBool FirmaPobierzDane( Biuro *biuro, const char *cAlias );
char *RokFirma( Biuro *biuro, int firma, char *cKatalog, int nRozmiar );
char *KatalogFirmy( int nRok, int firma, char *cKatalog, int nRozmiar );
void  FirmaZmieniona( Biuro *biuro );

/* dbbrowse.c */
void  DbBrowse( Control *browse, const char *alias );
xBool DbPusta( const char *alias );

/* us.c */
void  Us( Win *win, void *data );
xBool UsWybierz( Win *win, char *kod, int rozmiarKodu, char *nazwa, int rozmiarNazwy );
xBool UsNtx( void );
void  UsZakladka( Urzad *urzad, Win *strona );
void  UsOkno( Urzad *urzad );
void  UsPokaz( Urzad *urzad, const char *kod );
xBool UsZnajdz( const char *kod );

/* kontrahent.c */
void  Kontrahent( Win *win, void *data );
long  KontrahentWybierz( Win *win, Biuro *biuro );
xBool KontrahentNtx( void );

/* podmiot.c */
xBool PodmiotDbOpen( const char *cBaza, const char *cAlias );
void  PodmiotZakladki( Podmiot *podmiot, Control *zakladka, const char *cDane );
void  PodmiotOkno( Podmiot *podmiot );
xBool PodmiotEdycja( Podmiot *podmiot, Win *okno, StatusBar *pasekStanu, Tryb nTryb );

/* wydruk.c */
HPDF_Font WydrukCzcionka( HPDF_Doc pPdf, const char *cPlik );
void      WydrukUstawZnaki( HPDF_Page pPage, HPDF_Font pFont );
void      WydrukTekst( HPDF_Page pPage, HPDF_Font pFont, double nSize, double nX, double nY, const char *cTekst, double nSzer );
void      WydrukPrawo( HPDF_Page pPage, HPDF_Font pFont, double nSize, double nPrawo, double nY, const char *cTekst );
char     *WydrukKwota( double nKwota, char *cWynik, int nRozmiar );
char     *WydrukIlosc( double nIlosc, char *cWynik, int nRozmiar );
void      WydrukPasek( HPDF_Page pPage, HPDF_Font pBold, double nY, const double *aKolumna, int nKolumn, const char **aTytul, const xBool *aPrawo );
void      WydrukStrona( HPDF_Page pPage, HPDF_Font pFont, HPDF_Font pBold, double nX, double nY, const char *cTytul, const char *cNazwa, const char *cAdres1, const char *cAdres2, const char *cNip );
void      WydrukQr( HPDF_Page pPage, const char *cTekst, double nX, double nY, double nRozmiar );
char     *WydrukPlik( Biuro *biuro, const char *cNazwa, char *cPlik, int nRozmiar );
void      WydrukKarta( Biuro *biuro, const char *cAlias, const char *cTytul, const char *cNazwa );
char     *NToW( double nKwota, char *cWynik, int nRozmiar );

/* przelew.c */
void Przelew( Win *win, void *data );

/* opodatkowanie.c */
void  Opodatkowanie( Win *win, void *data );
xBool OpodatkowanieModul( Biuro *biuro, const char *cModul );

/* netuse.c */
xBool NetUse( const char *baza, xBool wylacznie, int sekundy, const char *alias, xBool bezPytania );
xBool NetPack( void );
xBool NetZap( void );
xBool NetAppBlank( int sekundy, xBool zdejmijBlokady );
xBool NetRLock( int sekundy );

#ifdef __cplusplus
}
#endif

#endif /* BIURO_H */
