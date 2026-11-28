/*
 * Program...: xBiuro
 * Plik......: src/biuro.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xwgui.h"
#include "biuro.h"

#define FONT_ICONS PROG_ZASO "font/codicon.ttf"

/* Okno postępu: pasek wypełnia się w pół sekundy, żeby użytkownik widział, że program pracuje */
typedef struct
{
   Win     *okno;
   Control *pasek;
} PostepOkno;

/* Moduły programu w kolejności paska bocznego (Moduły): nazwa, skrót (klucz w OpodatkowanieModul()) i opis */
static const char *aModuly[]       = { "Faktury", "Księgi rachunkowe", "PKPiR", "Ewidencja przychodów", "Wynagrodzenia" };
static const char *aSkrotModulow[] = { "FV", "KR", "PKPiR", "EP", "WY" };
static const char *aOpisModulow[]  = { "Fakturowanie", "Pełna księgowość", "Podatkowa księga przychodów i rozchodów", "Ryczałt od przychodów ewidencjonowanych", "Naliczanie wynagrodzeń" };
static const char *aPlikiModulow[] = { "faktura", "sfk", "pkpir", "ryczalt", "place" };

static Menu *MenuBiuro( Biuro *biuro );
static Menu *MenuZarzadzaj( Biuro *biuro );
static Menu *MenuAdministrator( Biuro *biuro );
static void  ZnajdzFirme( const char *name, xBool directory, void *data );
static int   PorownajNumery( const void *pierwszy, const void *drugi );
static void  Firmy( Biuro *biuro );
static void  FirmaNaLiscie( Biuro *biuro, int firma );
static void  ListaFirm( Biuro *biuro );
static void  UstawFirme( Biuro *biuro, int firma );
static void  Tytul( Win *win, void *data );
static void  WybierzFirme( Sidebar *sidebar, int wiersz, void *data );
static void  DodajFirme( Win *win, void *data );
static void  PodkatalogiFirmy( Biuro *biuro, int firma );
static void  NowyRok( Biuro *biuro, const char *cPoprzedni, int firma );
static void  Moduly( Biuro *biuro );
static void  Modul( Win *win, void *data );
static void  WybierzModul( Sidebar *sidebar, int wiersz, void *data );
static void  ZamknijModul( Biuro *biuro );
static xBool WywolajModul( Biuro *biuro, int nModul, const char *cFunkcja );
static void  PrzelaczAdministratora( Win *win, void *data );
static void  PostepKrok( Timer *timer, void *data );
static void  Postep( Biuro *biuro, const char *tekst );
static void  Ustawienia( Win *win, void *data );
static void  Archiwizacja( Win *win, void *data );
static void  Odtworzenie( Win *win, void *data );
static void  Indeksowanie( Win *win, void *data );
static void  Informacja( Win *win, void *data );
static void  Zakoncz( Win *win, void *data );
static void  ZmianaFirmy( Biuro *biuro );

