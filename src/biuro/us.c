/*
 * Program...: xBiuro
 * Plik......: src/us.c
 *
 */

#include <string.h>

#include "biuro.h"

/* Pola tabeli us z państwowych druków i danych KAS (zasada 13); te same pola ma narzędzie tmp/us-import */
static const DbField aDbf[] = {
   { "TYP",                'C',   4, 0 },   /* dane KAS: US, WUS */
   { "KOD_URZEDU",         'C',   4, 0 },   /* KodyUrzedowSkarbowych */
   { "NAZWA_URZEDU",       'C', 254, 0 },   /* jak w firmie */
   { "WOJEWODZTWO",        'C', 144, 0 },   /* TJednAdmin (36) */
   { "ULICA",              'C', 254, 0 },   /* TUlica (65) */
   { "NR_DOMU",            'C',  36, 0 },   /* TNrBudynku (9) */
   { "NR_LOKALU",          'C',  40, 0 },   /* TNrLokalu (10) */
   { "KOD_POCZTOWY",       'C',   8, 0 },   /* TKodPocztowy (8) */
   { "MIEJSCOWOSC",        'C', 224, 0 },   /* TMiejscowosc (56) */
   { "POCZTA",             'C', 224, 0 },   /* TAdresPolski Poczta (56) */
   { "TELEFON",            'C', 254, 0 },   /* dane KAS, kilka numerów */
   { "FAKS",               'C', 254, 0 },   /* dane KAS, kilka numerów */
   { "EMAIL",              'C', 254, 0 },   /* TAdresEmail (255) */
   { "STRONA_INTERNETOWA", 'C', 254, 0 }    /* NIP-8 poz. 90 */
};

#define POLA_US ( ( int )( sizeof( aDbf ) / sizeof( aDbf[ 0 ] ) ) )

/* Okno kartoteki i jego kontrolki, nazwane jak pola tabeli; zmienna lokalna Kartoteka(), przekazywana parametrem */
typedef struct
{
   Win       *okno;
   StatusBar *pasekStanu;
   Win       *zakladki[ 2 ];
   Control   *BRW;
   xBool      lWybor;
   char       cUlica[ 600 ];   /* tekst kolumny Ulica */
   Urzad      urzad;
   Control   *TELEFON;
   Control   *FAKS;
   Control   *EMAIL;
   Control   *STRONA_INTERNETOWA;
} OknoUs;

/* Okno szukania i wpisany tekst, który zostaje po zamknięciu okna */
typedef struct
{
   Win     *oSzukaj;
   Control *oTekst;
   char     cTekst[ 256 ];
} OknoSzukaj;

static xBool       Kartoteka( Win *win, xBool lWybor, char *kod, int rozmiarKodu, char *nazwa, int rozmiarNazwy );
static xBool       DbOpen( const char *cBaza );
static void        Przebudowa( const char *cBaza );
static void        Stan( OknoUs *oOkno );
static void        Ramka( Win *strona, int x, int y, int wysokosc, const char *podpis );
static void        Opis( Win *strona, int y, int dlugosc, const char *tekst );
static Control    *Pole( Win *strona, int x, int y, int dlugosc );
static const char *Kolumna( Control *browse, void *data );
static const char *KolumnaUlica( Control *browse, void *data );
static double      Numer( Control *browse, void *data );
static void        Zmiana( Control *browse, void *data );
static void        Okno( OknoUs *oOkno );
static void        Edycja( OknoUs *oOkno, Tryb nTryb );
static void        Pomoc( Toolbar *toolbar, void *data );
static void        Wybierz( Toolbar *toolbar, void *data );
static void        Dodaj( Toolbar *toolbar, void *data );
static void        Wstaw( Win *win, void *data );
static void        Kopia( Win *win, void *data );
static void        Popraw( Toolbar *toolbar, void *data );
static void        Usun( Toolbar *toolbar, void *data );
static void        Szukaj( Toolbar *toolbar, void *data );
static void        SzukajOk( Control *button, void *data );
static void        SzukajAnuluj( Control *button, void *data );
static void        Zamknij( Toolbar *toolbar, void *data );

// Kartoteka urzędów skarbowych z menu
// Us( win, data ) --> void
void Us( Win *win, void *data )
{
   UNUSED( data );

   Kartoteka( win, F, NULL, 0, NULL, 0 );
}

