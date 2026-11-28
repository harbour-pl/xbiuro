/*
 * Program...: xBiuro
 * Plik......: src/biuro/kontrahent.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "biuro.h"

/* Okno kartoteki: pola i zakładki podmiotu z podmiot.c; zmienna lokalna Kartoteka(), przekazywana parametrem.
   aZaznaczone to numery zaznaczonych rekordów (hZaznaczone w kontrahent.prg) */
typedef struct
{
   Biuro     *biuro;
   Win       *okno;
   StatusBar *pasekStanu;
   Control   *BRW;
   xBool      lWybor;
   long      *aZaznaczone;
   int        nZaznaczone;
   int        nMiejsca;
   char       cNazwa[ 2049 ];   /* tekst kolumny Nazwa */
   char       cUlica[ 600 ];    /* tekst kolumny Ulica */
   Podmiot    podmiot;
} OknoKontrahenta;

/* Okno szukania i wpisany tekst, który zostaje po zamknięciu okna */
typedef struct
{
   Win     *oSzukaj;
   Control *oTekst;
   char     cTekst[ 256 ];
} OknoSzukaj;

/* Okno pytania, dokąd drukować */
typedef struct
{
   Win     *oPytanie;
   Control *oPdf;
   Control *oSvg;
} OknoPytanie;

/* Wiersz wydruku: liczba porządkowa, NIP, nazwa, ulica, kod z miejscowością, telefon, e-mail */
typedef struct
{
   char cLp[ 16 ];
   char cNip[ 64 ];
   char cNazwa[ 2049 ];
   char cUlica[ 600 ];
   char cMiasto[ 300 ];
   char cTelefon[ 32 ];
   char cEmail[ 255 ];
} WierszDruku;

/* Miejsce na tekst w kolumnie PDF: strona i szerokość */
typedef struct
{
   HPDF_Page pPage;
   double    nSzer;
} MiejscePdf;

/* Czy tekst mieści się w kolumnie (bMiesci w DwieLinie() z kontrahent.prg) */
typedef xBool ( *Miesci )( const char *cTekst, void *data );

static long         Kartoteka( Win *win, Biuro *biuro, xBool lWybor );
static void         Stan( OknoKontrahenta *oOkno );
static const char  *Kolumna( Control *browse, void *data );
static const char  *KolumnaNazwa( Control *browse, void *data );
static const char  *KolumnaUlica( Control *browse, void *data );
static double       Numer( Control *browse, void *data );
static void         Zmiana( Control *browse, void *data );
static void         Pomoc( Toolbar *toolbar, void *data );
static void         Edycja( OknoKontrahenta *oOkno, Tryb nTryb );
static void         Wybierz( Toolbar *toolbar, void *data );
static void         Dodaj( Toolbar *toolbar, void *data );
static void         Wstaw( Win *win, void *data );
static void         Kopia( Win *win, void *data );
static void         Popraw( Toolbar *toolbar, void *data );
static void         Usun( Toolbar *toolbar, void *data );
static xBool        Zaznaczony( Control *browse, void *data );
static void         Zaznacz( Control *browse, xBool lZaznacz, void *data );
static xBool        SaZaznaczone( OknoKontrahenta *oOkno );
static void         Szukaj( Toolbar *toolbar, void *data );
static void         SzukajOk( Control *button, void *data );
static void         SzukajAnuluj( Control *button, void *data );
static void         DrukujBiezacego( Win *win, void *data );
static void         DrukujZaznaczone( Win *win, void *data );
static void         DrukujWszystkich( Win *win, void *data );
static void         DrukujKarte( Win *win, void *data );
static void         Drukuj( OknoKontrahenta *oOkno, Druk nZakres );
static void         Gdzie( Control *button, void *data );
static WierszDruku *DoDruku( OknoKontrahenta *oOkno, Druk nZakres, int *nDane );
static void         Wiersz( int nLp, WierszDruku *wiersz );
static void         DwieLinie( const char *cTekst, Miesci miesci, void *data, char *cLinia1, char *cLinia2, size_t nRozmiar );
static xBool        MiesciPdf( const char *cTekst, void *data );
static xBool        MiesciSvg( const char *cTekst, void *data );
static xBool        DrukPdf( const char *cPlik, const char *cTytul, WierszDruku *aDane, int nDane );
static xBool        DrukSvg( const char *cPlik, const char *cTytul, WierszDruku *aDane, int nDane );
static const char  *SvgTekst( const char *cTekst, int nZnakow, char *cWynik, size_t nRozmiar );
static void         Zamknij( Toolbar *toolbar, void *data );

// Kartoteka kontrahentów z menu
// Kontrahent( win, data ) --> void
void Kontrahent( Win *win, void *data )
{
   Kartoteka( win, ( Biuro * )data, F );
}

// Wybór kontrahenta z kartoteki (F12 przy wpisywaniu, np. odbiorca przelewu); numer rekordu, 0 bez wyboru
// KontrahentWybierz( win, biuro ) --> nRecNo
long KontrahentWybierz( Win *win, Biuro *biuro )
{
   return Kartoteka( win, biuro, T );
}