// Program xBiuro; rok danych z wiersza poleceń
// main( argc, argv ) --> nWynik
int main( int argc, char *argv[] )
{
   Biuro    biuro = { 0 };
   App     *app;
   Win     *win;
   Sidebar *side;
   int      rokNum;
   int      result;

   Set( SET_DATEFORMAT, "dd.mm.yyyy" );
   SetDeleted( T );

   rokNum = DateYear();

   if( argc > 1 )
   {
      int podany = atoi( argv[ 1 ] );

      if( podany >= 2000 && podany <= 2099 )
         rokNum = podany;
   }

   StringFormat( biuro.daneYYYY, "dane-%d/", rokNum );
   biuro.rok = rokNum;
   biuro.zmianaFirmy = ZmianaFirmy;

   Katalogi( &biuro );
   Firmy( &biuro );

   app = AppCreate();

   win = WinCreate( app, "xBiuro", 1000, 640 );
   biuro.win = win;
   WinSetBackground( win, 0x132639 );
   WinSetOnResize( win, Tytul, &biuro );

   WinSetState( win, WIN_MAXIMIZED );

   WinCenter( win );

   if( FileExists( PROG_ZASO "wekt/logo-xbiuro.ico" ) )
   {
      WinSetIcon( win, PROG_ZASO "wekt/logo-xbiuro.ico" );
   }

   MenuStyle( T, 32 );
   biuro.menu = MenuBiuro( &biuro );
   WinSetMenu( win, biuro.menu );

   biuro.status = StatusBarCreate( win, "" );

   side = SidebarCreate( win );
   biuro.side = side;
   if( FileExists( FONT_ICONS ) )
   {
      biuro.firmy = SidebarAddButton( side, FontIconCreate( FONT_ICONS, 0xEAF0, 0xE9EDF1, 0 ), "Firmy", "Ctrl+Shift+E" );
      for( int i = 0; i < biuro.liczbaFirm; ++i )
         FirmaNaLiscie( &biuro, biuro.numery[ i ] );

      biuro.moduly = SidebarAddButton( side, FontIconCreate( FONT_ICONS, 0xEAE6, 0xE9EDF1, 0 ), "Moduły", "Ctrl+Shift+X" );

      SidebarAddGlobalMenu( side, FontIconCreate( FONT_ICONS, 0xEB99, 0xE9EDF1, 0 ), "Administrator", MenuAdministrator( &biuro ) );
      SidebarAddGlobalMenu( side, FontIconCreate( FONT_ICONS, 0xEB51, 0xE9EDF1, 0 ), "Zarządzaj", MenuZarzadzaj( &biuro ) );

      // Na starcie panel jest zwinięty, a wybrany jest administrator, bez okna postępu
      UstawFirme( &biuro, 0 );
   }
   else
   {
      MsgInfo( "Brak czcionki " FONT_ICONS, "" );
   }

   WinShow( win );

   result = AppRun( app, win );

   AppDestroy( app );

   return result;
}

// Zakłada brakujące katalogi danych
// Katalogi( biuro ) --> void
void Katalogi( Biuro *biuro )
{
   DirCreate( DANE_BAZA );
   DirCreate( DANE_BAZA "kartoteka" );
   DirCreate( biuro->daneYYYY );
}

/* ---------------------------------------------------------------------------------------------
   Menu
   --------------------------------------------------------------------------------------------- */

// Menu główne
// MenuBiuro( biuro ) --> menu
static Menu *MenuBiuro( Biuro *biuro )
{
   Menu *menu;
   Menu *plik;
   Menu *firma;
   Menu *rozrachunki;
   Menu *kartoteka;
   Menu *pomoc;
   Menu *kopie;
   Menu *porzadek;

   menu = MenuCreate();

   plik = MenuAddMenu( menu, "Plik" );
   MenuAddSeparator( plik );
   kopie = MenuAddMenu( plik, "Obsługa kopii danych" );
   MenuAdd( kopie, "Archiwizacja baz danych", Archiwizacja, biuro, "" );
   MenuAdd( kopie, "Odtworzenie baz danych", Odtworzenie, biuro, "" );
   MenuAddSeparator( plik );
   porzadek = MenuAddMenu( plik, "Porządkowanie danych" );
   MenuAdd( porzadek, "Indeksowanie baz danych", Indeksowanie, biuro, "" );
   MenuAddSeparator( porzadek );
   MenuAddDisabled( porzadek, "Wyczyść dane" );
   MenuAddSeparator( plik );
   MenuAdd( plik, "Zakończ", Zakoncz, biuro, "Alt+F4" );

   firma = MenuAddMenu( menu, "Firma" );
   MenuAdd( firma, "Dane firmy", Firma, biuro, "" );
   MenuAdd( firma, "Forma opodatkowania", Opodatkowanie, biuro, "" );
   MenuAddSeparator( firma );
   rozrachunki = MenuAddMenu( firma, "Rozrachunki" );
   MenuAdd( rozrachunki, "Polecenie przelewu / wpłata gotówkowa", Przelew, biuro, "" );

   kartoteka = MenuAddMenu( menu, "Kartoteka" );
   MenuAdd( kartoteka, "Kontrahentów", Kontrahent, biuro, "" );
   MenuAddSeparator( kartoteka );
   MenuAdd( kartoteka, "Urzędów Skarbowych", Us, biuro, "" );
   MenuAddSeparator( kartoteka );
   MenuAddDisabled( kartoteka, "ZUS" );

   pomoc = MenuAddMenu( menu, "Pomoc" );
   MenuSetRight( menu, 4 );
   MenuAdd( pomoc, "Pomoc", Informacja, "Pomoc", "F1" );
   MenuAdd( pomoc, "Pomoc indeks", Informacja, "Pomoc indeks", "Alt+F1" );
   MenuAdd( pomoc, "Zawartość pomocy", Informacja, "Zawartość pomocy", "" );
   MenuAddSeparator( pomoc );
   MenuAdd( pomoc, "Witryna Online", Informacja, "Witryna Online", "" );
   MenuAdd( pomoc, "Kontakt z nami", Informacja, "Kontakt z nami", "" );
   MenuAddSeparator( pomoc );
   MenuAdd( pomoc, "Sprawdź aktualizację", Informacja, "Sprawdź aktualizację", "" );
   MenuAdd( pomoc, "Aktywuj produkt...", Informacja, "Aktywuj produkt", "" );
   MenuAddSeparator( pomoc );
   MenuAdd( pomoc, "xBiuro - Informacje", Informacja, "xBiuro", "" );

   return menu;
}