// Wybór urzędu z kartoteki: kod i nazwa wybranego urzędu
// UsWybierz( win, kod, rozmiarKodu, nazwa, rozmiarNazwy ) --> lWybrany
xBool UsWybierz( Win *win, char *kod, int rozmiarKodu, char *nazwa, int rozmiarNazwy )
{
   return Kartoteka( win, T, kod, rozmiarKodu, nazwa, rozmiarNazwy );
}

// PACK tabeli urzędów przy indeksowaniu
// UsNtx() --> lWynik
xBool UsNtx( void )
{
   char  cBaza[ 64 ];
   char  cPlik[ 64 ];
   xBool lWynik = T;

   StringFormat( cBaza, DANE_BAZA "kartoteka/us" );
   StringFormat( cPlik, "%s.dbf", cBaza );
   if( FileExists( cPlik ) )
   {
      if( NetUse( cBaza, T, NET_WAIT, "US", F ) )
      {
         lWynik = NetPack();
         DbCloseArea();
      }
      else
         lWynik = F;
   }

   return lWynik;
}

// Strona z danymi urzędu: typ, kod, nazwa i adres siedziby; wspólna dla kartoteki US i danych podmiotu
// UsZakladka( urzad, strona ) --> void
void UsZakladka( Urzad *urzad, Win *strona )
{
   Ramka( strona, 10, 8, 80, "Dane identyfikacyjne" );
   Opis( strona,  33, 15, "Typ / Kod urzędu:" );
   Opis( strona,  63, 15, "Nazwa urzędu:" );
   Ramka( strona, 10, 96, 180, "Adres siedziby" );
   Opis( strona, 123, 15, "Województwo:" );
   Opis( strona, 153, 15, "Ulica:" );
   Opis( strona, 183, 15, "Nr domu / lokalu:" );
   Opis( strona, 213, 15, "Kod / Miejscowość:" );
   Opis( strona, 243, 15, "Poczta:" );
   urzad->TYP          = Pole( strona, 175,  30,  4 );
   urzad->KOD_URZEDU   = Pole( strona, 230,  30,  4 );
   urzad->NAZWA_URZEDU = Pole( strona, 175,  60, 44 );
   urzad->WOJEWODZTWO  = Pole( strona, 175, 120, 36 );
   urzad->ULICA        = Pole( strona, 175, 150, 40 );
   urzad->NR_DOMU      = Pole( strona, 175, 180,  9 );
   urzad->NR_LOKALU    = Pole( strona, 274, 180, 10 );
   urzad->KOD_POCZTOWY = Pole( strona, 175, 210,  6 );
   urzad->MIEJSCOWOSC  = Pole( strona, 247, 210, 32 );
   urzad->POCZTA       = Pole( strona, 175, 240, 40 );
}