// PACK tabeli kontrahentów przy indeksowaniu
// KontrahentNtx() --> lWynik
xBool KontrahentNtx( void )
{
   char  cBaza[ 64 ];
   char  cPlik[ 64 ];
   xBool lWynik = T;

   StringFormat( cBaza, DANE_BAZA "kartoteka/kontrah" );
   StringFormat( cPlik, "%s.dbf", cBaza );
   if( FileExists( cPlik ) )
   {
      if( NetUse( cBaza, T, NET_WAIT, "KONTRAH", F ) )
      {
         lWynik = NetPack();
         DbCloseArea();
      }
      else
         lWynik = F;
   }

   return lWynik;
}

// Okno kartoteki; przy wyborze numer rekordu kontrahenta wybranego Enterem, 0 bez wyboru
// Kartoteka( win, biuro, lWybor ) --> nRecNo
static long Kartoteka( Win *win, Biuro *biuro, xBool lWybor )
{
   OknoKontrahenta  oknoKontrahenta = { 0 };
   OknoKontrahenta *oOkno           = &oknoKontrahenta;
   Toolbar         *pasek;
   Menu            *oDrukuj;
   Win             *panel;
   Control         *zakladka;
   char             cBaza[ 64 ];
   int              przycisk;
   long             nWybrany        = 0;

   StringFormat( cBaza, DANE_BAZA "kartoteka/kontrah" );
   if( !PodmiotDbOpen( cBaza, "KONTRAH" ) )
      return 0;

   oOkno->biuro  = biuro;
   oOkno->lWybor = lWybor;
   oOkno->okno   = DialogCreate( win, IIF( lWybor, "Wybór kontrahenta", "Kartoteka kontrahentów" ), 900, 780 );
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

   oDrukuj = MenuCreatePopup();
   MenuAdd( oDrukuj, "Drukuj bieżącego", DrukujBiezacego, oOkno, "" );
   MenuAdd( oDrukuj, "Drukuj zaznaczone", DrukujZaznaczone, oOkno, "" );
   MenuAdd( oDrukuj, "Drukuj wszystkich", DrukujWszystkich, oOkno, "" );
   MenuAddSeparator( oDrukuj );
   MenuAdd( oDrukuj, "Karta kontrahenta", DrukujKarte, oOkno, "" );
   przycisk = ToolbarAddMenu( pasek, "F9-Drukuj", oDrukuj );
   ToolbarSetShortcut( pasek, przycisk, "F9" );

   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "F12-Szukaj", Szukaj, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "F12" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, IIF( lWybor, "Esc-Rezygnuj", "Esc-Zamknij" ), Zamknij, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "Esc" );

   panel = PanelCreate( oOkno->okno, 0, 0, 0, 0 );
   zakladka = TabControlCreate( panel, 10, 10, WinWidth( panel ) - 20, 390 );
   ControlSetAnchor( zakladka, "LTR" );
   oOkno->podmiot.cAlias = "KONTRAH";
   PodmiotZakladki( &oOkno->podmiot, zakladka, "Dane kontrahenta" );

   // Przeglądarka kontrahentów
   oOkno->BRW = BrowseCreate( panel, 10, 410, WinWidth( panel ) - 20, WinHeight( panel ) - 420 );
   ControlSetAnchor( oOkno->BRW, "LTRB" );
   BrowseSetColor( oOkno->BRW, BROWSE_HEADBACKGROUND, 0x4D7326 );
   BrowseSetColor( oOkno->BRW, BROWSE_CHECKFOREGROUND, 0xFF0000 );
   DbBrowse( oOkno->BRW, "KONTRAH" );
   BrowseAddNumber( oOkno->BRW, "Poz.", 4, "9999", Numer, NULL );
   BrowseAddCheck( oOkno->BRW, "Chk", Zaznaczony, Zaznacz, oOkno );
   BrowseSetFreeze( oOkno->BRW, 2 );
   BrowseAddColumn( oOkno->BRW, "NIP", 20, BROWSE_LEFT, Kolumna, "NIP" );
   BrowseAddColumn( oOkno->BRW, "Nazwa kontrahenta", 40, BROWSE_LEFT, KolumnaNazwa, oOkno );
   BrowseAddColumn( oOkno->BRW, "Miejscowość", 20, BROWSE_LEFT, Kolumna, "MIEJSCOWOSC" );
   BrowseAddColumn( oOkno->BRW, "Ulica", 30, BROWSE_LEFT, KolumnaUlica, oOkno );
   ControlSetAction( oOkno->BRW, Zmiana, oOkno );

   DbSelectArea( "KONTRAH" );
   DbGoTop();
   PodmiotOkno( &oOkno->podmiot );
   Stan( oOkno );

   WinCenter( oOkno->okno );
   if( DialogShowModal( oOkno->okno ) == 1 && lWybor && !DbPusta( "KONTRAH" ) && !Eof() )
      nWybrany = RecNo();

   DbSelectArea( "KONTRAH" );
   DbCloseArea();
   free( oOkno->aZaznaczone );

   return nWybrany;
}

// Przeglądarka włączona tylko z kontrahentami
// Stan( oOkno ) --> void
static void Stan( OknoKontrahenta *oOkno )
{
   xBool lJest = !DbPusta( "KONTRAH" );

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

   DbSelectArea( "KONTRAH" );

   return FieldGetC( FieldPos( ( const char * )data ) );
}