// Menu koła zębatego na pasku bocznym
// MenuZarzadzaj( biuro ) --> menu
static Menu *MenuZarzadzaj( Biuro *biuro )
{
   Menu *menu = MenuCreatePopup();

   MenuAdd( menu, "Dodaj firmę", DodajFirme, biuro, "" );
   MenuAddSeparator( menu );
   MenuAdd( menu, "Ustawienia", Ustawienia, biuro, "" );

   return menu;
}

// Menu ikony administratora na dole paska bocznego, nad kołem zębatym
// MenuAdministrator( biuro ) --> menu
static Menu *MenuAdministrator( Biuro *biuro )
{
   Menu *menu = MenuCreatePopup();

   MenuAdd( menu, "Przełącz na administratora", PrzelaczAdministratora, biuro, "" );

   return menu;
}

/* ---------------------------------------------------------------------------------------------
   Firmy: każda w swoim katalogu na rok dane-RRRR/firma1, firma2 …; usuwa się je poza programem
   --------------------------------------------------------------------------------------------- */

// Dopisuje numer katalogu firmaN do listy firm
// ZnajdzFirme( name, directory, data ) --> void
static void ZnajdzFirme( const char *name, xBool directory, void *data )
{
   Biuro *biuro = ( Biuro * )data;
   int    numer;
   char   reszta;

   if( !directory || sscanf( name, "firma%d%c", &numer, &reszta ) != 1 || numer <= 0 )
      return;

   biuro->numery = ( int * )realloc( biuro->numery, ( size_t )( biuro->liczbaFirm + 1 ) * sizeof( int ) );
   biuro->numery[ biuro->liczbaFirm++ ] = numer;
}

// Porównanie numerów firm dla qsort()
// PorownajNumery( pierwszy, drugi ) --> nRoznica
static int PorownajNumery( const void *pierwszy, const void *drugi )
{
   return *( const int * )pierwszy - *( const int * )drugi;
}

// Numery firm z katalogów dane-RRRR/firmaN, rosnąco; w nowym roku firmy z poprzedniego roku (NowyRok()), a bez nich
// firma1
// Firmy( biuro ) --> void
static void Firmy( Biuro *biuro )
{
   char cPoprzedni[ 16 ];
   char cKatalog[ 64 ];
   char cAdmin[ 32 ];

   biuro->numery = NULL;
   biuro->liczbaFirm = 0;
   DirScan( biuro->daneYYYY, ZnajdzFirme, biuro );

   if( biuro->liczbaFirm == 0 )
   {
      StringFormat( cPoprzedni, "dane-%d/", biuro->rok - 1 );
      DirScan( cPoprzedni, ZnajdzFirme, biuro );
      for( int i = 0; i < biuro->liczbaFirm; ++i )
         NowyRok( biuro, cPoprzedni, biuro->numery[ i ] );
   }

   if( biuro->liczbaFirm == 0 )
   {
      PodkatalogiFirmy( biuro, 1 );
      ZnajdzFirme( "firma1", T, biuro );
   }

   // Administrator w nowym roku dostaje swoje dane z poprzedniego roku
   RokFirma( biuro, 0, cKatalog, sizeof( cKatalog ) );
   StringFormat( cPoprzedni, "dane-%d/", biuro->rok - 1 );
   StringFormat( cAdmin, "%sadmin", cPoprzedni );
   if( !DirExists( cKatalog ) && DirExists( cAdmin ) )
      NowyRok( biuro, cPoprzedni, 0 );

   qsort( biuro->numery, ( size_t )biuro->liczbaFirm, sizeof( int ), PorownajNumery );
}