// Pokazuje urząd z bieżącego rekordu bieżącego obszaru
// UsOkno( urzad ) --> void
void UsOkno( Urzad *urzad )
{
   ControlSetText( urzad->TYP,          FieldGetC( FieldPos( "TYP" ) ) );
   ControlSetText( urzad->KOD_URZEDU,   FieldGetC( FieldPos( "KOD_URZEDU" ) ) );
   ControlSetText( urzad->NAZWA_URZEDU, FieldGetC( FieldPos( "NAZWA_URZEDU" ) ) );
   ControlSetText( urzad->WOJEWODZTWO,  FieldGetC( FieldPos( "WOJEWODZTWO" ) ) );
   ControlSetText( urzad->ULICA,        FieldGetC( FieldPos( "ULICA" ) ) );
   ControlSetText( urzad->NR_DOMU,      FieldGetC( FieldPos( "NR_DOMU" ) ) );
   ControlSetText( urzad->NR_LOKALU,    FieldGetC( FieldPos( "NR_LOKALU" ) ) );
   ControlSetText( urzad->KOD_POCZTOWY, FieldGetC( FieldPos( "KOD_POCZTOWY" ) ) );
   ControlSetText( urzad->MIEJSCOWOSC,  FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
   ControlSetText( urzad->POCZTA,       FieldGetC( FieldPos( "POCZTA" ) ) );
}

// Otwiera kartotekę US w obszarze URZAD na urzędzie o kodzie kod; T, gdy jest (obszar zamyka wołający), F bez kartoteki
// albo bez urzędu o tym kodzie (obszar zamknięty)
// UsZnajdz( kod ) --> lJest
xBool UsZnajdz( const char *kod )
{
   char cBaza[ 64 ];
   char cPlik[ 64 ];
   char cKod[ 5 ];
   char cKodUrzedu[ 5 ];

   StringFormat( cKod, "%s", kod );
   AllTrim( cKod );
   StringFormat( cBaza, DANE_BAZA "kartoteka/us" );
   StringFormat( cPlik, "%s.dbf", cBaza );

   if( *cKod == 0 || !FileExists( cPlik ) || !NetUse( cBaza, F, NET_WAIT, "URZAD", T ) )
      return F;

   for( DbGoTop(); !Eof(); DbSkip( 1 ) )
   {
      StringFormat( cKodUrzedu, "%s", FieldGetC( FieldPos( "KOD_URZEDU" ) ) );
      if( strcmp( AllTrim( cKodUrzedu ), cKod ) == 0 )
         return T;
   }

   DbCloseArea();

   return F;
}

// Pokazuje urząd o kodzie kod z kartoteki US (UsZnajdz()); bez urzędu w kartotece sam kod
// UsPokaz( urzad, kod ) --> void
void UsPokaz( Urzad *urzad, const char *kod )
{
   char cKod[ 5 ];

   StringFormat( cKod, "%s", kod );
   AllTrim( cKod );

   if( UsZnajdz( cKod ) )
   {
      UsOkno( urzad );
      DbCloseArea();
   }
   else
   {
      ControlSetText( urzad->TYP, "" );
      ControlSetText( urzad->NAZWA_URZEDU, "" );
      ControlSetText( urzad->WOJEWODZTWO, "" );
      ControlSetText( urzad->ULICA, "" );
      ControlSetText( urzad->NR_DOMU, "" );
      ControlSetText( urzad->NR_LOKALU, "" );
      ControlSetText( urzad->KOD_POCZTOWY, "" );
      ControlSetText( urzad->MIEJSCOWOSC, "" );
      ControlSetText( urzad->POCZTA, "" );
   }

   ControlSetText( urzad->KOD_URZEDU, cKod );
}

// Okno kartoteki; przy wyborze kod i nazwa urzędu wybranego Enterem
// Kartoteka( win, lWybor, kod, rozmiarKodu, nazwa, rozmiarNazwy ) --> lWybrany
static xBool Kartoteka( Win *win, xBool lWybor, char *kod, int rozmiarKodu, char *nazwa, int rozmiarNazwy )
{
   static const char *nazwyZakladek[] = { "Dane urzędu", "Dane kontaktowe" };
   OknoUs   oknoUs   = { 0 };
   OknoUs  *oOkno    = &oknoUs;
   Toolbar *pasek;
   Win     *panel;
   Control *zakladka;
   Win     *strona;
   char     cBaza[ 64 ];
   int      przycisk;
   xBool    lWybrany = F;

   StringFormat( cBaza, DANE_BAZA "kartoteka/us" );
   if( !DbOpen( cBaza ) )
      return F;

   oOkno->lWybor = lWybor;
   oOkno->okno   = DialogCreate( win, IIF( lWybor, "Wybór urzędu", "Dane Krajowej Administracji Skarbowej" ), 900, 640 );
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
   if( lWybor )
   {
      przycisk = ToolbarAdd( pasek, "Enter-Wybierz", Wybierz, oOkno );
      ToolbarSetShortcut( pasek, przycisk, "Enter" );
      ToolbarAddSeparator( pasek );
   }
   przycisk = ToolbarAdd( pasek, "F2-Dodaj", Dodaj, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "F2" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "F4-Popraw", Popraw, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "F4" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "F8-Usuń", Usun, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "F8" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "F12-Szukaj", Szukaj, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "F12" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, IIF( lWybor, "Esc-Rezygnuj", "Esc-Zamknij" ), Zamknij, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "Esc" );

   panel = PanelCreate( oOkno->okno, 0, 0, 0, 0 );
   zakladka = TabControlCreate( panel, 10, 10, WinWidth( panel ) - 20, 310 );
   ControlSetAnchor( zakladka, "LTR" );
   for( int i = 0; i < 2; ++i )
      oOkno->zakladki[ i ] = TabControlAddPage( zakladka, nazwyZakladek[ i ] );

   // Dane urzędu
   UsZakladka( &oOkno->urzad, oOkno->zakladki[ 0 ] );

   // Dane kontaktowe
   strona = oOkno->zakladki[ 1 ];
   Ramka( strona, 10, 8, 140, "" );
   Opis( strona,  33, 15, "Telefon:" );
   Opis( strona,  63, 15, "Faks:" );
   Opis( strona,  93, 15, "E-mail:" );
   Opis( strona, 123, 15, "Strona internetowa:" );
   oOkno->TELEFON            = Pole( strona, 175,  30, 44 );
   oOkno->FAKS               = Pole( strona, 175,  60, 44 );
   oOkno->EMAIL              = Pole( strona, 175,  90, 44 );
   oOkno->STRONA_INTERNETOWA = Pole( strona, 175, 120, 44 );

   // Przeglądarka urzędów
   oOkno->BRW = BrowseCreate( panel, 10, 330, WinWidth( panel ) - 20, WinHeight( panel ) - 340 );
   ControlSetAnchor( oOkno->BRW, "LTRB" );
   BrowseSetColor( oOkno->BRW, BROWSE_HEADBACKGROUND, 0x4D7326 );
   DbBrowse( oOkno->BRW, "US" );
   BrowseAddNumber( oOkno->BRW, "Poz.", 4, "9999", Numer, NULL );
   BrowseAddColumn( oOkno->BRW, "Kod", 6, BROWSE_LEFT, Kolumna, "KOD_URZEDU" );
   BrowseSetFreeze( oOkno->BRW, 2 );
   BrowseAddColumn( oOkno->BRW, "Nazwa urzędu", 40, BROWSE_LEFT, Kolumna, "NAZWA_URZEDU" );
   BrowseAddColumn( oOkno->BRW, "Ulica", 30, BROWSE_LEFT, KolumnaUlica, oOkno );
   BrowseAddColumn( oOkno->BRW, "KodPP", 6, BROWSE_LEFT, Kolumna, "KOD_POCZTOWY" );
   BrowseAddColumn( oOkno->BRW, "Miejscowość", 20, BROWSE_LEFT, Kolumna, "MIEJSCOWOSC" );
   BrowseAddColumn( oOkno->BRW, "Województwo", 20, BROWSE_LEFT, Kolumna, "WOJEWODZTWO" );
   ControlSetAction( oOkno->BRW, Zmiana, oOkno );

   DbSelectArea( "US" );
   DbGoTop();
   Okno( oOkno );
   Stan( oOkno );

   WinCenter( oOkno->okno );
   if( DialogShowModal( oOkno->okno ) == 1 && lWybor && !DbPusta( "US" ) && !Eof() )
   {
      StringFormatSize( kod, ( size_t )rozmiarKodu, "%s", FieldGetC( FieldPos( "KOD_URZEDU" ) ) );
      StringFormatSize( nazwa, ( size_t )rozmiarNazwy, "%s", FieldGetC( FieldPos( "NAZWA_URZEDU" ) ) );
      RTrim( kod );
      RTrim( nazwa );
      lWybrany = T;
   }

   DbSelectArea( "US" );
   DbCloseArea();

   return lWybrany;
}

// Otwiera tabelę urzędów; nową bierze z prog-zaso/us.dbf, a bez niej zakłada pustą; przebudowuje przy innych polach
// DbOpen( cBaza ) --> lOtwarta
static xBool DbOpen( const char *cBaza )
{
   char cPlik[ 64 ];

   StringFormat( cPlik, "%s.dbf", cBaza );
   if( !FileExists( cPlik ) && !FileCopy( PROG_ZASO "us.dbf", cPlik ) )
      DbCreate( cBaza, aDbf, POLA_US );

   if( !NetUse( cBaza, F, NET_WAIT, "US", F ) )
      return F;

   if( FCount() != POLA_US )
   {
      DbCloseArea();
      MsgInfo( "Dodanie pola w bazie", "Dane Krajowej Administracji Skarbowej" );
      Przebudowa( cBaza );
      if( !NetUse( cBaza, F, NET_WAIT, "US", F ) )
         return F;
   }

   return T;
}

// Przebudowuje tabelę urzędów do pól aDbf
// Przebudowa( cBaza ) --> void
static void Przebudowa( const char *cBaza )
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
      DbCreate( cBaza, aDbf, POLA_US );
      if( NetUse( cBaza, F, NET_WAIT, "US", F ) )
      {
         DbAppendFrom( cTmp, NULL, NULL );
         DbCloseArea();
      }
      FErase( cTmpDbf );
      FErase( cTmpDbt );
      FErase( cStara );
   }
}

