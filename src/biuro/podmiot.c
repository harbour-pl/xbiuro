/*
 * Program...: xBiuro
 * Plik......: src/biuro/podmiot.c
 *
 * Wzór podmiotu, wspólny dla firmy (dane-RRRR/firmaN/firma.dbf) i kontrahenta (dane-baza/kartoteka/kontrah.dbf):
 * pola tabeli, zakładki okna, pokazywanie i poprawianie danych. Układ według NIP-2 i NIP-8 (zasada 9)
 *
 */

#include <string.h>

#include "biuro.h"

/* Pola tabeli podmiotu z państwowych druków i schematów (zasada 13): długość tekstu w bajtach UTF-8, 4 bajty na znak
   ze źródła, najwyżej 254; nazwa pełna, podstawa zwolnienia z VAT i uwagi w memo */
static const DbField aDbf[] = {
   { "NIP",                 'C',  50, 0 },   /* TNrNIP, u kontrahenta zagranicznego FA(3) NrID (50) */
   { "PELNA_NAZWA",         'M',  10, 0 },   /* NIP-2 poz. 6, FA(3) Nazwa (512) */
   { "SKROCONA_NAZWA",      'C', 254, 0 },   /* NIP-2 poz. 8 (240) */
   { "REGON",               'C',   9, 0 },   /* NIP-2 poz. 9, TNrREGON */
   { "NUMER_KRS",           'C',  10, 0 },   /* NIP-8 poz. 8, TNrKRS */
   { "KOD_KRAJU",           'C',   2, 0 },   /* NIP-2 B.5, TAdresPolski KodKraju */
   { "WOJEWODZTWO",         'C', 144, 0 },   /* TJednAdmin (36) */
   { "POWIAT",              'C', 144, 0 },   /* TJednAdmin (36) */
   { "GMINA",               'C', 144, 0 },   /* TJednAdmin (36) */
   { "ULICA",               'C', 254, 0 },   /* TUlica (65) */
   { "NR_DOMU",             'C',  36, 0 },   /* TNrBudynku (9) */
   { "NR_LOKALU",           'C',  40, 0 },   /* TNrLokalu (10) */
   { "KOD_POCZTOWY",        'C',   8, 0 },   /* TKodPocztowy (8) */
   { "MIEJSCOWOSC",         'C', 224, 0 },   /* TMiejscowosc (56) */
   { "POCZTA",              'C', 224, 0 },   /* TAdresPolski Poczta (56) */
   { "DATA_ROZPOCZECIA",    'D',   8, 0 },   /* CEIDG-1 */
   { "RODZAJ_DZIALALNOSCI", 'C', 254, 0 },   /* NIP-2 poz. 118 */
   { "KOD_PKD",             'C',   7, 0 },   /* NIP-2 poz. 119 */
   { "NUMER_BDO",           'C',   9, 0 },
   { "ORGAN_REJESTROWY",    'C', 254, 0 },   /* NIP-2 poz. 14 */
   { "NAZWA_REJESTRU",      'C', 254, 0 },   /* NIP-2 poz. 15 */
   { "DATA_REJESTRACJI",    'D',   8, 0 },   /* NIP-2 poz. 16 */
   { "NUMER_W_REJESTRZE",   'C',  80, 0 },   /* NIP-2 poz. 17 */
   { "DRUKUJ_RACHUNEK",     'L',   1, 0 },
   { "KRAJ_BANKU_1",        'C',   2, 0 },   /* NIP-2 kraj siedziby banku */
   { "NAZWA_BANKU_1",       'C', 254, 0 },   /* FA(3) NazwaBanku (256) */
   { "NUMER_RACHUNKU_1",    'C',  34, 0 },   /* FA(3) TNrRB (34) */
   { "KOD_SWIFT_1",         'C',  11, 0 },   /* NIP-2, FA(3) SWIFT */
   { "KRAJ_BANKU_2",        'C',   2, 0 },
   { "NAZWA_BANKU_2",       'C', 254, 0 },
   { "NUMER_RACHUNKU_2",    'C',  34, 0 },
   { "KOD_SWIFT_2",         'C',  11, 0 },
   { "DRUKUJ_RACHUNEK_NR",  'N',   1, 0 },   /* 1 albo 2 */
   { "TELEFON_1",           'C',  16, 0 },   /* FA(3) TNumerTelefonu (16) */
   { "TELEFON_2",           'C',  16, 0 },
   { "EMAIL",               'C', 254, 0 },   /* TAdresEmail (255) */
   { "STRONA_INTERNETOWA",  'C', 254, 0 },   /* NIP-8 poz. 90 */
   { "KOD_URZEDU",          'C',   4, 0 },   /* JPK KodUrzedu; dane urzędu z kartoteki US */
   { "PODATNIK_VAT",        'L',   1, 0 },
   { "PODSTAWA_ZWOLNIENIA", 'M',  10, 0 },   /* FA(3) P_19A */
   { "SRODOWISKO_KSEF",     'N',   1, 0 },
   { "UWAGI",               'M',  10, 0 }
};

#define POLA_PODMIOTU ( ( int )( sizeof( aDbf ) / sizeof( aDbf[ 0 ] ) ) )

/* Wybór urzędu F12 przy poprawianiu: podmiot i pole GET z kodem urzędu */
typedef struct
{
   Podmiot *podmiot;
   Control *oKod;
} WyborUrzedu;

static void     Przebudowa( const char *cBaza, const char *cAlias );
static void     Ramka( Win *strona, int x, int y, int wysokosc, const char *podpis );
static void     Opis( Win *strona, int y, int dlugosc, const char *tekst );
static Control *Pole( Win *strona, int x, int y, int dlugosc );
static void     Wybory( Podmiot *podmiot, xBool lWlaczone );
static void     WybierzUrzad( Win *win, void *data );

