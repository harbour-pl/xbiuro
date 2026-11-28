/*
 * Program...: xBiuro
 * Plik......: src/biuro/przelew.c
 *
 */

#include "biuro.h"

static const DbField aDbf[] = {
   { "NAZWA_ODBIORCY",             'C', 108, 0 },
   { "NAZWA_ODBIORCY_CD",          'C', 108, 0 },
   { "ULICA_ODBIORCY",             'C', 108, 0 },
   { "KOD_POCZTOWY_ODBIORCY",      'C',   8, 0 },
   { "MIEJSCOWOSC_ODBIORCY",       'C', 108, 0 },
   { "NR_RACHUNKU_ODBIORCY",       'C', 104, 0 },
   { "KWOTA",                      'N',  12, 2 },
   { "NR_RACHUNKU_ZLECENIODAWCY",  'C', 104, 0 },
   { "NAZWA_ZLECENIODAWCY",        'C', 108, 0 },
   { "NAZWA_ZLECENIODAWCY_CD",     'C', 108, 0 },
   { "ULICA_ZLECENIODAWCY",        'C', 108, 0 },
   { "KOD_POCZTOWY_ZLECENIODAWCY", 'C',   8, 0 },
   { "MIEJSCOWOSC_ZLECENIODAWCY",  'C', 108, 0 },
   { "TYTULEM",                    'C', 108, 0 },
   { "TYTULEM_CD",                 'C', 108, 0 },
   { "DATA_WPLATY",                'D',   8, 0 },
   { "DRUKUJ_DATE",                'L',   1, 0 }    /* data wpłaty na druku */
};

#define POLA_PRZELEWU ( ( int )( sizeof( aDbf ) / sizeof( aDbf[ 0 ] ) ) )

/* Maska rachunku na druku: dwie cyfry kontrolne i sześć grup po cztery */
#define MASKA_RACHUNKU "99 9999 9999 9999 9999 9999 9999"

/* Okno przelewów: kontrolki nazwane jak pola tabeli; zmienna lokalna Przelew(), przekazywana parametrem jak aSet */
typedef struct
{
   Biuro     *biuro;
   Win       *okno;
   StatusBar *pasekStanu;
   Win       *panel;
   Control   *BRW;
   Control   *NAZWA_ODBIORCY;
   Control   *NAZWA_ODBIORCY_CD;
   Control   *ULICA_ODBIORCY;
   Control   *KOD_POCZTOWY_ODBIORCY;
   Control   *MIEJSCOWOSC_ODBIORCY;
   Control   *NR_RACHUNKU_ODBIORCY;
   Control   *KWOTA;
   Control   *NR_RACHUNKU_ZLECENIODAWCY;
   Control   *NAZWA_ZLECENIODAWCY;
   Control   *NAZWA_ZLECENIODAWCY_CD;
   Control   *ULICA_ZLECENIODAWCY;
   Control   *KOD_POCZTOWY_ZLECENIODAWCY;
   Control   *MIEJSCOWOSC_ZLECENIODAWCY;
   Control   *TYTULEM;
   Control   *TYTULEM_CD;
   Control   *DATA_WPLATY;
   Control   *DRUKUJ_DATE;
   char       cData[ 16 ];
} OknoPrzelewu;

/* Wybór odbiorcy F12 przy wpisywaniu: okno i pola GET odbiorcy */
typedef struct
{
   OknoPrzelewu *oOkno;
   Control      *oNazwa;
   Control      *oNazwaCd;
   Control      *oUlica;
   Control      *oKod;
   Control      *oMiejscowosc;
   Control      *oRachunek;
} WyborOdbiorcy;

static xBool       PrzelewDbOpen( const char *cBaza );
static void        Przebudowa( const char *cBaza );
static void        Stan( OknoPrzelewu *oOkno );
static const char *Kolumna( Control *browse, void *data );
static const char *KolumnaData( Control *browse, void *data );
static double      KolumnaKwota( Control *browse, void *data );
static double      Numer( Control *browse, void *data );
static void        Zmiana( Control *browse, void *data );
static void        Okno( OknoPrzelewu *oOkno );
static void        Pomoc( Toolbar *toolbar, void *data );
static void        Nowy( OknoPrzelewu *oOkno, Tryb nTryb );
static void        Dodaj( Toolbar *toolbar, void *data );
static void        Wstaw( Win *win, void *data );
static void        Kopia( Win *win, void *data );
static void        Popraw( Toolbar *toolbar, void *data );
static void        Usun( Toolbar *toolbar, void *data );
static void        Drukuj( Toolbar *toolbar, void *data );
static xBool       DrukujDoPDF( HPDF_Doc pPdf, HPDF_Font pFont, const char *cPlik );
static void        Zamknij( Toolbar *toolbar, void *data );
static void        WybierzOdbiorce( Win *win, void *data );
static void        Dane( char *cNazwa, char *cNazwaCd, char *cUlica, char *cKod, char *cMiejscowosc, char *cRachunek, int nRozmiar );
static char       *Nazwa( const char *cKto, char *cWynik, int nRozmiar );
static char       *Adres( const char *cKto, char *cWynik, int nRozmiar );
static char       *Rachunek( const char *cTekst, char *cWynik, int nRozmiar );
static char       *Kwota( double nKwota, char *cWynik, int nRozmiar );
static void        Ramka( Win *strona, int x, int y, int szerokosc, int wysokosc, const char *podpis );
static void        Opis( Win *strona, int x, int y, const char *tekst );
static Control    *Pole( Win *strona, int x, int y, int dlugosc );
static Control    *Szerokosc( Control *pole, int szerokosc );