// Przeglądarka włączona tylko z urzędami
// Stan( oOkno ) --> void
static void Stan( OknoUs *oOkno )
{
   xBool lJest = !DbPusta( "US" );

   ControlSetEnabled( oOkno->BRW, lJest );
   BrowseRefresh( oOkno->BRW );

   if( lJest )
      ControlSetFocus( oOkno->BRW );
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

// Kolumna z polem o nazwie data
// Kolumna( browse, data ) --> cTekst
static const char *Kolumna( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "US" );

   return FieldGetC( FieldPos( ( const char * )data ) );
}

// Kolumna Ulica: ulica z numerem domu i lokalu
// KolumnaUlica( browse, data ) --> cTekst
static const char *KolumnaUlica( Control *browse, void *data )
{
   OknoUs *oOkno = ( OknoUs * )data;
   char    cUlica[ 255 ];
   char    cNrDomu[ 37 ];
   char    cNrLokalu[ 41 ];

   UNUSED( browse );

   DbSelectArea( "US" );
   StringFormat( cUlica, "%s", FieldGetC( FieldPos( "ULICA" ) ) );
   StringFormat( cNrDomu, "%s", FieldGetC( FieldPos( "NR_DOMU" ) ) );
   StringFormat( cNrLokalu, "%s", FieldGetC( FieldPos( "NR_LOKALU" ) ) );
   RTrim( cUlica );
   RTrim( cNrDomu );
   RTrim( cNrLokalu );
   StringFormat( oOkno->cUlica, "%s %s%s%s", cUlica, cNrDomu, IIF( *cNrLokalu == 0, "", "/" ), cNrLokalu );

   return oOkno->cUlica;
}