// Firma na liście paska bocznego
// FirmaNaLiscie( biuro, firma ) --> void
static void FirmaNaLiscie( Biuro *biuro, int firma )
{
   char numer[ 32 ];
   char nazwa[ 161 ];
   char nip[ 21 ];
   char opisNip[ 32 ];

   FirmaOpis( biuro, firma, nazwa, sizeof( nazwa ), nip, sizeof( nip ) );
   StringFormat( numer, "Firma %d", firma );
   StringFormat( opisNip, "%s%s", IIF( *nip == 0, "", "NIP " ), nip );

   SidebarAddItem( biuro->side, biuro->firmy, FontIconCreate( FONT_ICONS, 0xEAAC, 0xE9EDF1, 0 ), numer, nazwa, opisNip, WybierzFirme, biuro );
}

// Lista firm od nowa
// ListaFirm( biuro ) --> void
static void ListaFirm( Biuro *biuro )
{
   SidebarClearItems( biuro->side, biuro->firmy );
   for( int i = 0; i < biuro->liczbaFirm; ++i )
   {
      FirmaNaLiscie( biuro, biuro->numery[ i ] );
   }
}

// Wybiera firmę
// UstawFirme( biuro, firma ) --> void
static void UstawFirme( Biuro *biuro, int firma )
{
   char nazwa[ 2049 ];

   biuro->firma = firma;
   PodkatalogiFirmy( biuro, firma );
   FirmaNazwa( biuro, firma, nazwa, sizeof( nazwa ) );
   if( firma == 0 )
      StringFormat( biuro->tytul, "Administrator  %s", nazwa );
   else
      StringFormat( biuro->tytul, "Firma %d  %s", firma, nazwa );
   RTrim( biuro->tytul );

   Tytul( biuro->win, biuro );
   Moduly( biuro );
}

// Tytul( win, data ) --> void
static void Tytul( Win *win, void *data )
{
   Biuro *biuro = ( Biuro * )data;
   char   cOdstep[ 1024 ];
   char   cTytul[ 3200 ];
   int    nPoczatek;
   int    nStoEm;
   int    nStoSpacji;
   int    nOdstep;
   int    nEm;
   int    nSpacji;

   nPoczatek  = WinTitleWidth( win, "" );
   nStoEm     = WinTitleWidth( win, PadL( cOdstep, sizeof( cOdstep ), "", 100, " " ) ) - nPoczatek;
   nStoSpacji = WinTitleWidth( win, PadL( cOdstep, sizeof( cOdstep ), "", 100, " " ) ) - nPoczatek;
   nOdstep    = MAX( 2 * nStoSpacji / 100, ( WinWidth( win ) - WinTitleWidth( win, biuro->tytul ) + nPoczatek ) / 2 - WinTitleWidth( win, "xBiuro" ) );
   nEm        = nOdstep * 100 / nStoEm;
   nSpacji    = ( nOdstep - nEm * nStoEm / 100 ) * 100 / nStoSpacji;

   PadL( cOdstep, sizeof( cOdstep ), "", ( size_t )nEm, " " );
   StringFormat( cTytul, "xBiuro%s%*s%s", cOdstep, nSpacji, "", biuro->tytul );
   WinSetTitle( win, cTytul );
}

