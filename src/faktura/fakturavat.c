/*
 * Program...: xBiuro
 * Plik......: src/faktura/fakturavat.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "faktura.h"

typedef struct
{
   OknoPozycji pozycje;
   Biuro      *biuro;
   Faktura     hFkt;
   Win        *panel;
   int         aZamroz[ 5 ];
   char        cTemp[ 400 ];
   char        cNazwa[ 2100 ];
   Control    *NUMER_FAKTURY;
   Control    *DATA_WYSTAWIENIA;
   Control    *DATA_DOSTAWY;
   Control    *TERMIN_PLATNOSCI;
   Control    *PLAT_1;
   Control    *PLAT_2;
   Control    *PLAT_6;
   Control    *KOD_KRAJU_NABYWCY;
   Control    *NIP_NABYWCY;
   Control    *NAZWA_NABYWCY;
   Control    *ULICA_NABYWCY;
   Control    *KOD_POCZTOWY_NABYWCY;
   Control    *MIEJSCOWOSC_NABYWCY;
   Control    *SUMA;
   Control    *oTermin;
} OknoFaktury;

static xBool       Tymczasowa( OknoFaktury *oDialog );
static void        Stan( OknoFaktury *oDialog );
static char       *Numer( Biuro *biuro, int nNr, char *cNumer, int nRozmiar );
static char       *Data( int dData, char *cWynik, int nRozmiar );
static void        Okno( OknoFaktury *oDialog );
static void        Odswiez( void *data );
static void        Zmiana( Control *browse, void *data );
static const char *Kolumna( Control *browse, void *data );
static const char *KolumnaNazwa( Control *browse, void *data );
static double      KolumnaLiczba( Control *browse, void *data );
static void        Pomoc( Toolbar *toolbar, void *data );
static void        Pozycja( OknoFaktury *oDialog, xBool lNowa );
static void        Dodaj( Toolbar *toolbar, void *data );
static void        Wstaw( Win *win, void *data );
static void        Popraw( Toolbar *toolbar, void *data );
static void        Usun( Toolbar *toolbar, void *data );
static void        Naglowek( Toolbar *toolbar, void *data );
static void        Platnosc( Control *control, void *data );
static xBool       Poprawna( OknoFaktury *oDialog );
static void        Zapisz( Toolbar *toolbar, void *data );
static void        Nabywca( Toolbar *toolbar, void *data );
static void        Anuluj( Toolbar *toolbar, void *data );
static void        Opis( Win *strona, int x, int y, const char *tekst );
static Control    *Pole( Win *strona, int x, int y, int dlugosc );

void FakturaVat( Win *win, Biuro *biuro )
{
   OknoFaktury  oknoFaktury = { 0 };
   OknoFaktury *oDialog     = &oknoFaktury;
   Faktura     *hFkt        = &oDialog->hFkt;
   OknoPozycji *oPozycje    = &oDialog->pozycje;
   Win         *panel;
   Control     *BRW;
   xBool        lDrugi;
   char         cPlik[ 420 ];

   oDialog->biuro = biuro;
   if( !FirmaPobierzDane( biuro, "SPRZEDAWCA" ) )
      return;

   StringFormat( hFkt->PODSTAWA_ZWOLNIENIA, "%s", FieldGetC( FieldPos( "PODSTAWA_ZWOLNIENIA" ) ) );
   AllTrim( hFkt->PODSTAWA_ZWOLNIENIA );
   if( FieldGetL( FieldPos( "DRUKUJ_RACHUNEK" ) ) )
   {
      lDrugi = IIF( FieldGetN( FieldPos( "DRUKUJ_RACHUNEK_NR" ) ) == 2, T, F );
      StringFormat( hFkt->NAZWA_BANKU, "%s", FieldGetC( FieldPos( IIF( lDrugi, "NAZWA_BANKU_2", "NAZWA_BANKU_1" ) ) ) );
      StringFormat( hFkt->NUMER_RACHUNKU, "%s", FieldGetC( FieldPos( IIF( lDrugi, "NUMER_RACHUNKU_2", "NUMER_RACHUNKU_1" ) ) ) );
      AllTrim( hFkt->NAZWA_BANKU );
      AllTrim( hFkt->NUMER_RACHUNKU );
   }
   DbCloseArea();

   if( !Tymczasowa( oDialog ) )
      return;

   Numer( biuro, FakturyNastepnyNumer( "VAT" ), hFkt->NUMER_FAKTURY, sizeof( hFkt->NUMER_FAKTURY ) );
   hFkt->DATA_WYSTAWIENIA = Date();
   hFkt->DATA_DOSTAWY     = Date();
   hFkt->FORMA_PLATNOSCI  = PLATNOSC_GOTOWKA;
   hFkt->TERMIN_PLATNOSCI = Date();
   StringFormat( hFkt->KOD_KRAJU_NABYWCY, "PL" );

   oPozycje->okno    = DialogCreate( win, "Wystaw fakturę VAT", 1100, 700 );
   oPozycje->odswiez = Odswiez;
   oPozycje->data    = oDialog;
   WinSetBackground( oPozycje->okno, 0xE9EDF1 );
   WinSetSizable( oPozycje->okno, T );
   WinSetKey( oPozycje->okno, "Ins", Wstaw, oDialog );

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
   oDialog->aZamroz[ 0 ] = oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "F3-Nagłówek", Naglowek, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "F3" );
   ToolbarAddSeparator( oPozycje->pasek );
   oDialog->aZamroz[ 1 ] = oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "F4-Popraw", Popraw, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "F4" );
   ToolbarAddSeparator( oPozycje->pasek );
   oDialog->aZamroz[ 2 ] = oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "F8-Usuń", Usun, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "F8" );
   ToolbarAddSeparator( oPozycje->pasek );
   oDialog->aZamroz[ 3 ] = oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "F10-Zapisz", Zapisz, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "F10" );
   ToolbarAddSeparator( oPozycje->pasek );
   oDialog->aZamroz[ 4 ] = oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "F12-Nabywca", Nabywca, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "F12" );
   ToolbarAddSeparator( oPozycje->pasek );
   oPozycje->aPrzyciski[ oPozycje->nPrzyciskow ] = ToolbarAdd( oPozycje->pasek, "Esc-Anuluj", Anuluj, oDialog );
   ToolbarSetShortcut( oPozycje->pasek, oPozycje->aPrzyciski[ oPozycje->nPrzyciskow++ ], "Esc" );

   panel          = PanelCreate( oPozycje->okno, 0, 0, 0, 0 );
   oDialog->panel = panel;

   Opis( panel, 20,   7, "Numer faktury:" );
   Opis( panel, 20,  37, "Data wystawienia:" );
   Opis( panel, 20,  67, "Data sprzedaży:" );
   Opis( panel, 20,  97, "Płatność:" );
   Opis( panel, 20, 127, "Data płatności:" );
   oDialog->NUMER_FAKTURY    = Pole( panel, 190,   4, 14 );
   oDialog->DATA_WYSTAWIENIA = Pole( panel, 190,  34, 10 );
   oDialog->DATA_DOSTAWY     = Pole( panel, 190,  64, 10 );
   oDialog->TERMIN_PLATNOSCI = Pole( panel, 190, 124, 10 );

   oDialog->PLAT_1 = RadioButtonCreate( panel, 190, 96, 85, 20, "Gotówka" );
   oDialog->PLAT_2 = RadioButtonCreate( panel, 280, 96, 75, 20, "Karta" );
   oDialog->PLAT_6 = RadioButtonCreate( panel, 360, 96, 90, 20, "Przelew" );

   WinSetBackground( PanelCreate( panel, 460, 4, 1, 144 ), 0xA0A0A0 );

   Opis( panel, 475,  7, "Kod kraju / NIP:" );
   Opis( panel, 475, 37, "Nazwa kontrahenta:" );
   Opis( panel, 475, 67, "Ulica:" );
   Opis( panel, 475, 97, "Kod / Miejscowość:" );
   oDialog->KOD_KRAJU_NABYWCY    = Pole( panel, 655,  4,  2 );
   oDialog->NIP_NABYWCY          = Pole( panel, 691,  4, 20 );
   oDialog->NAZWA_NABYWCY        = Pole( panel, 655, 34, 40 );
   oDialog->ULICA_NABYWCY        = Pole( panel, 655, 64, 40 );
   oDialog->KOD_POCZTOWY_NABYWCY = Pole( panel, 655, 94,  6 );
   oDialog->MIEJSCOWOSC_NABYWCY  = Pole( panel, 727, 94, 32 );

   BRW           = BrowseCreate( panel, 10, 158, WinWidth( panel ) - 20, WinHeight( panel ) - 202 );
   oPozycje->BRW = BRW;
   ControlSetAnchor( BRW, "LTRB" );
   BrowseSetColor( BRW, BROWSE_HEADBACKGROUND, 0x4D7326 );
   DbBrowse( BRW, "FKTTEMP" );
   BrowseAddNumber( BRW, "Lp", 4, "9999", KolumnaLiczba, "NUMER_WIERSZA" );
   BrowseAddColumn( BRW, "Nazwa towaru / usługi", 36, BROWSE_LEFT, KolumnaNazwa, oDialog );
   BrowseAddNumber( BRW, "Ilość", 12, "99999999.999", KolumnaLiczba, "ILOSC" );
   BrowseAddColumn( BRW, "J.m.", 5, BROWSE_LEFT, Kolumna, "MIARA" );
   BrowseAddNumber( BRW, "Cena netto", 12, "999999999.99", KolumnaLiczba, "CENA_JEDNOSTKOWA_NETTO" );
   BrowseAddNumber( BRW, "Netto", 12, "999999999.99", KolumnaLiczba, "WARTOSC_NETTO" );
   BrowseAddColumn( BRW, "VAT", 3, BROWSE_LEFT, Kolumna, "STAWKA_PODATKU" );
   BrowseAddNumber( BRW, "Kwota VAT", 12, "999999999.99", KolumnaLiczba, "KWOTA_PODATKU" );
   BrowseAddNumber( BRW, "Brutto", 12, "999999999.99", KolumnaLiczba, "WARTOSC_BRUTTO" );
   ControlSetAction( BRW, Zmiana, oDialog );

   oDialog->SUMA = Pole( panel, 10, WinHeight( panel ) - 36, 100 );
   ControlSetAnchor( oDialog->SUMA, "LB" );

   ControlSetEnabled( oDialog->PLAT_1, F );
   ControlSetEnabled( oDialog->PLAT_2, F );
   ControlSetEnabled( oDialog->PLAT_6, F );
   Okno( oDialog );
   Stan( oDialog );

   WinCenter( oPozycje->okno );
   DialogShowModal( oPozycje->okno );

   DbSelectArea( "FKTTEMP" );
   DbCloseArea();
   StringFormat( cPlik, "%s.dbf", oDialog->cTemp );
   FErase( cPlik );
   StringFormat( cPlik, "%s.dbt", oDialog->cTemp );
   FErase( cPlik );
   DbSelectArea( "FKTHEAD" );
}

static xBool Tymczasowa( OknoFaktury *oDialog )
{
   const DbField *aWzor;
   char           cPlik[ 420 ];
   int            nPol;

   FakturyTymczasowa( "fkttemp", oDialog->cTemp, sizeof( oDialog->cTemp ) );
   StringFormat( cPlik, "%s.dbf", oDialog->cTemp );
   FErase( cPlik );
   StringFormat( cPlik, "%s.dbt", oDialog->cTemp );
   FErase( cPlik );
   aWzor = PozycjeStruct( &nPol );
   DbCreate( oDialog->cTemp, aWzor, nPol );

   return NetUse( oDialog->cTemp, T, NET_WAIT, "FKTTEMP", F );
}

static void Stan( OknoFaktury *oDialog )
{
   OknoPozycji *oPozycje = &oDialog->pozycje;
   xBool        lJest;

   DbSelectArea( "FKTTEMP" );
   lJest = IIF( LastRec() > 0, T, F );
   for( int i = 0; i < 5; ++i )
   {
      ToolbarEnable( oPozycje->pasek, oDialog->aZamroz[ i ], lJest );
   }
   ControlSetEnabled( oPozycje->BRW, lJest );
   BrowseRefresh( oPozycje->BRW );

   if( lJest )
      ControlSetFocus( oPozycje->BRW );
}

static char *Numer( Biuro *biuro, int nNr, char *cNumer, int nRozmiar )
{
   StringFormatSize( cNumer, ( size_t )nRozmiar, "FV/%d/%d", nNr, biuro->rok );

   return cNumer;
}

static char *Data( int dData, char *cWynik, int nRozmiar )
{
   if( dData == 0 )
      *cWynik = 0;
   else
      DToC( dData, cWynik, ( size_t )nRozmiar );

   return cWynik;
}

static void Okno( OknoFaktury *oDialog )
{
   Faktura *hFkt = &oDialog->hFkt;
   double   nNetto;
   double   nVat;
   xBool    lZw;
   char     cTekst[ 2100 ];
   char     cJednaLinia[ 2100 ];

   ControlSetText( oDialog->NUMER_FAKTURY, hFkt->NUMER_FAKTURY );
   ControlSetText( oDialog->DATA_WYSTAWIENIA, Data( hFkt->DATA_WYSTAWIENIA, cTekst, sizeof( cTekst ) ) );
   ControlSetText( oDialog->DATA_DOSTAWY, Data( hFkt->DATA_DOSTAWY, cTekst, sizeof( cTekst ) ) );
   ControlSetText( oDialog->TERMIN_PLATNOSCI, Data( hFkt->TERMIN_PLATNOSCI, cTekst, sizeof( cTekst ) ) );
   CheckBoxSetChecked( oDialog->PLAT_1, IIF( hFkt->FORMA_PLATNOSCI == PLATNOSC_GOTOWKA, T, F ) );
   CheckBoxSetChecked( oDialog->PLAT_2, IIF( hFkt->FORMA_PLATNOSCI == PLATNOSC_KARTA, T, F ) );
   CheckBoxSetChecked( oDialog->PLAT_6, IIF( hFkt->FORMA_PLATNOSCI == PLATNOSC_PRZELEW, T, F ) );
   ControlSetText( oDialog->KOD_KRAJU_NABYWCY, hFkt->KOD_KRAJU_NABYWCY );
   ControlSetText( oDialog->NIP_NABYWCY, hFkt->NIP_NABYWCY );
   StrTran( cTekst, sizeof( cTekst ), hFkt->NAZWA_NABYWCY, "\r\n", " " );
   ControlSetText( oDialog->NAZWA_NABYWCY, StrTran( cJednaLinia, sizeof( cJednaLinia ), cTekst, "\n", " " ) );
   ControlSetText( oDialog->ULICA_NABYWCY, hFkt->ULICA_NABYWCY );
   ControlSetText( oDialog->KOD_POCZTOWY_NABYWCY, hFkt->KOD_POCZTOWY_NABYWCY );
   ControlSetText( oDialog->MIEJSCOWOSC_NABYWCY, hFkt->MIEJSCOWOSC_NABYWCY );

   PozycjeSuma( "FKTTEMP", &nNetto, &nVat, &lZw );
   StringFormat( cTekst, " Pozycji: %ld     Netto: %.2f     VAT: %.2f     Brutto: %.2f", LastRec(), nNetto, nVat, nNetto + nVat );
   ControlSetText( oDialog->SUMA, cTekst );
}

static void Odswiez( void *data )
{
   Okno( ( OknoFaktury * )data );
}

static void Zmiana( Control *browse, void *data )
{
   UNUSED( browse );

   Okno( ( OknoFaktury * )data );
}

static const char *Kolumna( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "FKTTEMP" );

   return FieldGetC( FieldPos( ( const char * )data ) );
}

static const char *KolumnaNazwa( Control *browse, void *data )
{
   OknoFaktury *oDialog = ( OknoFaktury * )data;
   char         cTekst[ 2100 ];

   UNUSED( browse );

   DbSelectArea( "FKTTEMP" );
   StrTran( cTekst, sizeof( cTekst ), FieldGetC( FieldPos( "NAZWA_TOWARU" ) ), "\r\n", " " );

   return StrTran( oDialog->cNazwa, sizeof( oDialog->cNazwa ), cTekst, "\n", " " );
}

static double KolumnaLiczba( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "FKTTEMP" );

   return FieldGetN( FieldPos( ( const char * )data ) );
}

static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgHelp( "<a href=\"" USTAWA_VAT "\">Ustawa o podatku od towarów i usług</a> (art. 106e – dane na fakturze)\n\n"
            "Pozycje\n\n"
            "F2, Ins\tnowa pozycja w okienku Pozycje na fakturze\n"
            "F4\tpoprawienie pozycji w tym samym okienku\n"
            "F8\tusunięcie pozycji\n"
            "Strzałki\tpoprzednia / następna pozycja\n\n"
            "Faktura\n\n"
            "F3\tdaty, płatność i dane nabywcy; przy gotówce i karcie data płatności jest zamrożona i równa dacie wystawienia\n"
            "F12\tnabywca z kartoteki kontrahentów\n"
            "F10\tzapis faktury, numer FV/kolejny/rok nadaje program\n"
            "Esc\trezygnacja z faktury\n\n"
            "Wpisywanie danych\n\n"
            "Enter\tnastępne pole, na ostatnim zapis\n"
            "PgUp, PgDn\tzapis\n"
            "Esc\tbez zapisu\n\n"
            "Stawki VAT: 23, 22, 8, 7, 5, 4, 3, 0 (krajowe), zw (zwolnione), oo (odwrotne obciążenie), np (nie podlega).\n"
            "Netto pozycji = ilość x cena netto, VAT liczony od netto pozycji.", "Wystaw fakturę VAT" );
}

static void Pozycja( OknoFaktury *oDialog, xBool lNowa )
{
   PozycjeEdytuj( &oDialog->pozycje, "FKTTEMP", lNowa, "Pozycje na fakturze", "Faktura", F );
   Stan( oDialog );
}

static void Dodaj( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   Pozycja( ( OknoFaktury * )data, T );
}

static void Wstaw( Win *win, void *data )
{
   UNUSED( win );

   Pozycja( ( OknoFaktury * )data, T );
}

static void Popraw( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   Pozycja( ( OknoFaktury * )data, F );
}

static void Usun( Toolbar *toolbar, void *data )
{
   OknoFaktury *oDialog = ( OknoFaktury * )data;

   UNUSED( toolbar );

   PozycjeUsun( &oDialog->pozycje, "FKTTEMP", "Faktura" );
   Stan( oDialog );
}

static void Naglowek( Toolbar *toolbar, void *data )
{
   OknoFaktury *oDialog  = ( OknoFaktury * )data;
   Faktura     *hFkt     = &oDialog->hFkt;
   Win         *panel    = oDialog->panel;
   Control     *aWybor[] = { oDialog->PLAT_1, oDialog->PLAT_2, oDialog->PLAT_6 };
   Control     *GetList[ 12 ];
   int          nGet     = 0;
   int          nKlawisz;
   int          dWyst    = hFkt->DATA_WYSTAWIENIA;
   int          dSprz    = hFkt->DATA_DOSTAWY;
   int          dTermin  = hFkt->TERMIN_PLATNOSCI;
   char         cKraj[ 3 ];
   char         cNip[ 51 ];
   char         cNazwa[ 2049 ];
   char         cUlica[ 255 ];
   char         cKod[ 9 ];
   char         cMiasto[ 225 ];
   char         cTekst[ 16 ];

   UNUSED( toolbar );

   StringFormat( cKraj, "%-2s", hFkt->KOD_KRAJU_NABYWCY );
   StringFormat( cNip, "%s", hFkt->NIP_NABYWCY );
   StringFormat( cNazwa, "%s", hFkt->NAZWA_NABYWCY );
   StringFormat( cUlica, "%s", hFkt->ULICA_NABYWCY );
   StringFormat( cKod, "%s", hFkt->KOD_POCZTOWY_NABYWCY );
   StringFormat( cMiasto, "%s", hFkt->MIEJSCOWOSC_NABYWCY );

   GetList[ nGet++ ] = GetDate( panel, 190, 34, &dWyst );
   GetList[ nGet++ ] = GetDate( panel, 190, 64, &dSprz );
   GetList[ nGet++ ] = oDialog->PLAT_1;
   GetList[ nGet++ ] = oDialog->PLAT_2;
   GetList[ nGet++ ] = oDialog->PLAT_6;
   GetList[ nGet++ ] = oDialog->oTermin = GetDate( panel, 190, 124, &dTermin );
   GetList[ nGet++ ] = GetText( panel, 655, 4, 2, cKraj, sizeof( cKraj ), "@! AA" );
   GetList[ nGet++ ] = GetText( panel, 691, 4, 20, cNip, sizeof( cNip ), "" );
   GetList[ nGet++ ] = GetMemo( panel, 655, 34, 40, 4, cNazwa, sizeof( cNazwa ) );
   GetList[ nGet++ ] = GetText( panel, 655, 64, 40, cUlica, sizeof( cUlica ), "" );
   GetList[ nGet++ ] = GetText( panel, 655, 94, 6, cKod, sizeof( cKod ), "99-999" );
   GetList[ nGet++ ] = GetText( panel, 727, 94, 32, cMiasto, sizeof( cMiasto ), "" );
   for( int i = 0; i < 3; ++i )
   {
      ControlSetEnabled( aWybor[ i ], T );
      ControlSetAction( aWybor[ i ], Platnosc, oDialog );
   }
   ControlSetEnabled( oDialog->oTermin, CheckBoxChecked( oDialog->PLAT_6 ) );

   PozycjeTrybEdycji( oDialog->pozycje.pasekStanu, T );
   WinReadModal( oDialog->pozycje.okno, GetList, nGet, 1 );
   PozycjeTrybEdycji( oDialog->pozycje.pasekStanu, F );
   nKlawisz = ReadLastKey();

   for( int i = 0; i < 3; ++i )
   {
      ControlSetEnabled( aWybor[ i ], F );
      ControlSetAction( aWybor[ i ], NULL, NULL );
   }
   for( int i = 0; i < nGet; ++i )
   {
      if( GetList[ i ] != aWybor[ 0 ] && GetList[ i ] != aWybor[ 1 ] && GetList[ i ] != aWybor[ 2 ] )
         ControlDestroy( GetList[ i ] );
   }

   if( nKlawisz != K_ESC )
   {
      hFkt->DATA_WYSTAWIENIA = dWyst;
      hFkt->DATA_DOSTAWY     = dSprz;
      hFkt->FORMA_PLATNOSCI  = IIF( CheckBoxChecked( oDialog->PLAT_1 ), PLATNOSC_GOTOWKA, IIF( CheckBoxChecked( oDialog->PLAT_2 ), PLATNOSC_KARTA, PLATNOSC_PRZELEW ) );
      hFkt->TERMIN_PLATNOSCI = IIF( hFkt->FORMA_PLATNOSCI == PLATNOSC_PRZELEW, dTermin, dWyst );
      StringFormat( hFkt->KOD_KRAJU_NABYWCY, "%s", cKraj );
      StringFormat( hFkt->NIP_NABYWCY, "%s", cNip );
      StringFormat( hFkt->NAZWA_NABYWCY, "%s", AllTrim( cNazwa ) );
      StringFormat( hFkt->ULICA_NABYWCY, "%s", cUlica );
      StringFormat( hFkt->KOD_POCZTOWY_NABYWCY, "%s", IIF( *AllTrim( StrTran( cTekst, sizeof( cTekst ), cKod, "-", "" ) ) == 0, "", cKod ) );
      StringFormat( hFkt->MIEJSCOWOSC_NABYWCY, "%s", cMiasto );
   }

   Okno( oDialog );
}

static void Platnosc( Control *control, void *data )
{
   OknoFaktury *oDialog = ( OknoFaktury * )data;

   UNUSED( control );

   ControlSetEnabled( oDialog->oTermin, CheckBoxChecked( oDialog->PLAT_6 ) );
}

static xBool Poprawna( OknoFaktury *oDialog )
{
   Faktura *hFkt = &oDialog->hFkt;
   char     cTekst[ 200 ];
   char     cNip[ 51 ];

   if( *hFkt->NAZWA_NABYWCY == 0 )
   {
      MsgInfo( "Brak nabywcy: F12 z kartoteki albo F3 ręcznie.", "Faktura" );

      return F;
   }

   DbSelectArea( "FKTTEMP" );
   if( LastRec() == 0 )
   {
      MsgInfo( "Faktura nie ma pozycji: F2 dodaje pozycję.", "Faktura" );

      return F;
   }

   if( hFkt->DATA_WYSTAWIENIA == 0 || hFkt->DATA_WYSTAWIENIA / 10000 != oDialog->biuro->rok )
   {
      StringFormat( cTekst, "Data wystawienia musi być z roku %d.\nPopraw ją klawiszem F3.", oDialog->biuro->rok );
      MsgInfo( cTekst, "Faktura" );

      return F;
   }

   if( hFkt->DATA_WYSTAWIENIA > Date() )
   {
      MsgInfo( "Data wystawienia nie może być późniejsza niż dzisiaj,\nbo KSeF odrzuci fakturę. Popraw ją klawiszem F3.", "Faktura" );

      return F;
   }

   if( hFkt->FORMA_PLATNOSCI == PLATNOSC_PRZELEW && hFkt->TERMIN_PLATNOSCI < hFkt->DATA_WYSTAWIENIA )
   {
      MsgInfo( "Przy przelewie termin płatności nie może być wcześniejszy\nniż data wystawienia. Popraw go klawiszem F3.", "Faktura" );

      return F;
   }

   StringFormat( cNip, "%s", hFkt->NIP_NABYWCY );
   if( strcmp( hFkt->KOD_KRAJU_NABYWCY, "PL" ) == 0 && *AllTrim( cNip ) != 0 && !FirmaNipPoprawny( cNip ) )
   {
      StringFormat( cTekst, "Niepoprawny NIP nabywcy: %s\nPopraw go klawiszem F3.", cNip );
      MsgInfo( cTekst, "Faktura" );

      return F;
   }

   return T;
}

static void Zapisz( Toolbar *toolbar, void *data )
{
   OknoFaktury *oDialog = ( OknoFaktury * )data;
   Faktura     *hFkt    = &oDialog->hFkt;
   long        *aDodane;
   int          nDodane = 0;
   int          nNr;
   double       nNetto;
   double       nVat;
   xBool        lZw;
   char         cTekst[ 400 ];

   UNUSED( toolbar );

   if( !Poprawna( oDialog ) )
      return;

   if( !PozycjeSuma( "FKTTEMP", &nNetto, &nVat, &lZw ) )
   {
      MsgInfo( "Wartość faktury musi być mniejsza niż 1 000 000 000.", "Faktura" );

      return;
   }

   if( lZw && *hFkt->PODSTAWA_ZWOLNIENIA == 0 )
   {
      MsgInfo( "Pozycja ze stawką zw wymaga podstawy prawnej zwolnienia.\nWpisz ją w menu Firma, Dane firmy, zakładka VAT.", "Faktura" );

      return;
   }

   DbSelectArea( "FKTHEAD" );
   if( !FLock() )
   {
      MsgInfo( "Inne stanowisko zapisuje teraz fakturę. Spróbuj za chwilę.", "Faktura" );

      return;
   }
   nNr = FakturyNastepnyNumer( "VAT" );

   DbSelectArea( "FKTTEMP" );
   aDodane = ( long * )calloc( ( size_t )LastRec() + 1, sizeof( long ) );
   if( !PozycjeZapisz( "FKTTEMP", nNr, "VAT", 0, F, aDodane, &nDodane ) )
   {
      PozycjeWycofaj( aDodane, nDodane );
      DbSelectArea( "FKTHEAD" );
      DbUnlock();
      free( aDodane );
      MsgError( "Nie udało się zapisać pozycji. Faktura nie została zapisana.", "Faktura" );

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
      MsgError( "Nie udało się dopisać nagłówka faktury. Faktura nie została zapisana.", "Faktura" );

      return;
   }
   free( aDodane );

   Numer( oDialog->biuro, nNr, hFkt->NUMER_FAKTURY, sizeof( hFkt->NUMER_FAKTURY ) );
   FieldPutN( FieldPos( "KOLEJNY_NUMER" ), nNr );
   FieldPutC( FieldPos( "RODZAJ_FAKTURY" ), "VAT" );
   FieldPutC( FieldPos( "NUMER_FAKTURY" ), hFkt->NUMER_FAKTURY );
   FieldPutD( FieldPos( "DATA_WYSTAWIENIA" ), hFkt->DATA_WYSTAWIENIA );
   FieldPutD( FieldPos( "DATA_DOSTAWY" ), hFkt->DATA_DOSTAWY );
   FieldPutC( FieldPos( "KOD_KRAJU_NABYWCY" ), hFkt->KOD_KRAJU_NABYWCY );
   FieldPutC( FieldPos( "NIP_NABYWCY" ), hFkt->NIP_NABYWCY );
   FieldPutC( FieldPos( "NAZWA_NABYWCY" ), hFkt->NAZWA_NABYWCY );
   FieldPutC( FieldPos( "ULICA_NABYWCY" ), hFkt->ULICA_NABYWCY );
   FieldPutC( FieldPos( "KOD_POCZTOWY_NABYWCY" ), hFkt->KOD_POCZTOWY_NABYWCY );
   FieldPutC( FieldPos( "MIEJSCOWOSC_NABYWCY" ), hFkt->MIEJSCOWOSC_NABYWCY );
   FieldPutN( FieldPos( "FORMA_PLATNOSCI" ), hFkt->FORMA_PLATNOSCI );
   FieldPutD( FieldPos( "TERMIN_PLATNOSCI" ), hFkt->TERMIN_PLATNOSCI );
   FieldPutC( FieldPos( "NAZWA_BANKU" ), hFkt->NAZWA_BANKU );
   FieldPutC( FieldPos( "NUMER_RACHUNKU" ), hFkt->NUMER_RACHUNKU );
   FieldPutC( FieldPos( "PODSTAWA_ZWOLNIENIA" ), IIF( lZw, hFkt->PODSTAWA_ZWOLNIENIA, "" ) );
   FieldPutN( FieldPos( "WARTOSC_NETTO" ), nNetto );
   FieldPutN( FieldPos( "KWOTA_PODATKU" ), nVat );
   FieldPutN( FieldPos( "KWOTA_NALEZNOSCI" ), nNetto + nVat );
   FieldPutD( FieldPos( "DATA_ZAPISU" ), Date() );
   DbCommit();
   DbUnlock();
   DbSelectArea( "FKTROW" );
   DbUnlock();

   StringFormat( cTekst, "Zapisano fakturę %s\nBrutto: %.2f zł", hFkt->NUMER_FAKTURY, nNetto + nVat );
   MsgInfo( cTekst, "Faktura" );
   WinClose( oDialog->pozycje.okno );
}

static void Nabywca( Toolbar *toolbar, void *data )
{
   OknoFaktury *oDialog = ( OknoFaktury * )data;
   Faktura     *hFkt    = &oDialog->hFkt;
   long         nRecNo;
   char         cUlica[ 255 ];
   char         cDom[ 37 ];
   char         cLokal[ 41 ];

   UNUSED( toolbar );

   nRecNo = KontrahentWybierz( oDialog->pozycje.okno, oDialog->biuro );
   if( nRecNo > 0 && NetUse( DANE_BAZA "kartoteka/kontrah", F, NET_WAIT, "NABYWCA", T ) )
   {
      DbGoto( nRecNo );
      StringFormat( hFkt->KOD_KRAJU_NABYWCY, "%s", FieldGetC( FieldPos( "KOD_KRAJU" ) ) );
      StringFormat( hFkt->NIP_NABYWCY, "%s", FieldGetC( FieldPos( "NIP" ) ) );
      StringFormat( hFkt->NAZWA_NABYWCY, "%s", FieldGetC( FieldPos( "PELNA_NAZWA" ) ) );
      StringFormat( cUlica, "%s", FieldGetC( FieldPos( "ULICA" ) ) );
      StringFormat( cDom, "%s", FieldGetC( FieldPos( "NR_DOMU" ) ) );
      StringFormat( cLokal, "%s", FieldGetC( FieldPos( "NR_LOKALU" ) ) );
      StringFormat( hFkt->KOD_POCZTOWY_NABYWCY, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY" ) ) );
      StringFormat( hFkt->MIEJSCOWOSC_NABYWCY, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
      DbCloseArea();

      AllTrim( cUlica );
      AllTrim( cDom );
      AllTrim( cLokal );
      StringFormat( hFkt->ULICA_NABYWCY, "%s %s%s%s", cUlica, cDom, IIF( *cLokal == 0, "", "/" ), cLokal );
      AllTrim( hFkt->ULICA_NABYWCY );
      AllTrim( hFkt->NIP_NABYWCY );
      AllTrim( hFkt->NAZWA_NABYWCY );
      AllTrim( hFkt->KOD_POCZTOWY_NABYWCY );
      AllTrim( hFkt->MIEJSCOWOSC_NABYWCY );
      Okno( oDialog );
   }
   DbSelectArea( "FKTTEMP" );
}

static void Anuluj( Toolbar *toolbar, void *data )
{
   OknoFaktury *oDialog = ( OknoFaktury * )data;

   UNUSED( toolbar );

   DbSelectArea( "FKTTEMP" );
   if( LastRec() == 0 || MsgYesNo( "Zrezygnować z tej faktury?\nPozycje nie zostaną zapisane.", "Faktura" ) )
      WinClose( oDialog->pozycje.okno );
}

static void Opis( Win *strona, int x, int y, const char *tekst )
{
   SayCreate( strona, x, y, 0, 20, tekst );
}

static Control *Pole( Win *strona, int x, int y, int dlugosc )
{
   Control *pole = SayCreate( strona, x, y, dlugosc, 24, "" );

   LabelSetBorder( pole, T );

   return pole;
}