// Kolumna Poz.: numer rekordu
// Numer( browse, data ) --> nNumer
static double Numer( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "US" );

   return ( double )RecNo();
}

// Belka przeglądarki na innym urzędzie
// Zmiana( browse, data ) --> void
static void Zmiana( Control *browse, void *data )
{
   UNUSED( browse );

   Okno( ( OknoUs * )data );
}

// Pokazuje dane urzędu
// Okno( oOkno ) --> void
static void Okno( OknoUs *oOkno )
{
   DbSelectArea( "US" );

   // Dane urzędu
   UsOkno( &oOkno->urzad );

   // Dane kontaktowe
   ControlSetText( oOkno->TELEFON,            FieldGetC( FieldPos( "TELEFON" ) ) );
   ControlSetText( oOkno->FAKS,               FieldGetC( FieldPos( "FAKS" ) ) );
   ControlSetText( oOkno->EMAIL,              FieldGetC( FieldPos( "EMAIL" ) ) );
   ControlSetText( oOkno->STRONA_INTERNETOWA, FieldGetC( FieldPos( "STRONA_INTERNETOWA" ) ) );
}

// Nowy urząd, kopia bieżącego albo poprawienie bieżącego; zmienne cNAZWAPOLA jak pola tabeli
// Edycja( oOkno, nTryb ) --> void
static void Edycja( OknoUs *oOkno, Tryb nTryb )
{
   Control *GetList[ 16 ];
   long     nRecNo;
   char     cTYP[ 5 ];
   char     cKOD_URZEDU[ 5 ];
   char     cNAZWA_URZEDU[ 255 ];
   char     cWOJEWODZTWO[ 145 ];
   char     cULICA[ 255 ];
   char     cNR_DOMU[ 37 ];
   char     cNR_LOKALU[ 41 ];
   char     cKOD_POCZTOWY[ 9 ];
   char     cMIEJSCOWOSC[ 225 ];
   char     cPOCZTA[ 225 ];
   char     cTELEFON[ 255 ];
   char     cFAKS[ 255 ];
   char     cEMAIL[ 255 ];
   char     cSTRONA_INTERNETOWA[ 255 ];
   char     cTekst[ 255 ];
   int      nGet = 0;

   DbSelectArea( "US" );
   nRecNo = RecNo();

   if( ( nTryb == KOPIA || nTryb == POPRAW ) && Eof() )
      return;

   if( nTryb == NOWY )
   {
      DbGoBottom();
      DbSkip( 1 );
   }

   // Dane urzędu
   StringFormat( cTYP,          "%s", IIF( nTryb == NOWY, "US", FieldGetC( FieldPos( "TYP" ) ) ) );
   StringFormat( cKOD_URZEDU,   "%s", FieldGetC( FieldPos( "KOD_URZEDU" ) ) );
   StringFormat( cNAZWA_URZEDU, "%s", FieldGetC( FieldPos( "NAZWA_URZEDU" ) ) );
   StringFormat( cWOJEWODZTWO,  "%s", FieldGetC( FieldPos( "WOJEWODZTWO" ) ) );
   StringFormat( cULICA,        "%s", FieldGetC( FieldPos( "ULICA" ) ) );
   StringFormat( cNR_DOMU,      "%s", FieldGetC( FieldPos( "NR_DOMU" ) ) );
   StringFormat( cNR_LOKALU,    "%s", FieldGetC( FieldPos( "NR_LOKALU" ) ) );
   StringFormat( cKOD_POCZTOWY, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY" ) ) );
   StringFormat( cMIEJSCOWOSC,  "%s", FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
   StringFormat( cPOCZTA,       "%s", FieldGetC( FieldPos( "POCZTA" ) ) );

   // Dane kontaktowe
   StringFormat( cTELEFON,            "%s", FieldGetC( FieldPos( "TELEFON" ) ) );
   StringFormat( cFAKS,               "%s", FieldGetC( FieldPos( "FAKS" ) ) );
   StringFormat( cEMAIL,              "%s", FieldGetC( FieldPos( "EMAIL" ) ) );
   StringFormat( cSTRONA_INTERNETOWA, "%s", FieldGetC( FieldPos( "STRONA_INTERNETOWA" ) ) );

   // Dane urzędu
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 0 ], 175,  30,  4, cTYP, sizeof( cTYP ), "@!" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 0 ], 230,  30,  4, cKOD_URZEDU, sizeof( cKOD_URZEDU ), "9999" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 0 ], 175,  60, 44, cNAZWA_URZEDU, sizeof( cNAZWA_URZEDU ), "" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 0 ], 175, 120, 36, cWOJEWODZTWO, sizeof( cWOJEWODZTWO ), "" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 0 ], 175, 150, 40, cULICA, sizeof( cULICA ), "" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 0 ], 175, 180,  9, cNR_DOMU, sizeof( cNR_DOMU ), "" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 0 ], 274, 180, 10, cNR_LOKALU, sizeof( cNR_LOKALU ), "" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 0 ], 175, 210,  6, cKOD_POCZTOWY, sizeof( cKOD_POCZTOWY ), "99-999" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 0 ], 247, 210, 32, cMIEJSCOWOSC, sizeof( cMIEJSCOWOSC ), "" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 0 ], 175, 240, 40, cPOCZTA, sizeof( cPOCZTA ), "" );

   // Dane kontaktowe
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 1 ], 175,  30, 44, cTELEFON, sizeof( cTELEFON ), "" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 1 ], 175,  60, 44, cFAKS, sizeof( cFAKS ), "" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 1 ], 175,  90, 44, cEMAIL, sizeof( cEMAIL ), "" );
   GetList[ nGet++ ] = GetText( oOkno->zakladki[ 1 ], 175, 120, 44, cSTRONA_INTERNETOWA, sizeof( cSTRONA_INTERNETOWA ), "" );

   StatusBarSetText( oOkno->pasekStanu, 1, " PgUp PgDn Enter - zapisz, Esc - bez zapisu" );
   StatusBarSetBackground( oOkno->pasekStanu, 0x73264D );
   StatusBarSetForeground( oOkno->pasekStanu, 0xFFFFFF );

   WinReadModal( oOkno->okno, GetList, nGet, 2 );

   StatusBarSetText( oOkno->pasekStanu, 1, "" );
   StatusBarSetBackground( oOkno->pasekStanu, -1 );
   StatusBarSetForeground( oOkno->pasekStanu, -1 );

   for( int i = 0; i < nGet; ++i )
      ControlDestroy( GetList[ i ] );

   DbSelectArea( "US" );
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

   // Dane urzędu
   FieldPutC( FieldPos( "TYP" ),          cTYP );
   FieldPutC( FieldPos( "KOD_URZEDU" ),   cKOD_URZEDU );
   FieldPutC( FieldPos( "NAZWA_URZEDU" ), cNAZWA_URZEDU );
   FieldPutC( FieldPos( "WOJEWODZTWO" ),  cWOJEWODZTWO );
   FieldPutC( FieldPos( "ULICA" ),        cULICA );
   FieldPutC( FieldPos( "NR_DOMU" ),      cNR_DOMU );
   FieldPutC( FieldPos( "NR_LOKALU" ),    cNR_LOKALU );
   FieldPutC( FieldPos( "KOD_POCZTOWY" ), IIF( *AllTrim( StrTran( cTekst, sizeof( cTekst ), cKOD_POCZTOWY, "-", "" ) ) == 0, "", cKOD_POCZTOWY ) );
   FieldPutC( FieldPos( "MIEJSCOWOSC" ),  cMIEJSCOWOSC );
   FieldPutC( FieldPos( "POCZTA" ),       cPOCZTA );

   // Dane kontaktowe
   FieldPutC( FieldPos( "TELEFON" ),            cTELEFON );
   FieldPutC( FieldPos( "FAKS" ),               cFAKS );
   FieldPutC( FieldPos( "EMAIL" ),              cEMAIL );
   FieldPutC( FieldPos( "STRONA_INTERNETOWA" ), cSTRONA_INTERNETOWA );

   DbCommit();
   DbUnlock();

   Stan( oOkno );
   Okno( oOkno );
}