// Otwiera tabelę podmiotu cBaza w obszarze cAlias; brak pliku zakłada tabelę z pól aDbf, inne pola przebudowują ją
// PodmiotDbOpen( cBaza, cAlias ) --> lOtwarta
xBool PodmiotDbOpen( const char *cBaza, const char *cAlias )
{
   char cPlik[ 64 ];

   StringFormat( cPlik, "%s.dbf", cBaza );
   if( !FileExists( cPlik ) )
      DbCreate( cBaza, aDbf, POLA_PODMIOTU );

   if( !NetUse( cBaza, F, NET_WAIT, cAlias, F ) )
      return F;

   if( FCount() != POLA_PODMIOTU )
   {
      DbCloseArea();
      MsgInfo( "Dodanie pola w bazie", "Dane podmiotu" );
      Przebudowa( cBaza, cAlias );
      if( !NetUse( cBaza, F, NET_WAIT, cAlias, F ) )
         return F;
   }

   return T;
}

// Zakładki z danymi podmiotu na zakładce zakladka; cDane to nazwa pierwszej zakładki
// PodmiotZakladki( podmiot, zakladka, cDane ) --> void
void PodmiotZakladki( Podmiot *podmiot, Control *zakladka, const char *cDane )
{
   static const char *nazwyZakladek[] = { "", "Adres siedziby firmy", "Dane działalności", "Rachunki bankowe", "Dane kontaktowe", "Dane do urzędów", "VAT", "Uwagi" };

   Win *strona;

   podmiot->zakladka = zakladka;
   for( int i = 0; i < 8; ++i )
      podmiot->zakladki[ i ] = TabControlAddPage( zakladka, IIF( i == 0, cDane, nazwyZakladek[ i ] ) );

   // Dane: NIP jak poz. 1 druku, potem B.1 Dane identyfikacyjne
   strona = podmiot->zakladki[ 0 ];
   Ramka( strona, 10, 8, 170, "" );
   Opis( strona,  23, 15, "Identyfikator NIP:" );
   Opis( strona,  53, 15, "Nazwa pełna:" );
   Opis( strona,  83, 15, "Nazwa skrócona:" );
   Opis( strona, 113, 15, "Numer REGON:" );
   Opis( strona, 143, 15, "Numer KRS:" );
   podmiot->NIP            = Pole( strona, 175,  20, 34 );
   podmiot->PELNA_NAZWA    = Pole( strona, 175,  50, 40 );
   podmiot->SKROCONA_NAZWA = Pole( strona, 175,  80, 40 );
   podmiot->REGON          = Pole( strona, 175, 110,  9 );
   podmiot->NUMER_KRS      = Pole( strona, 175, 140, 10 );

   // Adres siedziby firmy: B.5 druku, od kraju
   strona = podmiot->zakladki[ 1 ];
   Ramka( strona, 10, 8, 260, "" );
   Opis( strona,  23, 15, "Kraj:" );
   Opis( strona,  53, 15, "Województwo:" );
   Opis( strona,  83, 15, "Powiat:" );
   Opis( strona, 113, 15, "Gmina:" );
   Opis( strona, 143, 15, "Ulica:" );
   Opis( strona, 173, 15, "Nr domu / lokalu:" );
   Opis( strona, 203, 15, "Kod / Miejscowość:" );
   Opis( strona, 233, 15, "Poczta:" );
   podmiot->KOD_KRAJU    = Pole( strona, 175,  20,  2 );
   podmiot->WOJEWODZTWO  = Pole( strona, 175,  50, 36 );
   podmiot->POWIAT       = Pole( strona, 175,  80, 36 );
   podmiot->GMINA        = Pole( strona, 175, 110, 36 );
   podmiot->ULICA        = Pole( strona, 175, 140, 40 );
   podmiot->NR_DOMU      = Pole( strona, 175, 170,  9 );
   podmiot->NR_LOKALU    = Pole( strona, 274, 170, 10 );
   podmiot->KOD_POCZTOWY = Pole( strona, 175, 200,  6 );
   podmiot->MIEJSCOWOSC  = Pole( strona, 247, 200, 32 );
   podmiot->POCZTA       = Pole( strona, 175, 230, 40 );

   // Dane działalności
   strona = podmiot->zakladki[ 2 ];
   Ramka( strona, 10, 8, 140, "Prowadzona działalność" );
   Opis( strona,  33, 15, "Data rozpoczęcia:" );
   Opis( strona,  63, 15, "Rodzaj działalności:" );
   Opis( strona,  93, 15, "Kod PKD:" );
   Opis( strona, 123, 15, "Numer BDO:" );
   Ramka( strona, 10, 156, 140, "Wpis do ewidencji lub rejestru" );
   Opis( strona, 181, 15, "Organ rejestrowy:" );
   Opis( strona, 211, 15, "Nazwa rejestru:" );
   Opis( strona, 241, 15, "Data rejestracji:" );
   Opis( strona, 271, 15, "Numer w rejestrze:" );
   podmiot->DATA_ROZPOCZECIA    = Pole( strona, 175,  30, 10 );
   podmiot->RODZAJ_DZIALALNOSCI = Pole( strona, 175,  60, 44 );
   podmiot->KOD_PKD             = Pole( strona, 175,  90,  7 );
   podmiot->NUMER_BDO           = Pole( strona, 175, 120,  9 );
   podmiot->ORGAN_REJESTROWY    = Pole( strona, 175, 178, 44 );
   podmiot->NAZWA_REJESTRU      = Pole( strona, 175, 208, 44 );
   podmiot->DATA_REJESTRACJI    = Pole( strona, 175, 238, 10 );
   podmiot->NUMER_W_REJESTRZE   = Pole( strona, 175, 268, 20 );

   // Rachunki bankowe
   strona = podmiot->zakladki[ 3 ];
   podmiot->DRUKUJ_RACHUNEK = CheckBoxCreate( strona, 20, 12, 320, 20, "Drukuj rachunek bankowy na fakturze" );
   Ramka( strona, 10, 38, 150, "Rachunek pierwszy" );
   Opis( strona,  63, 15, "Kraj siedziby banku:" );
   Opis( strona,  93, 15, "Nazwa banku:" );
   Opis( strona, 123, 15, "Pełny numer rachunku:" );
   Opis( strona, 153, 15, "Kod SWIFT:" );
   Ramka( strona, 10, 198, 150, "Rachunek drugi" );
   Opis( strona, 223, 15, "Kraj siedziby banku:" );
   Opis( strona, 253, 15, "Nazwa banku:" );
   Opis( strona, 283, 15, "Pełny numer rachunku:" );
   Opis( strona, 313, 15, "Kod SWIFT:" );
   podmiot->KRAJ_BANKU_1     = Pole( strona, 175,  60,  2 );
   podmiot->NAZWA_BANKU_1    = Pole( strona, 175,  90, 44 );
   podmiot->NUMER_RACHUNKU_1 = Pole( strona, 175, 120, 34 );
   podmiot->KOD_SWIFT_1      = Pole( strona, 175, 150, 11 );
   podmiot->KRAJ_BANKU_2     = Pole( strona, 175, 220,  2 );
   podmiot->NAZWA_BANKU_2    = Pole( strona, 175, 250, 44 );
   podmiot->NUMER_RACHUNKU_2 = Pole( strona, 175, 280, 34 );
   podmiot->KOD_SWIFT_2      = Pole( strona, 175, 310, 11 );
   podmiot->DRUKUJ_1         = RadioButtonCreate( strona, 540,  62, 110, 20, "Drukuj to" );
   podmiot->DRUKUJ_2         = RadioButtonCreate( strona, 540, 222, 110, 20, "Drukuj to" );

   // Dane kontaktowe
   strona = podmiot->zakladki[ 4 ];
   Ramka( strona, 10, 8, 140, "" );
   Opis( strona,  33, 15, "Telefon:" );
   Opis( strona,  63, 15, "Telefon drugi:" );
   Opis( strona,  93, 15, "E-mail:" );
   Opis( strona, 123, 15, "Strona internetowa:" );
   podmiot->TELEFON_1          = Pole( strona, 175,  30, 16 );
   podmiot->TELEFON_2          = Pole( strona, 175,  60, 16 );
   podmiot->EMAIL              = Pole( strona, 175,  90, 44 );
   podmiot->STRONA_INTERNETOWA = Pole( strona, 175, 120, 44 );

   // Dane do urzędów: urząd skarbowy z kartoteki US, jak w kartotece
   UsZakladka( &podmiot->urzad, podmiot->zakladki[ 5 ] );

   // VAT
   strona = podmiot->zakladki[ 6 ];
   podmiot->PODATNIK_VAT = CheckBoxCreate( strona, 20, 12, 320, 20, "Podmiot jest podatnikiem VAT" );
   Ramka( strona, 10, 38, 60, "Zwolnienie z VAT (stawka zw)" );
   Opis( strona, 63, 15, "Podstawa prawna:" );
   podmiot->PODSTAWA_ZWOLNIENIA = Pole( strona, 175, 60, 44 );

   // Uwagi
   strona = podmiot->zakladki[ 7 ];
   Ramka( strona, 10, 8, 50, "" );
   Opis( strona, 23, 15, "Uwagi:" );
   podmiot->UWAGI = Pole( strona, 175, 20, 44 );

   Wybory( podmiot, F );
}