// Okno przelewów wybranej firmy z menu Firma, Rozrachunki
// Przelew( win, data ) --> void
void Przelew( Win *win, void *data )
{
   OknoPrzelewu  oknoPrzelewu = { 0 };
   OknoPrzelewu *oOkno        = &oknoPrzelewu;
   Toolbar      *pasek;
   Win          *panel;
   char          cKatalog[ 64 ];
   char          cBaza[ 80 ];
   int           przycisk;
   int           nPol;

   oOkno->biuro = ( Biuro * )data;
   StringFormat( cBaza, "%sprzelew", RokFirma( oOkno->biuro, oOkno->biuro->firma, cKatalog, sizeof( cKatalog ) ) );
   if( !PrzelewDbOpen( cBaza ) )
      return;

   oOkno->okno = DialogCreate( win, "Polecenie przelewu / wpłata gotówkowa", 900, 720 );
   WinSetBackground( oOkno->okno, 0xE9EDF1 );
   WinSetSizable( oOkno->okno, T );
   WinSetKey( oOkno->okno, "F3", Kopia, oOkno );
   WinSetKey( oOkno->okno, "Ins", Wstaw, oOkno );

   oOkno->pasekStanu = StatusBarCreate( oOkno->okno, "" );
   StatusBarAdd( oOkno->pasekStanu, 0, "", F );
   StatusBarAdd( oOkno->pasekStanu, 60, "", F );
   StatusBarSetScoreboard( oOkno->pasekStanu, 2 );

   pasek = ToolbarCreate( oOkno->okno, TOOLBAR_BOTTOM );
   przycisk = ToolbarAdd( pasek, "F1-Pomoc", Pomoc, NULL );
   ToolbarSetShortcut( pasek, przycisk, "F1" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "F2-Dodaj", Dodaj, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "F2" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "F4-Popraw", Popraw, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "F4" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "F8-Usuń", Usun, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "F8" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "F9-Drukuj", Drukuj, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "F9" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "Esc-Zamknij", Zamknij, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "Esc" );

   panel        = PanelCreate( oOkno->okno, 0, 0, 0, 0 );
   oOkno->panel = panel;
   nPol         = WinWidth( panel ) / 2;

   Ramka( panel, 10, 8, nPol - 15, 214, "Odbiorca" );
   Opis( panel, 20,  28, "Nazwa:" );
   Opis( panel, 20, 111, "Adres:" );
   Opis( panel, 20, 168, "Nr rachunku:" );
   oOkno->NAZWA_ODBIORCY        = Szerokosc( Pole( panel, 20, 48, 36 ), nPol - 35 );
   oOkno->NAZWA_ODBIORCY_CD     = Szerokosc( Pole( panel, 20, 78, 36 ), nPol - 35 );
   oOkno->ULICA_ODBIORCY        = Szerokosc( Pole( panel, 92, 108, 28 ), nPol - 107 );
   oOkno->KOD_POCZTOWY_ODBIORCY = Pole( panel, 20, 138,  6 );
   oOkno->MIEJSCOWOSC_ODBIORCY  = Szerokosc( Pole( panel, 92, 138, 28 ), nPol - 107 );
   oOkno->NR_RACHUNKU_ODBIORCY  = Pole( panel, 20, 188, 32 );

   Ramka( panel, nPol + 5, 8, nPol - 15, 214, "Zleceniodawca" );
   Opis( panel, nPol + 15,  28, "Nazwa:" );
   Opis( panel, nPol + 15, 111, "Adres:" );
   Opis( panel, nPol + 15, 168, "Nr rachunku:" );
   oOkno->NAZWA_ZLECENIODAWCY        = Szerokosc( Pole( panel, nPol + 15, 48, 36 ), nPol - 35 );
   oOkno->NAZWA_ZLECENIODAWCY_CD     = Szerokosc( Pole( panel, nPol + 15, 78, 36 ), nPol - 35 );
   oOkno->ULICA_ZLECENIODAWCY        = Szerokosc( Pole( panel, nPol + 87, 108, 28 ), nPol - 107 );
   oOkno->KOD_POCZTOWY_ZLECENIODAWCY = Pole( panel, nPol + 15, 138,  6 );
   oOkno->MIEJSCOWOSC_ZLECENIODAWCY  = Szerokosc( Pole( panel, nPol + 87, 138, 28 ), nPol - 107 );
   oOkno->NR_RACHUNKU_ZLECENIODAWCY  = Pole( panel, nPol + 15, 188, 32 );

   Opis( panel, 20, 235, "Kwota:" );
   Opis( panel, 20, 265, "Tytułem:" );
   Opis( panel, 20, 325, "Data wpłaty:" );
   oOkno->KWOTA       = Pole( panel, 175, 232, 12 );
   oOkno->TYTULEM     = Szerokosc( Pole( panel, 175, 262, 27 ), WinWidth( panel ) - 195 );
   oOkno->TYTULEM_CD  = Szerokosc( Pole( panel, 175, 292, 27 ), WinWidth( panel ) - 195 );
   oOkno->DATA_WPLATY = Pole( panel, 175, 322, 10 );
   oOkno->DRUKUJ_DATE = CheckBoxCreate( panel, 290, 324, 150, 20, "Drukuj datę" );
   ControlSetEnabled( oOkno->DRUKUJ_DATE, F );

   oOkno->BRW = BrowseCreate( panel, 10, 356, WinWidth( panel ) - 20, WinHeight( panel ) - 366 );
   ControlSetAnchor( oOkno->BRW, "LTRB" );
   BrowseSetColor( oOkno->BRW, BROWSE_HEADBACKGROUND, 0x4D7326 );
   DbBrowse( oOkno->BRW, "PRZELEW" );
   BrowseAddNumber( oOkno->BRW, "Poz.", 4, "9999", Numer, NULL );
   BrowseSetFreeze( oOkno->BRW, 1 );
   BrowseAddColumn( oOkno->BRW, "Data wpłaty", 10,           BROWSE_LEFT, KolumnaData, oOkno );
   BrowseAddColumn( oOkno->BRW, "Nazwa odbiorcy", 27,        BROWSE_LEFT, Kolumna, "NAZWA_ODBIORCY" );
   BrowseAddNumber( oOkno->BRW, "Kwota", 12, "999999999.99", KolumnaKwota, NULL );
   BrowseAddColumn( oOkno->BRW, "Tytułem", 27,               BROWSE_LEFT, Kolumna, "TYTULEM" );
   BrowseAddColumn( oOkno->BRW, "Nr rachunku odbiorcy", 32,  BROWSE_LEFT, Kolumna, "NR_RACHUNKU_ODBIORCY" );
   ControlSetAction( oOkno->BRW, Zmiana, oOkno );

   DbSelectArea( "PRZELEW" );
   DbGoBottom();
   Okno( oOkno );
   Stan( oOkno );

   WinCenter( oOkno->okno );
   DialogShowModal( oOkno->okno );

   DbSelectArea( "PRZELEW" );
   DbCloseArea();
}

// Otwiera tabelę przelewów; brak pliku zakłada ją, inna liczba pól przebudowuje ją
// PrzelewDbOpen( cBaza ) --> lOtwarta
static xBool PrzelewDbOpen( const char *cBaza )
{
   char cPlik[ 96 ];

   StringFormat( cPlik, "%s.dbf", cBaza );
   if( !FileExists( cPlik ) )
      DbCreate( cBaza, aDbf, POLA_PRZELEWU );

   if( !NetUse( cBaza, F, NET_WAIT, "PRZELEW", F ) )
      return F;

   if( FCount() != POLA_PRZELEWU )
   {
      DbCloseArea();
      MsgInfo( "Dodanie pola w bazie", "Przelewy" );
      Przebudowa( cBaza );
      if( !NetUse( cBaza, F, NET_WAIT, "PRZELEW", F ) )
         return F;
   }

   return T;
}