// F1: pomoc
// Pomoc( toolbar, data ) --> void
static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgInfo( "Przeglądanie\n\n"
            "F1\tpomoc\n"
            "Enter\twybór urzędu, gdy okno otwarto F12 z danych firmy\n"
            "F2, Ins\tnowy urząd\n"
            "F3\tkopia bieżącego urzędu jako nowy\n"
            "F4\tpoprawienie urzędu\n"
            "F8\tusunięcie urzędu\n"
            "F12\tszukanie w kolumnie, na której stoi belka\n"
            "Strzałki\tpoprzedni / następny urząd, lewo / prawo kolumna\n"
            "PgUp, PgDn\tpoprzednia / następna strona\n"
            "Ctrl+Home\tpierwszy urząd\n"
            "Ctrl+End\tostatni urząd\n"
            "Esc\tzamknięcie kartoteki\n\n"
            "Wpisywanie danych\n\n"
            "Enter\tnastępne pole, na ostatnim polu ostatniej zakładki zapis\n"
            "Tab\tnastępne pole, bez zapisu\n"
            "Shift+Tab\tpoprzednie pole\n"
            "Ctrl+Tab\tnastępna zakładka danych\n"
            "PgUp, PgDn\tzapis\n"
            "Esc\tbez zapisu", "Dane Krajowej Administracji Skarbowej" );
}