// Kolumna Nazwa kontrahenta: nazwa pełna w jednej linii
// KolumnaNazwa( browse, data ) --> cTekst
static const char *KolumnaNazwa( Control *browse, void *data )
{
   OknoKontrahenta *oOkno = ( OknoKontrahenta * )data;
   char             cTekst[ 2049 ];

   UNUSED( browse );

   DbSelectArea( "KONTRAH" );
   StrTran( cTekst, sizeof( cTekst ), FieldGetC( FieldPos( "PELNA_NAZWA" ) ), "\r\n", " " );
   StrTran( oOkno->cNazwa, sizeof( oOkno->cNazwa ), cTekst, "\n", " " );

   return oOkno->cNazwa;
}

// Kolumna Ulica: ulica z numerem domu i lokalu
// KolumnaUlica( browse, data ) --> cTekst
static const char *KolumnaUlica( Control *browse, void *data )
{
   OknoKontrahenta *oOkno = ( OknoKontrahenta * )data;
   char             cUlica[ 255 ];
   char             cNrDomu[ 37 ];
   char             cNrLokalu[ 41 ];

   UNUSED( browse );

   DbSelectArea( "KONTRAH" );
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

   DbSelectArea( "KONTRAH" );

   return ( double )RecNo();
}

// Belka przeglądarki na innym kontrahencie
// Zmiana( browse, data ) --> void
static void Zmiana( Control *browse, void *data )
{
   UNUSED( browse );

   PodmiotOkno( &( ( OknoKontrahenta * )data )->podmiot );
}

// F1: pomoc
// Pomoc( toolbar, data ) --> void
static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgHelp( POMOC_PODMIOT
            "Przeglądanie\n\n"
            "F1\tpomoc\n"
            "Enter\twybór kontrahenta, gdy okno otwarto F12 z faktury\n"
            "F2, Ins\tnowy kontrahent\n"
            "F3\tkopia bieżącego kontrahenta jako nowy\n"
            "F4\tpoprawienie kontrahenta\n"
            "F8\tusunięcie kontrahenta\n"
            "F9\twydruk: bieżącego, zaznaczonych albo wszystkich; karta kontrahenta\n"
            "Spacja\tzaznaczenie / odznaczenie kontrahenta\n"
            "F12\tszukanie w kolumnie, na której stoi belka\n"
            "Strzałki\tpoprzedni / następny kontrahent, lewo / prawo kolumna\n"
            "PgUp, PgDn\tpoprzednia / następna strona\n"
            "Ctrl+Home\tpierwszy kontrahent\n"
            "Ctrl+End\tostatni kontrahent\n"
            "Esc\tzamknięcie kartoteki\n\n"
            "Wpisywanie danych\n\n"
            "Enter\tnastępne pole, na ostatnim polu ostatniej zakładki zapis\n"
            "Tab\tnastępne pole, bez zapisu\n"
            "Shift+Tab\tpoprzednie pole\n"
            "Ctrl+Tab\tnastępna zakładka danych\n"
            "Ins\twstawianie / zastępowanie znaków, Ins na pasku stanu\n"
            "Spacja\tna zakładce Rachunki bankowe: drukowanie rachunku, wybór rachunku\n"
            "Shift+Enter\tnowa linia w nazwie pełnej i uwagach\n"
            "PgUp, PgDn\tzapis\n"
            "Esc\tbez zapisu", "Kontrahenci" );
}

// Nowy kontrahent, kopia bieżącego albo poprawienie bieżącego (wzór z podmiot.c)
// Edycja( oOkno, nTryb ) --> void
static void Edycja( OknoKontrahenta *oOkno, Tryb nTryb )
{
   if( PodmiotEdycja( &oOkno->podmiot, oOkno->okno, oOkno->pasekStanu, nTryb ) )
      Stan( oOkno );
}

// Enter: wybiera kontrahenta
// Wybierz( toolbar, data ) --> void
static void Wybierz( Toolbar *toolbar, void *data )
{
   OknoKontrahenta *oOkno = ( OknoKontrahenta * )data;

   UNUSED( toolbar );

   if( !DbPusta( "KONTRAH" ) )
      DialogEnd( oOkno->okno, 1 );
}

// F2: nowy kontrahent
// Dodaj( toolbar, data ) --> void
static void Dodaj( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   Edycja( ( OknoKontrahenta * )data, NOWY );
}

// Ins: nowy kontrahent
// Wstaw( win, data ) --> void
static void Wstaw( Win *win, void *data )
{
   UNUSED( win );

   Edycja( ( OknoKontrahenta * )data, NOWY );
}

// F3: kopia bieżącego kontrahenta jako nowy
// Kopia( win, data ) --> void
static void Kopia( Win *win, void *data )
{
   UNUSED( win );

   Edycja( ( OknoKontrahenta * )data, KOPIA );
}

// F4: poprawienie kontrahenta
// Popraw( toolbar, data ) --> void
static void Popraw( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   Edycja( ( OknoKontrahenta * )data, POPRAW );
}