// Przebudowuje tabelę przelewów do pól aDbf: kopia, stara tabela jako cBaza.tmp, nowa tabela i APPEND FROM z kopii
// Przebudowa( cBaza ) --> void
static void Przebudowa( const char *cBaza )
{
   char cDbf[ 96 ];
   char cTmp[ 96 ];
   char cTmpDbf[ 96 ];
   char cStara[ 96 ];

   StringFormat( cDbf, "%s.dbf", cBaza );
   StringFormat( cTmp, "%s-tmp", cBaza );
   StringFormat( cTmpDbf, "%s.dbf", cTmp );
   StringFormat( cStara, "%s.tmp", cBaza );

   if( FileExists( cDbf ) )
   {
      FileCopy( cDbf, cTmpDbf );
      FRename( cDbf, cStara );
      DbCreate( cBaza, aDbf, POLA_PRZELEWU );
      if( NetUse( cBaza, F, NET_WAIT, "PRZELEW", F ) )
      {
         DbAppendFrom( cTmp, NULL, NULL );
         DbCloseArea();
      }
      FErase( cTmpDbf );
      FErase( cStara );
   }
}

// Przeglądarka włączona tylko z przelewami
// Stan( oOkno ) --> void
static void Stan( OknoPrzelewu *oOkno )
{
   xBool lJest = !DbPusta( "PRZELEW" );

   ControlSetEnabled( oOkno->BRW, lJest );
   BrowseRefresh( oOkno->BRW );

   if( lJest )
      ControlSetFocus( oOkno->BRW );
}

// Kolumna z polem o nazwie data
// Kolumna( browse, data ) --> cTekst
static const char *Kolumna( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "PRZELEW" );

   return FieldGetC( FieldPos( ( const char * )data ) );
}

// Kolumna Data wpłaty
// KolumnaData( browse, data ) --> cTekst
static const char *KolumnaData( Control *browse, void *data )
{
   OknoPrzelewu *oOkno = ( OknoPrzelewu * )data;

   UNUSED( browse );

   DbSelectArea( "PRZELEW" );

   return DToC( FieldGetD( FieldPos( "DATA_WPLATY" ) ), oOkno->cData, sizeof( oOkno->cData ) );
}

// Kolumna Kwota
// KolumnaKwota( browse, data ) --> nKwota
static double KolumnaKwota( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "PRZELEW" );

   return FieldGetN( FieldPos( "KWOTA" ) );
}

// Kolumna Poz.: numer rekordu
// Numer( browse, data ) --> nNumer
static double Numer( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "PRZELEW" );

   return ( double )RecNo();
}

// Belka przeglądarki na innym przelewie
// Zmiana( browse, data ) --> void
static void Zmiana( Control *browse, void *data )
{
   UNUSED( browse );

   Okno( ( OknoPrzelewu * )data );
}

// Pokazuje bieżący przelew
// Okno( oOkno ) --> void
static void Okno( OknoPrzelewu *oOkno )
{
   char cTekst[ 64 ];

   DbSelectArea( "PRZELEW" );

   ControlSetText( oOkno->NAZWA_ODBIORCY,             FieldGetC( FieldPos( "NAZWA_ODBIORCY" ) ) );
   ControlSetText( oOkno->NAZWA_ODBIORCY_CD,          FieldGetC( FieldPos( "NAZWA_ODBIORCY_CD" ) ) );
   ControlSetText( oOkno->ULICA_ODBIORCY,            FieldGetC( FieldPos( "ULICA_ODBIORCY" ) ) );
   ControlSetText( oOkno->KOD_POCZTOWY_ODBIORCY,      FieldGetC( FieldPos( "KOD_POCZTOWY_ODBIORCY" ) ) );
   ControlSetText( oOkno->MIEJSCOWOSC_ODBIORCY,       FieldGetC( FieldPos( "MIEJSCOWOSC_ODBIORCY" ) ) );
   ControlSetText( oOkno->NR_RACHUNKU_ODBIORCY,       FieldGetC( FieldPos( "NR_RACHUNKU_ODBIORCY" ) ) );
   ControlSetText( oOkno->KWOTA,                      IIF( Eof(), "", Kwota( FieldGetN( FieldPos( "KWOTA" ) ), cTekst, sizeof( cTekst ) ) ) );
   ControlSetText( oOkno->NR_RACHUNKU_ZLECENIODAWCY,  FieldGetC( FieldPos( "NR_RACHUNKU_ZLECENIODAWCY" ) ) );
   ControlSetText( oOkno->NAZWA_ZLECENIODAWCY,        FieldGetC( FieldPos( "NAZWA_ZLECENIODAWCY" ) ) );
   ControlSetText( oOkno->NAZWA_ZLECENIODAWCY_CD,     FieldGetC( FieldPos( "NAZWA_ZLECENIODAWCY_CD" ) ) );
   ControlSetText( oOkno->ULICA_ZLECENIODAWCY,       FieldGetC( FieldPos( "ULICA_ZLECENIODAWCY" ) ) );
   ControlSetText( oOkno->KOD_POCZTOWY_ZLECENIODAWCY, FieldGetC( FieldPos( "KOD_POCZTOWY_ZLECENIODAWCY" ) ) );
   ControlSetText( oOkno->MIEJSCOWOSC_ZLECENIODAWCY,  FieldGetC( FieldPos( "MIEJSCOWOSC_ZLECENIODAWCY" ) ) );
   ControlSetText( oOkno->TYTULEM,                    FieldGetC( FieldPos( "TYTULEM" ) ) );
   ControlSetText( oOkno->TYTULEM_CD,                 FieldGetC( FieldPos( "TYTULEM_CD" ) ) );
   ControlSetText( oOkno->DATA_WPLATY,                DToC( FieldGetD( FieldPos( "DATA_WPLATY" ) ), cTekst, sizeof( cTekst ) ) );
   CheckBoxSetChecked( oOkno->DRUKUJ_DATE,            FieldGetL( FieldPos( "DRUKUJ_DATE" ) ) );
}

// F1: pomoc
// Pomoc( toolbar, data ) --> void
static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgHelp( "<a href=\"" WZOR_PRZELEWU "\">Rozporządzenie w sprawie wzoru formularza wpłaty gotówkowej oraz polecenia przelewu</a> (załącznik nr 1 – wzór, załącznik nr 2 – objaśnienia)\n\n"
            "Przeglądanie\n\n"
            "F1\tpomoc\n"
            "F2, Ins\tnowy przelew; zleceniodawca z danych firmy\n"
            "F3\tkopia bieżącego przelewu jako nowy\n"
            "F4\tpoprawienie przelewu\n"
            "F8\tusunięcie przelewu\n"
            "F9\twydruk na druku polecenia przelewu / wpłaty gotówkowej, dwa egzemplarze na kartce\n"
            "Esc\tzamknięcie okna\n\n"
            "Wpisywanie danych\n\n"
            "F12\twybór odbiorcy z kartoteki kontrahentów\n"
            "Enter\tnastępne pole, na ostatnim polu zapis\n"
            "Tab\tnastępne pole, bez zapisu\n"
            "Shift+Tab\tpoprzednie pole\n"
            "PgUp, PgDn\tzapis\n"
            "Esc\tbez zapisu\n\n"
            "Bez rachunku zleceniodawcy (wpłata gotówkowa) druk ma w tym miejscu kwotę słownie.", "Przelewy" );
}