// Enter: wybiera urząd
// Wybierz( toolbar, data ) --> void
static void Wybierz( Toolbar *toolbar, void *data )
{
   OknoUs *oOkno = ( OknoUs * )data;

   UNUSED( toolbar );

   if( !DbPusta( "US" ) )
      DialogEnd( oOkno->okno, 1 );
}

// F2: nowy urząd
// Dodaj( toolbar, data ) --> void
static void Dodaj( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   Edycja( ( OknoUs * )data, NOWY );
}

// Ins: nowy urząd
// Wstaw( win, data ) --> void
static void Wstaw( Win *win, void *data )
{
   UNUSED( win );

   Edycja( ( OknoUs * )data, NOWY );
}

// F3: kopia bieżącego urzędu jako nowy
// Kopia( win, data ) --> void
static void Kopia( Win *win, void *data )
{
   UNUSED( win );

   Edycja( ( OknoUs * )data, KOPIA );
}

// F4: poprawienie urzędu
// Popraw( toolbar, data ) --> void
static void Popraw( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   Edycja( ( OknoUs * )data, POPRAW );
}

// F8: usunięcie urzędu po pytaniu
// Usun( toolbar, data ) --> void
static void Usun( Toolbar *toolbar, void *data )
{
   OknoUs *oOkno = ( OknoUs * )data;
   char    cNazwa[ 255 ];
   char    cPytanie[ 400 ];

   UNUSED( toolbar );

   DbSelectArea( "US" );
   if( Eof() )
      return;

   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_URZEDU" ) ) );
   StringFormat( cPytanie, "Czy na pewno usunąć urząd\n%s?", AllTrim( cNazwa ) );
   if( MsgYesNo( cPytanie, "Dane Krajowej Administracji Skarbowej" ) )
   {
      DbSelectArea( "US" );
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

// F12: szukanie tekstu w kolumnie, na której stoi belka, od następnego urzędu
// Szukaj( toolbar, data ) --> void
static void Szukaj( Toolbar *toolbar, void *data )
{
   OknoUs     *oOkno  = ( OknoUs * )data;
   OknoSzukaj  szukaj = { 0 };
   const char *cOpis;
   const char *cPole;
   Control    *oOk;
   Control    *oAnuluj;
   char        cWartosc[ 255 ];
   long        nRecNo;

   UNUSED( toolbar );

   DbSelectArea( "US" );
   if( Eof() )
      return;

   switch( BrowseColPos( oOkno->BRW ) )
   {
   case 2:
      cOpis = "Kod urzędu:";
      cPole = "KOD_URZEDU";
      break;
   case 4:
      cOpis = "Ulica:";
      cPole = "ULICA";
      break;
   case 6:
      cOpis = "Miejscowość:";
      cPole = "MIEJSCOWOSC";
      break;
   case 7:
      cOpis = "Województwo:";
      cPole = "WOJEWODZTWO";
      break;
   default:
      cOpis = "Nazwa urzędu:";
      cPole = "NAZWA_URZEDU";
   }

   szukaj.oSzukaj = DialogCreate( oOkno->okno, "Szukaj urzędu", 400, 120 );
   SayCreate( szukaj.oSzukaj, 10, 12, 41, 20, cOpis );
   szukaj.oTekst = EditCreate( szukaj.oSzukaj, 10, 36, 380, 24, "" );
   oOk           = ButtonCreate( szukaj.oSzukaj, 200, 76, 90, 28, "OK" );
   oAnuluj       = ButtonCreate( szukaj.oSzukaj, 300, 76, 90, 28, "Anuluj" );
   ButtonSetDefault( oOk, T );
   ControlSetAction( oOk, SzukajOk, &szukaj );
   ButtonSetCancel( oAnuluj, T );
   ControlSetAction( oAnuluj, SzukajAnuluj, &szukaj );
   WinCenter( szukaj.oSzukaj );
   DialogShowModal( szukaj.oSzukaj );

   Upper( AllTrim( szukaj.cTekst ) );
   if( *szukaj.cTekst == 0 )
      return;

   DbSelectArea( "US" );
   nRecNo = RecNo();
   for( ;; )
   {
      DbSkip( 1 );
      if( Eof() )
         DbGoTop();

      StringFormat( cWartosc, "%s", FieldGetC( FieldPos( cPole ) ) );
      if( strstr( Upper( cWartosc ), szukaj.cTekst ) != NULL )
      {
         BrowseRefresh( oOkno->BRW );
         Okno( oOkno );
         return;
      }

      if( RecNo() == nRecNo )
         break;
   }

   StringFormat( cWartosc, "Nie znaleziono: %s", szukaj.cTekst );
   MsgInfo( cWartosc, "Dane Krajowej Administracji Skarbowej" );
}

// OK w oknie szukania: zapamiętuje tekst i zamyka okno
// SzukajOk( button, data ) --> void
static void SzukajOk( Control *button, void *data )
{
   OknoSzukaj *szukaj = ( OknoSzukaj * )data;

   UNUSED( button );

   StringFormat( szukaj->cTekst, "%s", ControlText( szukaj->oTekst ) );
   DialogEnd( szukaj->oSzukaj, 1 );
}

// Anuluj w oknie szukania
// SzukajAnuluj( button, data ) --> void
static void SzukajAnuluj( Control *button, void *data )
{
   UNUSED( button );

   DialogEnd( ( ( OknoSzukaj * )data )->oSzukaj, 0 );
}

// Esc: zamyka okno
// Zamknij( toolbar, data ) --> void
static void Zamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( OknoUs * )data )->okno );
}