// Podwójny klik na firmie: wybiera ją
// WybierzFirme( sidebar, wiersz, data ) --> void
static void WybierzFirme( Sidebar *sidebar, int wiersz, void *data )
{
   Biuro *biuro = ( Biuro * )data;
   int    firma  = biuro->numery[ wiersz - 1 ];
   char   tekst[ 48 ];

   UNUSED( sidebar );

   SidebarSetPage( biuro->side, 0 );
   StringFormat( tekst, "Otwieranie firmy %d…", firma );
   Postep( biuro, tekst );
   ZamknijModul( biuro );
   UstawFirme( biuro, firma );
}

// Dodaje nową firmę
// DodajFirme( win, data ) --> void
static void DodajFirme( Win *win, void *data )
{
   Biuro *biuro = ( Biuro * )data;
   int    firma  = biuro->numery[ biuro->liczbaFirm - 1 ] + 1;
   char   pytanie[ 160 ];
   char   katalog[ 32 ];

   UNUSED( win );

   StringFormat( pytanie, "Dodać firmę %d?\n\nDodanie firmy założy dla niej nowe bazy danych.", firma );
   if( !MsgYesNo( pytanie, "Dodaj firmę" ) )
      return;

   StringFormat( pytanie, "Zakładanie firmy %d…", firma );
   Postep( biuro, pytanie );

   PodkatalogiFirmy( biuro, firma );
   StringFormat( katalog, "firma%d", firma );
   ZnajdzFirme( katalog, T, biuro );

   FirmaNaLiscie( biuro, firma );
   SidebarSetPage( biuro->side, biuro->firmy );
   UstawFirme( biuro, firma );
}

/* ---------------------------------------------------------------------------------------------
   Okno postępu
   --------------------------------------------------------------------------------------------- */

// Krok paska postępu
// PostepKrok( timer, data ) --> void
static void PostepKrok( Timer *timer, void *data )
{
   PostepOkno *postep  = ( PostepOkno * )data;
   int         wartosc = ProgressBarValue( postep->pasek ) + 5;

   ProgressBarSetValue( postep->pasek, wartosc );
   if( wartosc >= 100 )
   {
      TimerStop( timer );
      DialogEnd( postep->okno, 1 );
   }
}

// Okno postępu na pół sekundy
// Postep( biuro, tekst ) --> void
static void Postep( Biuro *biuro, const char *tekst )
{
   PostepOkno postep;

   postep.okno = DialogCreate( biuro->win, "xBiuro", 360, 90 );
   LabelCreate( postep.okno, 20, 16, 320, 20, tekst );
   postep.pasek = ProgressBarCreate( postep.okno, 20, 46, 320, 20 );
   TimerStart( TimerCreate( postep.okno, 25, PostepKrok, &postep ) );

   DialogShowModal( postep.okno );
}

// Zakłada katalog firmy na rok z podkatalogami modułów i wydruków
// PodkatalogiFirmy( biuro, firma ) --> void
static void PodkatalogiFirmy( Biuro *biuro, int firma )
{
   static const char *aPodkatalogi[] = { "faktura", "pkpir", "ryczalt", "sfk", "place", "wydruki" };

   char cKatalog[ 64 ];
   char cPodkatalog[ 80 ];

   RokFirma( biuro, firma, cKatalog, sizeof( cKatalog ) );
   DirCreate( cKatalog );
   for( int i = 0; i < ( int )( sizeof( aPodkatalogi ) / sizeof( aPodkatalogi[ 0 ] ) ); ++i )
   {
      StringFormat( cPodkatalog, "%s%s", cKatalog, aPodkatalogi[ i ] );
      DirCreate( cPodkatalog );
   }
}

// Nowy rok: firma z katalogu poprzedniego roku dostaje swoje dane firmy i formę opodatkowania
// NowyRok( biuro, cPoprzedni, firma ) --> void
static void NowyRok( Biuro *biuro, const char *cPoprzedni, int firma )
{
   static const char *aPliki[] = { "firma.dbf", "firma.dbt", "opodatkowanie.dbf" };

   char cKatalog[ 64 ];
   char cZrodlo[ 80 ];
   char cCel[ 80 ];

   PodkatalogiFirmy( biuro, firma );
   RokFirma( biuro, firma, cKatalog, sizeof( cKatalog ) );
   for( int i = 0; i < ( int )( sizeof( aPliki ) / sizeof( aPliki[ 0 ] ) ); ++i )
   {
      if( firma == 0 )
         StringFormat( cZrodlo, "%sadmin/%s", cPoprzedni, aPliki[ i ] );
      else
         StringFormat( cZrodlo, "%sfirma%d/%s", cPoprzedni, firma, aPliki[ i ] );
      StringFormat( cCel, "%s%s", cKatalog, aPliki[ i ] );
      if( FileExists( cZrodlo ) )
         FileCopy( cZrodlo, cCel );
   }
}