// Nowy przelew, kopia bieżącego albo poprawienie bieżącego; zmienne cNAZWAPOLA jak pola tabeli
// Nowy( oOkno, nTryb ) --> void
static void Nowy( OknoPrzelewu *oOkno, Tryb nTryb )
{
   Win           *panel = oOkno->panel;
   Control       *GetList[ 17 ];
   WyborOdbiorcy  wybor = { oOkno, NULL, NULL, NULL, NULL, NULL, NULL };
   int            nPol  = WinWidth( panel ) / 2;
   int            nGet  = 0;
   long           nRecNo;
   char           cNAZWA_ODBIORCY[ 109 ];
   char           cNAZWA_ODBIORCY_CD[ 109 ];
   char           cULICA_ODBIORCY[ 109 ];
   char           cKOD_POCZTOWY_ODBIORCY[ 109 ];
   char           cMIEJSCOWOSC_ODBIORCY[ 109 ];
   char           cNR_RACHUNKU_ODBIORCY[ 109 ];
   double         nKWOTA;
   char           cNR_RACHUNKU_ZLECENIODAWCY[ 109 ];
   char           cNAZWA_ZLECENIODAWCY[ 109 ];
   char           cNAZWA_ZLECENIODAWCY_CD[ 109 ];
   char           cULICA_ZLECENIODAWCY[ 109 ];
   char           cKOD_POCZTOWY_ZLECENIODAWCY[ 109 ];
   char           cMIEJSCOWOSC_ZLECENIODAWCY[ 109 ];
   char           cTYTULEM[ 109 ];
   char           cTYTULEM_CD[ 109 ];
   int            dDATA_WPLATY;
   char           cKatalog[ 64 ];
   char           cBaza[ 80 ];
   char           cTekst[ 109 ];

   DbSelectArea( "PRZELEW" );
   nRecNo = RecNo();

   if( ( nTryb == KOPIA || nTryb == POPRAW ) && Eof() )
      return;

   if( nTryb == NOWY )
   {
      DbGoBottom();
      DbSkip( 1 );
   }

   StringFormat( cNAZWA_ODBIORCY,             "%s", FieldGetC( FieldPos( "NAZWA_ODBIORCY" ) ) );
   StringFormat( cNAZWA_ODBIORCY_CD,          "%s", FieldGetC( FieldPos( "NAZWA_ODBIORCY_CD" ) ) );
   StringFormat( cULICA_ODBIORCY,             "%s", IIF( nTryb == NOWY, "ul. ", FieldGetC( FieldPos( "ULICA_ODBIORCY" ) ) ) );
   StringFormat( cKOD_POCZTOWY_ODBIORCY,      "%s", FieldGetC( FieldPos( "KOD_POCZTOWY_ODBIORCY" ) ) );
   StringFormat( cMIEJSCOWOSC_ODBIORCY,       "%s", FieldGetC( FieldPos( "MIEJSCOWOSC_ODBIORCY" ) ) );
   StringFormat( cNR_RACHUNKU_ODBIORCY,       "%s", FieldGetC( FieldPos( "NR_RACHUNKU_ODBIORCY" ) ) );
   nKWOTA = FieldGetN( FieldPos( "KWOTA" ) );
   StringFormat( cNR_RACHUNKU_ZLECENIODAWCY,  "%s", FieldGetC( FieldPos( "NR_RACHUNKU_ZLECENIODAWCY" ) ) );
   StringFormat( cNAZWA_ZLECENIODAWCY,        "%s", FieldGetC( FieldPos( "NAZWA_ZLECENIODAWCY" ) ) );
   StringFormat( cNAZWA_ZLECENIODAWCY_CD,     "%s", FieldGetC( FieldPos( "NAZWA_ZLECENIODAWCY_CD" ) ) );
   StringFormat( cULICA_ZLECENIODAWCY,        "%s", IIF( nTryb == NOWY, "ul. ", FieldGetC( FieldPos( "ULICA_ZLECENIODAWCY" ) ) ) );
   StringFormat( cKOD_POCZTOWY_ZLECENIODAWCY, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY_ZLECENIODAWCY" ) ) );
   StringFormat( cMIEJSCOWOSC_ZLECENIODAWCY,  "%s", FieldGetC( FieldPos( "MIEJSCOWOSC_ZLECENIODAWCY" ) ) );
   StringFormat( cTYTULEM,                    "%s", FieldGetC( FieldPos( "TYTULEM" ) ) );
   StringFormat( cTYTULEM_CD,                 "%s", FieldGetC( FieldPos( "TYTULEM_CD" ) ) );
   dDATA_WPLATY = IIF( nTryb == NOWY, Date(), FieldGetD( FieldPos( "DATA_WPLATY" ) ) );

   // Nowy przelew: zleceniodawcą jest wybrana firma (obszar ZLECENIODAWCA otwarty na chwilę)
   if( nTryb == NOWY )
   {
      StringFormat( cBaza, "%sfirma", RokFirma( oOkno->biuro, oOkno->biuro->firma, cKatalog, sizeof( cKatalog ) ) );
      if( NetUse( cBaza, F, NET_WAIT, "ZLECENIODAWCA", T ) )
      {
         Dane( cNAZWA_ZLECENIODAWCY, cNAZWA_ZLECENIODAWCY_CD, cULICA_ZLECENIODAWCY, cKOD_POCZTOWY_ZLECENIODAWCY, cMIEJSCOWOSC_ZLECENIODAWCY, cNR_RACHUNKU_ZLECENIODAWCY, sizeof( cNAZWA_ZLECENIODAWCY ) );
         DbCloseArea();
      }
      DbSelectArea( "PRZELEW" );
   }

   // Odbiorca
   GetList[ nGet++ ] = wybor.oNazwa       = Szerokosc( GetText( panel, 20, 48, 36, cNAZWA_ODBIORCY, sizeof( cNAZWA_ODBIORCY ), "" ), nPol - 35 );
   GetList[ nGet++ ] = wybor.oNazwaCd     = Szerokosc( GetText( panel, 20, 78, 36, cNAZWA_ODBIORCY_CD, sizeof( cNAZWA_ODBIORCY_CD ), "" ), nPol - 35 );
   GetList[ nGet++ ] = wybor.oUlica       = Szerokosc( GetText( panel, 92, 108, 28, cULICA_ODBIORCY, sizeof( cULICA_ODBIORCY ), "" ), nPol - 107 );
   GetList[ nGet++ ] = wybor.oKod         = GetText( panel, 20, 138,  6, cKOD_POCZTOWY_ODBIORCY, sizeof( cKOD_POCZTOWY_ODBIORCY ), "99-999" );
   GetList[ nGet++ ] = wybor.oMiejscowosc = Szerokosc( GetText( panel, 92, 138, 28, cMIEJSCOWOSC_ODBIORCY, sizeof( cMIEJSCOWOSC_ODBIORCY ), "" ), nPol - 107 );
   GetList[ nGet++ ] = wybor.oRachunek    = GetText( panel, 20, 188, 32, cNR_RACHUNKU_ODBIORCY, sizeof( cNR_RACHUNKU_ODBIORCY ), MASKA_RACHUNKU );

   // Zleceniodawca
   GetList[ nGet++ ] = Szerokosc( GetText( panel, nPol + 15, 48, 36, cNAZWA_ZLECENIODAWCY, sizeof( cNAZWA_ZLECENIODAWCY ), "" ), nPol - 35 );
   GetList[ nGet++ ] = Szerokosc( GetText( panel, nPol + 15, 78, 36, cNAZWA_ZLECENIODAWCY_CD, sizeof( cNAZWA_ZLECENIODAWCY_CD ), "" ), nPol - 35 );
   GetList[ nGet++ ] = Szerokosc( GetText( panel, nPol + 87, 108, 28, cULICA_ZLECENIODAWCY, sizeof( cULICA_ZLECENIODAWCY ), "" ), nPol - 107 );
   GetList[ nGet++ ] = GetText( panel, nPol + 15, 138,  6, cKOD_POCZTOWY_ZLECENIODAWCY, sizeof( cKOD_POCZTOWY_ZLECENIODAWCY ), "99-999" );
   GetList[ nGet++ ] = Szerokosc( GetText( panel, nPol + 87, 138, 28, cMIEJSCOWOSC_ZLECENIODAWCY, sizeof( cMIEJSCOWOSC_ZLECENIODAWCY ), "" ), nPol - 107 );
   GetList[ nGet++ ] = GetText( panel, nPol + 15, 188, 32, cNR_RACHUNKU_ZLECENIODAWCY, sizeof( cNR_RACHUNKU_ZLECENIODAWCY ), MASKA_RACHUNKU );

   // Przelew
   GetList[ nGet++ ] = GetNumber( panel, 175, 232, 12, &nKWOTA, "999999999.99" );
   GetList[ nGet++ ] = Szerokosc( GetText( panel, 175, 262, 27, cTYTULEM, sizeof( cTYTULEM ), "" ), WinWidth( panel ) - 195 );
   GetList[ nGet++ ] = Szerokosc( GetText( panel, 175, 292, 27, cTYTULEM_CD, sizeof( cTYTULEM_CD ), "" ), WinWidth( panel ) - 195 );
   GetList[ nGet++ ] = GetDate( panel, 175, 322, &dDATA_WPLATY );
   GetList[ nGet++ ] = oOkno->DRUKUJ_DATE;
   CheckBoxSetChecked( oOkno->DRUKUJ_DATE, IIF( nTryb == NOWY, T, FieldGetL( FieldPos( "DRUKUJ_DATE" ) ) ) );
   ControlSetEnabled( oOkno->DRUKUJ_DATE, T );

   StatusBarSetText( oOkno->pasekStanu, 1, " PgUp PgDn Enter - zapisz, Esc - bez zapisu, F12 - wybierz odbiorcę" );
   StatusBarSetBackground( oOkno->pasekStanu, 0x73264D );
   StatusBarSetForeground( oOkno->pasekStanu, 0xFFFFFF );

   SetKey( "F12", WybierzOdbiorce, &wybor );
   WinReadModal( oOkno->okno, GetList, nGet, 1 );
   SetKey( "F12", NULL, NULL );

   StatusBarSetText( oOkno->pasekStanu, 1, "" );
   StatusBarSetBackground( oOkno->pasekStanu, -1 );
   StatusBarSetForeground( oOkno->pasekStanu, -1 );

   // Pole wyboru daty zostaje w oknie, wyłączone do następnego poprawiania
   ControlSetEnabled( oOkno->DRUKUJ_DATE, F );
   for( int i = 0; i < nGet; ++i )
   {
      if( GetList[ i ] != oOkno->DRUKUJ_DATE )
         ControlDestroy( GetList[ i ] );
   }

   DbSelectArea( "PRZELEW" );
   if( ReadLastKey() == K_ESC )
   {
      DbGoto( nRecNo );
      Okno( oOkno );

      return;
   }

   if( nTryb == NOWY || nTryb == KOPIA )
   {
      if( !NetAppBlank( NET_WAIT, T ) )
      {
         DbGoto( nRecNo );
         Okno( oOkno );

         return;
      }
   }
   else if( !NetRLock( NET_WAIT ) )
   {
      Okno( oOkno );

      return;
   }

   // Kod pocztowy bez cyfr (sam myślnik maski) zapisany jako pusty, jak w podmiot.c
   FieldPutC( FieldPos( "NAZWA_ODBIORCY" ),             cNAZWA_ODBIORCY );
   FieldPutC( FieldPos( "NAZWA_ODBIORCY_CD" ),          cNAZWA_ODBIORCY_CD );
   FieldPutC( FieldPos( "ULICA_ODBIORCY" ),            cULICA_ODBIORCY );
   FieldPutC( FieldPos( "KOD_POCZTOWY_ODBIORCY" ),      IIF( *AllTrim( StrTran( cTekst, sizeof( cTekst ), cKOD_POCZTOWY_ODBIORCY, "-", "" ) ) == 0, "", cKOD_POCZTOWY_ODBIORCY ) );
   FieldPutC( FieldPos( "MIEJSCOWOSC_ODBIORCY" ),       cMIEJSCOWOSC_ODBIORCY );
   FieldPutC( FieldPos( "NR_RACHUNKU_ODBIORCY" ),       cNR_RACHUNKU_ODBIORCY );
   FieldPutN( FieldPos( "KWOTA" ),                      nKWOTA );
   FieldPutC( FieldPos( "NR_RACHUNKU_ZLECENIODAWCY" ),  cNR_RACHUNKU_ZLECENIODAWCY );
   FieldPutC( FieldPos( "NAZWA_ZLECENIODAWCY" ),        cNAZWA_ZLECENIODAWCY );
   FieldPutC( FieldPos( "NAZWA_ZLECENIODAWCY_CD" ),     cNAZWA_ZLECENIODAWCY_CD );
   FieldPutC( FieldPos( "ULICA_ZLECENIODAWCY" ),       cULICA_ZLECENIODAWCY );
   FieldPutC( FieldPos( "KOD_POCZTOWY_ZLECENIODAWCY" ), IIF( *AllTrim( StrTran( cTekst, sizeof( cTekst ), cKOD_POCZTOWY_ZLECENIODAWCY, "-", "" ) ) == 0, "", cKOD_POCZTOWY_ZLECENIODAWCY ) );
   FieldPutC( FieldPos( "MIEJSCOWOSC_ZLECENIODAWCY" ),  cMIEJSCOWOSC_ZLECENIODAWCY );
   FieldPutC( FieldPos( "TYTULEM" ),                    cTYTULEM );
   FieldPutC( FieldPos( "TYTULEM_CD" ),                 cTYTULEM_CD );
   FieldPutD( FieldPos( "DATA_WPLATY" ),                dDATA_WPLATY );
   FieldPutL( FieldPos( "DRUKUJ_DATE" ),                CheckBoxChecked( oOkno->DRUKUJ_DATE ) );

   DbCommit();
   DbUnlock();

   Okno( oOkno );
   Stan( oOkno );
}

// F2: nowy przelew
// Dodaj( toolbar, data ) --> void
static void Dodaj( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   Nowy( ( OknoPrzelewu * )data, NOWY );
}

// Ins: nowy przelew
// Wstaw( win, data ) --> void
static void Wstaw( Win *win, void *data )
{
   UNUSED( win );

   Nowy( ( OknoPrzelewu * )data, NOWY );
}

// F3: kopia bieżącego przelewu jako nowy
// Kopia( win, data ) --> void
static void Kopia( Win *win, void *data )
{
   UNUSED( win );

   Nowy( ( OknoPrzelewu * )data, KOPIA );
}

// F4: poprawienie przelewu
// Popraw( toolbar, data ) --> void
static void Popraw( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   Nowy( ( OknoPrzelewu * )data, POPRAW );
}

// F8: usunięcie przelewu po pytaniu
// Usun( toolbar, data ) --> void
static void Usun( Toolbar *toolbar, void *data )
{
   OknoPrzelewu *oOkno = ( OknoPrzelewu * )data;
   char          cOdbiorca[ 109 ];
   char          cZleceniodawca[ 109 ];
   char          cPytanie[ 400 ];

   UNUSED( toolbar );

   DbSelectArea( "PRZELEW" );
   if( Eof() )
      return;

   StringFormat( cOdbiorca, "%s", FieldGetC( FieldPos( "NAZWA_ODBIORCY" ) ) );
   StringFormat( cZleceniodawca, "%s", FieldGetC( FieldPos( "NAZWA_ZLECENIODAWCY" ) ) );
   StringFormat( cPytanie, "Czy na pewno usunąć przelew\n%s\n-----------------\n%s\n?", RTrim( cOdbiorca ), RTrim( cZleceniodawca ) );
   if( MsgYesNo( cPytanie, "Przelewy" ) )
   {
      DbSelectArea( "PRZELEW" );
      if( NetRLock( NET_WAIT ) )
      {
         DbDelete();
         DbCommit();
         DbUnlock();
         DbSkip( 1 );
         if( Eof() )
            DbGoBottom();
      }
      Stan( oOkno );
      Okno( oOkno );
   }
}

// F9: wydruk bieżącego przelewu do PDF w katalogu wydruków firmy, otwarty potem w programie Windows
// Drukuj( toolbar, data ) --> void
static void Drukuj( Toolbar *toolbar, void *data )
{
   OknoPrzelewu *oOkno = ( OknoPrzelewu * )data;
   HPDF_Doc      pPdf;
   HPDF_Font     pFont;
   char          cPlik[ 128 ];
   char          cTekst[ 300 ];

   UNUSED( toolbar );

   DbSelectArea( "PRZELEW" );
   if( Eof() )
      return;

   if( !FileExists( PROG_ZASO "druki/pp-519a.png" ) )
   {
      MsgError( "Brak druku " PROG_ZASO "druki/pp-519a.png.", "Przelewy" );

      return;
   }

   pPdf = HPDF_New( NULL, NULL );
   HPDF_SetCompressionMode( pPdf, HPDF_COMP_ALL );
   HPDF_UseUTFEncodings( pPdf );
   HPDF_SetCurrentEncoder( pPdf, "UTF-8" );
   HPDF_SetInfoAttr( pPdf, HPDF_INFO_TITLE, "Polecenie przelewu / wpłata gotówkowa" );
   HPDF_SetInfoAttr( pPdf, HPDF_INFO_CREATOR, "xBiuro" );
   pFont = WydrukCzcionka( pPdf, "arial.ttf" );
   if( pFont == NULL )
   {
      HPDF_Free( pPdf );
      MsgError( "Brak czcionki Arial w katalogu czcionek Windows.", "Przelewy" );

      return;
   }

   WydrukPlik( oOkno->biuro, "przelew.pdf", cPlik, sizeof( cPlik ) );
   if( !DrukujDoPDF( pPdf, pFont, cPlik ) )
   {
      StringFormat( cTekst, "Nie można zapisać pliku %s.\nPlik może być otwarty w innym programie.", cPlik );
      MsgError( cTekst, "Przelewy" );
   }
   else
      FileExecute( cPlik );

   HPDF_Free( pPdf );
}

// Bieżący przelew na druku pp-519a.png: dwa egzemplarze na kartce A4 rozdzielone linią cięcia, pozycje z przelew.prg;
// bez rachunku zleceniodawcy (wpłata gotówkowa) w jego miejscu kwota słownie
// DrukujDoPDF( pPdf, pFont, cPlik ) --> lZapis
static xBool DrukujDoPDF( HPDF_Doc pPdf, HPDF_Font pFont, const char *cPlik )
{
   static const HPDF_REAL aKreska[] = { 8, 7, 2, 7 };

   HPDF_Page  pPage  = HPDF_AddPage( pPdf );
   HPDF_Image pImage = HPDF_LoadPngImageFromFile( pPdf, PROG_ZASO "druki/pp-519a.png" );
   double     nSzer  = HPDF_Image_GetWidth( pImage ) / 4.0;
   double     nWys   = HPDF_Image_GetHeight( pImage ) / 4.0;
   double     nY;
   char       cOdbiorcy[ 109 ];
   char       cZleceniodawcy[ 109 ];
   char       cNazwaOdbiorcy[ 240 ];
   char       cNazwaZleceniodawcy[ 240 ];
   char       cAdresOdbiorcy[ 240 ];
   char       cAdresZleceniodawcy[ 240 ];
   char       cKwota[ 32 ];
   char       cSlownie[ 512 ];
   char       cData[ 16 ];

   HPDF_Page_SetSize( pPage, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT );
   HPDF_Page_DrawImage( pPage, pImage, 108, 475, ( HPDF_REAL )nSzer, ( HPDF_REAL )nWys );
   HPDF_Page_DrawImage( pPage, pImage, 108, 54, ( HPDF_REAL )nSzer, ( HPDF_REAL )nWys );

   HPDF_Page_SetDash( pPage, aKreska, 4, 0 );
   HPDF_Page_MoveTo( pPage, 0, 421 );
   HPDF_Page_LineTo( pPage, 595, 421 );
   HPDF_Page_Stroke( pPage );

   DbSelectArea( "PRZELEW" );
   FirmaCyfry( FieldGetC( FieldPos( "NR_RACHUNKU_ODBIORCY" ) ), cOdbiorcy, sizeof( cOdbiorcy ) );
   FirmaCyfry( FieldGetC( FieldPos( "NR_RACHUNKU_ZLECENIODAWCY" ) ), cZleceniodawcy, sizeof( cZleceniodawcy ) );
   Kwota( FieldGetN( FieldPos( "KWOTA" ) ), cKwota, sizeof( cKwota ) );
   NToW( FieldGetN( FieldPos( "KWOTA" ) ), cSlownie, sizeof( cSlownie ) );
   DToC( FieldGetD( FieldPos( "DATA_WPLATY" ) ), cData, sizeof( cData ) );
   Nazwa( "ODBIORCY", cNazwaOdbiorcy, sizeof( cNazwaOdbiorcy ) );
   Nazwa( "ZLECENIODAWCY", cNazwaZleceniodawcy, sizeof( cNazwaZleceniodawcy ) );
   Adres( "ODBIORCY", cAdresOdbiorcy, sizeof( cAdresOdbiorcy ) );
   Adres( "ZLECENIODAWCY", cAdresZleceniodawcy, sizeof( cAdresZleceniodawcy ) );

   // Górny egzemplarz, potem dolny o 421 punktów niżej
   for( int nEgzemplarz = 0; nEgzemplarz < 2; ++nEgzemplarz )
   {
      nY = -421 * nEgzemplarz;

      WydrukTekst( pPage, pFont, 9, 176, 764 + nY, cNazwaOdbiorcy, 400 );
      WydrukTekst( pPage, pFont, 9, 176, 739 + nY, cAdresOdbiorcy, 400 );

      // Rachunki i kwota w kratkach: 8 punktów odstępu między cyframi
      HPDF_Page_SetCharSpace( pPage, 8 );
      WydrukTekst( pPage, pFont, 12, 176, 714 + nY, cOdbiorcy, 420 );
      WydrukTekst( pPage, pFont, 12, 400, 689 + nY, cKwota, 200 );
      if( *cZleceniodawcy != 0 )
         WydrukTekst( pPage, pFont, 12, 176, 662 + nY, cZleceniodawcy, 420 );
      HPDF_Page_SetCharSpace( pPage, 0 );

      if( *cZleceniodawcy == 0 )
         WydrukTekst( pPage, pFont, 8, 176, 662 + nY, cSlownie, 400 );

      WydrukTekst( pPage, pFont, 9, 176, 639 + nY, cNazwaZleceniodawcy, 400 );
      WydrukTekst( pPage, pFont, 9, 176, 614 + nY, cAdresZleceniodawcy, 400 );
      WydrukTekst( pPage, pFont, 9, 176, 589 + nY, FieldGetC( FieldPos( "TYTULEM" ) ), 400 );
      WydrukTekst( pPage, pFont, 9, 176, 564 + nY, FieldGetC( FieldPos( "TYTULEM_CD" ) ), 400 );
      if( FieldGetL( FieldPos( "DRUKUJ_DATE" ) ) )
         WydrukTekst( pPage, pFont, 9, 176, 500 + nY, cData, 200 );
   }

   return IIF( HPDF_SaveToFile( pPdf, cPlik ) == HPDF_OK, T, F );
}

// Esc: zamyka okno
// Zamknij( toolbar, data ) --> void
static void Zamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( OknoPrzelewu * )data )->okno );
}