// F8: usunięcie kontrahenta po pytaniu
// Usun( toolbar, data ) --> void
static void Usun( Toolbar *toolbar, void *data )
{
   OknoKontrahenta *oOkno = ( OknoKontrahenta * )data;
   char             cNazwa[ 2049 ];
   char             cPytanie[ 2200 ];

   UNUSED( toolbar );

   DbSelectArea( "KONTRAH" );
   if( Eof() )
      return;

   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "PELNA_NAZWA" ) ) );
   StringFormat( cPytanie, "Czy na pewno usunąć kontrahenta\n%s?", AllTrim( cNazwa ) );
   if( MsgYesNo( cPytanie, "Kontrahenci" ) )
   {
      DbSelectArea( "KONTRAH" );
      if( NetRLock( NET_WAIT ) )
      {
         DbDelete();
         DbCommit();
         DbUnlock();
         Zaznacz( oOkno->BRW, F, oOkno );
         DbSkip( 1 );
         if( Eof() )
            DbGoBottom();
      }
      Stan( oOkno );
      PodmiotOkno( &oOkno->podmiot );
   }
}

// Kolumna Chk: czy bieżący kontrahent jest zaznaczony
// Zaznaczony( browse, data ) --> lZaznaczony
static xBool Zaznaczony( Control *browse, void *data )
{
   OknoKontrahenta *oOkno  = ( OknoKontrahenta * )data;
   long             nRecNo;

   UNUSED( browse );

   DbSelectArea( "KONTRAH" );
   nRecNo = RecNo();
   for( int i = 0; i < oOkno->nZaznaczone; ++i )
   {
      if( oOkno->aZaznaczone[ i ] == nRecNo )
         return T;
   }

   return F;
}

// Zaznacza albo odznacza bieżącego kontrahenta
// Zaznacz( browse, lZaznacz, data ) --> void
static void Zaznacz( Control *browse, xBool lZaznacz, void *data )
{
   OknoKontrahenta *oOkno = ( OknoKontrahenta * )data;
   long             nRecNo;

   if( Zaznaczony( browse, data ) == lZaznacz )
      return;

   nRecNo = RecNo();
   if( lZaznacz )
   {
      if( oOkno->nZaznaczone == oOkno->nMiejsca )
      {
         oOkno->nMiejsca    = MAX( 16, 2 * oOkno->nMiejsca );
         oOkno->aZaznaczone = ( long * )realloc( oOkno->aZaznaczone, ( size_t )oOkno->nMiejsca * sizeof( long ) );
      }
      oOkno->aZaznaczone[ oOkno->nZaznaczone++ ] = nRecNo;
   }
   else
   {
      for( int i = 0; i < oOkno->nZaznaczone; ++i )
      {
         if( oOkno->aZaznaczone[ i ] == nRecNo )
         {
            oOkno->aZaznaczone[ i ] = oOkno->aZaznaczone[ --oOkno->nZaznaczone ];
            break;
         }
      }
   }
}

// Czy jest zaznaczony choć jeden kontrahent
// SaZaznaczone( oOkno ) --> lSa
static xBool SaZaznaczone( OknoKontrahenta *oOkno )
{
   return IIF( oOkno->nZaznaczone > 0, T, F );
}

// F12: szukanie tekstu w kolumnie, na której stoi belka, od następnego kontrahenta
// Szukaj( toolbar, data ) --> void
static void Szukaj( Toolbar *toolbar, void *data )
{
   OknoKontrahenta *oOkno  = ( OknoKontrahenta * )data;
   OknoSzukaj       szukaj = { 0 };
   const char      *cOpis;
   const char      *cPole;
   Control         *oOk;
   Control         *oAnuluj;
   char             cWartosc[ 2049 ];
   long             nRecNo;

   UNUSED( toolbar );

   DbSelectArea( "KONTRAH" );
   if( Eof() )
      return;

   switch( BrowseColPos( oOkno->BRW ) )
   {
   case 3:
      cOpis = "NIP:";
      cPole = "NIP";
      break;
   case 5:
      cOpis = "Miejscowość:";
      cPole = "MIEJSCOWOSC";
      break;
   case 6:
      cOpis = "Ulica:";
      cPole = "ULICA";
      break;
   default:
      cOpis = "Nazwa kontrahenta:";
      cPole = "PELNA_NAZWA";
   }

   szukaj.oSzukaj = DialogCreate( oOkno->okno, "Szukaj kontrahenta", 400, 120 );
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

   DbSelectArea( "KONTRAH" );
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
         PodmiotOkno( &oOkno->podmiot );
         return;
      }

      if( RecNo() == nRecNo )
         break;
   }

   StringFormat( cWartosc, "Nie znaleziono: %s", szukaj.cTekst );
   MsgInfo( cWartosc, "Kontrahenci" );
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

/* ---------------------------------------------------------------------------------------------
   Wydruki
   --------------------------------------------------------------------------------------------- */

// F9: wydruk bieżącego kontrahenta
// DrukujBiezacego( win, data ) --> void
static void DrukujBiezacego( Win *win, void *data )
{
   UNUSED( win );

   DbSelectArea( "KONTRAH" );
   if( !Eof() )
      Drukuj( ( OknoKontrahenta * )data, BIEZACY );
}

// F9: wydruk zaznaczonych kontrahentów
// DrukujZaznaczone( win, data ) --> void
static void DrukujZaznaczone( Win *win, void *data )
{
   UNUSED( win );

   if( SaZaznaczone( ( OknoKontrahenta * )data ) )
      Drukuj( ( OknoKontrahenta * )data, ZAZNACZONE );
}

// F9: wydruk wszystkich kontrahentów
// DrukujWszystkich( win, data ) --> void
static void DrukujWszystkich( Win *win, void *data )
{
   UNUSED( win );

   DbSelectArea( "KONTRAH" );
   if( LastRec() > 0 )
      Drukuj( ( OknoKontrahenta * )data, WSZYSTKIE );
}

