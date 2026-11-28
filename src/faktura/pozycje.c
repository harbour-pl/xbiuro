/*
 * Program...: xBiuro
 * Plik......: src/faktura/pozycje.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "faktura.h"

#define MAKS_WARTOSC 1000000000.0

static const char *aStawki[]      = { "23", "22", "8", "7", "5", "4", "3", "0", "zw", "oo", "np" };
static const char *aStawkiWielkie[] = { "23", "22", "8", "7", "5", "4", "3", "0", "ZW", "OO", "NP" };

static void Zablokuj( OknoPozycji *oDialog, const char *cAlias, xBool lWlacz );
static void Opis( Win *strona, int x, int y, const char *tekst );

void PozycjeEdytuj( OknoPozycji *oDialog, const char *cAlias, xBool lNowa, const char *cTytul, const char *cKomunikat, xBool lZero )
{
   Control    *GetList[ 5 ];
   Win        *oOkno;
   long        nRecNo;
   int         nKlawisz;
   double      nIlosc;
   double      nCena;
   double      nNetto;
   double      nVat;
   const char *cJest;
   char        cNazwa[ 1025 ];
   char        cJm[ 41 ];
   char        cStawka[ 3 ];
   char        cTekst[ 300 ];

   DbSelectArea( cAlias );
   nRecNo = RecNo();
   if( !lNowa && Eof() )
      return;

   if( lNowa )
   {
      DbGoBottom();
      DbSkip( 1 );
   }

   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_TOWARU" ) ) );
   RTrim( cNazwa );
   nIlosc = IIF( lNowa, 1, FieldGetN( FieldPos( "ILOSC" ) ) );
   StringFormat( cJm, "%s", IIF( lNowa, "szt.", FieldGetC( FieldPos( "MIARA" ) ) ) );
   AllTrim( cJm );
   nCena = FieldGetN( FieldPos( "CENA_JEDNOSTKOWA_NETTO" ) );
   StringFormat( cTekst, "%s", IIF( lNowa, "23", FieldGetC( FieldPos( "STAWKA_PODATKU" ) ) ) );
   StringFormat( cStawka, "%-2s", AllTrim( cTekst ) );

   oOkno = DialogCreate( oDialog->okno, cTytul, 640, 200 );
   WinSetBackground( oOkno, 0xE9EDF1 );

   Opis( oOkno, 20,  23, "Nazwa:" );
   Opis( oOkno, 20,  53, "Ilość / j.m.:" );
   Opis( oOkno, 20,  83, "Cena netto:" );
   Opis( oOkno, 20, 113, "Stawka VAT:" );

   GetList[ 0 ] = GetText( oOkno, 175, 20, 48, cNazwa, sizeof( cNazwa ), "" );
   GetList[ 1 ] = GetNumber( oOkno, 175, 50, 10, &nIlosc, "999999.999" );
   GetList[ 2 ] = GetText( oOkno, 275, 50, 6, cJm, sizeof( cJm ), "" );
   GetList[ 3 ] = GetNumber( oOkno, 175, 80, 11, &nCena, "99999999.99" );
   GetList[ 4 ] = GetText( oOkno, 175, 110, 2, cStawka, sizeof( cStawka ), "@K XX" );

   Zablokuj( oDialog, cAlias, T );
   WinCenter( oOkno );
   WinShow( oOkno );
   PozycjeTrybEdycji( oDialog->pasekStanu, T );
   WinReadModal( oOkno, GetList, 5, 1 );
   PozycjeTrybEdycji( oDialog->pasekStanu, F );
   nKlawisz = ReadLastKey();
   WinClose( oOkno );
   Zablokuj( oDialog, cAlias, F );

   DbSelectArea( cAlias );
   if( nKlawisz == K_ESC || *AllTrim( cNazwa ) == 0 )
   {
      DbGoto( nRecNo );
      oDialog->odswiez( oDialog->data );

      return;
   }

   cJest = PozycjeStawka( cStawka );
   if( cJest == NULL )
   {
      StringFormat( cTekst, "Nieznana stawka VAT: %s\nDozwolone: 23, 22, 8, 7, 5, 4, 3, 0, zw, oo, np", AllTrim( cStawka ) );
      MsgInfo( cTekst, cKomunikat );
      DbGoto( nRecNo );
      oDialog->odswiez( oDialog->data );

      return;
   }

   if( nIlosc < 0 || ( nIlosc == 0 && !lZero ) || nCena < 0 || nIlosc * nCena >= MAKS_WARTOSC || nIlosc * nCena <= -MAKS_WARTOSC )
   {
      StringFormat( cTekst, "Ilość musi być %s, cena nieujemna,\na wartość pozycji mniejsza niż 1 000 000 000.", IIF( lZero, "nieujemna", "większa od zera" ) );
      MsgInfo( cTekst, cKomunikat );
      DbGoto( nRecNo );
      oDialog->odswiez( oDialog->data );

      return;
   }

   nNetto = Round( nIlosc * nCena, 2 );
   nVat   = IIF( *cJest >= '0' && *cJest <= '9', Round( nNetto * atoi( cJest ) / 100, 2 ), 0 );

   if( lNowa )
   {
      DbAppend( T );
      FieldPutN( FieldPos( "NUMER_WIERSZA" ), ( double )LastRec() );
   }
   FieldPutC( FieldPos( "NAZWA_TOWARU" ), cNazwa );
   FieldPutN( FieldPos( "ILOSC" ), nIlosc );
   FieldPutC( FieldPos( "MIARA" ), cJm );
   FieldPutN( FieldPos( "CENA_JEDNOSTKOWA_NETTO" ), nCena );
   FieldPutC( FieldPos( "STAWKA_PODATKU" ), cJest );
   FieldPutN( FieldPos( "WARTOSC_NETTO" ), nNetto );
   FieldPutN( FieldPos( "KWOTA_PODATKU" ), nVat );
   FieldPutN( FieldPos( "WARTOSC_BRUTTO" ), nNetto + nVat );
   FieldPutD( FieldPos( "DATA_ZAPISU" ), Date() );
   DbCommit();

   BrowseRefresh( oDialog->BRW );
   oDialog->odswiez( oDialog->data );
}

void PozycjeUsun( OknoPozycji *oDialog, const char *cAlias, const char *cKomunikat )
{
   char cNazwa[ 1025 ];
   char cPytanie[ 1100 ];
   int  nLp = 0;

   DbSelectArea( cAlias );
   if( Eof() )
      return;

   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_TOWARU" ) ) );
   StringFormat( cPytanie, "Usunąć pozycję\n%s?", AllTrim( cNazwa ) );
   if( !MsgYesNo( cPytanie, cKomunikat ) )
      return;

   DbSelectArea( cAlias );
   DbDelete();
   DbPack();
   for( DbGoTop(); !Eof(); DbSkip( 1 ) )
   {
      FieldPutN( FieldPos( "NUMER_WIERSZA" ), ++nLp );
   }
   DbCommit();
   DbGoTop();

   BrowseRefresh( oDialog->BRW );
   oDialog->odswiez( oDialog->data );
}

void PozycjeTrybEdycji( StatusBar *pasekStanu, xBool lWlacz )
{
   if( lWlacz )
   {
      StatusBarSetText( pasekStanu, 1, " PgUp PgDn Enter - zapisz, Esc - bez zapisu" );
      StatusBarSetBackground( pasekStanu, 0x73264D );
      StatusBarSetForeground( pasekStanu, 0xFFFFFF );
   }
   else
   {
      StatusBarSetText( pasekStanu, 1, "" );
      StatusBarSetBackground( pasekStanu, -1 );
      StatusBarSetForeground( pasekStanu, -1 );
   }
}

const char *PozycjeStawka( const char *cStawka )
{
   char cJest[ 8 ];

   StringFormat( cJest, "%s", cStawka );
   Upper( AllTrim( cJest ) );
   for( int i = 0; i < ( int )( sizeof( aStawki ) / sizeof( aStawki[ 0 ] ) ); ++i )
   {
      if( strcmp( cJest, aStawkiWielkie[ i ] ) == 0 )
         return aStawki[ i ];
   }

   return NULL;
}

xBool PozycjeSuma( const char *cAlias, double *nNetto, double *nVat, xBool *lZw )
{
   char cStawka[ 8 ];
   long nRecNo;

   DbSelectArea( cAlias );
   nRecNo = RecNo();
   *nNetto = 0;
   *nVat   = 0;
   *lZw    = F;
   for( DbGoTop(); !Eof(); DbSkip( 1 ) )
   {
      *nNetto += FieldGetN( FieldPos( "WARTOSC_NETTO" ) );
      *nVat   += FieldGetN( FieldPos( "KWOTA_PODATKU" ) );
      StringFormat( cStawka, "%s", FieldGetC( FieldPos( "STAWKA_PODATKU" ) ) );
      if( strcmp( AllTrim( cStawka ), "zw" ) == 0 )
         *lZw = T;
   }
   DbGoto( nRecNo );

   return IIF( *nNetto < MAKS_WARTOSC && *nNetto > -MAKS_WARTOSC && *nNetto + *nVat < MAKS_WARTOSC && *nNetto + *nVat > -MAKS_WARTOSC, T, F );
}

xBool PozycjeZapisz( const char *cAlias, int nNr, const char *cRodzaj, int nLpOd, xBool lPrzed, long *aDodane, int *nDodane )
{
   char   cIndeks[ 201 ];
   char   cNazwa[ 2049 ];
   char   cMiara[ 255 ];
   char   cStawka[ 6 ];
   double nLp;
   double nIlosc;
   double nCena;
   double nNetto;
   double nVat;
   double nBrutto;

   *nDodane = 0;
   DbSelectArea( cAlias );
   for( DbGoTop(); !Eof(); DbSkip( 1 ) )
   {
      nLp     = FieldGetN( FieldPos( "NUMER_WIERSZA" ) );
      nIlosc  = FieldGetN( FieldPos( "ILOSC" ) );
      nCena   = FieldGetN( FieldPos( "CENA_JEDNOSTKOWA_NETTO" ) );
      nNetto  = FieldGetN( FieldPos( "WARTOSC_NETTO" ) );
      nVat    = FieldGetN( FieldPos( "KWOTA_PODATKU" ) );
      nBrutto = FieldGetN( FieldPos( "WARTOSC_BRUTTO" ) );
      StringFormat( cIndeks, "%s", FieldGetC( FieldPos( "INDEKS" ) ) );
      StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_TOWARU" ) ) );
      StringFormat( cMiara, "%s", FieldGetC( FieldPos( "MIARA" ) ) );
      StringFormat( cStawka, "%s", FieldGetC( FieldPos( "STAWKA_PODATKU" ) ) );

      DbSelectArea( "FKTROW" );
      if( !NetAppBlank( NET_WAIT, F ) )
         return F;

      aDodane[ ( *nDodane )++ ] = RecNo();
      FieldPutN( FieldPos( "KOLEJNY_NUMER" ), nNr );
      FieldPutC( FieldPos( "RODZAJ_FAKTURY" ), cRodzaj );
      FieldPutN( FieldPos( "NUMER_WIERSZA" ), nLpOd + nLp );
      FieldPutC( FieldPos( "INDEKS" ), cIndeks );
      FieldPutC( FieldPos( "NAZWA_TOWARU" ), RTrim( cNazwa ) );
      FieldPutC( FieldPos( "MIARA" ), cMiara );
      FieldPutN( FieldPos( "ILOSC" ), nIlosc );
      FieldPutN( FieldPos( "CENA_JEDNOSTKOWA_NETTO" ), nCena );
      FieldPutN( FieldPos( "WARTOSC_NETTO" ), nNetto );
      FieldPutC( FieldPos( "STAWKA_PODATKU" ), cStawka );
      FieldPutN( FieldPos( "KWOTA_PODATKU" ), nVat );
      FieldPutN( FieldPos( "WARTOSC_BRUTTO" ), nBrutto );
      FieldPutL( FieldPos( "STAN_PRZED" ), lPrzed );
      FieldPutD( FieldPos( "DATA_ZAPISU" ), Date() );
      DbCommit();

      DbSelectArea( cAlias );
   }

   return T;
}

void PozycjeWycofaj( long *aDodane, int nDodane )
{
   DbSelectArea( "FKTROW" );
   for( int i = 0; i < nDodane; ++i )
   {
      DbGoto( aDodane[ i ] );
      DbDelete();
   }
   DbCommit();
   DbUnlock();
}

static void Zablokuj( OknoPozycji *oDialog, const char *cAlias, xBool lWlacz )
{
   for( int i = 0; i < oDialog->nPrzyciskow; ++i )
   {
      ToolbarEnable( oDialog->pasek, oDialog->aPrzyciski[ i ], !lWlacz );
   }

   DbSelectArea( cAlias );
   ControlSetEnabled( oDialog->BRW, IIF( lWlacz, F, IIF( LastRec() > 0, T, F ) ) );
}

static void Opis( Win *strona, int x, int y, const char *tekst )
{
   SayCreate( strona, x, y, 0, 20, tekst );
}