// F12 przy wpisywaniu: odbiorca z kartoteki kontrahentów (obszar ODBIORCA otwarty na chwilę) do pól odbiorcy
// WybierzOdbiorce( win, data ) --> void
static void WybierzOdbiorce( Win *win, void *data )
{
   WyborOdbiorcy *wybor = ( WyborOdbiorcy * )data;
   long           nRecNo;
   char           cNazwa[ 109 ];
   char           cNazwaCd[ 109 ];
   char           cUlica[ 109 ];
   char           cKod[ 109 ];
   char           cMiejscowosc[ 109 ];
   char           cRachunek[ 109 ];

   // Bez F12 w kartotece, bo tam F12 szuka, a poprawianie kontrahenta to też odczyt
   SetKey( "F12", NULL, NULL );
   nRecNo = KontrahentWybierz( win, wybor->oOkno->biuro );
   if( nRecNo > 0 && NetUse( DANE_BAZA "kartoteka/kontrah", F, NET_WAIT, "ODBIORCA", T ) )
   {
      DbGoto( nRecNo );
      Dane( cNazwa, cNazwaCd, cUlica, cKod, cMiejscowosc, cRachunek, sizeof( cNazwa ) );
      DbCloseArea();
      ControlSetText( wybor->oNazwa, cNazwa );
      ControlSetText( wybor->oNazwaCd, cNazwaCd );
      ControlSetText( wybor->oUlica, cUlica );
      ControlSetText( wybor->oKod, cKod );
      ControlSetText( wybor->oMiejscowosc, cMiejscowosc );
      ControlSetText( wybor->oRachunek, cRachunek );
   }
   DbSelectArea( "PRZELEW" );
   SetKey( "F12", WybierzOdbiorce, wybor );

   ControlSetFocus( wybor->oNazwa );
}

