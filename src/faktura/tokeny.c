/*
 * Program...: xBiuro
 * Plik......: src/faktura/tokeny.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "faktura.h"

#define TOKENY DANE_BAZA "ksef/tokeny"

static const DbField aDbf[] = {
   { "NAZWA_TOKENU",       'C', 240, 0 },
   { "NIP",                'C',  50, 0 },
   { "SRODOWISKO_KSEF",    'N',   1, 0 },
   { "TOKEN_ZASZYFROWANY", 'M',  10, 0 },
   { "DATA_DODANIA",       'D',   8, 0 },
   { "WAZNY_DO",           'D',   8, 0 },
   { "DATA_SPRAWDZENIA",   'D',   8, 0 },
   { "WYNIK_SPRAWDZENIA",  'M',  10, 0 }
};

#define POLA_TOKENOW ( ( int )( sizeof( aDbf ) / sizeof( aDbf[ 0 ] ) ) )

typedef struct
{
   Biuro     *biuro;
   Win       *okno;
   StatusBar *pasekStanu;
   Toolbar   *pasek;
   Win       *panel;
   Control   *BRW;
   int        aZamroz[ 2 ];
   Control   *NAZWA_TOKENU;
   Control   *TOKEN;
   Control   *NIP;
   Control   *SRODOWISKO_KSEF;
   Control   *WAZNY_DO;
   Control   *STAN;
   Control   *WYNIK_SPRAWDZENIA;
   char       cDodany[ 16 ];
   char       cWazny[ 16 ];
   char       cSprawdzony[ 16 ];
   char       cStan[ 64 ];
} OknoTokenow;

static xBool       DbOpen( void );
static void        Przebudowa( void );
static void        Stan( OknoTokenow *oDialog );
static void        Ostrzez( void );
static void        Opis( Win *strona, int y, const char *tekst );
static Control    *Pole( Win *strona, int x, int y, int dlugosc );
static void        Pomoc( Toolbar *toolbar, void *data );
static void        Okno( OknoTokenow *oDialog );
static void        Zmiana( Control *browse, void *data );
static const char *Kolumna( Control *browse, void *data );
static const char *KolumnaSrodowisko( Control *browse, void *data );
static const char *KolumnaDodany( Control *browse, void *data );
static const char *KolumnaWazny( Control *browse, void *data );
static const char *KolumnaSprawdzony( Control *browse, void *data );
static char       *Data( const char *cPole, char *cData, int nRozmiar );
static double      Numer( Control *browse, void *data );
static xBool       Zapisz( const char *cNazwa, const char *cNip, int nSrodowisko, const char *cToken, int dWaznyDo );
static void        Dodaj( Toolbar *toolbar, void *data );
static void        Wstaw( Win *win, void *data );
static void        Aplikacja( Toolbar *toolbar, void *data );
static void        Sprawdz( Toolbar *toolbar, void *data );
static void        Usun( Toolbar *toolbar, void *data );
static void        Zamknij( Toolbar *toolbar, void *data );

void Tokeny( Win *win, void *data )
{
   OknoTokenow  oknoTokenow = { 0 };
   OknoTokenow *oDialog     = &oknoTokenow;
   Win         *panel;
   int          przycisk;

   oDialog->biuro = ( Biuro * )data;
   if( !DbOpen() )
      return;

   oDialog->okno = DialogCreate( win, "Lista tokenów KSeF", 900, 600 );
   WinSetBackground( oDialog->okno, 0xE9EDF1 );
   WinSetSizable( oDialog->okno, T );
   WinSetKey( oDialog->okno, "Ins", Wstaw, oDialog );

   oDialog->pasekStanu = StatusBarCreate( oDialog->okno, "" );
   StatusBarAdd( oDialog->pasekStanu, 0, "", F );
   StatusBarAdd( oDialog->pasekStanu, 60, "", F );
   StatusBarSetScoreboard( oDialog->pasekStanu, 2 );

   oDialog->pasek = ToolbarCreate( oDialog->okno, TOOLBAR_BOTTOM );
   przycisk = ToolbarAdd( oDialog->pasek, "F1-Pomoc", Pomoc, NULL );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "F1" );
   ToolbarAddSeparator( oDialog->pasek );
   przycisk = ToolbarAdd( oDialog->pasek, "F2-Dodaj", Dodaj, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "F2" );
   ToolbarAddSeparator( oDialog->pasek );
   przycisk = ToolbarAdd( oDialog->pasek, "F3-Aplikacja KSeF", Aplikacja, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "F3" );
   ToolbarAddSeparator( oDialog->pasek );
   oDialog->aZamroz[ 0 ] = ToolbarAdd( oDialog->pasek, "F5-Sprawdź", Sprawdz, oDialog );
   ToolbarSetShortcut( oDialog->pasek, oDialog->aZamroz[ 0 ], "F5" );
   ToolbarAddSeparator( oDialog->pasek );
   oDialog->aZamroz[ 1 ] = ToolbarAdd( oDialog->pasek, "F8-Usuń", Usun, oDialog );
   ToolbarSetShortcut( oDialog->pasek, oDialog->aZamroz[ 1 ], "F8" );
   ToolbarAddSeparator( oDialog->pasek );
   przycisk = ToolbarAdd( oDialog->pasek, "Esc-Zamknij", Zamknij, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "Esc" );

   panel          = PanelCreate( oDialog->okno, 0, 0, 0, 0 );
   oDialog->panel = panel;

   Opis( panel,  13, "Nazwa tokenu:" );
   Opis( panel,  43, "Token:" );
   Opis( panel,  73, "NIP / środowisko:" );
   Opis( panel, 103, "Ważny do:" );
   Opis( panel, 133, "Sprawdzenie:" );
   oDialog->NAZWA_TOKENU      = Pole( panel, 190,  10, 60 );
   oDialog->TOKEN             = Pole( panel, 190,  40, 60 );
   oDialog->NIP               = Pole( panel, 190,  70, 20 );
   oDialog->SRODOWISKO_KSEF   = Pole( panel, 388,  70, 38 );
   oDialog->WAZNY_DO          = Pole( panel, 190, 100, 10 );
   oDialog->STAN              = Pole( panel, 298, 100, 24 );
   oDialog->WYNIK_SPRAWDZENIA = Pole( panel, 190, 130, 60 );

   oDialog->BRW = BrowseCreate( panel, 10, 170, WinWidth( panel ) - 20, WinHeight( panel ) - 180 );
   ControlSetAnchor( oDialog->BRW, "LTRB" );
   BrowseSetColor( oDialog->BRW, BROWSE_HEADBACKGROUND, 0x4D7326 );
   DbBrowse( oDialog->BRW, "TOKENY" );
   BrowseAddNumber( oDialog->BRW, "Poz.", 4, "9999", Numer, NULL );
   BrowseAddColumn( oDialog->BRW, "Nazwa tokenu", 32, BROWSE_LEFT, Kolumna, "NAZWA_TOKENU" );
   BrowseAddColumn( oDialog->BRW, "NIP", 12, BROWSE_LEFT, Kolumna, "NIP" );
   BrowseAddColumn( oDialog->BRW, "Środowisko", 12, BROWSE_LEFT, KolumnaSrodowisko, NULL );
   BrowseAddColumn( oDialog->BRW, "Dodany", 10, BROWSE_LEFT, KolumnaDodany, oDialog );
   BrowseAddColumn( oDialog->BRW, "Ważny do", 10, BROWSE_LEFT, KolumnaWazny, oDialog );
   BrowseAddColumn( oDialog->BRW, "Sprawdzony", 10, BROWSE_LEFT, KolumnaSprawdzony, oDialog );
   ControlSetAction( oDialog->BRW, Zmiana, oDialog );

   DbSelectArea( "TOKENY" );
   DbGoTop();
   Okno( oDialog );
   Stan( oDialog );
   Ostrzez();

   WinCenter( oDialog->okno );
   DialogShowModal( oDialog->okno );

   DbSelectArea( "TOKENY" );
   DbCloseArea();
}

char *TokenyPobierz( const char *cNipFirmy, int nSrodowiskoKsef )
{
   char *cToken = NULL;
   char  cNip[ 64 ];
   char  cJest[ 64 ];

   if( !DbOpen() )
      return NULL;

   StringFormat( cNip, "%s", cNipFirmy );
   AllTrim( cNip );
   for( DbGoBottom(); !Bof() && !Eof(); DbSkip( -1 ) )
   {
      StringFormat( cJest, "%s", FieldGetC( FieldPos( "NIP" ) ) );
      if( strcmp( AllTrim( cJest ), cNip ) == 0 && FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ) == nSrodowiskoKsef && ( FieldGetD( FieldPos( "WAZNY_DO" ) ) == 0 || FieldGetD( FieldPos( "WAZNY_DO" ) ) >= Date() ) )
      {
         cToken = KsefOdszyfruj( FieldGetC( FieldPos( "TOKEN_ZASZYFROWANY" ) ) );
         if( *cToken != 0 )
            break;
         free( cToken );
         cToken = NULL;
      }
   }
   DbCloseArea();

   return cToken;
}

xBool TokenyZapisz( const char *cNazwa, const char *cNip, int nSrodowisko, const char *cToken, int dWaznyDo )
{
   xBool lJest;

   if( !DbOpen() )
      return F;

   lJest = Zapisz( cNazwa, cNip, nSrodowisko, cToken, dWaznyDo );
   DbSelectArea( "TOKENY" );
   DbCloseArea();

   return lJest;
}

void TokenyNtx( void )
{
   char cPlik[ 64 ];

   StringFormat( cPlik, "%s.dbf", TOKENY );
   if( FileExists( cPlik ) && NetUse( TOKENY, T, NET_WAIT, "TOKENY", F ) )
   {
      NetPack();
      DbCloseArea();
   }
}

xBool FirmaPobierzKsef( Biuro *biuro, char *cNip, int nRozmiar, int *nSrodowisko )
{
   if( !FirmaPobierzDane( biuro, "FIRMA_KSEF" ) )
      return F;

   StringFormatSize( cNip, ( size_t )nRozmiar, "%s", FieldGetC( FieldPos( "NIP" ) ) );
   AllTrim( cNip );
   *nSrodowisko = IIF( FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ) == SRODOWISKO_PRODUKCYJNE, SRODOWISKO_PRODUKCYJNE, SRODOWISKO_TESTOWE );
   DbCloseArea();

   return T;
}

static xBool DbOpen( void )
{
   char cPlik[ 64 ];

   DirCreate( DANE_BAZA "ksef" );
   StringFormat( cPlik, "%s.dbf", TOKENY );
   if( !FileExists( cPlik ) )
      DbCreate( TOKENY, aDbf, POLA_TOKENOW );

   if( !NetUse( TOKENY, F, NET_WAIT, "TOKENY", F ) )
      return F;

   if( FCount() != POLA_TOKENOW )
   {
      DbCloseArea();
      MsgInfo( "Dodanie pola w bazie", "Tokeny" );
      Przebudowa();
      if( !NetUse( TOKENY, F, NET_WAIT, "TOKENY", F ) )
         return F;
   }

   return T;
}

static void Przebudowa( void )
{
   char cDbf[ 64 ];
   char cTmp[ 64 ];
   char cTmpDbf[ 64 ];
   char cStara[ 64 ];

   StringFormat( cDbf, "%s.dbf", TOKENY );
   StringFormat( cTmp, "%s-tmp", TOKENY );
   StringFormat( cTmpDbf, "%s.dbf", cTmp );
   StringFormat( cStara, "%s.tmp", TOKENY );

   if( FileExists( cDbf ) )
   {
      FileCopy( cDbf, cTmpDbf );
      FRename( cDbf, cStara );
      DbCreate( TOKENY, aDbf, POLA_TOKENOW );
      if( NetUse( TOKENY, F, NET_WAIT, "TOKENY", F ) )
      {
         DbAppendFrom( cTmp, NULL, NULL );
         DbCloseArea();
      }
      FErase( cTmpDbf );
      FErase( cStara );
   }
}

static void Stan( OknoTokenow *oDialog )
{
   xBool lJest = !DbPusta( "TOKENY" );

   ToolbarEnable( oDialog->pasek, oDialog->aZamroz[ 0 ], lJest );
   ToolbarEnable( oDialog->pasek, oDialog->aZamroz[ 1 ], lJest );
   ControlSetEnabled( oDialog->BRW, lJest );
   BrowseRefresh( oDialog->BRW );

   if( lJest )
      ControlSetFocus( oDialog->BRW );
}

static void Ostrzez( void )
{
   char   cTekst[ 4000 ] = "";
   char   cNazwa[ 241 ];
   char   cStan[ 64 ];
   size_t nDlugosc;
   long   nRecNo;
   int    dDo;

   DbSelectArea( "TOKENY" );
   nRecNo = RecNo();
   for( DbGoTop(); !Eof(); DbSkip( 1 ) )
   {
      dDo = FieldGetD( FieldPos( "WAZNY_DO" ) );
      if( dDo != 0 && DateDiff( dDo, Date() ) <= DNI_OSTRZEZENIA )
      {
         StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_TOKENU" ) ) );
         nDlugosc = strlen( cTekst );
         StringFormatSize( cTekst + nDlugosc, sizeof( cTekst ) - nDlugosc, "\n%s: %s", AllTrim( cNazwa ), KsefWaznosc( dDo, cStan, sizeof( cStan ) ) );
      }
   }
   DbGoto( nRecNo );

   if( *cTekst != 0 )
   {
      nDlugosc = strlen( cTekst );
      StringFormatSize( cTekst + nDlugosc, sizeof( cTekst ) - nDlugosc, "\n\nNowy token: menu KSeF, Tokeny / Certyfikaty, Generuj token,\nalbo Aplikacja Podatnika KSeF (F3)." );
      MsgInfo( cTekst + 1, "Tokeny KSeF do odnowienia" );
   }
}

static void Opis( Win *strona, int y, const char *tekst )
{
   SayCreate( strona, 20, y, 0, 20, tekst );
}

static Control *Pole( Win *strona, int x, int y, int dlugosc )
{
   Control *pole = SayCreate( strona, x, y, dlugosc, 24, "" );

   LabelSetBorder( pole, T );

   return pole;
}

static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgHelp( "Token KSeF to klucz, którym xBiuro loguje się do KSeF. Generuje się go w Aplikacji Podatnika KSeF, w zakładce Tokeny, dla NIP-u firmy z menu Firma, Dane firmy, i w jej środowisku (menu KSeF, Środowisko KSeF), albo certyfikatem w menu KSeF, Tokeny / Certyfikaty, Generuj token. Token jest ważny 90 dni, najwyżej rok; 30 dni przed końcem ważności lista ostrzega, a do wysyłki faktur xBiuro bierze tylko ważny token.\n\n"
            "F1\tpomoc\n"
            "F2, Ins\tnowy token: nazwa, token wklejony z Aplikacji Podatnika i data ważności (domyślnie 90 dni)\n"
            "F3\tAplikacja Podatnika KSeF w przeglądarce (środowisko firmy)\n"
            "F5\tsprawdzenie połączenia z KSeF tym tokenem\n"
            "F8\tusunięcie tokenu\n"
            "Strzałki\tpoprzedni / następny token\n"
            "Esc\tzamknięcie okna\n\n"
            "Wpisywanie danych\n\n"
            "Ctrl+V\twklejenie tokenu\n"
            "Enter\tnastępne pole, na ostatnim zapis\n"
            "PgUp, PgDn\tzapis\n"
            "Esc\tbez zapisu\n\n"
            "Token jest zaszyfrowany kluczem tego stanowiska (profil użytkownika Windows) i na ekranie widać tylko jego początek. Na innym komputerze token trzeba dodać ponownie.", "Lista tokenów KSeF" );
}

static void Okno( OknoTokenow *oDialog )
{
   char *cToken;
   char  cTekst[ 64 ];
   char  cKrotki[ 2049 ];
   char  cWynik[ 2100 ];

   DbSelectArea( "TOKENY" );
   cToken = KsefOdszyfruj( FieldGetC( FieldPos( "TOKEN_ZASZYFROWANY" ) ) );
   if( Eof() )
      *cTekst = 0;
   else if( *cToken == 0 )
      StringFormat( cTekst, "(nie można odczytać na tym stanowisku)" );
   else
   {
      StringFormat( cKrotki, "%s", cToken );
      StringFormat( cTekst, "%s…", ULeft( cKrotki, 24 ) );
   }
   free( cToken );

   StrTran( cWynik, sizeof( cWynik ), FieldGetC( FieldPos( "WYNIK_SPRAWDZENIA" ) ), "\n", " " );
   ControlSetText( oDialog->NAZWA_TOKENU, FieldGetC( FieldPos( "NAZWA_TOKENU" ) ) );
   ControlSetText( oDialog->TOKEN, cTekst );
   ControlSetText( oDialog->NIP, FieldGetC( FieldPos( "NIP" ) ) );
   ControlSetText( oDialog->SRODOWISKO_KSEF, IIF( Eof(), "", KsefSrodowisko( ( int )FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ), T ) ) );
   ControlSetText( oDialog->WAZNY_DO, Data( "WAZNY_DO", oDialog->cWazny, sizeof( oDialog->cWazny ) ) );
   ControlSetText( oDialog->STAN, IIF( Eof(), "", KsefWaznosc( FieldGetD( FieldPos( "WAZNY_DO" ) ), oDialog->cStan, sizeof( oDialog->cStan ) ) ) );
   ControlSetText( oDialog->WYNIK_SPRAWDZENIA, cWynik );
}

static void Zmiana( Control *browse, void *data )
{
   UNUSED( browse );

   Okno( ( OknoTokenow * )data );
}

static const char *Kolumna( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "TOKENY" );

   return FieldGetC( FieldPos( ( const char * )data ) );
}

static const char *KolumnaSrodowisko( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "TOKENY" );

   return KsefSrodowisko( ( int )FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ), F );
}

static const char *KolumnaDodany( Control *browse, void *data )
{
   OknoTokenow *oDialog = ( OknoTokenow * )data;

   UNUSED( browse );

   return Data( "DATA_DODANIA", oDialog->cDodany, sizeof( oDialog->cDodany ) );
}

static const char *KolumnaWazny( Control *browse, void *data )
{
   OknoTokenow *oDialog = ( OknoTokenow * )data;

   UNUSED( browse );

   return Data( "WAZNY_DO", oDialog->cWazny, sizeof( oDialog->cWazny ) );
}

static const char *KolumnaSprawdzony( Control *browse, void *data )
{
   OknoTokenow *oDialog = ( OknoTokenow * )data;

   UNUSED( browse );

   return Data( "DATA_SPRAWDZENIA", oDialog->cSprawdzony, sizeof( oDialog->cSprawdzony ) );
}

static char *Data( const char *cPole, char *cData, int nRozmiar )
{
   int dData;

   DbSelectArea( "TOKENY" );
   dData = FieldGetD( FieldPos( cPole ) );
   if( dData == 0 )
      *cData = 0;
   else
      DToC( dData, cData, ( size_t )nRozmiar );

   return cData;
}

static double Numer( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "TOKENY" );

   return ( double )RecNo();
}

static xBool Zapisz( const char *cNazwa, const char *cNip, int nSrodowisko, const char *cToken, int dWaznyDo )
{
   char *cSzyfr = KsefZaszyfruj( cToken );
   char  cTekst[ 400 ];

   if( cSzyfr == NULL )
   {
      StringFormat( cTekst, "Nie można utworzyć klucza szyfrowania w katalogu\n%s/xBiuro.\nToken nie został zapisany.", GetEnv( "APPDATA" ) );
      MsgError( cTekst, "Tokeny" );

      return F;
   }

   DbSelectArea( "TOKENY" );
   if( !NetAppBlank( NET_WAIT, T ) )
   {
      free( cSzyfr );

      return F;
   }

   FieldPutC( FieldPos( "NAZWA_TOKENU" ), cNazwa );
   FieldPutC( FieldPos( "NIP" ), cNip );
   FieldPutN( FieldPos( "SRODOWISKO_KSEF" ), nSrodowisko );
   FieldPutC( FieldPos( "TOKEN_ZASZYFROWANY" ), cSzyfr );
   FieldPutD( FieldPos( "DATA_DODANIA" ), Date() );
   FieldPutD( FieldPos( "WAZNY_DO" ), dWaznyDo );
   DbCommit();
   DbUnlock();
   free( cSzyfr );

   return T;
}

static void Dodaj( Toolbar *toolbar, void *data )
{
   OknoTokenow *oDialog = ( OknoTokenow * )data;
   Control     *GetList[ 3 ];
   long         nRecNo;
   int          nSrodowisko;
   int          dWaznyDo = DateAdd( Date(), 90 );
   char         cNazwa[ 241 ];
   char         cToken[ 2049 ];
   char         cNip[ 64 ];
   char         cTekst[ 400 ];

   UNUSED( toolbar );

   if( !FirmaPobierzKsef( oDialog->biuro, cNip, sizeof( cNip ), &nSrodowisko ) || *cNip == 0 )
   {
      MsgInfo( "Najpierw wpisz NIP firmy w menu Firma, Dane firmy.", "Tokeny" );

      return;
   }

   DbSelectArea( "TOKENY" );
   nRecNo  = RecNo();
   *cNazwa = 0;
   *cToken = 0;
   DbGoBottom();
   DbSkip( 1 );
   Okno( oDialog );
   ControlSetText( oDialog->NIP, cNip );
   ControlSetText( oDialog->SRODOWISKO_KSEF, KsefSrodowisko( nSrodowisko, T ) );

   GetList[ 0 ] = GetText( oDialog->panel, 190, 10, 60, cNazwa, sizeof( cNazwa ), "" );
   GetList[ 1 ] = GetText( oDialog->panel, 190, 40, 60, cToken, sizeof( cToken ), "" );
   GetList[ 2 ] = GetDate( oDialog->panel, 190, 100, &dWaznyDo );

   PozycjeTrybEdycji( oDialog->pasekStanu, T );
   WinReadModal( oDialog->okno, GetList, 3, 1 );
   PozycjeTrybEdycji( oDialog->pasekStanu, F );
   ControlDestroy( GetList[ 0 ] );
   ControlDestroy( GetList[ 1 ] );
   ControlDestroy( GetList[ 2 ] );

   DbSelectArea( "TOKENY" );
   if( ReadLastKey() == K_ESC || *AllTrim( cToken ) == 0 )
   {
      DbGoto( nRecNo );
      Okno( oDialog );

      return;
   }

   if( *AllTrim( cNazwa ) == 0 )
      StringFormat( cNazwa, "Token %s", DToC( Date(), cTekst, sizeof( cTekst ) ) );

   if( !Zapisz( cNazwa, cNip, nSrodowisko, cToken, dWaznyDo ) )
   {
      DbSelectArea( "TOKENY" );
      DbGoto( nRecNo );
      Okno( oDialog );

      return;
   }

   Okno( oDialog );
   Stan( oDialog );
}

static void Wstaw( Win *win, void *data )
{
   UNUSED( win );

   Dodaj( NULL, data );
}

static void Aplikacja( Toolbar *toolbar, void *data )
{
   OknoTokenow *oDialog     = ( OknoTokenow * )data;
   int          nSrodowisko = SRODOWISKO_TESTOWE;
   char         cNip[ 64 ];

   UNUSED( toolbar );

   FirmaPobierzKsef( oDialog->biuro, cNip, sizeof( cNip ), &nSrodowisko );
   DbSelectArea( "TOKENY" );
   KsefAplikacja( nSrodowisko, "Tokeny" );
}

static void Sprawdz( Toolbar *toolbar, void *data )
{
   OknoTokenow *oDialog = ( OknoTokenow * )data;
   Json        *bilety;
   char        *cToken;
   const char  *cWazny;
   int          nSrodowisko;
   char         cNip[ 64 ];
   char         cBlad[ 2000 ];
   char         cWynik[ 2100 ];
   char         cTekst[ 2200 ];

   UNUSED( toolbar );

   DbSelectArea( "TOKENY" );
   if( Eof() )
      return;

   nSrodowisko = ( int )FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) );
   StringFormat( cNip, "%s", FieldGetC( FieldPos( "NIP" ) ) );
   cToken = KsefOdszyfruj( FieldGetC( FieldPos( "TOKEN_ZASZYFROWANY" ) ) );

   StringFormat( cTekst, " Łączenie z KSeF (%s)...", KsefSrodowisko( nSrodowisko, F ) );
   StatusBarSetText( oDialog->pasekStanu, 1, cTekst );
   bilety = KsefZaloguj( AllTrim( cNip ), cToken, nSrodowisko, cBlad, sizeof( cBlad ) );
   StatusBarSetText( oDialog->pasekStanu, 1, "" );
   free( cToken );

   if( bilety == NULL )
   {
      StringFormat( cWynik, "Błąd: %s", cBlad );
      MsgError( cBlad, "Połączenie z KSeF" );
   }
   else
   {
      cWazny = JsonString( JsonGet( JsonGet( bilety, "accessToken" ), "validUntil" ) );
      cWazny = IIF( cWazny == NULL, "", cWazny );
      StringFormat( cWynik, "Połączono, bilet ważny do %s", cWazny );
      StringFormat( cTekst, "Połączono z KSeF (%s).\nBilet dostępu ważny do: %s", KsefSrodowisko( nSrodowisko, F ), cWazny );
      KsefWyloguj( nSrodowisko, bilety );
      JsonFree( bilety );
      MsgInfo( cTekst, "Połączenie z KSeF" );
   }

   DbSelectArea( "TOKENY" );
   if( NetRLock( NET_WAIT ) )
   {
      FieldPutD( FieldPos( "DATA_SPRAWDZENIA" ), Date() );
      FieldPutC( FieldPos( "WYNIK_SPRAWDZENIA" ), cWynik );
      DbCommit();
      DbUnlock();
   }
   BrowseRefresh( oDialog->BRW );
   Okno( oDialog );
}

static void Usun( Toolbar *toolbar, void *data )
{
   OknoTokenow *oDialog = ( OknoTokenow * )data;
   char         cNazwa[ 241 ];
   char         cPytanie[ 300 ];

   UNUSED( toolbar );

   DbSelectArea( "TOKENY" );
   if( Eof() )
      return;

   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_TOKENU" ) ) );
   StringFormat( cPytanie, "Czy na pewno usunąć token\n%s?", AllTrim( cNazwa ) );
   if( !MsgYesNo( cPytanie, "Tokeny" ) )
      return;

   DbSelectArea( "TOKENY" );
   if( NetRLock( NET_WAIT ) )
   {
      DbDelete();
      DbCommit();
      DbUnlock();
      DbSkip( 1 );
      if( Eof() )
         DbGoBottom();
   }
   Okno( oDialog );
   Stan( oDialog );
}

static void Zamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( OknoTokenow * )data )->okno );
}