// Moduły według ewidencji wybranej firmy na pasku bocznym (Moduły): lista jak firmy, z opisem i stanem
// Moduly( biuro ) --> void
static void Moduly( Biuro *biuro )
{
   xBool lDostepny;
   xBool lOtwarty;
   int   nWiersz;

   SidebarClearItems( biuro->side, biuro->moduly );

   lOtwarty = IIF( biuro->modul == 0, T, F );
   nWiersz  = SidebarAddItem( biuro->side, biuro->moduly, FontIconCreate( FONT_ICONS, 0xEAE6, IIF( lOtwarty, 0x6E7681, 0xE9EDF1 ), 0 ), "xBiuro", "Biuro rachunkowe", IIF( lOtwarty, "Otwarty", "Dostępny" ), WybierzModul, biuro );
   SidebarSetItemEnabled( biuro->side, biuro->moduly, nWiersz, !lOtwarty );

   for( int i = 0; i < ( int )( sizeof( aModuly ) / sizeof( aModuly[ 0 ] ) ); ++i )
   {
      lDostepny = OpodatkowanieModul( biuro, aSkrotModulow[ i ] );
      lOtwarty  = IIF( biuro->modul == i + 1, T, F );
      nWiersz   = SidebarAddItem( biuro->side, biuro->moduly, FontIconCreate( FONT_ICONS, 0xEAE6, IIF( lDostepny && !lOtwarty, 0xE9EDF1, 0x6E7681 ), 0 ), aModuly[ i ], aOpisModulow[ i ], IIF( lOtwarty, "Otwarty", IIF( lDostepny, "Dostępny", "Niedostępny przy tej ewidencji" ) ), WybierzModul, biuro );
      SidebarSetItemEnabled( biuro->side, biuro->moduly, nWiersz, lDostepny && !lOtwarty );
   }
}

/* ---------------------------------------------------------------------------------------------
   Akcje menu
   --------------------------------------------------------------------------------------------- */

// Ustawienia programu
// Ustawienia( win, data ) --> void
static void Ustawienia( Win *win, void *data )
{
   UNUSED( win );
   UNUSED( data );

   MsgInfo( "Ustawienia", "" );
}

// archiwizacja.prg
// Archiwizacja( win, data ) --> void
static void Archiwizacja( Win *win, void *data )
{
   UNUSED( win );
   UNUSED( data );
}

// odtworzenie.prg
// Odtworzenie( win, data ) --> void
static void Odtworzenie( Win *win, void *data )
{
   UNUSED( win );
   UNUSED( data );
}

// Indeksowanie() z xbiuro.prg
// Indeksowanie( win, data ) --> void
static void Indeksowanie( Win *win, void *data )
{
   Biuro *biuro = ( Biuro * )data;
   xBool  lWynik;

   UNUSED( win );

   if( MsgYesNo( "Indeksowanie i czyszczenie zbiorów", "Porządkowanie danych" ) )
   {
      lWynik = KontrahentNtx();
      if( !UsNtx() )
         lWynik = F;
      for( int nModul = 1; nModul <= ( int )( sizeof( aPlikiModulow ) / sizeof( aPlikiModulow[ 0 ] ) ); ++nModul )
      {
         WywolajModul( biuro, nModul, "ModulNtx" );
      }

      if( lWynik )
         MsgInfo( "Indeksowanie przebiegło prawidłowo", "Porządkowanie danych" );
   }
}


// Moduł z paska bocznego; na razie komunikat
// Modul( win, data ) --> void
static void Modul( Win *win, void *data )
{
   char cTekst[ 80 ];

   UNUSED( win );

   StringFormat( cTekst, "Moduł %s w przygotowaniu.", ( const char * )data );
   MsgInfo( cTekst, "Moduły" );
}


