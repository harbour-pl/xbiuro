/*
 * Program...: xBiuro
 * Plik......: src/faktura/fa3.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "faktura.h"

#define FA3_NAMESPACE "http://crd.gov.pl/wzor/2025/06/25/13775/"

typedef struct
{
   char  *data;
   size_t length;
   size_t capacity;
} Xml;

typedef struct
{
   char   cStawka[ 6 ];
   double nNetto;
   double nVat;
} SumaStawki;

static void        Dopisz( Xml *xml, const char *cTekst );
static void        Otworz( Xml *xml, const char *cNazwa );
static void        Zamknij( Xml *xml, const char *cNazwa );
static void        Element( Xml *xml, const char *cNazwa, const char *cWartosc );
static void        Strona( Xml *xml, const char *cNip, const char *cNazwa, const char *cKraj, const char *cAdres1, const char *cAdres2 );
static char       *Tekst( const char *cTekst, char *cWynik, int nRozmiar );
static char       *Pole( const char *cPole, char *cWynik, int nRozmiar );
static char       *AdresL1( const char *cUlica, const char *cNrDomu, const char *cNrLokalu, char *cWynik, int nRozmiar );
static char       *AdresL2( const char *cKodPocztowy, const char *cMiejscowosc, char *cWynik, int nRozmiar );
static char       *Data( int dData, char *cWynik, int nRozmiar );
static char       *Kwota( double nKwota, char *cWynik, int nRozmiar );
static const char *Stawka( const char *cStawka );
static SumaStawki *Suma( SumaStawki *aStawki, int nStawek, const char *cStawka );

char *Fa3GenerujXml( const char *cSprzedawca )
{
   static const char *aSumy[ 4 ][ 3 ] = { { "23", "22", "1" }, { "8", "7", "2" }, { "5", "5", "3" }, { "4", "3", "4" } };

   Xml         xml     = { NULL, 0, 0 };
   SumaStawki  aStawki[ 16 ];
   SumaStawki *suma;
   SumaStawki *druga;
   int         nStawek = 0;
   double      nZnak;
   double      nNetto;
   double      nVat;
   char        cKlucz[ 16 ];
   char        cJest[ 16 ];
   char        cNip[ 64 ];
   char        cNazwa[ 2100 ];
   char        cUlica[ 300 ];
   char        cDom[ 40 ];
   char        cLokal[ 44 ];
   char        cKod[ 12 ];
   char        cMiasto[ 230 ];
   char        cKraj[ 4 ];
   char        cAdres1[ 400 ];
   char        cAdres2[ 300 ];
   char        cTekst[ 2100 ];
   char        cData[ 16 ];
   char        cCzas[ 16 ];
   char        cStawka[ 8 ];
   xBool       lKor;

   DbSelectArea( cSprzedawca );
   Pole( "NIP", cNip, sizeof( cNip ) );
   Pole( "PELNA_NAZWA", cNazwa, sizeof( cNazwa ) );
   Pole( "ULICA", cUlica, sizeof( cUlica ) );
   Pole( "NR_DOMU", cDom, sizeof( cDom ) );
   Pole( "NR_LOKALU", cLokal, sizeof( cLokal ) );
   Pole( "KOD_POCZTOWY", cKod, sizeof( cKod ) );
   Pole( "MIEJSCOWOSC", cMiasto, sizeof( cMiasto ) );

   Dopisz( &xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Faktura xmlns=\"" FA3_NAMESPACE "\">" );
   Otworz( &xml, "Naglowek" );
   Dopisz( &xml, "<KodFormularza kodSystemowy=\"FA (3)\" wersjaSchemy=\"1-0E\">FA</KodFormularza>" );
   Element( &xml, "WariantFormularza", "3" );
   StringFormat( cTekst, "%sT%s", Data( Date(), cData, sizeof( cData ) ), Time( cCzas, sizeof( cCzas ) ) );
   Element( &xml, "DataWytworzeniaFa", cTekst );
   Element( &xml, "SystemInfo", "xBiuro" );
   Zamknij( &xml, "Naglowek" );

   Otworz( &xml, "Podmiot1" );
   Strona( &xml, cNip, cNazwa, "PL", AdresL1( cUlica, cDom, cLokal, cAdres1, sizeof( cAdres1 ) ), AdresL2( cKod, cMiasto, cAdres2, sizeof( cAdres2 ) ) );
   Zamknij( &xml, "Podmiot1" );

   DbSelectArea( "FKTHEAD" );
   Pole( "NIP_NABYWCY", cNip, sizeof( cNip ) );
   Pole( "NAZWA_NABYWCY", cNazwa, sizeof( cNazwa ) );
   Pole( "KOD_KRAJU_NABYWCY", cKraj, sizeof( cKraj ) );
   Pole( "ULICA_NABYWCY", cAdres1, sizeof( cAdres1 ) );
   Pole( "KOD_POCZTOWY_NABYWCY", cKod, sizeof( cKod ) );
   Pole( "MIEJSCOWOSC_NABYWCY", cMiasto, sizeof( cMiasto ) );
   if( *cKraj == 0 )
      StringFormat( cKraj, "PL" );

   Otworz( &xml, "Podmiot2" );
   Strona( &xml, cNip, cNazwa, cKraj, cAdres1, AdresL2( cKod, cMiasto, cAdres2, sizeof( cAdres2 ) ) );
   Element( &xml, "JST", "2" );
   Element( &xml, "GV", "2" );
   Zamknij( &xml, "Podmiot2" );

   FakturyKlucz( cKlucz, sizeof( cKlucz ) );
   lKor = IIF( strncmp( cKlucz, "KOR ", 4 ) == 0, T, F );

   DbSelectArea( "FKTROW" );
   for( DbSeek( cKlucz, F ); !Eof() && strcmp( FakturyKlucz( cJest, sizeof( cJest ) ), cKlucz ) == 0; DbSkip( 1 ) )
   {
      Pole( "STAWKA_PODATKU", cStawka, sizeof( cStawka ) );
      suma = Suma( aStawki, nStawek, cStawka );
      if( suma == NULL && nStawek < 16 )
      {
         suma = &aStawki[ nStawek++ ];
         StringFormat( suma->cStawka, "%s", cStawka );
         suma->nNetto = 0;
         suma->nVat   = 0;
      }
      nZnak = IIF( FieldGetL( FieldPos( "STAN_PRZED" ) ), -1, 1 );
      suma->nNetto += nZnak * FieldGetN( FieldPos( "WARTOSC_NETTO" ) );
      suma->nVat   += nZnak * FieldGetN( FieldPos( "KWOTA_PODATKU" ) );
   }

   DbSelectArea( "FKTHEAD" );
   Otworz( &xml, "Fa" );
   Element( &xml, "KodWaluty", "PLN" );
   Element( &xml, "P_1", Data( FieldGetD( FieldPos( "DATA_WYSTAWIENIA" ) ), cData, sizeof( cData ) ) );
   Element( &xml, "P_2", Pole( "NUMER_FAKTURY", cTekst, sizeof( cTekst ) ) );
   if( FieldGetD( FieldPos( "DATA_DOSTAWY" ) ) != 0 && FieldGetD( FieldPos( "DATA_DOSTAWY" ) ) != FieldGetD( FieldPos( "DATA_WYSTAWIENIA" ) ) )
      Element( &xml, "P_6", Data( FieldGetD( FieldPos( "DATA_DOSTAWY" ) ), cData, sizeof( cData ) ) );

   for( int i = 0; i < 4; ++i )
   {
      suma  = Suma( aStawki, nStawek, aSumy[ i ][ 0 ] );
      druga = IIF( strcmp( aSumy[ i ][ 0 ], aSumy[ i ][ 1 ] ) == 0, NULL, Suma( aStawki, nStawek, aSumy[ i ][ 1 ] ) );
      if( suma == NULL && druga == NULL )
         continue;

      nNetto = IIF( suma == NULL, 0, suma->nNetto ) + IIF( druga == NULL, 0, druga->nNetto );
      nVat   = IIF( suma == NULL, 0, suma->nVat ) + IIF( druga == NULL, 0, druga->nVat );
      StringFormat( cTekst, "P_13_%s", aSumy[ i ][ 2 ] );
      Element( &xml, cTekst, Kwota( nNetto, cData, sizeof( cData ) ) );
      StringFormat( cTekst, "P_14_%s", aSumy[ i ][ 2 ] );
      Element( &xml, cTekst, Kwota( nVat, cData, sizeof( cData ) ) );
   }
   if( ( suma = Suma( aStawki, nStawek, "0" ) ) != NULL )
      Element( &xml, "P_13_6_1", Kwota( suma->nNetto, cData, sizeof( cData ) ) );
   if( ( suma = Suma( aStawki, nStawek, "zw" ) ) != NULL )
      Element( &xml, "P_13_7", Kwota( suma->nNetto, cData, sizeof( cData ) ) );
   if( ( suma = Suma( aStawki, nStawek, "np" ) ) != NULL )
      Element( &xml, "P_13_8", Kwota( suma->nNetto, cData, sizeof( cData ) ) );
   if( ( suma = Suma( aStawki, nStawek, "oo" ) ) != NULL )
      Element( &xml, "P_13_10", Kwota( suma->nNetto, cData, sizeof( cData ) ) );
   Element( &xml, "P_15", Kwota( FieldGetN( FieldPos( "KWOTA_NALEZNOSCI" ) ), cData, sizeof( cData ) ) );

   Otworz( &xml, "Adnotacje" );
   Element( &xml, "P_16", "2" );
   Element( &xml, "P_17", "2" );
   Element( &xml, "P_18", IIF( Suma( aStawki, nStawek, "oo" ) == NULL, "2", "1" ) );
   Element( &xml, "P_18A", "2" );
   Otworz( &xml, "Zwolnienie" );
   if( Suma( aStawki, nStawek, "zw" ) != NULL )
   {
      Element( &xml, "P_19", "1" );
      Element( &xml, "P_19A", Pole( "PODSTAWA_ZWOLNIENIA", cTekst, sizeof( cTekst ) ) );
   }
   else
      Element( &xml, "P_19N", "1" );
   Zamknij( &xml, "Zwolnienie" );
   Dopisz( &xml, "<NoweSrodkiTransportu><P_22N>1</P_22N></NoweSrodkiTransportu>" );
   Element( &xml, "P_23", "2" );
   Dopisz( &xml, "<PMarzy><P_PMarzyN>1</P_PMarzyN></PMarzy>" );
   Zamknij( &xml, "Adnotacje" );

   if( lKor )
   {
      Element( &xml, "RodzajFaktury", "KOR" );
      if( *Pole( "PRZYCZYNA_KOREKTY", cTekst, sizeof( cTekst ) ) != 0 )
         Element( &xml, "PrzyczynaKorekty", cTekst );
      Otworz( &xml, "DaneFaKorygowanej" );
      Element( &xml, "DataWystFaKorygowanej", Data( FieldGetD( FieldPos( "DATA_FAKTURY_KORYGOWANEJ" ) ), cData, sizeof( cData ) ) );
      Element( &xml, "NrFaKorygowanej", Pole( "NUMER_FAKTURY_KORYGOWANEJ", cTekst, sizeof( cTekst ) ) );
      if( *Pole( "NUMER_KSEF_FAKTURY_KORYGOWANEJ", cTekst, sizeof( cTekst ) ) == 0 )
         Element( &xml, "NrKSeFN", "1" );
      else
      {
         Element( &xml, "NrKSeF", "1" );
         Element( &xml, "NrKSeFFaKorygowanej", cTekst );
      }
      Zamknij( &xml, "DaneFaKorygowanej" );
   }
   else
      Element( &xml, "RodzajFaktury", "VAT" );

   DbSelectArea( "FKTROW" );
   for( DbSeek( cKlucz, F ); !Eof() && strcmp( FakturyKlucz( cJest, sizeof( cJest ) ), cKlucz ) == 0; DbSkip( 1 ) )
   {
      Otworz( &xml, "FaWiersz" );
      StringFormat( cTekst, "%d", ( int )FieldGetN( FieldPos( "NUMER_WIERSZA" ) ) );
      Element( &xml, "NrWierszaFa", cTekst );
      Element( &xml, "P_7", Pole( "NAZWA_TOWARU", cTekst, sizeof( cTekst ) ) );
      Element( &xml, "P_8A", Pole( "MIARA", cTekst, sizeof( cTekst ) ) );
      StringFormat( cTekst, "%.3f", FieldGetN( FieldPos( "ILOSC" ) ) );
      Element( &xml, "P_8B", cTekst );
      Element( &xml, "P_9A", Kwota( FieldGetN( FieldPos( "CENA_JEDNOSTKOWA_NETTO" ) ), cData, sizeof( cData ) ) );
      Element( &xml, "P_11", Kwota( FieldGetN( FieldPos( "WARTOSC_NETTO" ) ), cData, sizeof( cData ) ) );
      Element( &xml, "P_12", Stawka( Pole( "STAWKA_PODATKU", cStawka, sizeof( cStawka ) ) ) );
      if( FieldGetL( FieldPos( "STAN_PRZED" ) ) )
         Element( &xml, "StanPrzed", "1" );
      Zamknij( &xml, "FaWiersz" );
   }

   DbSelectArea( "FKTHEAD" );
   Otworz( &xml, "Platnosc" );
   if( FieldGetD( FieldPos( "TERMIN_PLATNOSCI" ) ) != 0 )
   {
      Otworz( &xml, "TerminPlatnosci" );
      Element( &xml, "Termin", Data( FieldGetD( FieldPos( "TERMIN_PLATNOSCI" ) ), cData, sizeof( cData ) ) );
      Zamknij( &xml, "TerminPlatnosci" );
   }
   StringFormat( cTekst, "%d", IIF( FieldGetN( FieldPos( "FORMA_PLATNOSCI" ) ) == 0, PLATNOSC_PRZELEW, ( int )FieldGetN( FieldPos( "FORMA_PLATNOSCI" ) ) ) );
   Element( &xml, "FormaPlatnosci", cTekst );
   if( *Pole( "NUMER_RACHUNKU", cAdres1, sizeof( cAdres1 ) ) != 0 )
   {
      Otworz( &xml, "RachunekBankowy" );
      StrTran( cAdres2, sizeof( cAdres2 ), cAdres1, " ", "" );
      Element( &xml, "NrRB", StrTran( cTekst, sizeof( cTekst ), cAdres2, "PL", "" ) );
      if( *Pole( "NAZWA_BANKU", cTekst, sizeof( cTekst ) ) != 0 )
         Element( &xml, "NazwaBanku", cTekst );
      Zamknij( &xml, "RachunekBankowy" );
   }
   Zamknij( &xml, "Platnosc" );
   Zamknij( &xml, "Fa" );
   Dopisz( &xml, "</Faktura>\n" );

   return xml.data;
}

static void Dopisz( Xml *xml, const char *cTekst )
{
   size_t nDlugosc = strlen( cTekst );

   if( xml->length + nDlugosc + 1 > xml->capacity )
   {
      xml->capacity = ( xml->length + nDlugosc + 1 ) * 2;
      xml->data     = ( char * )realloc( xml->data, xml->capacity );
   }

   memcpy( xml->data + xml->length, cTekst, nDlugosc + 1 );
   xml->length += nDlugosc;
}

static void Otworz( Xml *xml, const char *cNazwa )
{
   Dopisz( xml, "<" );
   Dopisz( xml, cNazwa );
   Dopisz( xml, ">" );
}

static void Zamknij( Xml *xml, const char *cNazwa )
{
   Dopisz( xml, "</" );
   Dopisz( xml, cNazwa );
   Dopisz( xml, ">" );
}

static void Element( Xml *xml, const char *cNazwa, const char *cWartosc )
{
   char cZnak[ 2 ] = { 0, 0 };

   Otworz( xml, cNazwa );
   for( const char *c = cWartosc; *c != 0; ++c )
   {
      if( *c == '&' )
         Dopisz( xml, "&amp;" );
      else if( *c == '<' )
         Dopisz( xml, "&lt;" );
      else if( *c == '>' )
         Dopisz( xml, "&gt;" );
      else if( *c == '"' )
         Dopisz( xml, "&quot;" );
      else
      {
         cZnak[ 0 ] = *c;
         Dopisz( xml, cZnak );
      }
   }
   Zamknij( xml, cNazwa );
}

static void Strona( Xml *xml, const char *cNip, const char *cNazwa, const char *cKraj, const char *cAdres1, const char *cAdres2 )
{
   static const char *cUnia = "AT BE BG CY CZ DE DK EE EL GR ES FI FR HR HU IE IT LT LU LV MT NL PT RO SE SI SK XI";

   char cNr[ 64 ];
   char cCyfry[ 64 ];

   StrTran( cNr, sizeof( cNr ), cNip, " ", "" );
   AllTrim( cNr );
   if( strncmp( cNr, cKraj, 2 ) == 0 )
      memmove( cNr, cNr + 2, strlen( cNr + 2 ) + 1 );

   Otworz( xml, "DaneIdentyfikacyjne" );
   if( *cNr == 0 )
      Element( xml, "BrakID", "1" );
   else if( strcmp( cKraj, "PL" ) == 0 )
   {
      FirmaCyfry( cNr, cCyfry, sizeof( cCyfry ) );
      Element( xml, "NIP", cCyfry );
   }
   else if( strstr( cUnia, cKraj ) != NULL )
   {
      Element( xml, "KodUE", IIF( strcmp( cKraj, "GR" ) == 0, "EL", cKraj ) );
      Element( xml, "NrVatUE", IIF( strncmp( cNr, "EL", 2 ) == 0, cNr + 2, cNr ) );
   }
   else
   {
      Element( xml, "KodKraju", cKraj );
      Element( xml, "NrID", cNr );
   }
   Element( xml, "Nazwa", cNazwa );
   Zamknij( xml, "DaneIdentyfikacyjne" );

   Otworz( xml, "Adres" );
   Element( xml, "KodKraju", cKraj );
   Element( xml, "AdresL1", IIF( *cAdres1 == 0, cAdres2, cAdres1 ) );
   if( *cAdres1 != 0 && *cAdres2 != 0 )
      Element( xml, "AdresL2", cAdres2 );
   Zamknij( xml, "Adres" );
}

static char *Tekst( const char *cTekst, char *cWynik, int nRozmiar )
{
   const unsigned char *c        = ( const unsigned char * )cTekst;
   int                  nDlugosc = 0;

   while( *c != 0 && nDlugosc < nRozmiar - 4 )
   {
      if( *c == '\r' || *c == '\n' )
      {
         cWynik[ nDlugosc++ ] = ' ';
         c += IIF( c[ 0 ] == '\r' && c[ 1 ] == '\n', 2, 1 );
      }
      else if( ( *c < 0x20 && *c != '\t' ) || *c == 0x7F )
         ++c;
      else if( c[ 0 ] == 0xC2 && c[ 1 ] >= 0x80 && c[ 1 ] <= 0x9F && c[ 1 ] != 0x85 )
         c += 2;
      else if( c[ 0 ] == 0xEF && c[ 1 ] == 0xBF && ( c[ 2 ] == 0xBE || c[ 2 ] == 0xBF ) )
         c += 3;
      else
         cWynik[ nDlugosc++ ] = ( char )*c++;
   }
   cWynik[ nDlugosc ] = 0;

   return AllTrim( cWynik );
}

static char *Pole( const char *cPole, char *cWynik, int nRozmiar )
{
   return Tekst( FieldGetC( FieldPos( cPole ) ), cWynik, nRozmiar );
}

static char *AdresL1( const char *cUlica, const char *cNrDomu, const char *cNrLokalu, char *cWynik, int nRozmiar )
{
   StringFormatSize( cWynik, ( size_t )nRozmiar, "%s %s%s%s", cUlica, cNrDomu, IIF( *cNrLokalu == 0, "", "/" ), cNrLokalu );

   return AllTrim( cWynik );
}

static char *AdresL2( const char *cKodPocztowy, const char *cMiejscowosc, char *cWynik, int nRozmiar )
{
   StringFormatSize( cWynik, ( size_t )nRozmiar, "%s %s", cKodPocztowy, cMiejscowosc );

   return AllTrim( cWynik );
}

static char *Data( int dData, char *cWynik, int nRozmiar )
{
   StringFormatSize( cWynik, ( size_t )nRozmiar, "%04d-%02d-%02d", dData / 10000, dData / 100 % 100, dData % 100 );

   return cWynik;
}

static char *Kwota( double nKwota, char *cWynik, int nRozmiar )
{
   StringFormatSize( cWynik, ( size_t )nRozmiar, "%.2f", nKwota );

   return cWynik;
}

static const char *Stawka( const char *cStawka )
{
   if( strcmp( cStawka, "0" ) == 0 )
      return "0 KR";

   if( strcmp( cStawka, "np" ) == 0 )
      return "np I";

   return cStawka;
}

static SumaStawki *Suma( SumaStawki *aStawki, int nStawek, const char *cStawka )
{
   for( int i = 0; i < nStawek; ++i )
   {
      if( strcmp( aStawki[ i ].cStawka, cStawka ) == 0 )
         return &aStawki[ i ];
   }

   return NULL;
}