// F9: karta bieżącego kontrahenta ze wszystkimi danymi (wydruk.c)
// DrukujKarte( win, data ) --> void
static void DrukujKarte( Win *win, void *data )
{
   OknoKontrahenta *oOkno = ( OknoKontrahenta * )data;

   UNUSED( win );

   WydrukKarta( oOkno->biuro, "KONTRAH", "Karta kontrahenta", "karta-kontrahenta.pdf" );
}

// Wydruk kontrahentów do PDF albo SVG w katalogu wydruków wybranej firmy, otwarty potem w programie Windows
// Drukuj( oOkno, nZakres ) --> void
static void Drukuj( OknoKontrahenta *oOkno, Druk nZakres )
{
   OknoPytanie  pytanie = { 0 };
   Control     *oAnuluj;
   WierszDruku *aDane;
   int          nDane;
   int          nGdzie;
   const char  *cTytul;
   char         cPlik[ 64 ];
   char         cKomunikat[ 200 ];
   xBool        lZapis;

   pytanie.oPytanie = DialogCreate( oOkno->okno, "Wydruk kontrahentów", 330, 110 );
   SayCreate( pytanie.oPytanie, 10, 12, 34, 20, "Dokąd drukować?" );
   pytanie.oPdf = ButtonCreate( pytanie.oPytanie,  10, 56, 90, 28, "PDF" );
   pytanie.oSvg = ButtonCreate( pytanie.oPytanie, 115, 56, 90, 28, "SVG" );
   oAnuluj      = ButtonCreate( pytanie.oPytanie, 230, 56, 90, 28, "Anuluj" );
   ButtonSetDefault( pytanie.oPdf, T );
   ButtonSetCancel( oAnuluj, T );
   ControlSetAction( pytanie.oPdf, Gdzie, &pytanie );
   ControlSetAction( pytanie.oSvg, Gdzie, &pytanie );
   ControlSetAction( oAnuluj, Gdzie, &pytanie );
   WinCenter( pytanie.oPytanie );

   nGdzie = DialogShowModal( pytanie.oPytanie );
   if( nGdzie == 0 )
      return;

   aDane = DoDruku( oOkno, nZakres, &nDane );
   switch( nZakres )
   {
   case BIEZACY:
      cTytul = "Kontrahent";
      break;
   case ZAZNACZONE:
      cTytul = "Kontrahenci zaznaczeni";
      break;
   default:
      cTytul = "Wszyscy kontrahenci";
   }

   WydrukPlik( oOkno->biuro, IIF( nGdzie == 1, "kontrahenci.pdf", "kontrahenci.svg" ), cPlik, sizeof( cPlik ) );

   lZapis = IIF( nGdzie == 1, DrukPdf( cPlik, cTytul, aDane, nDane ), DrukSvg( cPlik, cTytul, aDane, nDane ) );
   free( aDane );

   if( !lZapis )
   {
      StringFormat( cKomunikat, "Nie można zapisać pliku %s.\nPlik może być otwarty w innym programie.", cPlik );
      MsgError( cKomunikat, "Wydruk kontrahentów" );
   }
   else
      FileExecute( cPlik );
}

// Przycisk okna pytania: PDF 1, SVG 2, Anuluj 0
// Gdzie( button, data ) --> void
static void Gdzie( Control *button, void *data )
{
   OknoPytanie *pytanie = ( OknoPytanie * )data;

   DialogEnd( pytanie->oPytanie, IIF( button == pytanie->oPdf, 1, IIF( button == pytanie->oSvg, 2, 0 ) ) );
}

// Wiersze wydruku dla zakresu; tablica do zwolnienia free()
// DoDruku( oOkno, nZakres, nDane ) --> aDane
static WierszDruku *DoDruku( OknoKontrahenta *oOkno, Druk nZakres, int *nDane )
{
   WierszDruku *aDane;
   long         nRecNo;

   DbSelectArea( "KONTRAH" );
   nRecNo = RecNo();
   aDane  = ( WierszDruku * )malloc( ( size_t )MAX( 1, LastRec() ) * sizeof( WierszDruku ) );
   *nDane = 0;

   if( nZakres == BIEZACY )
   {
      Wiersz( 1, &aDane[ 0 ] );
      *nDane = 1;

      return aDane;
   }

   DbGoTop();
   while( !Eof() )
   {
      if( nZakres == WSZYSTKIE || Zaznaczony( oOkno->BRW, oOkno ) )
      {
         Wiersz( *nDane + 1, &aDane[ *nDane ] );
         ++*nDane;
      }
      DbSkip( 1 );
   }
   DbGoto( nRecNo );

   return aDane;
}

