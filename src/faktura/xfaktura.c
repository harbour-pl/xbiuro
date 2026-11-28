/*
 * Program...: xBiuro
 * Plik......: src/faktura/xfaktura.c
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "faktura.h"

typedef struct
{
   Biuro *biuro;
   int   *aLata;
   int    nLat;
   int    nMaks;
} ListaLat;

typedef struct
{
   Win  *okno;
   int   aLata[ 64 ];
   int   nLat;
   int   nPoz;
   int   nWybor;
   int   nBiezacy;
   char  cTekst[ 32 ];
} WyborRoku;

static void        RokZnajdz( const char *cNazwa, xBool lKatalog, void *data );
static int         RokPorownaj( const void *pierwszy, const void *drugi );
static void        RokPoczatek( Control *browse, void *data );
static void        RokKoniec( Control *browse, void *data );
static int         RokSkip( Control *browse, int nIle, void *data );
static double      RokPozycja( Control *browse, void *data );
static const char *RokKolumna( Control *browse, void *data );
static void        RokWybrany( Toolbar *toolbar, void *data );
static void        RokRezygnuj( Toolbar *toolbar, void *data );

void ModulOtworz( Biuro *biuro )
{
   Menu *ksef;
   Menu *tokeny;
   Menu *faktury;

   ksef   = MenuInsertMenu( biuro->menu, 4, "KSeF" );
   tokeny = MenuAddMenu( ksef, "Tokeny / Certyfikaty" );
   MenuAdd( tokeny, "Generuj token", GenerujToken, biuro, "" );
   MenuAdd( tokeny, "Lista tokenów", Tokeny, biuro, "" );
   MenuAddSeparator( tokeny );
   MenuAdd( tokeny, "Lista certyfikatów", Certyfikaty, biuro, "" );
   MenuAddSeparator( ksef );
   MenuAdd( ksef, "Środowisko KSeF", SrodowiskoKsef, biuro, "" );

   faktury = MenuInsertMenu( biuro->menu, 5, "Faktury" );
   MenuAdd( faktury, "Faktury VAT", Faktury, biuro, "" );
   MenuAdd( faktury, "Faktury korygujące", FakturyKor, biuro, "" );
   MenuAddSeparator( faktury );
   MenuAddDisabled( faktury, "Faktury RR" );
}

void ModulZamknij( Biuro *biuro )
{
   MenuDelete( biuro->menu, 5 );
   MenuDelete( biuro->menu, 4 );
}

void ModulNtx( Biuro *biuro )
{
   FakturyNtx( biuro );
   TokenyNtx();
   CertyfikatyNtx();
}

int RokLista( Biuro *biuro, int *aLata, int nMaks )
{
   ListaLat lista = { biuro, aLata, 0, nMaks };

   DirScan( "", RokZnajdz, &lista );
   qsort( aLata, ( size_t )lista.nLat, sizeof( int ), RokPorownaj );

   return lista.nLat;
}

int RokWybierz( Win *win, Biuro *biuro, const char *cTytul )
{
   WyborRoku  wyborRoku = { 0 };
   WyborRoku *wybor     = &wyborRoku;
   Toolbar   *oToolbar;
   Win       *oPanel;
   Control   *oBrowse;
   int        przycisk;

   wybor->nLat     = RokLista( biuro, wybor->aLata, 64 );
   wybor->nBiezacy = biuro->rok;
   if( wybor->nLat == 0 )
   {
      MsgInfo( "Brak lat z fakturami.", cTytul );

      return 0;
   }

   wybor->okno = DialogCreate( win, cTytul, 320, 360 );
   WinSetBackground( wybor->okno, 0xE9EDF1 );

   oToolbar = ToolbarCreate( wybor->okno, TOOLBAR_BOTTOM );
   przycisk = ToolbarAdd( oToolbar, "Enter-Wybierz", RokWybrany, wybor );
   ToolbarSetShortcut( oToolbar, przycisk, "Enter" );
   ToolbarAddSeparator( oToolbar );
   przycisk = ToolbarAdd( oToolbar, "Esc-Rezygnuj", RokRezygnuj, wybor );
   ToolbarSetShortcut( oToolbar, przycisk, "Esc" );

   oPanel = PanelCreate( wybor->okno, 0, 0, 0, 0 );

   oBrowse = BrowseCreate( oPanel, 10, 10, WinWidth( oPanel ) - 20, WinHeight( oPanel ) - 20 );
   ControlSetAnchor( oBrowse, "LTRB" );
   BrowseSetColor( oBrowse, BROWSE_HEADBACKGROUND, 0x4D7326 );
   BrowseSetSource( oBrowse, RokPoczatek, RokKoniec, RokSkip, wybor );
   BrowseSetPosition( oBrowse, RokPozycja );
   BrowseAddColumn( oBrowse, "Rok", 20, BROWSE_LEFT, RokKolumna, wybor );
   ControlSetFocus( oBrowse );

   WinCenter( wybor->okno );
   DialogShowModal( wybor->okno );

   return wybor->nWybor;
}

static void RokZnajdz( const char *cNazwa, xBool lKatalog, void *data )
{
   ListaLat *lista = ( ListaLat * )data;
   int       nRok;
   char      cReszta;
   char      cKatalog[ 96 ];
   char      cPlik[ 128 ];

   if( !lKatalog || sscanf( cNazwa, "dane-%d%c", &nRok, &cReszta ) != 1 || lista->nLat >= lista->nMaks )
      return;

   StringFormat( cPlik, "%sfkthead.dbf", FakturyKatalog( lista->biuro, nRok, cKatalog, sizeof( cKatalog ) ) );
   if( FileExists( cPlik ) )
      lista->aLata[ lista->nLat++ ] = nRok;
}

static int RokPorownaj( const void *pierwszy, const void *drugi )
{
   return *( const int * )drugi - *( const int * )pierwszy;
}

static void RokPoczatek( Control *browse, void *data )
{
   UNUSED( browse );

   ( ( WyborRoku * )data )->nPoz = 0;
}

static void RokKoniec( Control *browse, void *data )
{
   WyborRoku *wybor = ( WyborRoku * )data;

   UNUSED( browse );

   wybor->nPoz = wybor->nLat - 1;
}

static int RokSkip( Control *browse, int nIle, void *data )
{
   WyborRoku *wybor  = ( WyborRoku * )data;
   int        nStary = wybor->nPoz;

   UNUSED( browse );

   wybor->nPoz = MAX( 0, MIN( wybor->nLat - 1, wybor->nPoz + nIle ) );

   return wybor->nPoz - nStary;
}

static double RokPozycja( Control *browse, void *data )
{
   WyborRoku *wybor = ( WyborRoku * )data;

   UNUSED( browse );

   return ( double )( wybor->nPoz + 1 ) / ( double )wybor->nLat;
}

static const char *RokKolumna( Control *browse, void *data )
{
   WyborRoku *wybor = ( WyborRoku * )data;
   int        nRok  = wybor->aLata[ wybor->nPoz ];

   UNUSED( browse );

   StringFormat( wybor->cTekst, "%d%s", nRok, IIF( nRok == wybor->nBiezacy, "  (bieżący)", "" ) );

   return wybor->cTekst;
}

static void RokWybrany( Toolbar *toolbar, void *data )
{
   WyborRoku *wybor = ( WyborRoku * )data;

   UNUSED( toolbar );

   wybor->nWybor = wybor->aLata[ wybor->nPoz ];
   DialogEnd( wybor->okno, 1 );
}

static void RokRezygnuj( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( WyborRoku * )data )->okno );
}