// Pokazuje dane bieżącego rekordu; nazwa pełna, podstawa zwolnienia i uwagi w jednej linii
// PodmiotOkno( podmiot ) --> void
void PodmiotOkno( Podmiot *podmiot )
{
   char cTekst[ 2049 ];
   char cLinia[ 2049 ];

   DbSelectArea( podmiot->cAlias );

   // Dane
   StrTran( cTekst, sizeof( cTekst ), FieldGetC( FieldPos( "PELNA_NAZWA" ) ), "\r\n", " " );
   StrTran( cLinia, sizeof( cLinia ), cTekst, "\n", " " );
   ControlSetText( podmiot->NIP,            FieldGetC( FieldPos( "NIP" ) ) );
   ControlSetText( podmiot->PELNA_NAZWA,    cLinia );
   ControlSetText( podmiot->SKROCONA_NAZWA, FieldGetC( FieldPos( "SKROCONA_NAZWA" ) ) );
   ControlSetText( podmiot->REGON,          FieldGetC( FieldPos( "REGON" ) ) );
   ControlSetText( podmiot->NUMER_KRS,      FieldGetC( FieldPos( "NUMER_KRS" ) ) );

   // Adres siedziby firmy
   ControlSetText( podmiot->KOD_KRAJU,    FieldGetC( FieldPos( "KOD_KRAJU" ) ) );
   ControlSetText( podmiot->WOJEWODZTWO,  FieldGetC( FieldPos( "WOJEWODZTWO" ) ) );
   ControlSetText( podmiot->POWIAT,       FieldGetC( FieldPos( "POWIAT" ) ) );
   ControlSetText( podmiot->GMINA,        FieldGetC( FieldPos( "GMINA" ) ) );
   ControlSetText( podmiot->ULICA,        FieldGetC( FieldPos( "ULICA" ) ) );
   ControlSetText( podmiot->NR_DOMU,      FieldGetC( FieldPos( "NR_DOMU" ) ) );
   ControlSetText( podmiot->NR_LOKALU,    FieldGetC( FieldPos( "NR_LOKALU" ) ) );
   ControlSetText( podmiot->KOD_POCZTOWY, FieldGetC( FieldPos( "KOD_POCZTOWY" ) ) );
   ControlSetText( podmiot->MIEJSCOWOSC,  FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
   ControlSetText( podmiot->POCZTA,       FieldGetC( FieldPos( "POCZTA" ) ) );

   // Dane działalności
   ControlSetText( podmiot->DATA_ROZPOCZECIA,    DToC( FieldGetD( FieldPos( "DATA_ROZPOCZECIA" ) ), cTekst, sizeof( cTekst ) ) );
   ControlSetText( podmiot->RODZAJ_DZIALALNOSCI, FieldGetC( FieldPos( "RODZAJ_DZIALALNOSCI" ) ) );
   ControlSetText( podmiot->KOD_PKD,             FieldGetC( FieldPos( "KOD_PKD" ) ) );
   ControlSetText( podmiot->NUMER_BDO,           FieldGetC( FieldPos( "NUMER_BDO" ) ) );
   ControlSetText( podmiot->ORGAN_REJESTROWY,    FieldGetC( FieldPos( "ORGAN_REJESTROWY" ) ) );
   ControlSetText( podmiot->NAZWA_REJESTRU,      FieldGetC( FieldPos( "NAZWA_REJESTRU" ) ) );
   ControlSetText( podmiot->DATA_REJESTRACJI,    DToC( FieldGetD( FieldPos( "DATA_REJESTRACJI" ) ), cTekst, sizeof( cTekst ) ) );
   ControlSetText( podmiot->NUMER_W_REJESTRZE,   FieldGetC( FieldPos( "NUMER_W_REJESTRZE" ) ) );

   // Rachunki bankowe
   CheckBoxSetChecked( podmiot->DRUKUJ_RACHUNEK, FieldGetL( FieldPos( "DRUKUJ_RACHUNEK" ) ) );
   ControlSetText( podmiot->KRAJ_BANKU_1,        FieldGetC( FieldPos( "KRAJ_BANKU_1" ) ) );
   ControlSetText( podmiot->NAZWA_BANKU_1,       FieldGetC( FieldPos( "NAZWA_BANKU_1" ) ) );
   ControlSetText( podmiot->NUMER_RACHUNKU_1,    FieldGetC( FieldPos( "NUMER_RACHUNKU_1" ) ) );
   ControlSetText( podmiot->KOD_SWIFT_1,         FieldGetC( FieldPos( "KOD_SWIFT_1" ) ) );
   ControlSetText( podmiot->KRAJ_BANKU_2,        FieldGetC( FieldPos( "KRAJ_BANKU_2" ) ) );
   ControlSetText( podmiot->NAZWA_BANKU_2,       FieldGetC( FieldPos( "NAZWA_BANKU_2" ) ) );
   ControlSetText( podmiot->NUMER_RACHUNKU_2,    FieldGetC( FieldPos( "NUMER_RACHUNKU_2" ) ) );
   ControlSetText( podmiot->KOD_SWIFT_2,         FieldGetC( FieldPos( "KOD_SWIFT_2" ) ) );
   CheckBoxSetChecked( podmiot->DRUKUJ_1,        IIF( FieldGetN( FieldPos( "DRUKUJ_RACHUNEK_NR" ) ) != 2, T, F ) );
   CheckBoxSetChecked( podmiot->DRUKUJ_2,        IIF( FieldGetN( FieldPos( "DRUKUJ_RACHUNEK_NR" ) ) == 2, T, F ) );

   // Dane kontaktowe
   ControlSetText( podmiot->TELEFON_1,          FieldGetC( FieldPos( "TELEFON_1" ) ) );
   ControlSetText( podmiot->TELEFON_2,          FieldGetC( FieldPos( "TELEFON_2" ) ) );
   ControlSetText( podmiot->EMAIL,              FieldGetC( FieldPos( "EMAIL" ) ) );
   ControlSetText( podmiot->STRONA_INTERNETOWA, FieldGetC( FieldPos( "STRONA_INTERNETOWA" ) ) );

   // VAT
   CheckBoxSetChecked( podmiot->PODATNIK_VAT, FieldGetL( FieldPos( "PODATNIK_VAT" ) ) );
   StrTran( cTekst, sizeof( cTekst ), FieldGetC( FieldPos( "PODSTAWA_ZWOLNIENIA" ) ), "\r\n", " " );
   StrTran( cLinia, sizeof( cLinia ), cTekst, "\n", " " );
   ControlSetText( podmiot->PODSTAWA_ZWOLNIENIA, cLinia );

   // Uwagi
   StrTran( cTekst, sizeof( cTekst ), FieldGetC( FieldPos( "UWAGI" ) ), "\r\n", " " );
   StrTran( cLinia, sizeof( cLinia ), cTekst, "\n", " " );
   ControlSetText( podmiot->UWAGI, cLinia );

   // Dane do urzędów: na końcu, bo czyta kartotekę US w innym obszarze
   StringFormat( cTekst, "%s", FieldGetC( FieldPos( "KOD_URZEDU" ) ) );
   UsPokaz( &podmiot->urzad, cTekst );
   DbSelectArea( podmiot->cAlias );
}

// Nowy podmiot, kopia bieżącego albo poprawienie bieżącego; zmienne cNAZWAPOLA jak pola tabeli. T, gdy zapisano
// PodmiotEdycja( podmiot, okno, pasekStanu, nTryb ) --> lZapisany
xBool PodmiotEdycja( Podmiot *podmiot, Win *okno, StatusBar *pasekStanu, Tryb nTryb )
{
   Control     *GetList[ 48 ];
   WyborUrzedu  wybor = { podmiot, NULL };
   Control     *oPodpowiedz;
   long         nRecNo;
   char         cNIP[ 51 ];
   char         cPELNA_NAZWA[ 2049 ];
   char         cSKROCONA_NAZWA[ 255 ];
   char         cREGON[ 10 ];
   char         cNUMER_KRS[ 11 ];
   char         cKOD_KRAJU[ 3 ];
   char         cWOJEWODZTWO[ 145 ];
   char         cPOWIAT[ 145 ];
   char         cGMINA[ 145 ];
   char         cULICA[ 255 ];
   char         cNR_DOMU[ 37 ];
   char         cNR_LOKALU[ 41 ];
   char         cKOD_POCZTOWY[ 9 ];
   char         cMIEJSCOWOSC[ 225 ];
   char         cPOCZTA[ 225 ];
   int          dDATA_ROZPOCZECIA;
   char         cRODZAJ_DZIALALNOSCI[ 255 ];
   char         cKOD_PKD[ 8 ];
   char         cNUMER_BDO[ 10 ];
   char         cORGAN_REJESTROWY[ 255 ];
   char         cNAZWA_REJESTRU[ 255 ];
   int          dDATA_REJESTRACJI;
   char         cNUMER_W_REJESTRZE[ 81 ];
   char         cKRAJ_BANKU_1[ 3 ];
   char         cNAZWA_BANKU_1[ 255 ];
   char         cNUMER_RACHUNKU_1[ 35 ];
   char         cKOD_SWIFT_1[ 12 ];
   char         cKRAJ_BANKU_2[ 3 ];
   char         cNAZWA_BANKU_2[ 255 ];
   char         cNUMER_RACHUNKU_2[ 35 ];
   char         cKOD_SWIFT_2[ 12 ];
   char         cTELEFON_1[ 17 ];
   char         cTELEFON_2[ 17 ];
   char         cEMAIL[ 255 ];
   char         cSTRONA_INTERNETOWA[ 255 ];
   char         cKOD_URZEDU[ 5 ];
   char         cPODSTAWA_ZWOLNIENIA[ 1025 ];
   char         cUWAGI[ 801 ];
   char         cTekst[ 255 ];
   char         cPytanie[ 400 ];
   int          nGet = 0;

   DbSelectArea( podmiot->cAlias );
   nRecNo = RecNo();

   if( ( nTryb == KOPIA || nTryb == POPRAW ) && Eof() )
      return F;

   if( nTryb == NOWY )
   {
      DbGoBottom();
      DbSkip( 1 );
   }

   // Dane
   StringFormat( cNIP,            "%s", FieldGetC( FieldPos( "NIP" ) ) );
   StringFormat( cPELNA_NAZWA,    "%s", FieldGetC( FieldPos( "PELNA_NAZWA" ) ) );
   StringFormat( cSKROCONA_NAZWA, "%s", FieldGetC( FieldPos( "SKROCONA_NAZWA" ) ) );
   StringFormat( cREGON,          "%s", FieldGetC( FieldPos( "REGON" ) ) );
   StringFormat( cNUMER_KRS,      "%s", FieldGetC( FieldPos( "NUMER_KRS" ) ) );

   // Adres siedziby firmy
   StringFormat( cKOD_KRAJU,    "%s", IIF( nTryb == NOWY, "PL", FieldGetC( FieldPos( "KOD_KRAJU" ) ) ) );
   StringFormat( cWOJEWODZTWO,  "%s", FieldGetC( FieldPos( "WOJEWODZTWO" ) ) );
   StringFormat( cPOWIAT,       "%s", FieldGetC( FieldPos( "POWIAT" ) ) );
   StringFormat( cGMINA,        "%s", FieldGetC( FieldPos( "GMINA" ) ) );
   StringFormat( cULICA,        "%s", FieldGetC( FieldPos( "ULICA" ) ) );
   StringFormat( cNR_DOMU,      "%s", FieldGetC( FieldPos( "NR_DOMU" ) ) );
   StringFormat( cNR_LOKALU,    "%s", FieldGetC( FieldPos( "NR_LOKALU" ) ) );
   StringFormat( cKOD_POCZTOWY, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY" ) ) );
   StringFormat( cMIEJSCOWOSC,  "%s", FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
   StringFormat( cPOCZTA,       "%s", FieldGetC( FieldPos( "POCZTA" ) ) );

   // Dane działalności
   dDATA_ROZPOCZECIA = FieldGetD( FieldPos( "DATA_ROZPOCZECIA" ) );
   StringFormat( cRODZAJ_DZIALALNOSCI, "%s", FieldGetC( FieldPos( "RODZAJ_DZIALALNOSCI" ) ) );
   StringFormat( cKOD_PKD,             "%s", FieldGetC( FieldPos( "KOD_PKD" ) ) );
   StringFormat( cNUMER_BDO,           "%s", FieldGetC( FieldPos( "NUMER_BDO" ) ) );
   StringFormat( cORGAN_REJESTROWY,    "%s", FieldGetC( FieldPos( "ORGAN_REJESTROWY" ) ) );
   StringFormat( cNAZWA_REJESTRU,      "%s", FieldGetC( FieldPos( "NAZWA_REJESTRU" ) ) );
   dDATA_REJESTRACJI = FieldGetD( FieldPos( "DATA_REJESTRACJI" ) );
   StringFormat( cNUMER_W_REJESTRZE,   "%s", FieldGetC( FieldPos( "NUMER_W_REJESTRZE" ) ) );

   // Rachunki bankowe
   StringFormat( cKRAJ_BANKU_1,     "%s", FieldGetC( FieldPos( "KRAJ_BANKU_1" ) ) );
   StringFormat( cNAZWA_BANKU_1,    "%s", FieldGetC( FieldPos( "NAZWA_BANKU_1" ) ) );
   StringFormat( cNUMER_RACHUNKU_1, "%s", FieldGetC( FieldPos( "NUMER_RACHUNKU_1" ) ) );
   StringFormat( cKOD_SWIFT_1,      "%s", FieldGetC( FieldPos( "KOD_SWIFT_1" ) ) );
   StringFormat( cKRAJ_BANKU_2,     "%s", FieldGetC( FieldPos( "KRAJ_BANKU_2" ) ) );
   StringFormat( cNAZWA_BANKU_2,    "%s", FieldGetC( FieldPos( "NAZWA_BANKU_2" ) ) );
   StringFormat( cNUMER_RACHUNKU_2, "%s", FieldGetC( FieldPos( "NUMER_RACHUNKU_2" ) ) );
   StringFormat( cKOD_SWIFT_2,      "%s", FieldGetC( FieldPos( "KOD_SWIFT_2" ) ) );

   // Dane kontaktowe
   StringFormat( cTELEFON_1,          "%s", FieldGetC( FieldPos( "TELEFON_1" ) ) );
   StringFormat( cTELEFON_2,          "%s", FieldGetC( FieldPos( "TELEFON_2" ) ) );
   StringFormat( cEMAIL,              "%s", FieldGetC( FieldPos( "EMAIL" ) ) );
   StringFormat( cSTRONA_INTERNETOWA, "%s", FieldGetC( FieldPos( "STRONA_INTERNETOWA" ) ) );

   // Dane do urzędów
   StringFormat( cKOD_URZEDU, "%s", FieldGetC( FieldPos( "KOD_URZEDU" ) ) );

   // VAT
   StringFormat( cPODSTAWA_ZWOLNIENIA, "%s", FieldGetC( FieldPos( "PODSTAWA_ZWOLNIENIA" ) ) );

   // Uwagi
   StringFormat( cUWAGI, "%s", FieldGetC( FieldPos( "UWAGI" ) ) );

   // Dane
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 0 ], 175,  20, 34, cNIP, sizeof( cNIP ), "" );
   GetList[ nGet++ ] = GetMemo( podmiot->zakladki[ 0 ], 175,  50, 40, 4, cPELNA_NAZWA, sizeof( cPELNA_NAZWA ) );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 0 ], 175,  80, 40, cSKROCONA_NAZWA, sizeof( cSKROCONA_NAZWA ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 0 ], 175, 110,  9, cREGON, sizeof( cREGON ), "999999999" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 0 ], 175, 140, 10, cNUMER_KRS, sizeof( cNUMER_KRS ), "9999999999" );

   // Adres siedziby firmy
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 1 ], 175,  20,  2, cKOD_KRAJU, sizeof( cKOD_KRAJU ), "@! AA" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 1 ], 175,  50, 36, cWOJEWODZTWO, sizeof( cWOJEWODZTWO ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 1 ], 175,  80, 36, cPOWIAT, sizeof( cPOWIAT ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 1 ], 175, 110, 36, cGMINA, sizeof( cGMINA ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 1 ], 175, 140, 40, cULICA, sizeof( cULICA ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 1 ], 175, 170,  9, cNR_DOMU, sizeof( cNR_DOMU ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 1 ], 274, 170, 10, cNR_LOKALU, sizeof( cNR_LOKALU ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 1 ], 175, 200,  6, cKOD_POCZTOWY, sizeof( cKOD_POCZTOWY ), "99-999" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 1 ], 247, 200, 32, cMIEJSCOWOSC, sizeof( cMIEJSCOWOSC ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 1 ], 175, 230, 40, cPOCZTA, sizeof( cPOCZTA ), "" );

   // Dane działalności
   GetList[ nGet++ ] = GetDate( podmiot->zakladki[ 2 ], 175,  30, &dDATA_ROZPOCZECIA );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 2 ], 175,  60, 44, cRODZAJ_DZIALALNOSCI, sizeof( cRODZAJ_DZIALALNOSCI ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 2 ], 175,  90,  7, cKOD_PKD, sizeof( cKOD_PKD ), "@! 99.99.A" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 2 ], 175, 120,  9, cNUMER_BDO, sizeof( cNUMER_BDO ), "999999999" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 2 ], 175, 178, 44, cORGAN_REJESTROWY, sizeof( cORGAN_REJESTROWY ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 2 ], 175, 208, 44, cNAZWA_REJESTRU, sizeof( cNAZWA_REJESTRU ), "" );
   GetList[ nGet++ ] = GetDate( podmiot->zakladki[ 2 ], 175, 238, &dDATA_REJESTRACJI );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 2 ], 175, 268, 20, cNUMER_W_REJESTRZE, sizeof( cNUMER_W_REJESTRZE ), "" );

   // Rachunki bankowe
   GetList[ nGet++ ] = podmiot->DRUKUJ_RACHUNEK;
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 3 ], 175,  60,  2, cKRAJ_BANKU_1, sizeof( cKRAJ_BANKU_1 ), "@! AA" );
   GetList[ nGet++ ] = podmiot->DRUKUJ_1;
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 3 ], 175,  90, 44, cNAZWA_BANKU_1, sizeof( cNAZWA_BANKU_1 ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 3 ], 175, 120, 34, cNUMER_RACHUNKU_1, sizeof( cNUMER_RACHUNKU_1 ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 3 ], 175, 150, 11, cKOD_SWIFT_1, sizeof( cKOD_SWIFT_1 ), "@!" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 3 ], 175, 220,  2, cKRAJ_BANKU_2, sizeof( cKRAJ_BANKU_2 ), "@! AA" );
   GetList[ nGet++ ] = podmiot->DRUKUJ_2;
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 3 ], 175, 250, 44, cNAZWA_BANKU_2, sizeof( cNAZWA_BANKU_2 ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 3 ], 175, 280, 34, cNUMER_RACHUNKU_2, sizeof( cNUMER_RACHUNKU_2 ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 3 ], 175, 310, 11, cKOD_SWIFT_2, sizeof( cKOD_SWIFT_2 ), "@!" );

   // Dane kontaktowe
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 4 ], 175,  30, 16, cTELEFON_1, sizeof( cTELEFON_1 ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 4 ], 175,  60, 16, cTELEFON_2, sizeof( cTELEFON_2 ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 4 ], 175,  90, 44, cEMAIL, sizeof( cEMAIL ), "" );
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 4 ], 175, 120, 44, cSTRONA_INTERNETOWA, sizeof( cSTRONA_INTERNETOWA ), "" );

   // Dane do urzędów
   wybor.oKod = GetText( podmiot->zakladki[ 5 ], 230,  30,  4, cKOD_URZEDU, sizeof( cKOD_URZEDU ), "9999" );
   GetList[ nGet++ ] = wybor.oKod;
   oPodpowiedz = SayCreate( podmiot->zakladki[ 5 ], 300, 33, 20, 20, "F12 - Wybierz urząd" );
   ControlSetColor( oPodpowiedz, 0x73264D );

   // VAT
   GetList[ nGet++ ] = podmiot->PODATNIK_VAT;
   GetList[ nGet++ ] = GetText( podmiot->zakladki[ 6 ], 175,  60, 44, cPODSTAWA_ZWOLNIENIA, sizeof( cPODSTAWA_ZWOLNIENIA ), "" );

   // Uwagi
   GetList[ nGet++ ] = GetMemo( podmiot->zakladki[ 7 ], 175,  20, 44, 4, cUWAGI, sizeof( cUWAGI ) );

   Wybory( podmiot, T );

   StatusBarSetText( pasekStanu, 1, " PgUp PgDn Enter - zapisz, Esc - bez zapisu" );
   StatusBarSetBackground( pasekStanu, 0x73264D );
   StatusBarSetForeground( pasekStanu, 0xFFFFFF );

   SetKey( "F12", WybierzUrzad, &wybor );
   // Odczyt od pierwszego pola zakładki, która jest teraz widoczna (first 0), a nie od NIP na pierwszej
   WinReadModal( okno, GetList, nGet, 0 );
   SetKey( "F12", NULL, NULL );
   ControlDestroy( oPodpowiedz );

   StatusBarSetText( pasekStanu, 1, "" );
   StatusBarSetBackground( pasekStanu, -1 );
   StatusBarSetForeground( pasekStanu, -1 );

   Wybory( podmiot, F );
   for( int i = 0; i < nGet; ++i )
   {
      if( GetList[ i ] != podmiot->DRUKUJ_RACHUNEK && GetList[ i ] != podmiot->DRUKUJ_1 && GetList[ i ] != podmiot->DRUKUJ_2 && GetList[ i ] != podmiot->PODATNIK_VAT )
         ControlDestroy( GetList[ i ] );
   }

   DbSelectArea( podmiot->cAlias );
   if( ReadLastKey() == K_ESC )
   {
      DbGoto( nRecNo );
      PodmiotOkno( podmiot );

      return F;
   }

   AllTrim( cNIP );
   if( strcmp( cKOD_KRAJU, "PL" ) == 0 && *cNIP != 0 && !FirmaNipPoprawny( cNIP ) )
   {
      StringFormat( cPytanie, "Niepoprawny NIP: %s\nFaktury z tym NIP-em nie zostaną zapisane, dopóki NIP nie będzie poprawny.\nZapisać mimo to?", cNIP );
      if( !MsgYesNo( cPytanie, "Dane podmiotu" ) )
      {
         DbGoto( nRecNo );
         PodmiotOkno( podmiot );

         return F;
      }
   }
   else if( strcmp( cKOD_KRAJU, "PL" ) == 0 && *cNIP != 0 )
      FirmaCyfry( cNIP, cNIP, sizeof( cNIP ) );

   DbSelectArea( podmiot->cAlias );
   if( nTryb == NOWY || nTryb == KOPIA )
   {
      if( !NetAppBlank( NET_WAIT, T ) )
      {
         DbGoto( nRecNo );
         PodmiotOkno( podmiot );

         return F;
      }
   }
   else if( !NetRLock( NET_WAIT ) )
   {
      PodmiotOkno( podmiot );

      return F;
   }

   // Dane
   FieldPutC( FieldPos( "NIP" ),            cNIP );
   FieldPutC( FieldPos( "PELNA_NAZWA" ),    AllTrim( cPELNA_NAZWA ) );
   FieldPutC( FieldPos( "SKROCONA_NAZWA" ), cSKROCONA_NAZWA );
   FieldPutC( FieldPos( "REGON" ),          cREGON );
   FieldPutC( FieldPos( "NUMER_KRS" ),      cNUMER_KRS );

   // Adres siedziby firmy
   FieldPutC( FieldPos( "KOD_KRAJU" ),    cKOD_KRAJU );
   FieldPutC( FieldPos( "WOJEWODZTWO" ),  cWOJEWODZTWO );
   FieldPutC( FieldPos( "POWIAT" ),       cPOWIAT );
   FieldPutC( FieldPos( "GMINA" ),        cGMINA );
   FieldPutC( FieldPos( "ULICA" ),        cULICA );
   FieldPutC( FieldPos( "NR_DOMU" ),      cNR_DOMU );
   FieldPutC( FieldPos( "NR_LOKALU" ),    cNR_LOKALU );
   FieldPutC( FieldPos( "KOD_POCZTOWY" ), IIF( *AllTrim( StrTran( cTekst, sizeof( cTekst ), cKOD_POCZTOWY, "-", "" ) ) == 0, "", cKOD_POCZTOWY ) );
   FieldPutC( FieldPos( "MIEJSCOWOSC" ),  cMIEJSCOWOSC );
   FieldPutC( FieldPos( "POCZTA" ),       cPOCZTA );

   // Dane działalności
   FieldPutD( FieldPos( "DATA_ROZPOCZECIA" ),    dDATA_ROZPOCZECIA );
   FieldPutC( FieldPos( "RODZAJ_DZIALALNOSCI" ), cRODZAJ_DZIALALNOSCI );
   FieldPutC( FieldPos( "KOD_PKD" ),             IIF( *AllTrim( StrTran( cTekst, sizeof( cTekst ), cKOD_PKD, ".", "" ) ) == 0, "", cKOD_PKD ) );
   FieldPutC( FieldPos( "NUMER_BDO" ),           cNUMER_BDO );
   FieldPutC( FieldPos( "ORGAN_REJESTROWY" ),    cORGAN_REJESTROWY );
   FieldPutC( FieldPos( "NAZWA_REJESTRU" ),      cNAZWA_REJESTRU );
   FieldPutD( FieldPos( "DATA_REJESTRACJI" ),    dDATA_REJESTRACJI );
   FieldPutC( FieldPos( "NUMER_W_REJESTRZE" ),   cNUMER_W_REJESTRZE );

   // Rachunki bankowe
   FieldPutL( FieldPos( "DRUKUJ_RACHUNEK" ),    CheckBoxChecked( podmiot->DRUKUJ_RACHUNEK ) );
   FieldPutC( FieldPos( "KRAJ_BANKU_1" ),       cKRAJ_BANKU_1 );
   FieldPutC( FieldPos( "NAZWA_BANKU_1" ),      cNAZWA_BANKU_1 );
   FieldPutC( FieldPos( "NUMER_RACHUNKU_1" ),   cNUMER_RACHUNKU_1 );
   FieldPutC( FieldPos( "KOD_SWIFT_1" ),        cKOD_SWIFT_1 );
   FieldPutC( FieldPos( "KRAJ_BANKU_2" ),       cKRAJ_BANKU_2 );
   FieldPutC( FieldPos( "NAZWA_BANKU_2" ),      cNAZWA_BANKU_2 );
   FieldPutC( FieldPos( "NUMER_RACHUNKU_2" ),   cNUMER_RACHUNKU_2 );
   FieldPutC( FieldPos( "KOD_SWIFT_2" ),        cKOD_SWIFT_2 );
   FieldPutN( FieldPos( "DRUKUJ_RACHUNEK_NR" ), IIF( CheckBoxChecked( podmiot->DRUKUJ_2 ), 2, 1 ) );

   // Dane kontaktowe
   FieldPutC( FieldPos( "TELEFON_1" ),          cTELEFON_1 );
   FieldPutC( FieldPos( "TELEFON_2" ),          cTELEFON_2 );
   FieldPutC( FieldPos( "EMAIL" ),              cEMAIL );
   FieldPutC( FieldPos( "STRONA_INTERNETOWA" ), cSTRONA_INTERNETOWA );

   // Dane do urzędów
   FieldPutC( FieldPos( "KOD_URZEDU" ), cKOD_URZEDU );

   // VAT
   FieldPutL( FieldPos( "PODATNIK_VAT" ),        CheckBoxChecked( podmiot->PODATNIK_VAT ) );
   FieldPutC( FieldPos( "PODSTAWA_ZWOLNIENIA" ), RTrim( cPODSTAWA_ZWOLNIENIA ) );

   // Uwagi
   FieldPutC( FieldPos( "UWAGI" ), RTrim( cUWAGI ) );

   DbCommit();
   DbUnlock();

   PodmiotOkno( podmiot );

   return T;
}

