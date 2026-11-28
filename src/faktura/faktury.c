/*
 * Program...: xBiuro
 * Plik......: src/faktura/faktury.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "faktura.h"

static const DbField aFaktury[] = {
   { "KOLEJNY_NUMER",                  'N',   6, 0 },
   { "RODZAJ_FAKTURY",                 'C',   7, 0 },
   { "NUMER_FAKTURY",                  'C', 254, 0 },
   { "DATA_WYSTAWIENIA",               'D',   8, 0 },
   { "DATA_DOSTAWY",                   'D',   8, 0 },
   { "KOD_KRAJU_NABYWCY",              'C',   2, 0 },
   { "NIP_NABYWCY",                    'C',  50, 0 },
   { "NAZWA_NABYWCY",                  'M',  10, 0 },
   { "KOD_POCZTOWY_NABYWCY",           'C',   8, 0 },
   { "MIEJSCOWOSC_NABYWCY",            'C', 224, 0 },
   { "ULICA_NABYWCY",                  'C', 254, 0 },
   { "FORMA_PLATNOSCI",                'N',   1, 0 },
   { "TERMIN_PLATNOSCI",               'D',   8, 0 },
   { "NAZWA_BANKU",                    'C', 254, 0 },
   { "NUMER_RACHUNKU",                 'C',  34, 0 },
   { "PODSTAWA_ZWOLNIENIA",            'M',  10, 0 },
   { "WARTOSC_NETTO",                  'N',  12, 2 },
   { "KWOTA_PODATKU",                  'N',  12, 2 },
   { "KWOTA_NALEZNOSCI",               'N',  12, 2 },
   { "NUMER_KSEF",                     'C',  35, 0 },
   { "STATUS_KSEF",                    'N',   1, 0 },
   { "DATA_PRZYJECIA_KSEF",            'C',  30, 0 },
   { "OPIS_STATUSU_KSEF",              'M',  10, 0 },
   { "NUMER_SESJI_KSEF",               'C',  40, 0 },
   { "NUMER_REFERENCYJNY_KSEF",        'C',  40, 0 },
   { "TRYB_WYSTAWIENIA",               'N',   1, 0 },
   { "TERMIN_PRZESLANIA_KSEF",         'D',   8, 0 },
   { "SRODOWISKO_KSEF",                'N',   1, 0 },
   { "NUMER_FAKTURY_KORYGOWANEJ",      'C', 254, 0 },
   { "DATA_FAKTURY_KORYGOWANEJ",       'D',   8, 0 },
   { "NUMER_KSEF_FAKTURY_KORYGOWANEJ", 'C',  35, 0 },
   { "PRZYCZYNA_KOREKTY",              'M',  10, 0 },
   { "DATA_ZAPISU",                    'D',   8, 0 }
};

static const DbField aPozycje[] = {
   { "KOLEJNY_NUMER",          'N',   6, 0 },
   { "RODZAJ_FAKTURY",         'C',   7, 0 },
   { "NUMER_WIERSZA",          'N',   4, 0 },
   { "INDEKS",                 'C', 200, 0 },
   { "NAZWA_TOWARU",           'M',  10, 0 },
   { "MIARA",                  'C', 254, 0 },
   { "ILOSC",                  'N',  12, 3 },
   { "CENA_JEDNOSTKOWA_NETTO", 'N',  12, 2 },
   { "WARTOSC_NETTO",          'N',  12, 2 },
   { "STAWKA_PODATKU",         'C',   5, 0 },
   { "KWOTA_PODATKU",          'N',  12, 2 },
   { "WARTOSC_BRUTTO",         'N',  12, 2 },
   { "STAN_PRZED",             'L',   1, 0 },
   { "DATA_ZAPISU",            'D',   8, 0 }
};

#define POLA_FAKTURY ( ( int )( sizeof( aFaktury ) / sizeof( aFaktury[ 0 ] ) ) )
#define POLA_POZYCJI ( ( int )( sizeof( aPozycje ) / sizeof( aPozycje[ 0 ] ) ) )

typedef struct
{
   Biuro     *biuro;
   Win       *okno;
   StatusBar *pasekStanu;
   Toolbar   *pasek;
   Control   *BRW;
   int        aZamroz[ 5 ];
   char       cData[ 16 ];
   char       cNabywca[ 2049 ];
} OknoFaktur;

typedef struct
{
   char cNazwa[ 2100 ];
   char cTekst[ 64 ];
} OknoPodgladu;

typedef struct
{
   char   cStawka[ 6 ];
   double aSuma[ 3 ];
} Stawka;

static xBool       DbOpen( const char *cBaza, const DbField *aWzor, int nPol, const char *cAlias );
static void        Przebudowa( const char *cBaza, const DbField *aWzor, int nPol, const char *cAlias );
static void        PackDatabase( const char *cBaza, const char *cAlias );
static xBool       Indeks( const char *cBaza, const char *cAlias );
static void        Klucz( char *cKlucz, int nRozmiar, void *data );
static char       *KluczFaktury( const char *cRodzaj, int nNr, char *cKlucz, int nRozmiar );
static void        Stan( OknoFaktur *oDialog );
static const char *Kolumna( Control *browse, void *data );
static const char *KolumnaKsef( Control *browse, void *data );
static const char *KolumnaData( Control *browse, void *data );
static const char *KolumnaNabywca( Control *browse, void *data );
static double      KolumnaKwota( Control *browse, void *data );
static double      Numer( Control *browse, void *data );
static void        Pomoc( Toolbar *toolbar, void *data );
static void        Nowa( Toolbar *toolbar, void *data );
static void        Wstaw( Win *win, void *data );
static void        Podglad( Toolbar *toolbar, void *data );
static void        WystawOffline( Toolbar *toolbar, void *data );
static void        Wyslij( Toolbar *toolbar, void *data );
static void        SprawdzKsef( Toolbar *toolbar, void *data );
static void        Drukuj( Toolbar *toolbar, void *data );
static void        Zamknij( Toolbar *toolbar, void *data );
static double      PodgladNumer( Control *browse, void *data );
static const char *PodgladNazwa( Control *browse, void *data );
static const char *PodgladIlosc( Control *browse, void *data );
static double      PodgladKwota( Control *browse, void *data );
static const char *PodgladPole( Control *browse, void *data );
static void        PodgladZamknij( Toolbar *toolbar, void *data );
static xBool       DrukPdf( Biuro *biuro, const char *cPlik, char *cBlad, int nRozmiar );
static HPDF_Page   DrukStrona( HPDF_Doc pPdf, HPDF_Font pFont );
static const char *WydrukFormaPlatnosci( int nForma );
static char       *WydrukQrAdres( Biuro *biuro, char *cAdres, int nRozmiar );
static char       *Linia( const char *cTekst, char *cWynik, int nRozmiar );
static void        Opis( Win *strona, int x, int y, const char *tekst );
static Control    *Pole( Win *strona, int x, int y, int dlugosc, const char *tekst );
static void        Przyjeta( const char *cSesja, const char *cFaktura, void *data );
static char       *Token( const char *cNip, int nSrodowisko );
static void        Wynik( Biuro *biuro, Control *browse, WynikKsef *wynik, const char *cBlad );

void Faktury( Win *win, void *data )
{
   OknoFaktur  oknoFaktur = { 0 };
   OknoFaktur *oDialog    = &oknoFaktur;
   Win        *panel;
   char        cTytul[ 64 ];
   int         przycisk;

   oDialog->biuro = ( Biuro * )data;
   if( !FakturyOtworz( oDialog->biuro ) )
      return;
   OfflinePrzypomnij();
   FakturyZakres( "VAT" );

   StringFormat( cTytul, "Faktury VAT %d", oDialog->biuro->rok );
   oDialog->okno = DialogCreate( win, cTytul, 1000, 700 );
   WinSetBackground( oDialog->okno, 0xE9EDF1 );
   WinSetSizable( oDialog->okno, T );
   WinSetKey( oDialog->okno, "Ins", Wstaw, oDialog );

   oDialog->pasekStanu = StatusBarCreate( oDialog->okno, "" );

   oDialog->pasek = ToolbarCreate( oDialog->okno, TOOLBAR_BOTTOM );
   przycisk = ToolbarAdd( oDialog->pasek, "F1-Pomoc", Pomoc, NULL );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "F1" );
   ToolbarAddSeparator( oDialog->pasek );
   przycisk = ToolbarAdd( oDialog->pasek, "F2-Nowa", Nowa, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "F2" );
   ToolbarAddSeparator( oDialog->pasek );
   oDialog->aZamroz[ 0 ] = ToolbarAdd( oDialog->pasek, "F3-Podgląd", Podglad, oDialog );
   ToolbarSetShortcut( oDialog->pasek, oDialog->aZamroz[ 0 ], "F3" );
   ToolbarAddSeparator( oDialog->pasek );
   oDialog->aZamroz[ 1 ] = ToolbarAdd( oDialog->pasek, "F5-Offline", WystawOffline, oDialog );
   ToolbarSetShortcut( oDialog->pasek, oDialog->aZamroz[ 1 ], "F5" );
   ToolbarAddSeparator( oDialog->pasek );
   oDialog->aZamroz[ 2 ] = ToolbarAdd( oDialog->pasek, "F6-Wyślij", Wyslij, oDialog );
   ToolbarSetShortcut( oDialog->pasek, oDialog->aZamroz[ 2 ], "F6" );
   ToolbarAddSeparator( oDialog->pasek );
   oDialog->aZamroz[ 3 ] = ToolbarAdd( oDialog->pasek, "F7-Sprawdź", SprawdzKsef, oDialog );
   ToolbarSetShortcut( oDialog->pasek, oDialog->aZamroz[ 3 ], "F7" );
   ToolbarAddSeparator( oDialog->pasek );
   oDialog->aZamroz[ 4 ] = ToolbarAdd( oDialog->pasek, "F9-Drukuj", Drukuj, oDialog );
   ToolbarSetShortcut( oDialog->pasek, oDialog->aZamroz[ 4 ], "F9" );
   ToolbarAddSeparator( oDialog->pasek );
   przycisk = ToolbarAdd( oDialog->pasek, "Esc", Zamknij, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "Esc" );

   panel = PanelCreate( oDialog->okno, 0, 0, 0, 0 );

   oDialog->BRW = BrowseCreate( panel, 10, 10, WinWidth( panel ) - 20, WinHeight( panel ) - 20 );
   ControlSetAnchor( oDialog->BRW, "LTRB" );
   BrowseSetColor( oDialog->BRW, BROWSE_HEADBACKGROUND, 0x4D7326 );
   DbBrowse( oDialog->BRW, "FKTHEAD" );
   BrowseAddColumn( oDialog->BRW, "W KSeF", 7, BROWSE_LEFT, KolumnaKsef, NULL );
   BrowseAddNumber( oDialog->BRW, "Poz.", 4, "9999", Numer, NULL );
   BrowseAddColumn( oDialog->BRW, "Numer", 14, BROWSE_LEFT, Kolumna, "NUMER_FAKTURY" );
   BrowseAddColumn( oDialog->BRW, "Wystawiona", 10, BROWSE_LEFT, KolumnaData, oDialog );
   BrowseAddColumn( oDialog->BRW, "NIP nabywcy", 12, BROWSE_LEFT, Kolumna, "NIP_NABYWCY" );
   BrowseAddColumn( oDialog->BRW, "Nabywca", 34, BROWSE_LEFT, KolumnaNabywca, oDialog );
   BrowseAddNumber( oDialog->BRW, "Netto", 12, "999999999.99", KolumnaKwota, "WARTOSC_NETTO" );
   BrowseAddNumber( oDialog->BRW, "VAT", 12, "999999999.99", KolumnaKwota, "KWOTA_PODATKU" );
   BrowseAddNumber( oDialog->BRW, "Brutto", 12, "999999999.99", KolumnaKwota, "KWOTA_NALEZNOSCI" );

   DbSelectArea( "FKTHEAD" );
   DbGoBottom();
   Stan( oDialog );

   WinCenter( oDialog->okno );
   DialogShowModal( oDialog->okno );

   FakturyZamknij();
}

xBool FakturyOtworz( Biuro *biuro )
{
   char cKatalog[ 80 ];
   char cFaktury[ 96 ];
   char cPozycje[ 96 ];

   FakturyKatalog( biuro, biuro->rok, cKatalog, sizeof( cKatalog ) );
   StringFormat( cFaktury, "%sfkthead", cKatalog );
   StringFormat( cPozycje, "%sfktrow", cKatalog );

   if( !DbOpen( cFaktury, aFaktury, POLA_FAKTURY, "FKTHEAD" ) )
      return F;

   if( !DbOpen( cPozycje, aPozycje, POLA_POZYCJI, "FKTROW" ) || !Indeks( cFaktury, "FKTHEAD" ) || !Indeks( cPozycje, "FKTROW" ) )
   {
      FakturyZamknij();

      return F;
   }

   return T;
}

void FakturyZamknij( void )
{
   if( DbSelectArea( "FKTROW" ) )
      DbCloseArea();

   if( DbSelectArea( "FKTHEAD" ) )
      DbCloseArea();
}

void FakturyZakres( const char *cRodzaj )
{
   char cOd[ 16 ];
   char cDo[ 16 ];

   DbSelectArea( "FKTHEAD" );
   OrdScope( TOPSCOPE, KluczFaktury( cRodzaj, 0, cOd, sizeof( cOd ) ) );
   OrdScope( BOTTOMSCOPE, KluczFaktury( cRodzaj, 999999, cDo, sizeof( cDo ) ) );
}

int FakturyNastepnyNumer( const char *cRodzaj )
{
   char cKlucz[ 16 ];
   char cJest[ 16 ];
   long nRecNo;
   int  nNr = 0;

   DbSelectArea( "FKTHEAD" );
   nRecNo = RecNo();
   DbSeek( KluczFaktury( cRodzaj, 999999, cKlucz, sizeof( cKlucz ) ), T );
   DbSkip( -1 );
   StringFormat( cJest, "%-7s", cRodzaj );
   if( !Bof() && !Eof() && strcmp( FieldGetC( FieldPos( "RODZAJ_FAKTURY" ) ), cJest ) == 0 )
      nNr = ( int )FieldGetN( FieldPos( "KOLEJNY_NUMER" ) );
   DbGoto( nRecNo );

   return nNr + 1;
}

char *FakturyKatalog( Biuro *biuro, int nRok, char *cKatalog, int nRozmiar )
{
   char cFirma[ 64 ];

   StringFormatSize( cKatalog, ( size_t )nRozmiar, "%sfaktura/", KatalogFirmy( nRok, biuro->firma, cFirma, sizeof( cFirma ) ) );

   return cKatalog;
}

char *FakturyKlucz( char *cKlucz, int nRozmiar )
{
   StringFormatSize( cKlucz, ( size_t )nRozmiar, "%s%6d", FieldGetC( FieldPos( "RODZAJ_FAKTURY" ) ), ( int )FieldGetN( FieldPos( "KOLEJNY_NUMER" ) ) );

   return cKlucz;
}

char *FakturyPlik( Biuro *biuro, const char *cKatalog, const char *cRozszerzenie, char *cPlik, int nRozmiar )
{
   char cFaktury[ 96 ];
   char cKatalogPliku[ 128 ];
   char cNumer[ 300 ];
   char cNazwa[ 300 ];

   StringFormat( cKatalogPliku, "%s%s", FakturyKatalog( biuro, biuro->rok, cFaktury, sizeof( cFaktury ) ), cKatalog );
   if( !DirExists( cKatalogPliku ) )
      DirCreate( cKatalogPliku );

   DbSelectArea( "FKTHEAD" );
   StringFormat( cNumer, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY" ) ) );
   StrTran( cNazwa, sizeof( cNazwa ), AllTrim( cNumer ), "/", "_" );
   StringFormatSize( cPlik, ( size_t )nRozmiar, "%s/%s%s", cKatalogPliku, cNazwa, cRozszerzenie );

   return cPlik;
}

char *FakturyTymczasowa( const char *cNazwa, char *cBaza, int nRozmiar )
{
   char cTemp[ 300 ];
   char cKatalog[ 320 ];

   StringFormat( cKatalog, "%sxBiuro", DirTemp( cTemp, sizeof( cTemp ) ) );
   if( !DirExists( cKatalog ) )
      DirCreate( cKatalog );

   StringFormatSize( cBaza, ( size_t )nRozmiar, "%s/%s", cKatalog, cNazwa );

   return cBaza;
}

int FakturySrodowiskoKsef( int nSrodowiskoFirmy )
{
   DbSelectArea( "FKTHEAD" );

   return IIF( FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ) > 0, ( int )FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ), nSrodowiskoFirmy );
}

xBool FakturySrodowisko( int nSrodowiskoFirmy, const char *cTytul )
{
   int  nSrodowisko;
   char cNumer[ 300 ];
   char cTekst[ 600 ];

   DbSelectArea( "FKTHEAD" );
   nSrodowisko = ( int )FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) );
   if( nSrodowisko > 0 && nSrodowisko != nSrodowiskoFirmy )
   {
      StringFormat( cNumer, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY" ) ) );
      StringFormat( cTekst, "Faktura %s była wysyłana do środowiska KSeF %s,\na firma ma teraz środowisko %s (menu KSeF, Środowisko KSeF).", AllTrim( cNumer ), IIF( nSrodowisko == SRODOWISKO_PRODUKCYJNE, "produkcyjne", "testowe" ), IIF( nSrodowiskoFirmy == SRODOWISKO_PRODUKCYJNE, "produkcyjne", "testowe" ) );
      MsgInfo( cTekst, cTytul );

      return F;
   }

   return T;
}

char *FakturyBase64Url( const void *data, size_t nRozmiarDanych, char *cWynik, int nRozmiar )
{
   char *cBase64  = Base64Encode( data, nRozmiarDanych );
   int   nDlugosc = 0;

   for( const char *c = cBase64; *c != 0 && *c != '=' && nDlugosc < nRozmiar - 1; ++c )
   {
      cWynik[ nDlugosc++ ] = IIF( *c == '+', '-', IIF( *c == '/', '_', *c ) );
   }
   cWynik[ nDlugosc ] = 0;
   free( cBase64 );

   return cWynik;
}

void FakturyWyslij( Biuro *biuro, StatusBar *pasekStanu, Control *browse )
{
   WynikKsef wynik;
   char     *cToken;
   char     *cXml;
   size_t    nXml;
   int       nSrodowisko;
   xBool     lOffline;
   xBool     lJest;
   char      cNip[ 64 ];
   char      cNazwa[ 2100 ];
   char      cUlica[ 300 ];
   char      cMiasto[ 230 ];
   char      cNumer[ 300 ];
   char      cPlik[ 400 ];
   char      cTekst[ 800 ];
   char      cBlad[ 2000 ];

   DbSelectArea( "FKTHEAD" );
   if( Eof() )
      return;

   StringFormat( cNumer, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY" ) ) );
   AllTrim( cNumer );
   if( FieldGetN( FieldPos( "STATUS_KSEF" ) ) == KSEF_PRZYJETA )
   {
      StringFormat( cNip, "%s", FieldGetC( FieldPos( "NUMER_KSEF" ) ) );
      StringFormat( cTekst, "Faktura jest już w KSeF.\nNumer KSeF: %s", AllTrim( cNip ) );
      MsgInfo( cTekst, "KSeF" );

      return;
   }

   if( FieldGetN( FieldPos( "STATUS_KSEF" ) ) == KSEF_W_TOKU )
   {
      MsgInfo( "KSeF jeszcze przetwarza tę fakturę.\nSprawdź ją klawiszem F7.", "KSeF" );

      return;
   }

   if( !FirmaPobierzDane( biuro, "SPRZEDAWCA" ) )
      return;

   StringFormat( cNip, "%s", FieldGetC( FieldPos( "NIP" ) ) );
   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "PELNA_NAZWA" ) ) );
   StringFormat( cUlica, "%s", FieldGetC( FieldPos( "ULICA" ) ) );
   StringFormat( cMiasto, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
   nSrodowisko = IIF( FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ) == SRODOWISKO_PRODUKCYJNE, SRODOWISKO_PRODUKCYJNE, SRODOWISKO_TESTOWE );
   AllTrim( cNip );

   if( !FakturySrodowisko( nSrodowisko, "KSeF" ) )
   {
      DbSelectArea( "SPRZEDAWCA" );
      DbCloseArea();

      return;
   }

   if( *cNip == 0 || *AllTrim( cNazwa ) == 0 || ( *AllTrim( cUlica ) == 0 && *AllTrim( cMiasto ) == 0 ) )
   {
      MsgInfo( "Uzupełnij NIP, nazwę i adres firmy:\nmenu Firma, Dane firmy.", "KSeF" );
      DbSelectArea( "SPRZEDAWCA" );
      DbCloseArea();

      return;
   }

   DbSelectArea( "FKTHEAD" );
   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_NABYWCY" ) ) );
   StringFormat( cUlica, "%s", FieldGetC( FieldPos( "ULICA_NABYWCY" ) ) );
   StringFormat( cMiasto, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC_NABYWCY" ) ) );
   if( *AllTrim( cNazwa ) == 0 || ( *AllTrim( cUlica ) == 0 && *AllTrim( cMiasto ) == 0 ) )
   {
      MsgInfo( "Faktura nie ma nazwy albo adresu nabywcy.", "KSeF" );
      DbSelectArea( "SPRZEDAWCA" );
      DbCloseArea();

      return;
   }

   cToken = Token( cNip, nSrodowisko );
   if( cToken == NULL )
   {
      DbSelectArea( "SPRZEDAWCA" );
      DbCloseArea();

      return;
   }

   DbSelectArea( "FKTHEAD" );
   lOffline = IIF( FieldGetN( FieldPos( "TRYB_WYSTAWIENIA" ) ) > 0, T, F );
   StringFormat( cTekst, "Wysłać fakturę %s do KSeF\n(środowisko %s%s)?", cNumer, IIF( nSrodowisko == SRODOWISKO_PRODUKCYJNE, "produkcyjne", "testowe" ), IIF( lOffline, ", faktura offline", "" ) );
   if( !MsgYesNo( cTekst, "KSeF" ) )
   {
      free( cToken );
      DbSelectArea( "SPRZEDAWCA" );
      DbCloseArea();

      return;
   }

   FakturyPlik( biuro, "xml", ".xml", cPlik, sizeof( cPlik ) );
   if( lOffline )
   {
      cXml = MemoRead( cPlik, &nXml );
      if( cXml == NULL || nXml == 0 )
      {
         StringFormat( cTekst, "Brak pliku XML faktury offline:\n%s", cPlik );
         MsgError( cTekst, "KSeF" );
      }
   }
   else
   {
      cXml = Fa3GenerujXml( "SPRZEDAWCA" );
      nXml = strlen( cXml );
      if( !MemoWrit( cPlik, cXml, nXml ) )
      {
         StringFormat( cTekst, "Nie można zapisać pliku %s.\nFaktura nie została wysłana.", cPlik );
         MsgError( cTekst, "KSeF" );
         nXml = 0;
      }
   }
   DbSelectArea( "SPRZEDAWCA" );
   DbCloseArea();

   if( cXml == NULL || nXml == 0 )
   {
      free( cXml );
      free( cToken );

      return;
   }

   StatusBarSetText( pasekStanu, 1, " Wysyłanie faktury do KSeF..." );
   lJest = KsefWyslij( cNip, cToken, nSrodowisko, cXml, nXml, lOffline, Przyjeta, &nSrodowisko, &wynik, cBlad, sizeof( cBlad ) );
   StatusBarSetText( pasekStanu, 1, "" );
   free( cXml );
   free( cToken );

   Wynik( biuro, browse, IIF( lJest, &wynik, NULL ), cBlad );
   KsefWynikFree( &wynik );
}

void FakturySprawdz( Biuro *biuro, StatusBar *pasekStanu, Control *browse )
{
   WynikKsef wynik;
   char     *cToken;
   int       nSrodowisko;
   int       nStatus;
   xBool     lJest;
   char      cNip[ 64 ];
   char      cNumer[ 64 ];
   char      cSesja[ 64 ];
   char      cFaktura[ 64 ];
   char      cPlik[ 400 ];
   char      cTekst[ 800 ];
   char      cBlad[ 2000 ];

   DbSelectArea( "FKTHEAD" );
   if( Eof() )
      return;

   nStatus = ( int )FieldGetN( FieldPos( "STATUS_KSEF" ) );
   if( nStatus == KSEF_PRZYJETA && FileExists( FakturyPlik( biuro, "upo", ".xml", cPlik, sizeof( cPlik ) ) ) )
   {
      DbSelectArea( "FKTHEAD" );
      StringFormat( cNumer, "%s", FieldGetC( FieldPos( "NUMER_KSEF" ) ) );
      StringFormat( cTekst, "Faktura jest w KSeF.\nNumer KSeF: %s\nUPO: %s", AllTrim( cNumer ), cPlik );
      MsgInfo( cTekst, "KSeF" );

      return;
   }

   if( nStatus != KSEF_W_TOKU && nStatus != KSEF_PRZYJETA )
   {
      MsgInfo( "Faktura nie została przyjęta przez KSeF.\nWyślij ją klawiszem F6.", "KSeF" );

      return;
   }

   if( !FirmaPobierzKsef( biuro, cNip, sizeof( cNip ), &nSrodowisko ) || !FakturySrodowisko( nSrodowisko, "KSeF" ) )
      return;

   cToken = Token( cNip, nSrodowisko );
   if( cToken == NULL )
      return;

   DbSelectArea( "FKTHEAD" );
   StringFormat( cSesja, "%s", FieldGetC( FieldPos( "NUMER_SESJI_KSEF" ) ) );
   StringFormat( cFaktura, "%s", FieldGetC( FieldPos( "NUMER_REFERENCYJNY_KSEF" ) ) );

   StatusBarSetText( pasekStanu, 1, " Sprawdzanie faktury w KSeF..." );
   lJest = KsefSprawdz( cNip, cToken, nSrodowisko, AllTrim( cSesja ), AllTrim( cFaktura ), &wynik, cBlad, sizeof( cBlad ) );
   StatusBarSetText( pasekStanu, 1, "" );
   free( cToken );

   Wynik( biuro, browse, IIF( lJest, &wynik, NULL ), cBlad );
   KsefWynikFree( &wynik );
}

void FakturyNtx( Biuro *biuro )
{
   char cKatalog[ 80 ];
   char cBaza[ 96 ];
   char cIndeks[ 112 ];

   FakturyKatalog( biuro, biuro->rok, cKatalog, sizeof( cKatalog ) );

   StringFormat( cBaza, "%sfkthead", cKatalog );
   PackDatabase( cBaza, "FKTHEAD" );
   StringFormat( cIndeks, "%s1.ntx", cBaza );
   FErase( cIndeks );

   StringFormat( cBaza, "%sfktrow", cKatalog );
   PackDatabase( cBaza, "FKTROW" );
   StringFormat( cIndeks, "%s1.ntx", cBaza );
   FErase( cIndeks );
}

const char *FakturyWKsef( void )
{
   int nStatus;

   DbSelectArea( "FKTHEAD" );
   nStatus = ( int )FieldGetN( FieldPos( "STATUS_KSEF" ) );

   if( nStatus == KSEF_PRZYJETA )
      return "Tak";

   if( nStatus == KSEF_W_TOKU )
      return "W toku";

   if( nStatus == KSEF_ODRZUCONA )
      return "Odrzuc.";

   if( FieldGetN( FieldPos( "TRYB_WYSTAWIENIA" ) ) > 0 )
      return "Offline";

   return "Nie";
}

xBool FakturyJest( void )
{
   return !DbPusta( "FKTHEAD" );
}

const DbField *PozycjeStruct( int *nPol )
{
   *nPol = POLA_POZYCJI;

   return aPozycje;
}

void FakturyPodglad( Win *win )
{
   OknoPodgladu  oknoPodgladu = { 0 };
   OknoPodgladu *oPodglad     = &oknoPodgladu;
   Win          *oDialog;
   Win          *oPanel;
   Win          *oLinia;
   Toolbar      *oToolbar;
   Control      *oBrowse;
   Control      *oPlat;
   Control      *oSuma;
   char          cKlucz[ 16 ];
   char          cTytul[ 300 ];
   char          cTekst[ 2100 ];
   char          cNetto[ 32 ];
   char          cVat[ 32 ];
   char          cBrutto[ 32 ];
   int           nForma;
   int           nStatus;
   int           przycisk;
   long          nPozycji = 0;

   DbSelectArea( "FKTHEAD" );
   if( Eof() )
      return;

   Klucz( cKlucz, sizeof( cKlucz ), NULL );
   DbSelectArea( "FKTROW" );
   OrdScope( TOPSCOPE, cKlucz );
   OrdScope( BOTTOMSCOPE, cKlucz );
   DbGoTop();
   nPozycji = OrdKeyCount();

   DbSelectArea( "FKTHEAD" );
   StringFormat( cTytul, "Podgląd faktury %s", Linia( FieldGetC( FieldPos( "NUMER_FAKTURY" ) ), cTekst, sizeof( cTekst ) ) );
   oDialog = DialogCreate( win, cTytul, 1100, 700 );
   WinSetBackground( oDialog, 0xE9EDF1 );
   WinSetSizable( oDialog, T );

   oToolbar = ToolbarCreate( oDialog, TOOLBAR_BOTTOM );
   przycisk = ToolbarAdd( oToolbar, "Esc", PodgladZamknij, oDialog );
   ToolbarSetShortcut( oToolbar, przycisk, "Esc" );
   ToolbarAddSeparator( oToolbar );

   oPanel = PanelCreate( oDialog, 0, 0, 0, 0 );

   Opis( oPanel, 20,   7, "Numer faktury:" );
   Opis( oPanel, 20,  37, "Data wystawienia:" );
   Opis( oPanel, 20,  67, "Data sprzedaży:" );
   Opis( oPanel, 20,  97, "Płatność:" );
   Opis( oPanel, 20, 127, "Data płatności:" );
   Pole( oPanel, 190,   4, 14, Linia( FieldGetC( FieldPos( "NUMER_FAKTURY" ) ), cTekst, sizeof( cTekst ) ) );
   Pole( oPanel, 190,  34, 10, DToC( FieldGetD( FieldPos( "DATA_WYSTAWIENIA" ) ), cTekst, sizeof( cTekst ) ) );
   Pole( oPanel, 190,  64, 10, DToC( FieldGetD( FieldPos( "DATA_DOSTAWY" ) ), cTekst, sizeof( cTekst ) ) );
   Pole( oPanel, 190, 124, 10, DToC( FieldGetD( FieldPos( "TERMIN_PLATNOSCI" ) ), cTekst, sizeof( cTekst ) ) );

   nForma = ( int )FieldGetN( FieldPos( "FORMA_PLATNOSCI" ) );
   oPlat = RadioButtonCreate( oPanel, 190, 96, 85, 20, "Gotówka" );
   CheckBoxSetChecked( oPlat, IIF( nForma == PLATNOSC_GOTOWKA, T, F ) );
   ControlSetEnabled( oPlat, F );
   oPlat = RadioButtonCreate( oPanel, 280, 96, 75, 20, "Karta" );
   CheckBoxSetChecked( oPlat, IIF( nForma == PLATNOSC_KARTA, T, F ) );
   ControlSetEnabled( oPlat, F );
   oPlat = RadioButtonCreate( oPanel, 360, 96, 90, 20, "Przelew" );
   CheckBoxSetChecked( oPlat, IIF( nForma == PLATNOSC_PRZELEW, T, F ) );
   ControlSetEnabled( oPlat, F );

   WinSetBackground( PanelCreate( oPanel, 460, 4, 1, 144 ), 0xA0A0A0 );

   Opis( oPanel, 475,  7, "Kod kraju / NIP:" );
   Opis( oPanel, 475, 37, "Nazwa kontrahenta:" );
   Opis( oPanel, 475, 67, "Ulica:" );
   Opis( oPanel, 475, 97, "Kod / Miejscowość:" );
   Pole( oPanel, 655,  4,  2, FieldGetC( FieldPos( "KOD_KRAJU_NABYWCY" ) ) );
   Pole( oPanel, 691,  4, 20, FieldGetC( FieldPos( "NIP_NABYWCY" ) ) );
   Pole( oPanel, 655, 34, 38, Linia( FieldGetC( FieldPos( "NAZWA_NABYWCY" ) ), cTekst, sizeof( cTekst ) ) );
   Pole( oPanel, 655, 64, 38, FieldGetC( FieldPos( "ULICA_NABYWCY" ) ) );
   Pole( oPanel, 655, 94,  6, FieldGetC( FieldPos( "KOD_POCZTOWY_NABYWCY" ) ) );
   Pole( oPanel, 727, 94, 30, FieldGetC( FieldPos( "MIEJSCOWOSC_NABYWCY" ) ) );

   oLinia = PanelCreate( oPanel, 10, 156, WinWidth( oPanel ) - 20, 1 );
   WinSetBackground( oLinia, 0xA0A0A0 );
   PanelSetAnchor( oLinia, "LTR" );

   Opis( oPanel, 20, 169, "Numer KSeF:" );
   nStatus = ( int )FieldGetN( FieldPos( "STATUS_KSEF" ) );
   if( nStatus == KSEF_PRZYJETA )
      StringFormat( cTekst, "%s", FieldGetC( FieldPos( "NUMER_KSEF" ) ) );
   else if( nStatus == KSEF_ODRZUCONA )
      StringFormat( cTekst, "odrzucona: %s", Linia( FieldGetC( FieldPos( "OPIS_STATUSU_KSEF" ) ), cTytul, sizeof( cTytul ) ) );
   else if( FieldGetN( FieldPos( "TRYB_WYSTAWIENIA" ) ) > 0 )
      StringFormat( cTekst, "offline, wyślij do %s", DToC( FieldGetD( FieldPos( "TERMIN_PRZESLANIA_KSEF" ) ), cTytul, sizeof( cTytul ) ) );
   else
      StringFormat( cTekst, "nie wysłana" );
   Pole( oPanel, 190, 166, 50, cTekst );

   oBrowse = BrowseCreate( oPanel, 10, 200, WinWidth( oPanel ) - 20, WinHeight( oPanel ) - 244 );
   ControlSetAnchor( oBrowse, "LTRB" );
   BrowseSetColor( oBrowse, BROWSE_HEADBACKGROUND, 0x4D7326 );
   DbBrowse( oBrowse, "FKTROW" );
   BrowseAddNumber( oBrowse, "Poz.", 4, "9999", PodgladNumer, NULL );
   BrowseAddColumn( oBrowse, "Nazwa towaru / usługi", 30, BROWSE_LEFT, PodgladNazwa, oPodglad );
   BrowseAddColumn( oBrowse, "Ilość", 10, BROWSE_RIGHT, PodgladIlosc, oPodglad );
   BrowseAddColumn( oBrowse, "J.m.", 5, BROWSE_LEFT, PodgladPole, "MIARA" );
   BrowseAddNumber( oBrowse, "Cena netto", 12, "999999999.99", PodgladKwota, "CENA_JEDNOSTKOWA_NETTO" );
   BrowseAddNumber( oBrowse, "Netto", 12, "999999999.99", PodgladKwota, "WARTOSC_NETTO" );
   BrowseAddColumn( oBrowse, "VAT", 3, BROWSE_LEFT, PodgladPole, "STAWKA_PODATKU" );
   BrowseAddNumber( oBrowse, "Kwota VAT", 12, "999999999.99", PodgladKwota, "KWOTA_PODATKU" );
   BrowseAddNumber( oBrowse, "Brutto", 12, "999999999.99", PodgladKwota, "WARTOSC_BRUTTO" );

   DbSelectArea( "FKTHEAD" );
   StringFormat( cTekst, " Pozycji: %ld     Netto: %s     VAT: %s     Brutto: %s zł", nPozycji, WydrukKwota( FieldGetN( FieldPos( "WARTOSC_NETTO" ) ), cNetto, sizeof( cNetto ) ), WydrukKwota( FieldGetN( FieldPos( "KWOTA_PODATKU" ) ), cVat, sizeof( cVat ) ), WydrukKwota( FieldGetN( FieldPos( "KWOTA_NALEZNOSCI" ) ), cBrutto, sizeof( cBrutto ) ) );
   oSuma = Pole( oPanel, 10, WinHeight( oPanel ) - 36, 100, cTekst );
   ControlSetAnchor( oSuma, "LB" );

   ControlSetFocus( oBrowse );
   WinCenter( oDialog );
   DialogShowModal( oDialog );

   DbSelectArea( "FKTROW" );
   OrdScope( TOPSCOPE, NULL );
   OrdScope( BOTTOMSCOPE, NULL );
   DbSelectArea( "FKTHEAD" );
}

void FakturyDrukuj( Biuro *biuro )
{
   char cNazwa[ 300 ];
   char cTekst[ 300 ];
   char cPlik[ 400 ];
   char cBlad[ 600 ];

   DbSelectArea( "FKTHEAD" );
   if( Eof() )
      return;

   if( !FirmaPobierzDane( biuro, "SPRZEDAWCA" ) )
      return;

   DbSelectArea( "FKTHEAD" );
   StringFormat( cTekst, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY" ) ) );
   StrTran( cNazwa, sizeof( cNazwa ), AllTrim( cTekst ), "/", "_" );
   StringFormat( cTekst, "%s.pdf", cNazwa );
   WydrukPlik( biuro, cTekst, cPlik, sizeof( cPlik ) );

   if( !DrukPdf( biuro, cPlik, cBlad, sizeof( cBlad ) ) )
      MsgError( cBlad, "Wydruk faktury" );
   else
      FileExecute( cPlik );

   DbSelectArea( "SPRZEDAWCA" );
   DbCloseArea();
   DbSelectArea( "FKTHEAD" );
}

static xBool DbOpen( const char *cBaza, const DbField *aWzor, int nPol, const char *cAlias )
{
   char cPlik[ 128 ];

   StringFormat( cPlik, "%s.dbf", cBaza );
   if( !FileExists( cPlik ) )
      DbCreate( cBaza, aWzor, nPol );

   if( !NetUse( cBaza, F, NET_WAIT, cAlias, F ) )
      return F;

   if( FCount() != nPol )
   {
      DbCloseArea();
      MsgInfo( "Dodanie pola w bazie", "Faktury" );
      Przebudowa( cBaza, aWzor, nPol, cAlias );
      if( !NetUse( cBaza, F, NET_WAIT, cAlias, F ) )
         return F;
   }

   return T;
}

static void Przebudowa( const char *cBaza, const DbField *aWzor, int nPol, const char *cAlias )
{
   char cDbf[ 128 ];
   char cTmp[ 128 ];
   char cTmpDbf[ 128 ];
   char cStara[ 128 ];
   char cIndeks[ 128 ];

   StringFormat( cDbf, "%s.dbf", cBaza );
   StringFormat( cTmp, "%s-tmp", cBaza );
   StringFormat( cTmpDbf, "%s.dbf", cTmp );
   StringFormat( cStara, "%s.tmp", cBaza );
   StringFormat( cIndeks, "%s1.ntx", cBaza );

   if( FileExists( cDbf ) )
   {
      FileCopy( cDbf, cTmpDbf );
      FRename( cDbf, cStara );
      DbCreate( cBaza, aWzor, nPol );
      if( NetUse( cBaza, F, NET_WAIT, cAlias, F ) )
      {
         DbAppendFrom( cTmp, NULL, NULL );
         DbCloseArea();
      }
      FErase( cTmpDbf );
      FErase( cStara );
      FErase( cIndeks );
   }
}

static void PackDatabase( const char *cBaza, const char *cAlias )
{
   char cPlik[ 128 ];

   StringFormat( cPlik, "%s.dbf", cBaza );
   if( FileExists( cPlik ) && NetUse( cBaza, T, NET_WAIT, cAlias, F ) )
   {
      NetPack();
      DbCloseArea();
   }
}

static xBool Indeks( const char *cBaza, const char *cAlias )
{
   char cIndeks[ 128 ];
   char cPlik[ 128 ];

   StringFormat( cIndeks, "%s1", cBaza );
   StringFormat( cPlik, "%s.ntx", cIndeks );

   DbSelectArea( cAlias );
   if( FileExists( cPlik ) && DbSetIndex( cIndeks, Klucz, NULL ) )
      return T;

   DbCloseArea();
   if( !NetUse( cBaza, T, NET_WAIT, cAlias, F ) )
      return F;
   OrdCreate( cIndeks, "RODZAJ_FAKTURY + Str( KOLEJNY_NUMER, 6 )", Klucz, NULL, F );
   DbCloseArea();

   if( !NetUse( cBaza, F, NET_WAIT, cAlias, F ) )
      return F;

   return DbSetIndex( cIndeks, Klucz, NULL );
}

static void Klucz( char *cKlucz, int nRozmiar, void *data )
{
   UNUSED( data );

   FakturyKlucz( cKlucz, nRozmiar );
}

static char *KluczFaktury( const char *cRodzaj, int nNr, char *cKlucz, int nRozmiar )
{
   StringFormatSize( cKlucz, ( size_t )nRozmiar, "%-7s%6d", cRodzaj, nNr );

   return cKlucz;
}

static void Stan( OknoFaktur *oDialog )
{
   xBool lJest = FakturyJest();

   for( int i = 0; i < 5; ++i )
   {
      ToolbarEnable( oDialog->pasek, oDialog->aZamroz[ i ], lJest );
   }
   ControlSetEnabled( oDialog->BRW, lJest );
   BrowseRefresh( oDialog->BRW );

   if( lJest )
      ControlSetFocus( oDialog->BRW );
}

static const char *Kolumna( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "FKTHEAD" );

   return FieldGetC( FieldPos( ( const char * )data ) );
}

static const char *KolumnaKsef( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   return FakturyWKsef();
}

static const char *KolumnaData( Control *browse, void *data )
{
   OknoFaktur *oDialog = ( OknoFaktur * )data;

   UNUSED( browse );

   DbSelectArea( "FKTHEAD" );

   return DToC( FieldGetD( FieldPos( "DATA_WYSTAWIENIA" ) ), oDialog->cData, sizeof( oDialog->cData ) );
}

static const char *KolumnaNabywca( Control *browse, void *data )
{
   OknoFaktur *oDialog = ( OknoFaktur * )data;

   UNUSED( browse );

   DbSelectArea( "FKTHEAD" );

   return Linia( FieldGetC( FieldPos( "NAZWA_NABYWCY" ) ), oDialog->cNabywca, sizeof( oDialog->cNabywca ) );
}

static double KolumnaKwota( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "FKTHEAD" );

   return FieldGetN( FieldPos( ( const char * )data ) );
}

static double Numer( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "FKTHEAD" );

   return ( double )RecNo();
}

static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgHelp( "<a href=\"" USTAWA_VAT "\">Ustawa o podatku od towarów i usług</a> (art. 106e – dane na fakturze)\n\n"
            "F1\tpomoc\n"
            "F2, Ins\tnowa faktura VAT\n"
            "F3\tpodgląd faktury: nabywca, pozycje, sumy, KSeF\n"
            "F5\twystawienie faktury w trybie offline (offline24, niedostępność lub awaria KSeF) z kodem QR certyfikatu\n"
            "F6\twysłanie faktury do KSeF tokenem z listy tokenów; kopia XML zostaje w katalogu faktur firmy (xml)\n"
            "F7\tsprawdzenie faktury w KSeF i pobranie UPO (katalog upo)\n"
            "F9\twydruk faktury do PDF w katalogu wydruków firmy\n"
            "Strzałki\tpoprzednia / następna faktura\n"
            "Esc\tzamknięcie okna", "Lista faktur" );
}

static void Nowa( Toolbar *toolbar, void *data )
{
   OknoFaktur *oDialog = ( OknoFaktur * )data;

   UNUSED( toolbar );

   FakturaVat( oDialog->okno, oDialog->biuro );
   Stan( oDialog );
}

static void Wstaw( Win *win, void *data )
{
   UNUSED( win );

   Nowa( NULL, data );
}

static void Podglad( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   FakturyPodglad( ( ( OknoFaktur * )data )->okno );
}

static void WystawOffline( Toolbar *toolbar, void *data )
{
   OknoFaktur *oDialog = ( OknoFaktur * )data;

   UNUSED( toolbar );

   Offline( oDialog->okno, oDialog->biuro, oDialog->BRW );
}

static void Wyslij( Toolbar *toolbar, void *data )
{
   OknoFaktur *oDialog = ( OknoFaktur * )data;

   UNUSED( toolbar );

   FakturyWyslij( oDialog->biuro, oDialog->pasekStanu, oDialog->BRW );
}

static void SprawdzKsef( Toolbar *toolbar, void *data )
{
   OknoFaktur *oDialog = ( OknoFaktur * )data;

   UNUSED( toolbar );

   FakturySprawdz( oDialog->biuro, oDialog->pasekStanu, oDialog->BRW );
}

static void Drukuj( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   FakturyDrukuj( ( ( OknoFaktur * )data )->biuro );
}

static void Zamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( OknoFaktur * )data )->okno );
}

static double PodgladNumer( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "FKTROW" );

   return FieldGetN( FieldPos( "NUMER_WIERSZA" ) );
}

static const char *PodgladNazwa( Control *browse, void *data )
{
   OknoPodgladu *oPodglad = ( OknoPodgladu * )data;
   char          cNazwa[ 2049 ];
   xBool         lKor;

   UNUSED( browse );

   DbSelectArea( "FKTHEAD" );
   lKor = IIF( strncmp( FieldGetC( FieldPos( "RODZAJ_FAKTURY" ) ), "KOR", 3 ) == 0, T, F );
   DbSelectArea( "FKTROW" );
   Linia( FieldGetC( FieldPos( "NAZWA_TOWARU" ) ), cNazwa, sizeof( cNazwa ) );
   StringFormat( oPodglad->cNazwa, "%s%s", IIF( FieldGetL( FieldPos( "STAN_PRZED" ) ), "[przed] ", IIF( lKor, "[po] ", "" ) ), cNazwa );

   return oPodglad->cNazwa;
}

static const char *PodgladIlosc( Control *browse, void *data )
{
   OknoPodgladu *oPodglad = ( OknoPodgladu * )data;

   UNUSED( browse );

   DbSelectArea( "FKTROW" );

   return WydrukIlosc( FieldGetN( FieldPos( "ILOSC" ) ), oPodglad->cTekst, sizeof( oPodglad->cTekst ) );
}

static double PodgladKwota( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "FKTROW" );

   return FieldGetN( FieldPos( ( const char * )data ) );
}

static const char *PodgladPole( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "FKTROW" );

   return FieldGetC( FieldPos( ( const char * )data ) );
}

static void PodgladZamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( Win * )data );
}

static xBool DrukPdf( Biuro *biuro, const char *cPlik, char *cBlad, int nRozmiar )
{
   static const double aKolumna[]   = { 40, 60, 245, 290, 320, 380, 440, 470, 515, 555 };
   static const char  *aTytul[]     = { "Lp", "Nazwa towaru / usługi", "Ilość", "J.m.", "Cena netto", "Netto", "VAT", "Kwota VAT", "Brutto" };
   static const xBool  aPrawo[]     = { T, F, T, F, T, T, T, T, T };
   static const double aSumy[]      = { 320, 440, 470, 515, 555 };
   static const xBool  aSumyPrawo[] = { F, T, T, T };

   HPDF_Doc   pPdf     = HPDF_New( NULL, NULL );
   HPDF_Font  pFont;
   HPDF_Font  pBold;
   HPDF_Page  pPage;
   Stawka     aStawki[ 16 ];
   int        nStawek  = 0;
   int        nStrona  = 1;
   int        nSekcja  = 0;
   int        nStawka;
   double     nY;
   double     nZnak;
   double     nBrutto;
   xBool      lKor;
   xBool      lZapis;
   const char *aSumyTytul[ 4 ];
   char       cKlucz[ 16 ];
   char       cJest[ 16 ];
   char       cRodzaj[ 8 ];
   char       cTytul[ 300 ];
   char       cTekst[ 2100 ];
   char       cNazwa[ 2100 ];
   char       cAdres1[ 600 ];
   char       cAdres2[ 300 ];
   char       cNip[ 64 ];
   char       cKwota[ 32 ];
   char       cStawka[ 8 ];
   char       cUlica[ 255 ];
   char       cDom[ 37 ];
   char       cLokal[ 41 ];
   char       cKod[ 9 ];
   char       cMiasto[ 225 ];
   char       cQr[ 400 ];
   char       cKodII[ 1200 ];

   DbSelectArea( "FKTHEAD" );
   StringFormat( cRodzaj, "%s", FieldGetC( FieldPos( "RODZAJ_FAKTURY" ) ) );
   RTrim( cRodzaj );
   lKor = IIF( strcmp( cRodzaj, "KOR" ) == 0, T, F );
   KluczFaktury( cRodzaj, ( int )FieldGetN( FieldPos( "KOLEJNY_NUMER" ) ), cKlucz, sizeof( cKlucz ) );
   StringFormat( cTytul, "%s%s", IIF( lKor, "Faktura korygująca ", "Faktura VAT " ), Linia( FieldGetC( FieldPos( "NUMER_FAKTURY" ) ), cTekst, sizeof( cTekst ) ) );

   HPDF_SetCompressionMode( pPdf, HPDF_COMP_ALL );
   HPDF_UseUTFEncodings( pPdf );
   HPDF_SetCurrentEncoder( pPdf, "UTF-8" );
   HPDF_SetInfoAttr( pPdf, HPDF_INFO_TITLE, cTytul );
   HPDF_SetInfoAttr( pPdf, HPDF_INFO_CREATOR, "xBiuro" );
   pFont = WydrukCzcionka( pPdf, "arial.ttf" );
   pBold = WydrukCzcionka( pPdf, "arialbd.ttf" );
   if( pFont == NULL || pBold == NULL )
   {
      HPDF_Free( pPdf );
      StringFormatSize( cBlad, ( size_t )nRozmiar, "Brak czcionki Arial w katalogu %s\\Fonts.", GetEnv( "WINDIR" ) );

      return F;
   }

   pPage = DrukStrona( pPdf, pFont );

   WydrukPrawo( pPage, pBold, 16, 555, 800, cTytul );
   StringFormat( cTekst, "Data wystawienia: %s", DToC( FieldGetD( FieldPos( "DATA_WYSTAWIENIA" ) ), cKwota, sizeof( cKwota ) ) );
   WydrukPrawo( pPage, pFont, 9, 555, 784, cTekst );
   nY = 772;
   if( !lKor )
   {
      StringFormat( cTekst, "Data sprzedaży: %s", DToC( FieldGetD( FieldPos( "DATA_DOSTAWY" ) ), cKwota, sizeof( cKwota ) ) );
      WydrukPrawo( pPage, pFont, 9, 555, nY, cTekst );
      nY -= 12;
   }
   StringFormat( cTekst, "%s", FieldGetC( FieldPos( "NUMER_KSEF" ) ) );
   if( *AllTrim( cTekst ) != 0 )
   {
      StringFormat( cNazwa, "Numer KSeF: %s", cTekst );
      WydrukPrawo( pPage, pFont, 9, 555, nY, cNazwa );
   }

   DbSelectArea( "SPRZEDAWCA" );
   StringFormat( cUlica, "%s", FieldGetC( FieldPos( "ULICA" ) ) );
   StringFormat( cDom, "%s", FieldGetC( FieldPos( "NR_DOMU" ) ) );
   StringFormat( cLokal, "%s", FieldGetC( FieldPos( "NR_LOKALU" ) ) );
   StringFormat( cKod, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY" ) ) );
   StringFormat( cMiasto, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
   StringFormat( cNip, "%s", FieldGetC( FieldPos( "NIP" ) ) );
   AllTrim( cUlica );
   AllTrim( cDom );
   AllTrim( cLokal );
   AllTrim( cKod );
   AllTrim( cMiasto );
   AllTrim( cNip );
   StringFormat( cAdres1, "%s %s%s%s", cUlica, cDom, IIF( *cLokal == 0, "", "/" ), cLokal );
   StringFormat( cAdres2, "%s %s", cKod, cMiasto );
   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "PELNA_NAZWA" ) ) );
   WydrukStrona( pPage, pFont, pBold, 40, 730, "Sprzedawca", cNazwa, AllTrim( cAdres1 ), AllTrim( cAdres2 ), cNip );

   DbSelectArea( "FKTHEAD" );
   StringFormat( cAdres1, "%s", FieldGetC( FieldPos( "ULICA_NABYWCY" ) ) );
   StringFormat( cKod, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY_NABYWCY" ) ) );
   StringFormat( cMiasto, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC_NABYWCY" ) ) );
   StringFormat( cAdres2, "%s %s", AllTrim( cKod ), AllTrim( cMiasto ) );
   StringFormat( cTekst, "%s", FieldGetC( FieldPos( "NIP_NABYWCY" ) ) );
   StringFormat( cKwota, "%s", FieldGetC( FieldPos( "KOD_KRAJU_NABYWCY" ) ) );
   AllTrim( cTekst );
   StringFormat( cNip, "%s%s", IIF( *cTekst == 0 || strcmp( cKwota, "PL" ) == 0, "", cKwota ), cTekst );
   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_NABYWCY" ) ) );
   WydrukStrona( pPage, pFont, pBold, 310, 730, "Nabywca", cNazwa, AllTrim( cAdres1 ), AllTrim( cAdres2 ), cNip );

   if( lKor )
   {
      StringFormat( cAdres1, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY_KORYGOWANEJ" ) ) );
      StringFormat( cAdres2, "%s", FieldGetC( FieldPos( "NUMER_KSEF_FAKTURY_KORYGOWANEJ" ) ) );
      AllTrim( cAdres1 );
      AllTrim( cAdres2 );
      StringFormat( cTekst, "Korekta faktury %s z dnia %s%s%s", cAdres1, DToC( FieldGetD( FieldPos( "DATA_FAKTURY_KORYGOWANEJ" ) ), cKwota, sizeof( cKwota ) ), IIF( *cAdres2 == 0, "", ", numer KSeF " ), cAdres2 );
      WydrukTekst( pPage, pFont, 9, 40, 632, cTekst, 515 );
      StringFormat( cTekst, "Przyczyna korekty: %s", Linia( FieldGetC( FieldPos( "PRZYCZYNA_KOREKTY" ) ), cNazwa, sizeof( cNazwa ) ) );
      WydrukTekst( pPage, pFont, 9, 40, 619, cTekst, 515 );
      nY = 596;
   }
   else
      nY = 610;

   WydrukPasek( pPage, pBold, nY, aKolumna, 10, aTytul, aPrawo );
   nY -= 17;
   HPDF_Page_SetRGBStroke( pPage, 0.75f, 0.75f, 0.75f );
   HPDF_Page_SetLineWidth( pPage, 0.5f );

   DbSelectArea( "FKTROW" );
   DbSeek( cKlucz, F );
   for( ; !Eof(); DbSkip( 1 ) )
   {
      Klucz( cJest, sizeof( cJest ), NULL );
      if( strcmp( cJest, cKlucz ) != 0 )
         break;

      if( nY < 120 )
      {
         StringFormat( cTekst, "Strona %d", nStrona );
         WydrukPrawo( pPage, pFont, 8, 555, 40, cTekst );
         pPage = DrukStrona( pPdf, pFont );
         ++nStrona;
         StringFormat( cTekst, "%s (ciąg dalszy)", cTytul );
         WydrukPrawo( pPage, pBold, 12, 555, 800, cTekst );
         nY = 770;
         WydrukPasek( pPage, pBold, nY, aKolumna, 10, aTytul, aPrawo );
         nY -= 17;
         HPDF_Page_SetRGBStroke( pPage, 0.75f, 0.75f, 0.75f );
         HPDF_Page_SetLineWidth( pPage, 0.5f );
      }

      if( lKor && nSekcja != IIF( FieldGetL( FieldPos( "STAN_PRZED" ) ), 1, 2 ) )
      {
         nSekcja = IIF( FieldGetL( FieldPos( "STAN_PRZED" ) ), 1, 2 );
         WydrukTekst( pPage, pBold, 8, aKolumna[ 1 ] + 3, nY, IIF( nSekcja == 1, "Przed korektą", "Po korekcie" ), 200 );
         nY -= 15;
      }

      StringFormat( cTekst, "%d", ( int )FieldGetN( FieldPos( "NUMER_WIERSZA" ) ) );
      WydrukPrawo( pPage, pFont, 8, aKolumna[ 1 ] - 3, nY, cTekst );
      WydrukTekst( pPage, pFont, 8, aKolumna[ 1 ] + 3, nY, Linia( FieldGetC( FieldPos( "NAZWA_TOWARU" ) ), cNazwa, sizeof( cNazwa ) ), aKolumna[ 2 ] - aKolumna[ 1 ] - 6 );
      WydrukPrawo( pPage, pFont, 8, aKolumna[ 3 ] - 3, nY, WydrukIlosc( FieldGetN( FieldPos( "ILOSC" ) ), cKwota, sizeof( cKwota ) ) );
      StringFormat( cTekst, "%s", FieldGetC( FieldPos( "MIARA" ) ) );
      WydrukTekst( pPage, pFont, 8, aKolumna[ 3 ] + 3, nY, AllTrim( cTekst ), aKolumna[ 4 ] - aKolumna[ 3 ] - 6 );
      WydrukPrawo( pPage, pFont, 8, aKolumna[ 5 ] - 3, nY, WydrukKwota( FieldGetN( FieldPos( "CENA_JEDNOSTKOWA_NETTO" ) ), cKwota, sizeof( cKwota ) ) );
      WydrukPrawo( pPage, pFont, 8, aKolumna[ 6 ] - 3, nY, WydrukKwota( FieldGetN( FieldPos( "WARTOSC_NETTO" ) ), cKwota, sizeof( cKwota ) ) );
      StringFormat( cStawka, "%s", FieldGetC( FieldPos( "STAWKA_PODATKU" ) ) );
      AllTrim( cStawka );
      StringFormat( cTekst, "%s%s", cStawka, IIF( *cStawka >= '0' && *cStawka <= '9', "%", "" ) );
      WydrukPrawo( pPage, pFont, 8, aKolumna[ 7 ] - 3, nY, cTekst );
      WydrukPrawo( pPage, pFont, 8, aKolumna[ 8 ] - 3, nY, WydrukKwota( FieldGetN( FieldPos( "KWOTA_PODATKU" ) ), cKwota, sizeof( cKwota ) ) );
      WydrukPrawo( pPage, pFont, 8, aKolumna[ 9 ] - 3, nY, WydrukKwota( FieldGetN( FieldPos( "WARTOSC_BRUTTO" ) ), cKwota, sizeof( cKwota ) ) );
      HPDF_Page_MoveTo( pPage, ( HPDF_REAL )aKolumna[ 0 ], ( HPDF_REAL )( nY - 5 ) );
      HPDF_Page_LineTo( pPage, ( HPDF_REAL )aKolumna[ 9 ], ( HPDF_REAL )( nY - 5 ) );
      HPDF_Page_Stroke( pPage );

      for( nStawka = 0; nStawka < nStawek && strcmp( aStawki[ nStawka ].cStawka, cStawka ) != 0; ++nStawka )
      {
      }
      if( nStawka == nStawek && nStawek < 16 )
      {
         StringFormat( aStawki[ nStawek ].cStawka, "%s", cStawka );
         aStawki[ nStawek ].aSuma[ 0 ] = aStawki[ nStawek ].aSuma[ 1 ] = aStawki[ nStawek ].aSuma[ 2 ] = 0;
         ++nStawek;
      }
      nZnak = IIF( FieldGetL( FieldPos( "STAN_PRZED" ) ), -1, 1 );
      aStawki[ nStawka ].aSuma[ 0 ] += nZnak * FieldGetN( FieldPos( "WARTOSC_NETTO" ) );
      aStawki[ nStawka ].aSuma[ 1 ] += nZnak * FieldGetN( FieldPos( "KWOTA_PODATKU" ) );
      aStawki[ nStawka ].aSuma[ 2 ] += nZnak * FieldGetN( FieldPos( "WARTOSC_BRUTTO" ) );

      nY -= 15;
   }

   nY -= 8;
   aSumyTytul[ 0 ] = IIF( lKor, "Zmiana", "Razem" );
   aSumyTytul[ 1 ] = "VAT";
   aSumyTytul[ 2 ] = "Kwota VAT";
   aSumyTytul[ 3 ] = "Brutto";
   WydrukPasek( pPage, pBold, nY, aSumy, 5, aSumyTytul, aSumyPrawo );
   nY -= 15;
   for( nStawka = 0; nStawka < nStawek; ++nStawka )
   {
      WydrukPrawo( pPage, pFont, 8, 437, nY, WydrukKwota( aStawki[ nStawka ].aSuma[ 0 ], cKwota, sizeof( cKwota ) ) );
      StringFormat( cTekst, "%s%s", aStawki[ nStawka ].cStawka, IIF( *aStawki[ nStawka ].cStawka >= '0' && *aStawki[ nStawka ].cStawka <= '9', "%", "" ) );
      WydrukPrawo( pPage, pFont, 8, 467, nY, cTekst );
      WydrukPrawo( pPage, pFont, 8, 512, nY, WydrukKwota( aStawki[ nStawka ].aSuma[ 1 ], cKwota, sizeof( cKwota ) ) );
      WydrukPrawo( pPage, pFont, 8, 552, nY, WydrukKwota( aStawki[ nStawka ].aSuma[ 2 ], cKwota, sizeof( cKwota ) ) );
      nY -= 13;
   }

   DbSelectArea( "FKTHEAD" );
   nBrutto = FieldGetN( FieldPos( "KWOTA_NALEZNOSCI" ) );
   WydrukPrawo( pPage, pBold, 8, 437, nY, WydrukKwota( FieldGetN( FieldPos( "WARTOSC_NETTO" ) ), cKwota, sizeof( cKwota ) ) );
   WydrukPrawo( pPage, pBold, 8, 467, nY, "razem" );
   WydrukPrawo( pPage, pBold, 8, 512, nY, WydrukKwota( FieldGetN( FieldPos( "KWOTA_PODATKU" ) ), cKwota, sizeof( cKwota ) ) );
   WydrukPrawo( pPage, pBold, 8, 552, nY, WydrukKwota( nBrutto, cKwota, sizeof( cKwota ) ) );

   nY -= 30;
   StringFormat( cTekst, "%s%s zł", IIF( nBrutto < 0, "Do zwrotu: ", "Do zapłaty: " ), WydrukKwota( IIF( nBrutto < 0, -nBrutto, nBrutto ), cKwota, sizeof( cKwota ) ) );
   WydrukTekst( pPage, pBold, 12, 40, nY, cTekst, 515 );
   nY -= 15;
   StringFormat( cTekst, "Słownie: %s", NToW( IIF( nBrutto < 0, -nBrutto, nBrutto ), cNazwa, sizeof( cNazwa ) ) );
   WydrukTekst( pPage, pFont, 9, 40, nY, cTekst, 515 );

   if( !lKor )
   {
      nY -= 18;
      StringFormat( cTekst, "Forma płatności: %s", WydrukFormaPlatnosci( ( int )FieldGetN( FieldPos( "FORMA_PLATNOSCI" ) ) ) );
      if( FieldGetD( FieldPos( "TERMIN_PLATNOSCI" ) ) != 0 )
      {
         StringFormat( cNazwa, "%s     Termin płatności: %s", cTekst, DToC( FieldGetD( FieldPos( "TERMIN_PLATNOSCI" ) ), cKwota, sizeof( cKwota ) ) );
         StringFormat( cTekst, "%s", cNazwa );
      }
      WydrukTekst( pPage, pFont, 9, 40, nY, cTekst, 515 );

      StringFormat( cAdres2, "%s", FieldGetC( FieldPos( "NUMER_RACHUNKU" ) ) );
      if( *AllTrim( cAdres2 ) != 0 )
      {
         nY -= 13;
         StringFormat( cAdres1, "%s", FieldGetC( FieldPos( "NAZWA_BANKU" ) ) );
         StringFormat( cTekst, "Konto: %s  %s", AllTrim( cAdres1 ), cAdres2 );
         WydrukTekst( pPage, pFont, 9, 40, nY, cTekst, 515 );
      }
   }

   for( nStawka = 0; nStawka < nStawek && strcmp( aStawki[ nStawka ].cStawka, "zw" ) != 0; ++nStawka )
   {
   }
   Linia( FieldGetC( FieldPos( "PODSTAWA_ZWOLNIENIA" ) ), cNazwa, sizeof( cNazwa ) );
   if( nStawka < nStawek && *cNazwa != 0 )
   {
      nY -= 13;
      StringFormat( cTekst, "Podstawa zwolnienia z VAT: %s", cNazwa );
      WydrukTekst( pPage, pFont, 9, 40, nY, cTekst, 515 );
   }

   if( *WydrukQrAdres( biuro, cQr, sizeof( cQr ) ) != 0 )
   {
      if( nY < 190 )
      {
         StringFormat( cTekst, "Strona %d", nStrona );
         WydrukPrawo( pPage, pFont, 8, 555, 40, cTekst );
         pPage = DrukStrona( pPdf, pFont );
         ++nStrona;
         nY = 800;
      }
      WydrukQr( pPage, cQr, 40, nY - 120, 96 );
      StringFormat( cTekst, "%s", FieldGetC( FieldPos( "NUMER_KSEF" ) ) );
      WydrukTekst( pPage, pFont, 7, 40, nY - 130, IIF( *AllTrim( cTekst ) == 0, "OFFLINE", cTekst ), 200 );
      if( *OfflineKodII( biuro, cKodII, sizeof( cKodII ) ) != 0 )
      {
         WydrukQr( pPage, cKodII, 435, nY - 144, 120 );
         WydrukTekst( pPage, pFont, 7, 435, nY - 154, "CERTYFIKAT", 120 );
      }
   }

   if( nStrona > 1 )
   {
      StringFormat( cTekst, "Strona %d", nStrona );
      WydrukPrawo( pPage, pFont, 8, 555, 40, cTekst );
   }

   lZapis = IIF( HPDF_SaveToFile( pPdf, cPlik ) == HPDF_OK, T, F );
   HPDF_Free( pPdf );
   if( !lZapis )
      StringFormatSize( cBlad, ( size_t )nRozmiar, "Nie można zapisać pliku %s.\nPlik może być otwarty w innym programie.", cPlik );

   return lZapis;
}

static HPDF_Page DrukStrona( HPDF_Doc pPdf, HPDF_Font pFont )
{
   HPDF_Page pPage = HPDF_AddPage( pPdf );

   HPDF_Page_SetSize( pPage, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT );
   WydrukUstawZnaki( pPage, pFont );

   return pPage;
}

static const char *WydrukFormaPlatnosci( int nForma )
{
   if( nForma == PLATNOSC_GOTOWKA )
      return "gotówka";

   if( nForma == PLATNOSC_KARTA )
      return "karta";

   return "przelew";
}

static char *WydrukQrAdres( Biuro *biuro, char *cAdres, int nRozmiar )
{
   Bytes  skrot;
   char  *cXml;
   size_t nXml;
   int    nSrodowisko;
   int    dData;
   char   cPlik[ 400 ];
   char   cSkrot[ 64 ];
   char   cNip[ 64 ];

   *cAdres = 0;
   DbSelectArea( "FKTHEAD" );
   if( FieldGetN( FieldPos( "STATUS_KSEF" ) ) != KSEF_PRZYJETA && FieldGetN( FieldPos( "TRYB_WYSTAWIENIA" ) ) == 0 )
      return cAdres;

   cXml = MemoRead( FakturyPlik( biuro, "xml", ".xml", cPlik, sizeof( cPlik ) ), &nXml );
   if( cXml == NULL || nXml == 0 )
   {
      free( cXml );

      return cAdres;
   }
   skrot = HashData( "SHA256", cXml, nXml );
   free( cXml );
   FakturyBase64Url( skrot.data, skrot.size, cSkrot, sizeof( cSkrot ) );
   BytesFree( &skrot );

   DbSelectArea( "SPRZEDAWCA" );
   FirmaCyfry( FieldGetC( FieldPos( "NIP" ) ), cNip, sizeof( cNip ) );
   nSrodowisko = FakturySrodowiskoKsef( IIF( FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ) == SRODOWISKO_PRODUKCYJNE, SRODOWISKO_PRODUKCYJNE, SRODOWISKO_TESTOWE ) );
   DbSelectArea( "FKTHEAD" );
   dData       = FieldGetD( FieldPos( "DATA_WYSTAWIENIA" ) );
   StringFormatSize( cAdres, ( size_t )nRozmiar, "%s/invoice/%s/%02d-%02d-%04d/%s", IIF( nSrodowisko == SRODOWISKO_PRODUKCYJNE, "https://qr.ksef.mf.gov.pl", "https://qr-test.ksef.mf.gov.pl" ), cNip, dData % 100, dData / 100 % 100, dData / 10000, cSkrot );

   return cAdres;
}

static char *Linia( const char *cTekst, char *cWynik, int nRozmiar )
{
   char cBezCr[ 2100 ];

   StrTran( cBezCr, sizeof( cBezCr ), cTekst, "\r\n", " " );
   StrTran( cWynik, ( size_t )nRozmiar, cBezCr, "\n", " " );

   return AllTrim( cWynik );
}

static void Opis( Win *strona, int x, int y, const char *tekst )
{
   SayCreate( strona, x, y, 0, 20, tekst );
}

static Control *Pole( Win *strona, int x, int y, int dlugosc, const char *tekst )
{
   Control *pole = SayCreate( strona, x, y, dlugosc, 24, tekst );

   LabelSetBorder( pole, T );

   return pole;
}

static void Przyjeta( const char *cSesja, const char *cFaktura, void *data )
{
   char cNumer[ 300 ];
   char cTekst[ 900 ];

   DbSelectArea( "FKTHEAD" );
   if( NetRLock( NET_WAIT ) )
   {
      FieldPutC( FieldPos( "NUMER_SESJI_KSEF" ), cSesja );
      FieldPutC( FieldPos( "NUMER_REFERENCYJNY_KSEF" ), cFaktura );
      FieldPutN( FieldPos( "STATUS_KSEF" ), KSEF_W_TOKU );
      FieldPutN( FieldPos( "SRODOWISKO_KSEF" ), *( int * )data );
      DbCommit();
      DbUnlock();
   }
   else
   {
      StringFormat( cNumer, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY" ) ) );
      StringFormat( cTekst, "Faktura %s dotarła do KSeF, ale nie udało się zapisać tego w bazie.\nNumer sesji: %s\nNumer faktury w sesji: %s\n\nZanotuj te numery i nie wysyłaj faktury ponownie klawiszem F6.", AllTrim( cNumer ), cSesja, cFaktura );
      MsgError( cTekst, "KSeF" );
   }
}

static char *Token( const char *cNip, int nSrodowisko )
{
   char *cToken = TokenyPobierz( cNip, nSrodowisko );
   char  cTekst[ 400 ];

   if( cToken == NULL )
   {
      StringFormat( cTekst, "Brak ważnego tokenu KSeF dla NIP %s i środowiska firmy (menu KSeF, Środowisko KSeF).\nDodaj go w menu KSeF, Tokeny / Certyfikaty: Generuj token albo Lista tokenów.", cNip );
      MsgInfo( cTekst, "KSeF" );
   }

   return cToken;
}

static void Wynik( Biuro *biuro, Control *browse, WynikKsef *wynik, const char *cBlad )
{
   const char *cNumer;
   const char *cData;
   const char *cOpisKsef;
   char        cOpis[ 2000 ];
   char        cPlik[ 400 ] = "";
   char        cTekst[ 3000 ];
   int         nKod;
   xBool       lZapis;
   xBool       lUpo = F;

   DbSelectArea( "FKTHEAD" );
   lZapis = NetRLock( NET_WAIT );

   if( wynik == NULL )
   {
      if( lZapis )
      {
         StrTran( cOpis, sizeof( cOpis ), cBlad, "\n", " " );
         FieldPutC( FieldPos( "OPIS_STATUSU_KSEF" ), cOpis );
         DbCommit();
         DbUnlock();
      }
      StringFormat( cTekst, "%s%s", cBlad, IIF( FieldGetN( FieldPos( "STATUS_KSEF" ) ) == KSEF_W_TOKU, "\n\nFaktura dotarła do KSeF. Sprawdź ją za chwilę klawiszem F7.", "" ) );
      MsgError( cTekst, "KSeF" );
      BrowseRefresh( browse );

      return;
   }

   nKod = KsefKod( wynik->json );
   if( nKod == 200 )
   {
      cNumer    = JsonString( JsonGet( wynik->json, "ksefNumber" ) );
      cData     = JsonString( JsonGet( wynik->json, "acquisitionDate" ) );
      cOpisKsef = JsonString( JsonGet( JsonGet( wynik->json, "status" ), "description" ) );
      if( lZapis )
      {
         FieldPutC( FieldPos( "NUMER_SESJI_KSEF" ), wynik->cSesja );
         FieldPutC( FieldPos( "NUMER_REFERENCYJNY_KSEF" ), wynik->cFaktura );
         FieldPutN( FieldPos( "STATUS_KSEF" ), KSEF_PRZYJETA );
         FieldPutC( FieldPos( "NUMER_KSEF" ), cNumer );
         FieldPutC( FieldPos( "DATA_PRZYJECIA_KSEF" ), IIF( cData == NULL, "", cData ) );
         FieldPutC( FieldPos( "OPIS_STATUSU_KSEF" ), IIF( cOpisKsef == NULL, "", cOpisKsef ) );
         DbCommit();
         DbUnlock();
      }

      if( wynik->cUpo != NULL )
         lUpo = MemoWrit( FakturyPlik( biuro, "upo", ".xml", cPlik, sizeof( cPlik ) ), wynik->cUpo, wynik->nUpo );

      StringFormat( cOpis, "UPO: %s", cPlik );
      StringFormat( cTekst, "Faktura przyjęta w KSeF.\nNumer KSeF: %s\n%s%s", cNumer, IIF( lUpo, cOpis, "UPO jeszcze niedostępne, pobierz je później klawiszem F7." ), IIF( lZapis, "", "\n\nNie udało się zapisać numeru KSeF w bazie.\nZanotuj go i sprawdź fakturę ponownie klawiszem F7." ) );
      MsgInfo( cTekst, "KSeF" );
   }
   else
   {
      KsefOpisStatusu( wynik->json, cOpis, sizeof( cOpis ) );
      if( lZapis )
      {
         FieldPutC( FieldPos( "NUMER_SESJI_KSEF" ), wynik->cSesja );
         FieldPutC( FieldPos( "NUMER_REFERENCYJNY_KSEF" ), wynik->cFaktura );
         FieldPutN( FieldPos( "STATUS_KSEF" ), IIF( nKod == 100 || nKod == 150, KSEF_W_TOKU, KSEF_ODRZUCONA ) );
         StrTran( cTekst, sizeof( cTekst ), cOpis, "\n", " " );
         FieldPutC( FieldPos( "OPIS_STATUSU_KSEF" ), cTekst );
         DbCommit();
         DbUnlock();
      }

      if( nKod == 100 || nKod == 150 )
      {
         if( lZapis )
            StringFormat( cTekst, "KSeF jeszcze przetwarza fakturę.\nSprawdź ją za chwilę klawiszem F7." );
         else
            StringFormat( cTekst, "KSeF jeszcze przetwarza fakturę.\nSprawdź ją za chwilę klawiszem F7.\n\nNie udało się zapisać stanu w bazie.\nNumer sesji: %s\nNumer faktury w sesji: %s", wynik->cSesja, wynik->cFaktura );
         MsgInfo( cTekst, "KSeF" );
      }
      else
      {
         StringFormat( cTekst, "%s%s", cOpis, IIF( lZapis, "", "\n\nNie udało się zapisać odpowiedzi KSeF w bazie." ) );
         MsgError( cTekst, "KSeF" );
      }
   }
   BrowseRefresh( browse );
}
