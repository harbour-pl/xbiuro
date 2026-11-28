/*
 * Program...: xBiuro
 * Plik......: src/faktura/fakturykor.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "faktura.h"

typedef struct
{
   char   INDEKS[ 201 ];
   char   NAZWA_TOWARU[ 2049 ];
   char   MIARA[ 255 ];
   double ILOSC;
   double CENA_JEDNOSTKOWA_NETTO;
   double WARTOSC_NETTO;
   char   STAWKA_PODATKU[ 6 ];
   double KWOTA_PODATKU;
   double WARTOSC_BRUTTO;
} PozycjaPrzed;

typedef struct
{
   int           nRok;
   long          nRecNo;
   char          cNumer[ 255 ];
   int           dData;
   char          cKsef[ 36 ];
   char          cNabywca[ 2100 ];
   double        nNetto;
   double        nVat;
   char          cPo[ 255 ];
   PozycjaPrzed *aPozycje;
   int           nPozycji;
} DaneKorekty;

typedef struct
{
   int  nRok;
   char cKlucz[ 16 ];
   char cNumer[ 255 ];
} Ostatnia;

typedef struct
{
   Biuro     *biuro;
   Win       *okno;
   StatusBar *pasekStanu;
   Toolbar   *pasek;
   Control   *BRW;
   int        aZamroz[ 5 ];
   char       cData[ 16 ];
   char       cNabywca[ 2100 ];
} OknoKorekt;

typedef struct
{
   OknoPozycji pozycje;
   Biuro      *biuro;
   DaneKorekty hKor;
   int         aZamroz[ 2 ];
   char        cTemp[ 400 ];
   char        cNazwa[ 2100 ];
   Control    *SUMA;
} OknoKorekty;

typedef struct
{
   Win  *okno;
   long  nWybor;
   char  cData[ 16 ];
   char  cNabywca[ 2100 ];
} OknoWyboru;

static xBool       Tymczasowa( OknoKorekty *oDialog );
static char       *Numer( Biuro *biuro, int nNr, char *cNumer, int nRozmiar );
static void        Stan( OknoKorekt *oDialog );
static const char *Kolumna( Control *browse, void *data );
static const char *KolumnaKsef( Control *browse, void *data );
static const char *KolumnaData( Control *browse, void *data );
static const char *KolumnaNabywca( Control *browse, void *data );
static double      KolumnaKwota( Control *browse, void *data );
static void        Pomoc( Toolbar *toolbar, void *data );
static void        Nowa( Toolbar *toolbar, void *data );
static void        Wstaw( Win *win, void *data );
static void        Podglad( Toolbar *toolbar, void *data );
static void        WystawOffline( Toolbar *toolbar, void *data );
static void        Wyslij( Toolbar *toolbar, void *data );
static void        SprawdzKsef( Toolbar *toolbar, void *data );
static void        Drukuj( Toolbar *toolbar, void *data );
static void        Zamknij( Toolbar *toolbar, void *data );
static long        Wybierz( Win *win, Biuro *biuro, int nRok );
static xBool       WyborVat( void *data );
static const char *WyborKsef( Control *browse, void *data );
static const char *WyborKolumna( Control *browse, void *data );
static const char *WyborData( Control *browse, void *data );
static const char *WyborNabywca( Control *browse, void *data );
static double      WyborKwota( Control *browse, void *data );
static void        WyborEnter( Toolbar *toolbar, void *data );
static void        WyborZamknij( Toolbar *toolbar, void *data );
static void        Korekta( Win *win, Biuro *biuro, int nRok, long nKorygowana );
static void        StanKorekty( OknoKorekty *oDialog );
static void        Suma( OknoKorekty *oDialog );
static void        Odswiez( void *data );
static void        Zmiana( Control *browse, void *data );
static const char *KorektaKolumna( Control *browse, void *data );
static const char *KorektaNazwa( Control *browse, void *data );
static double      KorektaLiczba( Control *browse, void *data );
static void        Pozycja( OknoKorekty *oDialog, xBool lNowa );
static void        Dodaj( Toolbar *toolbar, void *data );
static void        WstawPozycje( Win *win, void *data );
static void        Popraw( Toolbar *toolbar, void *data );
static void        Usun( Toolbar *toolbar, void *data );
static void        Zapisz( Toolbar *toolbar, void *data );
static void        Anuluj( Toolbar *toolbar, void *data );
static xBool       OstatniaKorekta( Biuro *biuro, int nRok, const char *cNumer, Ostatnia *aOstatnia );
static xBool       PozycjeZrodla( Biuro *biuro, int nRok, const char *cKlucz, DaneKorekty *hKor );
static xBool       RokBiezacy( Biuro *biuro, int dWyst );
static xBool       Powod( OknoKorekty *oDialog, char *cPowod, int nRozmiar );
static void        Glowa( Faktura *hGlowa );
static char       *Linia( const char *cTekst, char *cWynik, int nRozmiar );
static void        Opis( Win *strona, int x, int y, const char *tekst );
static Control    *Pole( Win *strona, int x, int y, int dlugosc, const char *tekst );

void FakturyKor( Win *win, void *data )
{
   OknoKorekt  oknoKorekt = { 0 };
   OknoKorekt *oDialog    = &oknoKorekt;
   Win        *panel;
   char        cTytul[ 64 ];
   int         przycisk;

   oDialog->biuro = ( Biuro * )data;
   if( !FakturyOtworz( oDialog->biuro ) )
      return;
   OfflinePrzypomnij();
   FakturyZakres( "KOR" );

   StringFormat( cTytul, "Faktury korygujące %d", oDialog->biuro->rok );
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
   przycisk = ToolbarAdd( oDialog->pasek, "Esc-Zamknij", Zamknij, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "Esc" );

   panel = PanelCreate( oDialog->okno, 0, 0, 0, 0 );

   oDialog->BRW = BrowseCreate( panel, 10, 10, WinWidth( panel ) - 20, WinHeight( panel ) - 20 );
   ControlSetAnchor( oDialog->BRW, "LTRB" );
   BrowseSetColor( oDialog->BRW, BROWSE_HEADBACKGROUND, 0x4D7326 );
   DbBrowse( oDialog->BRW, "FKTHEAD" );
   BrowseAddColumn( oDialog->BRW, "W KSeF", 7, BROWSE_LEFT, KolumnaKsef, NULL );
   BrowseAddColumn( oDialog->BRW, "Numer", 14, BROWSE_LEFT, Kolumna, "NUMER_FAKTURY" );
   BrowseAddColumn( oDialog->BRW, "Wystawiona", 10, BROWSE_LEFT, KolumnaData, oDialog );
   BrowseAddColumn( oDialog->BRW, "Do faktury", 14, BROWSE_LEFT, Kolumna, "NUMER_FAKTURY_KORYGOWANEJ" );
   BrowseAddColumn( oDialog->BRW, "Nabywca", 30, BROWSE_LEFT, KolumnaNabywca, oDialog );
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

static xBool Tymczasowa( OknoKorekty *oDialog )
{
   const DbField *aWzor;
   char           cPlik[ 420 ];
   int            nPol;

   FakturyTymczasowa( "kortemp", oDialog->cTemp, sizeof( oDialog->cTemp ) );
   StringFormat( cPlik, "%s.dbf", oDialog->cTemp );
   FErase( cPlik );
   StringFormat( cPlik, "%s.dbt", oDialog->cTemp );
   FErase( cPlik );
   aWzor = PozycjeStruct( &nPol );
   DbCreate( oDialog->cTemp, aWzor, nPol );

   return NetUse( oDialog->cTemp, T, NET_WAIT, "KORTEMP", F );
}

static char *Numer( Biuro *biuro, int nNr, char *cNumer, int nRozmiar )
{
   StringFormatSize( cNumer, ( size_t )nRozmiar, "KOR/%d/%d", nNr, biuro->rok );

   return cNumer;
}

static void Stan( OknoKorekt *oDialog )
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
   OknoKorekt *oDialog = ( OknoKorekt * )data;

   UNUSED( browse );

   DbSelectArea( "FKTHEAD" );

   return DToC( FieldGetD( FieldPos( "DATA_WYSTAWIENIA" ) ), oDialog->cData, sizeof( oDialog->cData ) );
}

static const char *KolumnaNabywca( Control *browse, void *data )
{
   OknoKorekt *oDialog = ( OknoKorekt * )data;

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

static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgHelp( "<a href=\"" USTAWA_VAT "\">Ustawa o podatku od towarów i usług</a> (art. 106j – faktura korygująca)\n\n"
            "F1\tpomoc\n"
            "F2, Ins\tnowa korekta: wybór roku i faktury VAT, potem poprawa pozycji; korekta dostaje dzisiejszą datę i numer w bieżącym roku\n"
            "F3\tpodgląd korekty: pozycje przed korektą i po korekcie\n"
            "F5\twystawienie korekty w trybie offline\n"
            "F6\twysłanie korekty do KSeF\n"
            "F7\tsprawdzenie korekty w KSeF i pobranie UPO\n"
            "F9\twydruk korekty do PDF w katalogu wydruków firmy\n"
            "Esc\tzamknięcie okna\n\n"
            "Okno korekty\n\n"
            "Na starcie są pozycje faktury korygowanej. Popraw je tak, jak powinny wyglądać po korekcie:\n\n"
            "F2, Ins\tnowa pozycja\n"
            "F4\tpoprawienie pozycji\n"
            "F8\tusunięcie pozycji\n"
            "F10\tzapis: przyczyna korekty, numer KOR/kolejny/rok\n"
            "Esc\trezygnacja z korekty\n\n"
            "Program zapisuje stan przed korektą i po korekcie, a kwoty korekty to różnica między nimi. Ujemna kwota oznacza zwrot dla nabywcy.", "Faktury korygujące" );
}

static void Nowa( Toolbar *toolbar, void *data )
{
   OknoKorekt *oDialog = ( OknoKorekt * )data;
   int         nRok;
   long        nKorygowana;

   UNUSED( toolbar );

   if( !RokBiezacy( oDialog->biuro, Date() ) )
      return;

   nRok = RokWybierz( oDialog->okno, oDialog->biuro, "Rok faktury korygowanej" );
   if( nRok == 0 )
      return;

   nKorygowana = Wybierz( oDialog->okno, oDialog->biuro, nRok );
   if( nKorygowana > 0 )
      Korekta( oDialog->okno, oDialog->biuro, nRok, nKorygowana );

   DbSelectArea( "FKTHEAD" );
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

   FakturyPodglad( ( ( OknoKorekt * )data )->okno );
}

static void WystawOffline( Toolbar *toolbar, void *data )
{
   OknoKorekt *oDialog = ( OknoKorekt * )data;

   UNUSED( toolbar );

   Offline( oDialog->okno, oDialog->biuro, oDialog->BRW );
}

static void Wyslij( Toolbar *toolbar, void *data )
{
   OknoKorekt *oDialog = ( OknoKorekt * )data;

   UNUSED( toolbar );

   FakturyWyslij( oDialog->biuro, oDialog->pasekStanu, oDialog->BRW );
}

static void SprawdzKsef( Toolbar *toolbar, void *data )
{
   OknoKorekt *oDialog = ( OknoKorekt * )data;

   UNUSED( toolbar );

   FakturySprawdz( oDialog->biuro, oDialog->pasekStanu, oDialog->BRW );
}

static void Drukuj( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   FakturyDrukuj( ( ( OknoKorekt * )data )->biuro );
}

static void Zamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( OknoKorekt * )data )->okno );
}

static long Wybierz( Win *win, Biuro *biuro, int nRok )
{
   OknoWyboru  oknoWyboru = { 0 };
   OknoWyboru *oDialog    = &oknoWyboru;
   Toolbar    *oToolbar;
   Win        *oPanel;
   Control    *oBrowse;
   char        cKatalog[ 96 ];
   char        cBaza[ 112 ];
   char        cTekst[ 200 ];
   int         przycisk;

   StringFormat( cBaza, "%sfkthead", FakturyKatalog( biuro, nRok, cKatalog, sizeof( cKatalog ) ) );
   if( !NetUse( cBaza, F, NET_WAIT, "FKTWYBOR", F ) )
      return 0;

   DbSetFilter( WyborVat, NULL );
   DbGoBottom();
   if( Eof() )
   {
      DbCloseArea();
      StringFormat( cTekst, "Brak faktur VAT do skorygowania w roku %d.", nRok );
      MsgInfo( cTekst, "Faktura korygująca" );

      return 0;
   }

   StringFormat( cTekst, "Wybierz fakturę VAT do korekty - rok %d", nRok );
   oDialog->okno = DialogCreate( win, cTekst, 900, 560 );
   WinSetBackground( oDialog->okno, 0xE9EDF1 );
   WinSetSizable( oDialog->okno, T );

   oToolbar = ToolbarCreate( oDialog->okno, TOOLBAR_BOTTOM );
   przycisk = ToolbarAdd( oToolbar, "Enter-Wybierz", WyborEnter, oDialog );
   ToolbarSetShortcut( oToolbar, przycisk, "Enter" );
   ToolbarAddSeparator( oToolbar );
   przycisk = ToolbarAdd( oToolbar, "Esc-Rezygnuj", WyborZamknij, oDialog );
   ToolbarSetShortcut( oToolbar, przycisk, "Esc" );

   oPanel = PanelCreate( oDialog->okno, 0, 0, 0, 0 );

   oBrowse = BrowseCreate( oPanel, 10, 10, WinWidth( oPanel ) - 20, WinHeight( oPanel ) - 20 );
   ControlSetAnchor( oBrowse, "LTRB" );
   BrowseSetColor( oBrowse, BROWSE_HEADBACKGROUND, 0x4D7326 );
   DbBrowse( oBrowse, "FKTWYBOR" );
   BrowseAddColumn( oBrowse, "W KSeF", 6, BROWSE_LEFT, WyborKsef, NULL );
   BrowseAddColumn( oBrowse, "Numer", 14, BROWSE_LEFT, WyborKolumna, "NUMER_FAKTURY" );
   BrowseAddColumn( oBrowse, "Wystawiona", 10, BROWSE_LEFT, WyborData, oDialog );
   BrowseAddColumn( oBrowse, "Nabywca", 34, BROWSE_LEFT, WyborNabywca, oDialog );
   BrowseAddNumber( oBrowse, "Brutto", 12, "999999999.99", WyborKwota, "KWOTA_NALEZNOSCI" );
   ControlSetFocus( oBrowse );

   WinCenter( oDialog->okno );
   DialogShowModal( oDialog->okno );

   DbSelectArea( "FKTWYBOR" );
   DbCloseArea();

   return oDialog->nWybor;
}

static xBool WyborVat( void *data )
{
   UNUSED( data );

   return IIF( strncmp( FieldGetC( FieldPos( "RODZAJ_FAKTURY" ) ), "VAT ", 4 ) == 0, T, F );
}

static const char *WyborKsef( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "FKTWYBOR" );

   return IIF( FieldGetN( FieldPos( "STATUS_KSEF" ) ) == KSEF_PRZYJETA, "Tak", "Nie" );
}

static const char *WyborKolumna( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "FKTWYBOR" );

   return FieldGetC( FieldPos( ( const char * )data ) );
}

static const char *WyborData( Control *browse, void *data )
{
   OknoWyboru *oDialog = ( OknoWyboru * )data;

   UNUSED( browse );

   DbSelectArea( "FKTWYBOR" );

   return DToC( FieldGetD( FieldPos( "DATA_WYSTAWIENIA" ) ), oDialog->cData, sizeof( oDialog->cData ) );
}

static const char *WyborNabywca( Control *browse, void *data )
{
   OknoWyboru *oDialog = ( OknoWyboru * )data;

   UNUSED( browse );

   DbSelectArea( "FKTWYBOR" );

   return Linia( FieldGetC( FieldPos( "NAZWA_NABYWCY" ) ), oDialog->cNabywca, sizeof( oDialog->cNabywca ) );
}

static double WyborKwota( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "FKTWYBOR" );

   return FieldGetN( FieldPos( ( const char * )data ) );
}

static void WyborEnter( Toolbar *toolbar, void *data )
{
   OknoWyboru *oDialog = ( OknoWyboru * )data;

   UNUSED( toolbar );

   DbSelectArea( "FKTWYBOR" );
   oDialog->nWybor = RecNo();
   DialogEnd( oDialog->okno, 1 );
}

static void WyborZamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( OknoWyboru * )data )->okno );
}

static void Korekta( Win *win, Biuro *biuro, int nRok, long nKorygowana )
{
   OknoKorekty   oknoKorekty = { 0 };
   OknoKorekty  *oDialog     = &oknoKorekty;
   DaneKorekty  *hKor        = &oDialog->hKor;
   OknoPozycji  *oPozycje    = &oDialog->pozycje;
   Ostatnia      ostatnia;
   PozycjaPrzed *aPoz;
   Win          *panel;
   Control      *BRW;
   xBool         lOstatnia;
   xBool         lJest;
   char          cKatalog[ 96 ];
   char          cBaza[ 112 ];
   char          cKlucz[ 16 ];
   char          cTytul[ 600 ];
   char          cTekst[ 600 ];
   char          cData[ 16 ];
   char          cPlik[ 420 ];

   oDialog->biuro = biuro;
   StringFormat( cBaza, "%sfkthead", FakturyKatalog( biuro, nRok, cKatalog, sizeof( cKatalog ) ) );
   if( !NetUse( cBaza, F, NET_WAIT, "FKTHEAD_ZRODLO", F ) )
      return;

   DbGoto( nKorygowana );
   hKor->nRok   = nRok;
   hKor->nRecNo = nKorygowana;
   StringFormat( hKor->cNumer, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY" ) ) );
   AllTrim( hKor->cNumer );
   hKor->dData = FieldGetD( FieldPos( "DATA_WYSTAWIENIA" ) );
   if( FieldGetN( FieldPos( "STATUS_KSEF" ) ) == KSEF_PRZYJETA )
   {
      StringFormat( hKor->cKsef, "%s", FieldGetC( FieldPos( "NUMER_KSEF" ) ) );
      AllTrim( hKor->cKsef );
   }
   Linia( FieldGetC( FieldPos( "NAZWA_NABYWCY" ) ), hKor->cNabywca, sizeof( hKor->cNabywca ) );
   hKor->nNetto = FieldGetN( FieldPos( "WARTOSC_NETTO" ) );
   hKor->nVat   = FieldGetN( FieldPos( "KWOTA_PODATKU" ) );
   FakturyKlucz( cKlucz, sizeof( cKlucz ) );

   lOstatnia = OstatniaKorekta( biuro, biuro->rok, hKor->cNumer, &ostatnia );
   if( !lOstatnia && nRok != biuro->rok )
      lOstatnia = OstatniaKorekta( biuro, nRok, hKor->cNumer, &ostatnia );

   if( lOstatnia )
   {
      StringFormat( hKor->cPo, "%s", ostatnia.cNumer );
      lJest = PozycjeZrodla( biuro, ostatnia.nRok, ostatnia.cKlucz, hKor );
   }
   else
      lJest = PozycjeZrodla( biuro, nRok, cKlucz, hKor );

   if( !lJest || !Tymczasowa( oDialog ) )
   {
      free( hKor->aPozycje );
      DbSelectArea( "FKTHEAD_ZRODLO" );
      DbCloseArea();
      DbSelectArea( "FKTHEAD" );

      return;
   }

   if( *hKor->cPo != 0 )
   {
      hKor->nNetto = 0;
      hKor->nVat   = 0;
   }
   DbSelectArea( "KORTEMP" );
   for( int i = 0; i < hKor->nPozycji; ++i )
   {
      aPoz = &hKor->aPozycje[ i ];
      if( *hKor->cPo != 0 )
      {
         hKor->nNetto += aPoz->WARTOSC_NETTO;
         hKor->nVat   += aPoz->KWOTA_PODATKU;
      }
      DbAppend( T );
      FieldPutN( FieldPos( "NUMER_WIERSZA" ), ( double )LastRec() );
      FieldPutC( FieldPos( "INDEKS" ), aPoz->INDEKS );
      FieldPutC( FieldPos( "NAZWA_TOWARU" ), aPoz->NAZWA_TOWARU );
      FieldPutC( FieldPos( "MIARA" ), aPoz->MIARA );
      FieldPutN( FieldPos( "ILOSC" ), aPoz->ILOSC );
      FieldPutN( FieldPos( "CENA_JEDNOSTKOWA_NETTO" ), aPoz->CENA_JEDNOSTKOWA_NETTO );
      FieldPutN( FieldPos( "WARTOSC_NETTO" ), aPoz->WARTOSC_NETTO );
      FieldPutC( FieldPos( "STAWKA_PODATKU" ), aPoz->STAWKA_PODATKU );
      FieldPutN( FieldPos( "KWOTA_PODATKU" ), aPoz->KWOTA_PODATKU );
      FieldPutN( FieldPos( "WARTOSC_BRUTTO" ), aPoz->WARTOSC_BRUTTO );
   }
   DbCommit();
   DbGoTop();

   StringFormat( cTytul, "Faktura korygująca do %s%s%s", hKor->cNumer, IIF( *hKor->cPo == 0, "", " - stan po korekcie " ), hKor->cPo );
   oPozycje->okno    = DialogCreate( win, cTytul, 1100, 700 );
   oPozycje->odswiez = Odswiez;
   oPozycje->data    = oDialog;
   WinSetBackground( oPozycje->okno, 0xE9EDF1 );
   WinSetSizable( oPozycje->okno, T );
   WinSetKey( oPozycje->okno, "Ins", WstawPozycje, oDialog );

   oPozycje->pasekStanu = StatusBarCreate( oPozycje->okno, "" );
   StatusBarAdd( oPozycje->pasekStanu, 0, "", F );
   StatusBarAdd( oPozycje->pasekStanu, 60, "", F );
   StatusBarSetScoreboard( oPozycje->pasekStanu, 2 );

   oPozycje->pasek = ToolbarCreate( oPozycje->okno, TOOLBAR_BOTTOM );
   oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "F1-Pomoc", Pomoc, NULL );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "F1" );
   ToolbarAddSeparator( oPozycje->pasek );
   oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "F2-Dodaj pozycję", Dodaj, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "F2" );
   ToolbarAddSeparator( oPozycje->pasek );
   oDialog->aZamroz[ 0 ] = oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "F4-Popraw", Popraw, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "F4" );
   ToolbarAddSeparator( oPozycje->pasek );
   oDialog->aZamroz[ 1 ] = oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "F8-Usuń", Usun, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "F8" );
   ToolbarAddSeparator( oPozycje->pasek );
   oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "F10-Zapisz", Zapisz, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "F10" );
   ToolbarAddSeparator( oPozycje->pasek );
   oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "Esc-Anuluj", Anuluj, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "Esc" );

   panel = PanelCreate( oPozycje->okno, 0, 0, 0, 0 );

   Opis( panel, 20,   7, "Numer korekty:" );
   Opis( panel, 20,  37, "Data wystawienia:" );
   Opis( panel, 20,  67, "Faktura korygowana:" );
   Opis( panel, 20,  97, "Numer KSeF:" );
   Opis( panel, 20, 127, "Nabywca:" );
   DbSelectArea( "FKTHEAD" );
   Pole( panel, 200,   4, 14, Numer( biuro, FakturyNastepnyNumer( "KOR" ), cTekst, sizeof( cTekst ) ) );
   Pole( panel, 200,  34, 10, DToC( Date(), cData, sizeof( cData ) ) );
   StringFormat( cTekst, "%s z dnia %s", hKor->cNumer, DToC( hKor->dData, cData, sizeof( cData ) ) );
   Pole( panel, 200,  64, 40, cTekst );
   Pole( panel, 200,  94, 40, IIF( *hKor->cKsef == 0, "faktura poza KSeF", hKor->cKsef ) );
   Pole( panel, 200, 124, 60, hKor->cNabywca );

   BRW           = BrowseCreate( panel, 10, 158, WinWidth( panel ) - 20, WinHeight( panel ) - 202 );
   oPozycje->BRW = BRW;
   ControlSetAnchor( BRW, "LTRB" );
   BrowseSetColor( BRW, BROWSE_HEADBACKGROUND, 0x4D7326 );
   DbBrowse( BRW, "KORTEMP" );
   BrowseAddNumber( BRW, "Lp", 4, "9999", KorektaLiczba, "NUMER_WIERSZA" );
   BrowseAddColumn( BRW, "Po korekcie: nazwa towaru / usługi", 36, BROWSE_LEFT, KorektaNazwa, oDialog );
   BrowseAddNumber( BRW, "Ilość", 12, "99999999.999", KorektaLiczba, "ILOSC" );
   BrowseAddColumn( BRW, "J.m.", 5, BROWSE_LEFT, KorektaKolumna, "MIARA" );
   BrowseAddNumber( BRW, "Cena netto", 12, "999999999.99", KorektaLiczba, "CENA_JEDNOSTKOWA_NETTO" );
   BrowseAddNumber( BRW, "Netto", 12, "999999999.99", KorektaLiczba, "WARTOSC_NETTO" );
   BrowseAddColumn( BRW, "VAT", 3, BROWSE_LEFT, KorektaKolumna, "STAWKA_PODATKU" );
   BrowseAddNumber( BRW, "Kwota VAT", 12, "999999999.99", KorektaLiczba, "KWOTA_PODATKU" );
   BrowseAddNumber( BRW, "Brutto", 12, "999999999.99", KorektaLiczba, "WARTOSC_BRUTTO" );
   ControlSetAction( BRW, Zmiana, oDialog );

   oDialog->SUMA = Pole( panel, 10, WinHeight( panel ) - 36, 100, "" );
   ControlSetAnchor( oDialog->SUMA, "LB" );

   Suma( oDialog );
   StanKorekty( oDialog );

   WinCenter( oPozycje->okno );
   DialogShowModal( oPozycje->okno );

   DbSelectArea( "KORTEMP" );
   DbCloseArea();
   StringFormat( cPlik, "%s.dbf", oDialog->cTemp );
   FErase( cPlik );
   StringFormat( cPlik, "%s.dbt", oDialog->cTemp );
   FErase( cPlik );
   DbSelectArea( "FKTHEAD_ZRODLO" );
   DbCloseArea();
   free( hKor->aPozycje );
   DbSelectArea( "FKTHEAD" );
}

static void StanKorekty( OknoKorekty *oDialog )
{
   OknoPozycji *oPozycje = &oDialog->pozycje;
   xBool        lJest;

   DbSelectArea( "KORTEMP" );
   lJest = IIF( LastRec() > 0, T, F );
   ToolbarEnable( oPozycje->pasek, oDialog->aZamroz[ 0 ], lJest );
   ToolbarEnable( oPozycje->pasek, oDialog->aZamroz[ 1 ], lJest );
   ControlSetEnabled( oPozycje->BRW, lJest );
   BrowseRefresh( oPozycje->BRW );

   if( lJest )
      ControlSetFocus( oPozycje->BRW );
}

static void Suma( OknoKorekty *oDialog )
{
   double nNetto;
   double nVat;
   xBool  lZw;
   char   cTekst[ 200 ];

   PozycjeSuma( "KORTEMP", &nNetto, &nVat, &lZw );
   nNetto -= oDialog->hKor.nNetto;
   nVat   -= oDialog->hKor.nVat;
   StringFormat( cTekst, " Zmiana po korekcie:     Netto: %.2f     VAT: %.2f     Brutto: %.2f", nNetto, nVat, nNetto + nVat );
   ControlSetText( oDialog->SUMA, cTekst );
}

static void Odswiez( void *data )
{
   Suma( ( OknoKorekty * )data );
}

static void Zmiana( Control *browse, void *data )
{
   UNUSED( browse );

   Suma( ( OknoKorekty * )data );
}

static const char *KorektaKolumna( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "KORTEMP" );

   return FieldGetC( FieldPos( ( const char * )data ) );
}

static const char *KorektaNazwa( Control *browse, void *data )
{
   OknoKorekty *oDialog = ( OknoKorekty * )data;

   UNUSED( browse );

   DbSelectArea( "KORTEMP" );

   return Linia( FieldGetC( FieldPos( "NAZWA_TOWARU" ) ), oDialog->cNazwa, sizeof( oDialog->cNazwa ) );
}

static double KorektaLiczba( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "KORTEMP" );

   return FieldGetN( FieldPos( ( const char * )data ) );
}

static void Pozycja( OknoKorekty *oDialog, xBool lNowa )
{
   PozycjeEdytuj( &oDialog->pozycje, "KORTEMP", lNowa, "Pozycja po korekcie", "Faktura korygująca", T );
   StanKorekty( oDialog );
}

static void Dodaj( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   Pozycja( ( OknoKorekty * )data, T );
}

static void WstawPozycje( Win *win, void *data )
{
   UNUSED( win );

   Pozycja( ( OknoKorekty * )data, T );
}

static void Popraw( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   Pozycja( ( OknoKorekty * )data, F );
}

static void Usun( Toolbar *toolbar, void *data )
{
   OknoKorekty *oDialog = ( OknoKorekty * )data;

   UNUSED( toolbar );

   PozycjeUsun( &oDialog->pozycje, "KORTEMP", "Faktura korygująca" );
   StanKorekty( oDialog );
}

static void Zapisz( Toolbar *toolbar, void *data )
{
   OknoKorekty  *oDialog = ( OknoKorekty * )data;
   DaneKorekty  *hKor    = &oDialog->hKor;
   Faktura       hGlowa  = { 0 };
   PozycjaPrzed *aPoz;
   long         *aDodane;
   int           nDodane = 0;
   int           nNr;
   int           dWyst   = Date();
   double        nNetto;
   double        nVat;
   xBool         lZw;
   xBool         lOkay   = T;
   char          cPowod[ 1025 ];
   char          cNumer[ 64 ];
   char          cTekst[ 400 ];

   UNUSED( toolbar );

   if( !RokBiezacy( oDialog->biuro, dWyst ) )
      return;

   if( !PozycjeSuma( "KORTEMP", &nNetto, &nVat, &lZw ) )
   {
      MsgInfo( "Wartość faktury musi być mniejsza niż 1 000 000 000.", "Faktura korygująca" );

      return;
   }

   if( Round( nNetto, 2 ) == Round( hKor->nNetto, 2 ) && Round( nVat, 2 ) == Round( hKor->nVat, 2 ) && !MsgYesNo( "Korekta nie zmienia kwot faktury.\nZapisać ją mimo to?", "Faktura korygująca" ) )
      return;

   if( !Powod( oDialog, cPowod, sizeof( cPowod ) ) )
      return;

   DbSelectArea( "FKTHEAD_ZRODLO" );
   DbGoto( hKor->nRecNo );
   Glowa( &hGlowa );
   if( lZw && *hGlowa.PODSTAWA_ZWOLNIENIA == 0 )
   {
      if( FirmaPobierzDane( oDialog->biuro, "SPRZEDAWCA" ) )
      {
         StringFormat( hGlowa.PODSTAWA_ZWOLNIENIA, "%s", FieldGetC( FieldPos( "PODSTAWA_ZWOLNIENIA" ) ) );
         AllTrim( hGlowa.PODSTAWA_ZWOLNIENIA );
         DbCloseArea();
      }
      if( *hGlowa.PODSTAWA_ZWOLNIENIA == 0 )
      {
         MsgInfo( "Pozycja ze stawką zw wymaga podstawy prawnej zwolnienia.\nWpisz ją w menu Firma, Dane firmy, zakładka VAT.", "Faktura korygująca" );

         return;
      }
   }

   DbSelectArea( "FKTHEAD" );
   if( !FLock() )
   {
      MsgInfo( "Inne stanowisko zapisuje teraz fakturę. Spróbuj za chwilę.", "Faktura korygująca" );

      return;
   }
   nNr = FakturyNastepnyNumer( "KOR" );

   DbSelectArea( "KORTEMP" );
   aDodane = ( long * )calloc( ( size_t )( hKor->nPozycji + LastRec() + 1 ), sizeof( long ) );
   for( int nLp = 1; nLp <= hKor->nPozycji; ++nLp )
   {
      aPoz = &hKor->aPozycje[ nLp - 1 ];
      DbSelectArea( "FKTROW" );
      if( !NetAppBlank( NET_WAIT, F ) )
      {
         lOkay = F;
         break;
      }
      aDodane[ nDodane++ ] = RecNo();
      FieldPutN( FieldPos( "KOLEJNY_NUMER" ), nNr );
      FieldPutC( FieldPos( "RODZAJ_FAKTURY" ), "KOR" );
      FieldPutN( FieldPos( "NUMER_WIERSZA" ), nLp );
      FieldPutC( FieldPos( "INDEKS" ), aPoz->INDEKS );
      FieldPutC( FieldPos( "NAZWA_TOWARU" ), aPoz->NAZWA_TOWARU );
      FieldPutC( FieldPos( "MIARA" ), aPoz->MIARA );
      FieldPutN( FieldPos( "ILOSC" ), aPoz->ILOSC );
      FieldPutN( FieldPos( "CENA_JEDNOSTKOWA_NETTO" ), aPoz->CENA_JEDNOSTKOWA_NETTO );
      FieldPutN( FieldPos( "WARTOSC_NETTO" ), aPoz->WARTOSC_NETTO );
      FieldPutC( FieldPos( "STAWKA_PODATKU" ), aPoz->STAWKA_PODATKU );
      FieldPutN( FieldPos( "KWOTA_PODATKU" ), aPoz->KWOTA_PODATKU );
      FieldPutN( FieldPos( "WARTOSC_BRUTTO" ), aPoz->WARTOSC_BRUTTO );
      FieldPutL( FieldPos( "STAN_PRZED" ), T );
      FieldPutD( FieldPos( "DATA_ZAPISU" ), Date() );
      DbCommit();
   }

   if( lOkay )
   {
      int nPoKorekcie = 0;

      lOkay   = PozycjeZapisz( "KORTEMP", nNr, "KOR", hKor->nPozycji, F, aDodane + nDodane, &nPoKorekcie );
      nDodane += nPoKorekcie;
   }

   if( !lOkay )
   {
      PozycjeWycofaj( aDodane, nDodane );
      DbSelectArea( "FKTHEAD" );
      DbUnlock();
      free( aDodane );
      MsgError( "Nie udało się zapisać pozycji. Korekta nie została zapisana.", "Faktura korygująca" );

      return;
   }

   DbSelectArea( "FKTHEAD" );
   DbAppend( F );
   if( NetErr() )
   {
      PozycjeWycofaj( aDodane, nDodane );
      DbSelectArea( "FKTHEAD" );
      DbUnlock();
      free( aDodane );
      MsgError( "Nie udało się dopisać nagłówka korekty. Korekta nie została zapisana.", "Faktura korygująca" );

      return;
   }
   free( aDodane );

   Numer( oDialog->biuro, nNr, cNumer, sizeof( cNumer ) );
   FieldPutN( FieldPos( "KOLEJNY_NUMER" ), nNr );
   FieldPutC( FieldPos( "RODZAJ_FAKTURY" ), "KOR" );
   FieldPutC( FieldPos( "NUMER_FAKTURY" ), cNumer );
   FieldPutD( FieldPos( "DATA_WYSTAWIENIA" ), dWyst );
   FieldPutD( FieldPos( "DATA_DOSTAWY" ), hGlowa.DATA_DOSTAWY );
   FieldPutC( FieldPos( "KOD_KRAJU_NABYWCY" ), hGlowa.KOD_KRAJU_NABYWCY );
   FieldPutC( FieldPos( "NIP_NABYWCY" ), hGlowa.NIP_NABYWCY );
   FieldPutC( FieldPos( "NAZWA_NABYWCY" ), hGlowa.NAZWA_NABYWCY );
   FieldPutC( FieldPos( "ULICA_NABYWCY" ), hGlowa.ULICA_NABYWCY );
   FieldPutC( FieldPos( "KOD_POCZTOWY_NABYWCY" ), hGlowa.KOD_POCZTOWY_NABYWCY );
   FieldPutC( FieldPos( "MIEJSCOWOSC_NABYWCY" ), hGlowa.MIEJSCOWOSC_NABYWCY );
   FieldPutN( FieldPos( "FORMA_PLATNOSCI" ), hGlowa.FORMA_PLATNOSCI );
   FieldPutD( FieldPos( "TERMIN_PLATNOSCI" ), hGlowa.TERMIN_PLATNOSCI );
   FieldPutC( FieldPos( "NAZWA_BANKU" ), hGlowa.NAZWA_BANKU );
   FieldPutC( FieldPos( "NUMER_RACHUNKU" ), hGlowa.NUMER_RACHUNKU );
   FieldPutC( FieldPos( "PODSTAWA_ZWOLNIENIA" ), hGlowa.PODSTAWA_ZWOLNIENIA );
   FieldPutC( FieldPos( "NUMER_FAKTURY_KORYGOWANEJ" ), hKor->cNumer );
   FieldPutD( FieldPos( "DATA_FAKTURY_KORYGOWANEJ" ), hKor->dData );
   FieldPutC( FieldPos( "NUMER_KSEF_FAKTURY_KORYGOWANEJ" ), hKor->cKsef );
   FieldPutC( FieldPos( "PRZYCZYNA_KOREKTY" ), cPowod );
   FieldPutN( FieldPos( "WARTOSC_NETTO" ), nNetto - hKor->nNetto );
   FieldPutN( FieldPos( "KWOTA_PODATKU" ), nVat - hKor->nVat );
   FieldPutN( FieldPos( "KWOTA_NALEZNOSCI" ), nNetto + nVat - hKor->nNetto - hKor->nVat );
   FieldPutD( FieldPos( "DATA_ZAPISU" ), Date() );
   DbCommit();
   DbUnlock();
   DbSelectArea( "FKTROW" );
   DbUnlock();

   StringFormat( cTekst, "Zapisano fakturę korygującą %s\nZmiana brutto: %.2f zł", cNumer, nNetto + nVat - hKor->nNetto - hKor->nVat );
   MsgInfo( cTekst, "Faktura korygująca" );
   WinClose( oDialog->pozycje.okno );
}

static void Anuluj( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   if( MsgYesNo( "Zrezygnować z korekty?", "Faktura korygująca" ) )
      WinClose( ( ( OknoKorekty * )data )->pozycje.okno );
}

static xBool OstatniaKorekta( Biuro *biuro, int nRok, const char *cNumer, Ostatnia *aOstatnia )
{
   char  cKatalog[ 96 ];
   char  cBaza[ 112 ];
   char  cPlik[ 128 ];
   char  cJest[ 255 ];
   xBool lJest = F;

   StringFormat( cBaza, "%sfkthead", FakturyKatalog( biuro, nRok, cKatalog, sizeof( cKatalog ) ) );
   StringFormat( cPlik, "%s.dbf", cBaza );
   if( !FileExists( cPlik ) || !NetUse( cBaza, F, NET_WAIT, "FKTSZUKAJ", F ) )
      return F;

   for( DbGoBottom(); !Bof() && !Eof(); DbSkip( -1 ) )
   {
      StringFormat( cJest, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY_KORYGOWANEJ" ) ) );
      if( strncmp( FieldGetC( FieldPos( "RODZAJ_FAKTURY" ) ), "KOR ", 4 ) == 0 && strcmp( AllTrim( cJest ), cNumer ) == 0 )
      {
         aOstatnia->nRok = nRok;
         FakturyKlucz( aOstatnia->cKlucz, sizeof( aOstatnia->cKlucz ) );
         StringFormat( aOstatnia->cNumer, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY" ) ) );
         AllTrim( aOstatnia->cNumer );
         lJest = T;
         break;
      }
   }
   DbCloseArea();

   return lJest;
}

static xBool PozycjeZrodla( Biuro *biuro, int nRok, const char *cKlucz, DaneKorekty *hKor )
{
   PozycjaPrzed *aPoz;
   char          cKatalog[ 96 ];
   char          cBaza[ 112 ];
   char          cJest[ 16 ];

   StringFormat( cBaza, "%sfktrow", FakturyKatalog( biuro, nRok, cKatalog, sizeof( cKatalog ) ) );
   if( !NetUse( cBaza, F, NET_WAIT, "FKTROW_ZRODLO", F ) )
      return F;

   for( DbGoTop(); !Eof(); DbSkip( 1 ) )
   {
      if( strcmp( FakturyKlucz( cJest, sizeof( cJest ) ), cKlucz ) != 0 || FieldGetL( FieldPos( "STAN_PRZED" ) ) )
         continue;

      hKor->aPozycje = ( PozycjaPrzed * )realloc( hKor->aPozycje, ( size_t )( hKor->nPozycji + 1 ) * sizeof( PozycjaPrzed ) );
      aPoz           = &hKor->aPozycje[ hKor->nPozycji++ ];
      StringFormat( aPoz->INDEKS, "%s", FieldGetC( FieldPos( "INDEKS" ) ) );
      StringFormat( aPoz->NAZWA_TOWARU, "%s", FieldGetC( FieldPos( "NAZWA_TOWARU" ) ) );
      StringFormat( aPoz->MIARA, "%s", FieldGetC( FieldPos( "MIARA" ) ) );
      StringFormat( aPoz->STAWKA_PODATKU, "%s", FieldGetC( FieldPos( "STAWKA_PODATKU" ) ) );
      RTrim( aPoz->NAZWA_TOWARU );
      RTrim( aPoz->MIARA );
      AllTrim( aPoz->STAWKA_PODATKU );
      aPoz->ILOSC                  = FieldGetN( FieldPos( "ILOSC" ) );
      aPoz->CENA_JEDNOSTKOWA_NETTO = FieldGetN( FieldPos( "CENA_JEDNOSTKOWA_NETTO" ) );
      aPoz->WARTOSC_NETTO          = FieldGetN( FieldPos( "WARTOSC_NETTO" ) );
      aPoz->KWOTA_PODATKU          = FieldGetN( FieldPos( "KWOTA_PODATKU" ) );
      aPoz->WARTOSC_BRUTTO         = FieldGetN( FieldPos( "WARTOSC_BRUTTO" ) );
   }
   DbCloseArea();

   return T;
}

static xBool RokBiezacy( Biuro *biuro, int dWyst )
{
   char cTekst[ 300 ];
   char cData[ 16 ];

   if( dWyst / 10000 != biuro->rok )
   {
      StringFormat( cTekst, "Korekta ma datę wystawienia %s,\na program pracuje na roku %d.\nUruchom xBiuro dla roku %d, aby wystawić korektę.", DToC( dWyst, cData, sizeof( cData ) ), biuro->rok, dWyst / 10000 );
      MsgInfo( cTekst, "Faktura korygująca" );

      return F;
   }

   return T;
}

static xBool Powod( OknoKorekty *oDialog, char *cPowod, int nRozmiar )
{
   Win     *oPytanie;
   Control *GetList[ 1 ];
   int      nKlawisz;

   *cPowod = 0;
   oPytanie = DialogCreate( oDialog->pozycje.okno, "Faktura korygująca", 560, 110 );
   WinSetBackground( oPytanie, 0xE9EDF1 );
   Opis( oPytanie, 20, 12, "Przyczyna korekty, np. zwrot towaru, błędna cena:" );
   GetList[ 0 ] = GetText( oPytanie, 20, 40, 56, cPowod, nRozmiar, "" );

   WinCenter( oPytanie );
   WinShow( oPytanie );
   PozycjeTrybEdycji( oDialog->pozycje.pasekStanu, T );
   WinReadModal( oPytanie, GetList, 1, 1 );
   PozycjeTrybEdycji( oDialog->pozycje.pasekStanu, F );
   nKlawisz = ReadLastKey();
   WinClose( oPytanie );

   AllTrim( cPowod );

   return IIF( nKlawisz != K_ESC, T, F );
}

static void Glowa( Faktura *hGlowa )
{
   hGlowa->DATA_DOSTAWY     = FieldGetD( FieldPos( "DATA_DOSTAWY" ) );
   hGlowa->FORMA_PLATNOSCI  = ( int )FieldGetN( FieldPos( "FORMA_PLATNOSCI" ) );
   hGlowa->TERMIN_PLATNOSCI = FieldGetD( FieldPos( "TERMIN_PLATNOSCI" ) );
   StringFormat( hGlowa->KOD_KRAJU_NABYWCY, "%s", FieldGetC( FieldPos( "KOD_KRAJU_NABYWCY" ) ) );
   StringFormat( hGlowa->NIP_NABYWCY, "%s", FieldGetC( FieldPos( "NIP_NABYWCY" ) ) );
   StringFormat( hGlowa->NAZWA_NABYWCY, "%s", FieldGetC( FieldPos( "NAZWA_NABYWCY" ) ) );
   StringFormat( hGlowa->ULICA_NABYWCY, "%s", FieldGetC( FieldPos( "ULICA_NABYWCY" ) ) );
   StringFormat( hGlowa->KOD_POCZTOWY_NABYWCY, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY_NABYWCY" ) ) );
   StringFormat( hGlowa->MIEJSCOWOSC_NABYWCY, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC_NABYWCY" ) ) );
   StringFormat( hGlowa->NAZWA_BANKU, "%s", FieldGetC( FieldPos( "NAZWA_BANKU" ) ) );
   StringFormat( hGlowa->NUMER_RACHUNKU, "%s", FieldGetC( FieldPos( "NUMER_RACHUNKU" ) ) );
   StringFormat( hGlowa->PODSTAWA_ZWOLNIENIA, "%s", FieldGetC( FieldPos( "PODSTAWA_ZWOLNIENIA" ) ) );
   RTrim( hGlowa->NAZWA_NABYWCY );
   AllTrim( hGlowa->PODSTAWA_ZWOLNIENIA );
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