// Przebudowuje tabelę podmiotu do pól aDbf: kopia, stara tabela jako cBaza.tmp, nowa tabela i APPEND FROM z kopii
// Przebudowa( cBaza, cAlias ) --> void
static void Przebudowa( const char *cBaza, const char *cAlias )
{
   char cDbf[ 64 ];
   char cDbt[ 64 ];
   char cTmp[ 64 ];
   char cTmpDbf[ 64 ];
   char cTmpDbt[ 64 ];
   char cStara[ 64 ];

   StringFormat( cDbf, "%s.dbf", cBaza );
   StringFormat( cDbt, "%s.dbt", cBaza );
   StringFormat( cTmp, "%s-tmp", cBaza );
   StringFormat( cTmpDbf, "%s.dbf", cTmp );
   StringFormat( cTmpDbt, "%s.dbt", cTmp );
   StringFormat( cStara, "%s.tmp", cBaza );

   if( FileExists( cDbf ) )
   {
      FileCopy( cDbf, cTmpDbf );
      if( FileExists( cDbt ) )
         FileCopy( cDbt, cTmpDbt );
      FRename( cDbf, cStara );
      DbCreate( cBaza, aDbf, POLA_PODMIOTU );
      if( NetUse( cBaza, F, NET_WAIT, cAlias, F ) )
      {
         DbAppendFrom( cTmp, NULL, NULL );
         DbCloseArea();
      }
      FErase( cTmpDbf );
      FErase( cTmpDbt );
      FErase( cStara );
   }
}