// Wiersz wydruku z bieżącego kontrahenta
// Wiersz( nLp, wiersz ) --> void
static void Wiersz( int nLp, WierszDruku *wiersz )
{
   char cKraj[ 3 ];
   char cNip[ 51 ];
   char cUlica[ 255 ];
   char cNrDomu[ 37 ];
   char cNrLokalu[ 41 ];
   char cKod[ 9 ];
   char cMiejscowosc[ 225 ];

   StringFormat( cKraj, "%s", FieldGetC( FieldPos( "KOD_KRAJU" ) ) );
   StringFormat( cNip, "%s", FieldGetC( FieldPos( "NIP" ) ) );
   StringFormat( cUlica, "%s", FieldGetC( FieldPos( "ULICA" ) ) );
   StringFormat( cNrDomu, "%s", FieldGetC( FieldPos( "NR_DOMU" ) ) );
   StringFormat( cNrLokalu, "%s", FieldGetC( FieldPos( "NR_LOKALU" ) ) );
   StringFormat( cKod, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY" ) ) );
   StringFormat( cMiejscowosc, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
   AllTrim( cKraj );
   AllTrim( cNip );
   AllTrim( cUlica );
   AllTrim( cNrDomu );
   AllTrim( cNrLokalu );
   AllTrim( cKod );
   AllTrim( cMiejscowosc );

   StringFormat( wiersz->cLp, "%d.", nLp );
   StringFormat( wiersz->cNip, "%s %s", cKraj, cNip );
   StringFormat( wiersz->cNazwa, "%s", FieldGetC( FieldPos( "PELNA_NAZWA" ) ) );
   StringFormat( wiersz->cUlica, "%s %s%s%s", cUlica, cNrDomu, IIF( *cNrLokalu == 0, "", "/" ), cNrLokalu );
   StringFormat( wiersz->cMiasto, "%s %s", cKod, cMiejscowosc );
   StringFormat( wiersz->cTelefon, "%s", FieldGetC( FieldPos( "TELEFON_1" ) ) );
   StringFormat( wiersz->cEmail, "%s", FieldGetC( FieldPos( "EMAIL" ) ) );
   AllTrim( wiersz->cNip );
   AllTrim( wiersz->cNazwa );
   AllTrim( wiersz->cUlica );
   AllTrim( wiersz->cMiasto );
   AllTrim( wiersz->cTelefon );
   AllTrim( wiersz->cEmail );
}

// Tekst w dwóch liniach: do końca pierwszej linii nazwy albo tyle słów, ile mieści się w kolumnie, i reszta
// DwieLinie( cTekst, miesci, data, cLinia1, cLinia2, nRozmiar ) --> void
static void DwieLinie( const char *cTekst, Miesci miesci, void *data, char *cLinia1, char *cLinia2, size_t nRozmiar )
{
   const char *cEol = strchr( cTekst, '\n' );
   char        cReszta[ 2049 ];
   char       *cSpacja;

   if( cEol != NULL )
   {
      StringFormatSize( cLinia1, nRozmiar, "%.*s", ( int )( cEol - cTekst ), cTekst );
      StrTran( cReszta, sizeof( cReszta ), cEol + 1, "\r", "" );
      StrTran( cLinia2, nRozmiar, cReszta, "\n", " " );
      StrTran( cReszta, sizeof( cReszta ), cLinia1, "\r", "" );
      StringFormatSize( cLinia1, nRozmiar, "%s", cReszta );
      AllTrim( cLinia2 );

      return;
   }

   StringFormatSize( cLinia1, nRozmiar, "%s", cTekst );
   while( !miesci( cLinia1, data ) && ( cSpacja = strrchr( cLinia1, ' ' ) ) != NULL )
   {
      *cSpacja = 0;
      RTrim( cLinia1 );
   }
   StringFormatSize( cLinia2, nRozmiar, "%s", cTekst + strlen( cLinia1 ) );
   AllTrim( cLinia2 );
}

// Tekst mieści się w kolumnie PDF
// MiesciPdf( cTekst, data ) --> lMiesci
static xBool MiesciPdf( const char *cTekst, void *data )
{
   MiejscePdf *miejsce = ( MiejscePdf * )data;

   return IIF( HPDF_Page_TextWidth( miejsce->pPage, cTekst ) <= miejsce->nSzer, T, F );
}

// Tekst mieści się w kolumnie SVG o liczbie znaków data
// MiesciSvg( cTekst, data ) --> lMiesci
static xBool MiesciSvg( const char *cTekst, void *data )
{
   return IIF( ULen( cTekst ) <= ( size_t )*( int * )data, T, F );
}

// Wydruk do PDF przez libharu: A4 poziomo, 16 kontrahentów na stronie
// DrukPdf( cPlik, cTytul, aDane, nDane ) --> lZapis
static xBool DrukPdf( const char *cPlik, const char *cTytul, WierszDruku *aDane, int nDane )
{
   static const char  *aNaglowek[] = { "Lp.", "NIP", "Nazwa", "Adres", "Telefon", "e-mail" };
   static const double aKolumna[]  = { 40, 68, 195, 430, 640, 715, 802 };

   int         nNaStrone = 16;
   int         nStron    = MAX( 1, ( nDane + nNaStrone - 1 ) / nNaStrone );
   HPDF_Doc    pPdf      = HPDF_New( NULL, NULL );
   HPDF_Font   pFont;
   HPDF_Font   pBold;
   HPDF_Page   pPage;
   MiejscePdf  miejsce;
   double      nY;
   xBool       lZapis;
   char        cTekst[ 300 ];
   char        cData[ 16 ];
   char        cGodzina[ 16 ];
   char        cNazwa1[ 2049 ];
   char        cNazwa2[ 2049 ];
   const char *aLinia1[ 6 ];
   const char *aLinia2[ 6 ];

   HPDF_SetCompressionMode( pPdf, HPDF_COMP_ALL );
   HPDF_UseUTFEncodings( pPdf );
   HPDF_SetCurrentEncoder( pPdf, "UTF-8" );
   HPDF_SetInfoAttr( pPdf, HPDF_INFO_TITLE, cTytul );
   HPDF_SetInfoAttr( pPdf, HPDF_INFO_CREATOR, "xBiuro" );
   pFont = WydrukCzcionka( pPdf, "arial.ttf" );
   pBold = WydrukCzcionka( pPdf, "arialbd.ttf" );

   DToC( Date(), cData, sizeof( cData ) );
   Time( cGodzina, sizeof( cGodzina ) );
   cGodzina[ 5 ] = 0;

   for( int nStrona = 1; nStrona <= nStron; ++nStrona )
   {
      pPage = HPDF_AddPage( pPdf );
      HPDF_Page_SetSize( pPage, HPDF_PAGE_SIZE_A4, HPDF_PAGE_LANDSCAPE );

      WydrukTekst( pPage, pBold, 16, 40, 520, cTytul, 500 );
      StringFormat( cTekst, "Liczba kontrahentów: %d", nDane );
      WydrukTekst( pPage, pFont, 9, 40, 504, cTekst, 300 );
      StringFormat( cTekst, "Wydruk: %s %s", cData, cGodzina );
      WydrukTekst( pPage, pFont, 9, 650, 520, cTekst, 152 );

      HPDF_Page_SetRGBFill( pPage, 0.302f, 0.451f, 0.149f );
      HPDF_Page_Rectangle( pPage, ( HPDF_REAL )aKolumna[ 0 ], 470, ( HPDF_REAL )( aKolumna[ 6 ] - aKolumna[ 0 ] ), 18 );
      HPDF_Page_Fill( pPage );
      HPDF_Page_SetRGBFill( pPage, 1, 1, 1 );
      for( int nKolumna = 0; nKolumna < 6; ++nKolumna )
         WydrukTekst( pPage, pBold, 9, aKolumna[ nKolumna ] + 4, 475, aNaglowek[ nKolumna ], aKolumna[ nKolumna + 1 ] - aKolumna[ nKolumna ] - 8 );
      HPDF_Page_SetRGBFill( pPage, 0, 0, 0 );

      HPDF_Page_SetLineWidth( pPage, 0.5f );
      HPDF_Page_SetRGBStroke( pPage, 0.75f, 0.75f, 0.75f );
      nY = 470;
      for( int nWiersz = ( nStrona - 1 ) * nNaStrone; nWiersz < MIN( nStrona * nNaStrone, nDane ); ++nWiersz )
      {
         HPDF_Page_SetFontAndSize( pPage, pFont, 9 );
         miejsce.pPage = pPage;
         miejsce.nSzer = aKolumna[ 3 ] - aKolumna[ 2 ] - 8;
         DwieLinie( aDane[ nWiersz ].cNazwa, MiesciPdf, &miejsce, cNazwa1, cNazwa2, sizeof( cNazwa1 ) );

         aLinia1[ 0 ] = aDane[ nWiersz ].cLp;
         aLinia1[ 1 ] = aDane[ nWiersz ].cNip;
         aLinia1[ 2 ] = cNazwa1;
         aLinia1[ 3 ] = aDane[ nWiersz ].cUlica;
         aLinia1[ 4 ] = aDane[ nWiersz ].cTelefon;
         aLinia1[ 5 ] = aDane[ nWiersz ].cEmail;
         aLinia2[ 0 ] = "";
         aLinia2[ 1 ] = "";
         aLinia2[ 2 ] = cNazwa2;
         aLinia2[ 3 ] = aDane[ nWiersz ].cMiasto;
         aLinia2[ 4 ] = "";
         aLinia2[ 5 ] = "";
         for( int nKolumna = 0; nKolumna < 6; ++nKolumna )
         {
            WydrukTekst( pPage, pFont, 9, aKolumna[ nKolumna ] + 4, nY - 11, aLinia1[ nKolumna ], aKolumna[ nKolumna + 1 ] - aKolumna[ nKolumna ] - 8 );
            WydrukTekst( pPage, pFont, 9, aKolumna[ nKolumna ] + 4, nY - 22, aLinia2[ nKolumna ], aKolumna[ nKolumna + 1 ] - aKolumna[ nKolumna ] - 8 );
         }

         nY -= 26;
         HPDF_Page_MoveTo( pPage, ( HPDF_REAL )aKolumna[ 0 ], ( HPDF_REAL )nY );
         HPDF_Page_LineTo( pPage, ( HPDF_REAL )aKolumna[ 6 ], ( HPDF_REAL )nY );
         HPDF_Page_Stroke( pPage );
      }

      StringFormat( cTekst, "Strona %d z %d", nStrona, nStron );
      WydrukTekst( pPage, pFont, 8, 700, 25, cTekst, 102 );
   }

   lZapis = IIF( HPDF_SaveToFile( pPdf, cPlik ) == HPDF_OK, T, F );
   HPDF_Free( pPdf );

   return lZapis;
}

// Wydruk do SVG: tabela czcionką o stałej szerokości
// DrukSvg( cPlik, cTytul, aDane, nDane ) --> lZapis
static xBool DrukSvg( const char *cPlik, const char *cTytul, WierszDruku *aDane, int nDane )
{
   static const char *aNaglowek[] = { "Lp.", "NIP", "Nazwa", "Adres", "Telefon", "E-mail" };
   static const int   aZnaki[]    = { 5, 24, 40, 40, 16, 30 };

   double      nZnak = 7.2;
   const char *cFont = "Consolas, Courier New, monospace";
   int         nSzer = 40 + ( int )( 155 * nZnak );
   int         nWys  = 130 + nDane * 36;
   Svg        *svg   = SvgInit( cPlik, nSzer, nWys, 0 );
   int         nZnakow;
   double      nX;
   double      nY;
   char        cTekst[ 300 ];
   char        cData[ 16 ];
   char        cGodzina[ 16 ];
   char        cNazwa1[ 2049 ];
   char        cNazwa2[ 2049 ];
   char        cKomorka[ 2100 ];
   const char *aLinia1[ 6 ];
   const char *aLinia2[ 6 ];

   if( svg == NULL )
      return F;

   DToC( Date(), cData, sizeof( cData ) );
   Time( cGodzina, sizeof( cGodzina ) );
   cGodzina[ 5 ] = 0;

   SvgFilledRect( svg, 0, 0, nSzer, nWys, "#ffffff", 0 );
   SvgText( svg, 20, 34, cTytul, "Arial", 20, FONT_WEIGHT_BOLD, "#000000", SVG_ALIGN_LEFT );
   StringFormat( cTekst, "Liczba kontrahentów: %d", nDane );
   SvgText( svg, 20, 56, cTekst, "Arial", 12, FONT_WEIGHT_NORMAL, "#000000", SVG_ALIGN_LEFT );
   StringFormat( cTekst, "Wydruk: %s %s", cData, cGodzina );
   SvgText( svg, nSzer - 20, 34, cTekst, "Arial", 12, FONT_WEIGHT_NORMAL, "#000000", SVG_ALIGN_RIGHT );

   nY = 70;
   SvgFilledRect( svg, 20, nY, nSzer - 40, 20, "#4d7326", 0 );
   nX = 20;
   for( int nKolumna = 0; nKolumna < 6; ++nKolumna )
   {
      SvgText( svg, nX + 4, nY + 14, aNaglowek[ nKolumna ], cFont, 12, FONT_WEIGHT_BOLD, "#ffffff", SVG_ALIGN_LEFT );
      nX += aZnaki[ nKolumna ] * nZnak;
   }

   nY += 20;
   for( int nWiersz = 0; nWiersz < nDane; ++nWiersz )
   {
      nZnakow = aZnaki[ 2 ] - 1;
      DwieLinie( aDane[ nWiersz ].cNazwa, MiesciSvg, &nZnakow, cNazwa1, cNazwa2, sizeof( cNazwa1 ) );

      aLinia1[ 0 ] = aDane[ nWiersz ].cLp;
      aLinia1[ 1 ] = aDane[ nWiersz ].cNip;
      aLinia1[ 2 ] = cNazwa1;
      aLinia1[ 3 ] = aDane[ nWiersz ].cUlica;
      aLinia1[ 4 ] = aDane[ nWiersz ].cTelefon;
      aLinia1[ 5 ] = aDane[ nWiersz ].cEmail;
      aLinia2[ 0 ] = "";
      aLinia2[ 1 ] = "";
      aLinia2[ 2 ] = cNazwa2;
      aLinia2[ 3 ] = aDane[ nWiersz ].cMiasto;
      aLinia2[ 4 ] = "";
      aLinia2[ 5 ] = "";
      nX = 20;
      for( int nKolumna = 0; nKolumna < 6; ++nKolumna )
      {
         SvgText( svg, nX + 4, nY + 14, SvgTekst( aLinia1[ nKolumna ], aZnaki[ nKolumna ] - 1, cKomorka, sizeof( cKomorka ) ), cFont, 12, FONT_WEIGHT_NORMAL, "#000000", SVG_ALIGN_LEFT );
         SvgText( svg, nX + 4, nY + 30, SvgTekst( aLinia2[ nKolumna ], aZnaki[ nKolumna ] - 1, cKomorka, sizeof( cKomorka ) ), cFont, 12, FONT_WEIGHT_NORMAL, "#000000", SVG_ALIGN_LEFT );
         nX += aZnaki[ nKolumna ] * nZnak;
      }

      nY += 36;
      SvgLine( svg, 20, nY, nSzer - 20, nY, "#bfbfbf" );
   }

   SvgClose( svg );

   return T;
}

// Tekst do kolumny SVG; dłuższy niż nZnakow ucięty z „…” na końcu
// SvgTekst( cTekst, nZnakow, cWynik, nRozmiar ) --> cWynik
static const char *SvgTekst( const char *cTekst, int nZnakow, char *cWynik, size_t nRozmiar )
{
   char cPoczatek[ 2100 ];

   if( ULen( cTekst ) <= ( size_t )nZnakow )
   {
      StringFormatSize( cWynik, nRozmiar, "%s", cTekst );

      return cWynik;
   }

   StringFormat( cPoczatek, "%s", cTekst );
   ULeft( cPoczatek, ( size_t )nZnakow - 1 );
   StringFormatSize( cWynik, nRozmiar, "%s…", cPoczatek );

   return cWynik;
}

// Esc: zamyka okno
// Zamknij( toolbar, data ) --> void
static void Zamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( OknoKontrahenta * )data )->okno );
}