// Podwójny klik na module na pasku bocznym: moduł albo informacja, czemu jest niedostępny
// WybierzModul( sidebar, wiersz, data ) --> void
static void WybierzModul( Sidebar *sidebar, int wiersz, void *data )
{
   Biuro *biuro = ( Biuro * )data;
   char   cTekst[ 200 ];

   UNUSED( sidebar );

   if( wiersz == 1 )
   {
      if( biuro->modul == 0 )
         MsgInfo( "xBiuro jest już otwarte.", "Moduły" );
      else if( MsgYesNo( "Czy wrócić do xBiura?", "Moduły" ) )
      {
         SidebarSetPage( biuro->side, 0 );
         Postep( biuro, "Powrót do xBiura…" );
         ZamknijModul( biuro );
         Moduly( biuro );
      }
   }
   else if( biuro->modul == wiersz - 1 )
   {
      StringFormat( cTekst, "Moduł %s jest już otwarty.", aModuly[ wiersz - 2 ] );
      MsgInfo( cTekst, "Moduły" );
   }
   else if( OpodatkowanieModul( biuro, aSkrotModulow[ wiersz - 2 ] ) )
   {
      StringFormat( cTekst, "Czy przejść do modułu %s?", aModuly[ wiersz - 2 ] );
      if( MsgYesNo( cTekst, "Moduły" ) )
      {
         SidebarSetPage( biuro->side, 0 );
         StringFormat( cTekst, "Otwieranie modułu %s…", aModuly[ wiersz - 2 ] );
         Postep( biuro, cTekst );
         ZamknijModul( biuro );
         biuro->modul = wiersz - 1;
         if( !WywolajModul( biuro, biuro->modul, "ModulOtworz" ) )
            Modul( biuro->win, ( void * )aModuly[ wiersz - 2 ] );
         Moduly( biuro );
      }
   }
   else
   {
      StringFormat( cTekst, "Moduł %s nie jest dostępny przy ewidencji wybranej firmy.\nEwidencję zmienia się w menu Firma, Forma opodatkowania.", aModuly[ wiersz - 2 ] );
      MsgInfo( cTekst, "Moduły" );
   }
}

static void ZamknijModul( Biuro *biuro )
{
   if( biuro->modul > 0 )
      WywolajModul( biuro, biuro->modul, "ModulZamknij" );

   biuro->modul = 0;
}

static xBool WywolajModul( Biuro *biuro, int nModul, const char *cFunkcja )
{
   ModulAkcja akcja;
   void      *biblioteka;
   char       cPlik[ 64 ];

   StringFormat( cPlik, PROG_MODU "%s", aPlikiModulow[ nModul - 1 ] );
   biblioteka = LibLoad( cPlik );
   if( biblioteka == NULL )
      return F;

   akcja = ( ModulAkcja )LibFunction( biblioteka, cFunkcja );
   if( akcja == NULL )
      return F;

   akcja( biuro );

   return T;
}

// Przełącza program na dane administratora (dane-RRRR/admin/); powrót do firmy przez wybór firmy na liście
// PrzelaczAdministratora( win, data ) --> void
static void PrzelaczAdministratora( Win *win, void *data )
{
   Biuro *biuro = ( Biuro * )data;

   UNUSED( win );

   SidebarSetPage( biuro->side, 0 );
   Postep( biuro, "Przełączanie na administratora…" );
   ZamknijModul( biuro );
   UstawFirme( biuro, 0 );
}

// Komunikat z tekstem data
// Informacja( win, data ) --> void
static void Informacja( Win *win, void *data )
{
   UNUSED( win );

   MsgInfo( ( const char * )data, "" );
}

// Kończy program
// Zakoncz( win, data ) --> void
static void Zakoncz( Win *win, void *data )
{
   UNUSED( data );

   WinClose( win );
}

// Odświeża listę firm i tytuł okna po zmianie danych firmy
// ZmianaFirmy( biuro ) --> void
static void ZmianaFirmy( Biuro *biuro )
{
   ListaFirm( biuro );
   UstawFirme( biuro, biuro->firma );
}