// Ramka grupy pól
// Ramka( strona, x, y, wysokosc, podpis ) --> void
static void Ramka( Win *strona, int x, int y, int wysokosc, const char *podpis )
{
   GroupBoxSetLineColor( GroupBoxCreate( strona, x, y, WinWidth( strona ) - 2 * x, wysokosc, podpis ), 0xDCDCDC );
}

// Opis pola
// Opis( strona, y, dlugosc, tekst ) --> void
static void Opis( Win *strona, int y, int dlugosc, const char *tekst )
{
   SayCreate( strona, 20, y, dlugosc, 20, tekst );
}

// Pole do oglądania
// Pole( strona, x, y, dlugosc ) --> pole
static Control *Pole( Win *strona, int x, int y, int dlugosc )
{
   Control *pole = SayCreate( strona, x, y, dlugosc, 24, "" );

   LabelSetBorder( pole, T );

   return pole;
}

// Pola wyboru (drukowanie rachunku, który rachunek, podatnik VAT) włączone tylko przy poprawianiu
// Wybory( podmiot, lWlaczone ) --> void
static void Wybory( Podmiot *podmiot, xBool lWlaczone )
{
   ControlSetEnabled( podmiot->DRUKUJ_RACHUNEK, lWlaczone );
   ControlSetEnabled( podmiot->DRUKUJ_1, lWlaczone );
   ControlSetEnabled( podmiot->DRUKUJ_2, lWlaczone );
   ControlSetEnabled( podmiot->PODATNIK_VAT, lWlaczone );
}

// F12 przy poprawianiu, na zakładce z urzędem: wybór urzędu z kartoteki US do pola kodu i jego dane na stronie
// WybierzUrzad( win, data ) --> void
static void WybierzUrzad( Win *win, void *data )
{
   WyborUrzedu *wybor = ( WyborUrzedu * )data;
   char         cKod[ 5 ];
   char         cNazwa[ 255 ];

   if( TabControlSelected( wybor->podmiot->zakladka ) != 6 )
      return;

   // Bez F12 w kartotece US, bo poprawianie urzędu w niej to też odczyt
   SetKey( "F12", NULL, NULL );
   if( UsWybierz( win, cKod, sizeof( cKod ), cNazwa, sizeof( cNazwa ) ) )
   {
      ControlSetText( wybor->oKod, cKod );
      UsPokaz( &wybor->podmiot->urzad, cKod );
   }
   SetKey( "F12", WybierzUrzad, wybor );

   ControlSetFocus( wybor->oKod );
}