// Nazwa (skrócona, a bez niej pełna) do wiersza druku po 27 znaków z pustym drugim wierszem nazwy, ulica z numerem
// domu i lokalu, kod pocztowy, miejscowość i rachunek podmiotu z bieżącego rekordu (firma albo kontrahent); każdy
// bufor ma nRozmiar bajtów
// Dane( cNazwa, cNazwaCd, cUlica, cKod, cMiejscowosc, cRachunek, nRozmiar ) --> void
static void Dane( char *cNazwa, char *cNazwaCd, char *cUlica, char *cKod, char *cMiejscowosc, char *cRachunek, int nRozmiar )
{
   char cTekst[ 2049 ];
   char cNrDomu[ 37 ];
   char cNrLokalu[ 41 ];

   StringFormat( cTekst, "%s", FieldGetC( FieldPos( "SKROCONA_NAZWA" ) ) );
   if( *AllTrim( cTekst ) == 0 )
      StrTran( cTekst, sizeof( cTekst ), FieldGetC( FieldPos( "PELNA_NAZWA" ) ), "\r\n", " " );
   StrTran( cNazwa, ( size_t )nRozmiar, cTekst, "\n", " " );
   ULeft( AllTrim( cNazwa ), 27 );
   *cNazwaCd = 0;

   // Ulica z „ul.” w polu, bez opisu przy nim; bez ulicy (mała miejscowość) w jej miejscu nazwa miejscowości z numerem domu
   StringFormat( cTekst, "%s", FieldGetC( FieldPos( "ULICA" ) ) );
   StringFormat( cNrDomu, "%s", FieldGetC( FieldPos( "NR_DOMU" ) ) );
   StringFormat( cNrLokalu, "%s", FieldGetC( FieldPos( "NR_LOKALU" ) ) );
   StringFormatSize( cKod, ( size_t )nRozmiar, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY" ) ) );
   StringFormatSize( cMiejscowosc, ( size_t )nRozmiar, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
   AllTrim( cTekst );
   AllTrim( cNrDomu );
   AllTrim( cNrLokalu );
   AllTrim( cKod );
   AllTrim( cMiejscowosc );
   StringFormatSize( cUlica, ( size_t )nRozmiar, "%s%s %s%s%s", IIF( *cTekst == 0, "", "ul. " ), IIF( *cTekst == 0, cMiejscowosc, cTekst ), cNrDomu, IIF( *cNrLokalu == 0, "", "/" ), cNrLokalu );
   AllTrim( cUlica );

   Rachunek( FieldGetC( FieldPos( IIF( FieldGetN( FieldPos( "DRUKUJ_RACHUNEK_NR" ) ) == 2, "NUMER_RACHUNKU_2", "NUMER_RACHUNKU_1" ) ) ), cRachunek, nRozmiar );
}

// Pierwszy wiersz nazwy na druku: oba wiersze nazwy bieżącego przelewu, jak w przelew.prg; cKto to ODBIORCY albo
// ZLECENIODAWCY
// Nazwa( cKto, cWynik, nRozmiar ) --> cWynik
static char *Nazwa( const char *cKto, char *cWynik, int nRozmiar )
{
   char cPole[ 40 ];
   char cNazwa[ 109 ];
   char cNazwaCd[ 109 ];

   StringFormat( cPole, "NAZWA_%s", cKto );
   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( cPole ) ) );
   StringFormat( cPole, "NAZWA_%s_CD", cKto );
   StringFormat( cNazwaCd, "%s", FieldGetC( FieldPos( cPole ) ) );
   StringFormatSize( cWynik, ( size_t )nRozmiar, "%s %s", AllTrim( cNazwa ), AllTrim( cNazwaCd ) );

   return AllTrim( cWynik );
}

// Drugi wiersz nazwy na druku: ulica, kod pocztowy i miejscowość bieżącego przelewu; cKto to ODBIORCY albo ZLECENIODAWCY
// Adres( cKto, cWynik, nRozmiar ) --> cWynik
static char *Adres( const char *cKto, char *cWynik, int nRozmiar )
{
   char cPole[ 40 ];
   char cUlica[ 109 ];
   char cKod[ 9 ];
   char cMiejscowosc[ 109 ];

   StringFormat( cPole, "ULICA_%s", cKto );
   StringFormat( cUlica, "%s", FieldGetC( FieldPos( cPole ) ) );
   StringFormat( cPole, "KOD_POCZTOWY_%s", cKto );
   StringFormat( cKod, "%s", FieldGetC( FieldPos( cPole ) ) );
   StringFormat( cPole, "MIEJSCOWOSC_%s", cKto );
   StringFormat( cMiejscowosc, "%s", FieldGetC( FieldPos( cPole ) ) );
   AllTrim( cUlica );
   AllTrim( cKod );
   AllTrim( cMiejscowosc );
   StringFormatSize( cWynik, ( size_t )nRozmiar, "%s%s%s %s", cUlica, IIF( *cUlica == 0, "", ", " ), cKod, cMiejscowosc );

   return AllTrim( cWynik );
}

// Numer rachunku jak na druku: same cyfry, dwie kontrolne i grupy po cztery rozdzielone spacją
// Rachunek( cTekst, cWynik, nRozmiar ) --> cWynik
static char *Rachunek( const char *cTekst, char *cWynik, int nRozmiar )
{
   char cCyfry[ 64 ];
   int  nDlugosc = 0;

   FirmaCyfry( cTekst, cCyfry, sizeof( cCyfry ) );
   for( int i = 0; cCyfry[ i ] != 0 && nDlugosc < nRozmiar - 2; ++i )
   {
      if( i == 2 || ( i > 2 && ( i - 2 ) % 4 == 0 ) )
         cWynik[ nDlugosc++ ] = ' ';
      cWynik[ nDlugosc++ ] = cCyfry[ i ];
   }
   cWynik[ nDlugosc ] = 0;

   return cWynik;
}

// Kwota z przecinkiem przed groszami, jak na druku
// Kwota( nKwota, cWynik, nRozmiar ) --> cWynik
static char *Kwota( double nKwota, char *cWynik, int nRozmiar )
{
   char cTekst[ 32 ];

   StringFormat( cTekst, "%.2f", nKwota );

   return StrTran( cWynik, ( size_t )nRozmiar, cTekst, ".", "," );
}

// Ramka grupy pól
// Ramka( strona, x, y, szerokosc, wysokosc, podpis ) --> void
static void Ramka( Win *strona, int x, int y, int szerokosc, int wysokosc, const char *podpis )
{
   GroupBoxSetLineColor( GroupBoxCreate( strona, x, y, szerokosc, wysokosc, podpis ), 0xDCDCDC );
}

// Opis pola
// Opis( strona, x, y, tekst ) --> void
static void Opis( Win *strona, int x, int y, const char *tekst )
{
   SayCreate( strona, x, y, 0, 20, tekst );
}

// Pole do oglądania
// Pole( strona, x, y, dlugosc ) --> pole
static Control *Pole( Win *strona, int x, int y, int dlugosc )
{
   Control *pole = SayCreate( strona, x, y, dlugosc, 24, "" );

   LabelSetBorder( pole, T );

   return pole;
}

// Pole do prawej krawędzi ramki: szerokość w pikselach zamiast długości w znakach
// Szerokosc( pole, szerokosc ) --> pole
static Control *Szerokosc( Control *pole, int szerokosc )
{
   ControlSetWidth( pole, szerokosc );

   return pole;
}
