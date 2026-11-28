/*
 * Program...: xBiuro
 * Plik......: src/wydruk.c
 *
 * Wydruki PDF (libharu) wspólne dla modułów, według wydruk.prg z xFaktura: czcionka, tekst, znak programu na stronie
 * i karta podmiotu (firmy albo kontrahenta) ze wszystkimi danymi w kolejności zakładek okna
 *
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "biuro.h"
#include "xsiec.h"

/* Karta podmiotu w budowie: dokument, bieżąca strona, czcionki i wysokość następnego wiersza */
typedef struct
{
   HPDF_Doc  pPdf;
   HPDF_Page pPage;
   HPDF_Font pFont;
   HPDF_Font pBold;
   HPDF_Page aStrony[ 32 ];
   int       nStron;
   double    nY;
} Karta;

static void KartaStrona( Karta *karta );
static void KartaSekcja( Karta *karta, const char *cTytul );
static void KartaWiersz( Karta *karta, const char *cOpis, const char *cWartosc );
static void KartaPole( Karta *karta, const char *cOpis, const char *cPole );
static void KartaData( Karta *karta, const char *cOpis, const char *cPole );
static void KartaTakNie( Karta *karta, const char *cOpis, const char *cPole );
static void KartaRachunek( Karta *karta, const char *cOpis, int nRachunek );
static void Slownie( long long nValue, char *cWynik, size_t nRozmiar );
static void Dopisz( char *cTekst, size_t nRozmiar, const char *cSlowo );

// Czcionka TrueType z katalogu czcionek Windows, z polskimi literami (UTF-8); NULL, gdy jej nie ma
// WydrukCzcionka( pPdf, cPlik ) --> pFont
HPDF_Font WydrukCzcionka( HPDF_Doc pPdf, const char *cPlik )
{
   char        cSciezka[ 300 ];
   const char *cNazwa;

   StringFormat( cSciezka, "%s\\Fonts\\%s", GetEnv( "WINDIR" ), cPlik );
   if( !FileExists( cSciezka ) )
      return NULL;

   cNazwa = HPDF_LoadTTFontFromFile( pPdf, cSciezka, HPDF_TRUE );
   if( cNazwa == NULL )
      return NULL;

   return HPDF_GetFont( pPdf, cNazwa, "UTF-8" );
}

// Znak programu na stronie: trzy kółka w lewym górnym rogu i stopka „Wydrukowano w programie xBiuro”
// WydrukUstawZnaki( pPage, pFont ) --> void
void WydrukUstawZnaki( HPDF_Page pPage, HPDF_Font pFont )
{
   static const float aKolor[ 3 ][ 3 ] = { { 0.451f, 0.149f, 0.302f }, { 0.149f, 0.153f, 0.451f }, { 0.149f, 0.451f, 0.451f } };

   const char *cStopka  = "Wydrukowano w programie xBiuro";
   float       nPromien = 11.34f;
   float       nGora    = HPDF_Page_GetHeight( pPage ) - 35;

   for( int nKolko = 0; nKolko < 3; ++nKolko )
   {
      HPDF_Page_SetRGBFill( pPage, aKolor[ nKolko ][ 0 ], aKolor[ nKolko ][ 1 ], aKolor[ nKolko ][ 2 ] );
      HPDF_Page_Circle( pPage, 40 + nPromien + nKolko * ( 2 * nPromien + 6 ), nGora, nPromien );
      HPDF_Page_Fill( pPage );
   }

   HPDF_Page_SetRGBFill( pPage, 0.55f, 0.55f, 0.55f );
   HPDF_Page_SetFontAndSize( pPage, pFont, 8 );
   HPDF_Page_BeginText( pPage );
   HPDF_Page_TextOut( pPage, ( HPDF_Page_GetWidth( pPage ) - HPDF_Page_TextWidth( pPage, cStopka ) ) / 2, 25, cStopka );
   HPDF_Page_EndText( pPage );
   HPDF_Page_SetRGBFill( pPage, 0, 0, 0 );
}

// Tekst na stronie; za szeroki ucięty z „…” na końcu
// WydrukTekst( pPage, pFont, nSize, nX, nY, cTekst, nSzer ) --> void
void WydrukTekst( HPDF_Page pPage, HPDF_Font pFont, double nSize, double nX, double nY, const char *cTekst, double nSzer )
{
   char cWynik[ 2100 ];
   char cProba[ 2100 ];

   StringFormat( cWynik, "%s", cTekst );
   HPDF_Page_SetFontAndSize( pPage, pFont, ( HPDF_REAL )nSize );
   if( HPDF_Page_TextWidth( pPage, cWynik ) > nSzer )
   {
      for( ;; )
      {
         StringFormat( cProba, "%s…", cWynik );
         if( *cWynik == 0 || HPDF_Page_TextWidth( pPage, cProba ) <= nSzer )
            break;
         ULeft( cWynik, ULen( cWynik ) - 1 );
      }
      StringFormat( cWynik, "%s", cProba );
   }

   HPDF_Page_BeginText( pPage );
   HPDF_Page_TextOut( pPage, ( HPDF_REAL )nX, ( HPDF_REAL )nY, cWynik );
   HPDF_Page_EndText( pPage );
}

// Tekst wyrównany do prawej krawędzi nPrawo
// WydrukPrawo( pPage, pFont, nSize, nPrawo, nY, cTekst ) --> void
void WydrukPrawo( HPDF_Page pPage, HPDF_Font pFont, double nSize, double nPrawo, double nY, const char *cTekst )
{
   HPDF_Page_SetFontAndSize( pPage, pFont, ( HPDF_REAL )nSize );
   HPDF_Page_BeginText( pPage );
   HPDF_Page_TextOut( pPage, ( HPDF_REAL )nPrawo - HPDF_Page_TextWidth( pPage, cTekst ), ( HPDF_REAL )nY, cTekst );
   HPDF_Page_EndText( pPage );
}

char *WydrukKwota( double nKwota, char *cWynik, int nRozmiar )
{
   char cLiczba[ 32 ];
   int  nCalosc;
   int  nDlugosc = 0;

   StringFormat( cLiczba, "%.2f", IIF( nKwota < 0, -nKwota, nKwota ) );
   nCalosc = ( int )strlen( cLiczba ) - 3;

   if( nKwota < 0 && nDlugosc < nRozmiar - 1 )
      cWynik[ nDlugosc++ ] = '-';
   for( int i = 0; i < nCalosc && nDlugosc < nRozmiar - 2; ++i )
   {
      if( i > 0 && ( nCalosc - i ) % 3 == 0 )
         cWynik[ nDlugosc++ ] = ' ';
      cWynik[ nDlugosc++ ] = cLiczba[ i ];
   }
   cWynik[ nDlugosc ] = 0;
   StringFormatSize( cWynik + nDlugosc, ( size_t )( nRozmiar - nDlugosc ), ",%s", cLiczba + nCalosc + 1 );

   return cWynik;
}

char *WydrukIlosc( double nIlosc, char *cWynik, int nRozmiar )
{
   char cLiczba[ 32 ];
   int  nDlugosc;

   StringFormat( cLiczba, "%.3f", nIlosc );
   nDlugosc = ( int )strlen( cLiczba );
   while( cLiczba[ nDlugosc - 1 ] == '0' )
   {
      cLiczba[ --nDlugosc ] = 0;
   }
   if( cLiczba[ nDlugosc - 1 ] == '.' )
      cLiczba[ --nDlugosc ] = 0;

   return StrTran( cWynik, ( size_t )nRozmiar, cLiczba, ".", "," );
}

void WydrukPasek( HPDF_Page pPage, HPDF_Font pBold, double nY, const double *aKolumna, int nKolumn, const char **aTytul, const xBool *aPrawo )
{
   HPDF_Page_SetRGBFill( pPage, 0.302f, 0.451f, 0.149f );
   HPDF_Page_Rectangle( pPage, ( HPDF_REAL )aKolumna[ 0 ], ( HPDF_REAL )( nY - 5 ), ( HPDF_REAL )( aKolumna[ nKolumn - 1 ] - aKolumna[ 0 ] ), 16 );
   HPDF_Page_Fill( pPage );
   HPDF_Page_SetRGBFill( pPage, 1, 1, 1 );
   for( int nKolumna = 0; nKolumna < nKolumn - 1; ++nKolumna )
   {
      if( aPrawo[ nKolumna ] )
         WydrukPrawo( pPage, pBold, 8, aKolumna[ nKolumna + 1 ] - 3, nY, aTytul[ nKolumna ] );
      else
         WydrukTekst( pPage, pBold, 8, aKolumna[ nKolumna ] + 3, nY, aTytul[ nKolumna ], aKolumna[ nKolumna + 1 ] - aKolumna[ nKolumna ] - 6 );
   }
   HPDF_Page_SetRGBFill( pPage, 0, 0, 0 );
}

void WydrukStrona( HPDF_Page pPage, HPDF_Font pFont, HPDF_Font pBold, double nX, double nY, const char *cTytul, const char *cNazwa, const char *cAdres1, const char *cAdres2, const char *cNip )
{
   const char *aAdres[ 2 ] = { cAdres1, cAdres2 };
   char        cLinia[ 2049 ];
   size_t      nDlugosc;

   WydrukTekst( pPage, pFont, 8, nX, nY, cTytul, 240 );
   HPDF_Page_SetRGBStroke( pPage, 0.302f, 0.451f, 0.149f );
   HPDF_Page_SetLineWidth( pPage, 0.8f );
   HPDF_Page_MoveTo( pPage, ( HPDF_REAL )nX, ( HPDF_REAL )( nY - 4 ) );
   HPDF_Page_LineTo( pPage, ( HPDF_REAL )( nX + 245 ), ( HPDF_REAL )( nY - 4 ) );
   HPDF_Page_Stroke( pPage );
   nY -= 18;

   while( *cNazwa != 0 )
   {
      nDlugosc = strcspn( cNazwa, "\r\n" );
      StringFormat( cLinia, "%.*s", ( int )nDlugosc, cNazwa );
      if( *AllTrim( cLinia ) != 0 )
      {
         WydrukTekst( pPage, pBold, 10, nX, nY, cLinia, 245 );
         nY -= 13;
      }
      cNazwa += nDlugosc;
      cNazwa += strspn( cNazwa, "\r\n" );
   }

   for( int i = 0; i < 2; ++i )
   {
      if( *aAdres[ i ] != 0 )
      {
         WydrukTekst( pPage, pFont, 9, nX, nY, aAdres[ i ], 245 );
         nY -= 12;
      }
   }

   if( *cNip != 0 )
   {
      StringFormat( cLinia, "NIP: %s", cNip );
      WydrukTekst( pPage, pFont, 9, nX, nY, cLinia, 245 );
   }
}

void WydrukQr( HPDF_Page pPage, const char *cTekst, double nX, double nY, double nRozmiar )
{
   unsigned char *aModul = ( unsigned char * )malloc( QR_MAX_SIZE * QR_MAX_SIZE );
   int            nRzedow = QrEncode( cTekst, QR_ECC_MEDIUM, aModul );
   double         nModul;
   int            nOd;

   if( nRzedow == 0 )
   {
      free( aModul );

      return;
   }

   nModul = nRozmiar / nRzedow;
   HPDF_Page_SetRGBFill( pPage, 0, 0, 0 );
   for( int nRzad = 0; nRzad < nRzedow; ++nRzad )
   {
      for( int nKol = 0; nKol < nRzedow; )
      {
         if( aModul[ nRzad * nRzedow + nKol ] == 0 )
         {
            ++nKol;
            continue;
         }

         nOd = nKol;
         while( nKol < nRzedow && aModul[ nRzad * nRzedow + nKol ] != 0 )
            ++nKol;
         HPDF_Page_Rectangle( pPage, ( HPDF_REAL )( nX + nOd * nModul ), ( HPDF_REAL )( nY + nRozmiar - ( nRzad + 1 ) * nModul ), ( HPDF_REAL )( ( nKol - nOd ) * nModul ), ( HPDF_REAL )nModul );
      }
   }
   HPDF_Page_Fill( pPage );
   free( aModul );
}

// Ścieżka wydruku cNazwa w katalogu wydruków wybranej firmy: dane-RRRR/firmaN/wydruki/ (katalog zakładany, gdy go nie ma)
// WydrukPlik( biuro, cNazwa, cPlik, nRozmiar ) --> cPlik
char *WydrukPlik( Biuro *biuro, const char *cNazwa, char *cPlik, int nRozmiar )
{
   char cFirma[ 64 ];
   char cKatalog[ 80 ];

   StringFormat( cKatalog, "%swydruki", RokFirma( biuro, biuro->firma, cFirma, sizeof( cFirma ) ) );
   if( !DirExists( cKatalog ) )
      DirCreate( cKatalog );
   StringFormatSize( cPlik, ( size_t )nRozmiar, "%s/%s", cKatalog, cNazwa );

   return cPlik;
}

// Kwota słownie jak KwotaSlownie() z ntow.prg: złote słownie i grosze, np. „sto dwadzieścia trzy zł 45/100”
// NToW( nKwota, cWynik, nRozmiar ) --> cWynik
char *NToW( double nKwota, char *cWynik, int nRozmiar )
{
   long long nGrosze = llround( fabs( nKwota ) * 100 );
   char      cZlote[ 512 ];

   Slownie( IIF( nKwota < 0, -( nGrosze / 100 ), nGrosze / 100 ), cZlote, sizeof( cZlote ) );
   StringFormatSize( cWynik, ( size_t )nRozmiar, "%s zł %02lld/100", cZlote, nGrosze % 100 );

   return cWynik;
}

// Karta podmiotu z bieżącego rekordu obszaru cAlias do PDF cNazwa w katalogu wydruków wybranej firmy, otwarta potem
// w programie Windows
// WydrukKarta( biuro, cAlias, cTytul, cNazwa ) --> void
void WydrukKarta( Biuro *biuro, const char *cAlias, const char *cTytul, const char *cNazwa )
{
   Karta karta = { 0 };
   char  cSciezka[ 128 ];
   char  cTekst[ 300 ];
   char  cData[ 16 ];
   char  cGodzina[ 16 ];
   char  cKod[ 5 ];
   char  cUlica[ 600 ];
   char  cNr[ 100 ];
   char  cMiasto[ 300 ];

   DbSelectArea( cAlias );
   if( Eof() )
      return;

   karta.pPdf = HPDF_New( NULL, NULL );
   HPDF_SetCompressionMode( karta.pPdf, HPDF_COMP_ALL );
   HPDF_UseUTFEncodings( karta.pPdf );
   HPDF_SetCurrentEncoder( karta.pPdf, "UTF-8" );
   HPDF_SetInfoAttr( karta.pPdf, HPDF_INFO_TITLE, cTytul );
   HPDF_SetInfoAttr( karta.pPdf, HPDF_INFO_CREATOR, "xBiuro" );
   karta.pFont = WydrukCzcionka( karta.pPdf, "arial.ttf" );
   karta.pBold = WydrukCzcionka( karta.pPdf, "arialbd.ttf" );
   if( karta.pFont == NULL || karta.pBold == NULL )
   {
      HPDF_Free( karta.pPdf );
      MsgError( "Brak czcionki Arial w katalogu czcionek Windows.", cTytul );

      return;
   }

   KartaStrona( &karta );
   DToC( Date(), cData, sizeof( cData ) );
   Time( cGodzina, sizeof( cGodzina ) );
   cGodzina[ 5 ] = 0;
   StringFormat( cTekst, "Wydruk: %s %s", cData, cGodzina );
   WydrukPrawo( karta.pPage, karta.pFont, 9, 555, 803, cTekst );
   WydrukTekst( karta.pPage, karta.pBold, 16, 40, 760, cTytul, 515 );
   karta.nY = 735;

   // Dane
   KartaSekcja( &karta, "Dane identyfikacyjne" );
   KartaPole( &karta, "Identyfikator NIP", "NIP" );
   KartaPole( &karta, "Nazwa pełna", "PELNA_NAZWA" );
   KartaPole( &karta, "Nazwa skrócona", "SKROCONA_NAZWA" );
   KartaPole( &karta, "Numer REGON", "REGON" );
   KartaPole( &karta, "Numer KRS", "NUMER_KRS" );

   // Adres siedziby firmy
   KartaSekcja( &karta, "Adres siedziby firmy" );
   KartaPole( &karta, "Kraj", "KOD_KRAJU" );
   KartaPole( &karta, "Województwo", "WOJEWODZTWO" );
   KartaPole( &karta, "Powiat", "POWIAT" );
   KartaPole( &karta, "Gmina", "GMINA" );
   KartaPole( &karta, "Ulica", "ULICA" );
   StringFormat( cUlica, "%s", FieldGetC( FieldPos( "NR_DOMU" ) ) );
   StringFormat( cNr, "%s", FieldGetC( FieldPos( "NR_LOKALU" ) ) );
   AllTrim( cUlica );
   AllTrim( cNr );
   StringFormat( cTekst, "%s%s%s", cUlica, IIF( *cNr == 0, "", " / " ), cNr );
   KartaWiersz( &karta, "Nr domu / lokalu", cTekst );
   StringFormat( cUlica, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY" ) ) );
   StringFormat( cMiasto, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
   StringFormat( cTekst, "%s %s", AllTrim( cUlica ), AllTrim( cMiasto ) );
   KartaWiersz( &karta, "Kod / Miejscowość", AllTrim( cTekst ) );
   KartaPole( &karta, "Poczta", "POCZTA" );

   // Dane działalności
   KartaSekcja( &karta, "Dane działalności" );
   KartaData( &karta, "Data rozpoczęcia", "DATA_ROZPOCZECIA" );
   KartaPole( &karta, "Rodzaj działalności", "RODZAJ_DZIALALNOSCI" );
   KartaPole( &karta, "Kod PKD", "KOD_PKD" );
   KartaPole( &karta, "Numer BDO", "NUMER_BDO" );
   KartaPole( &karta, "Organ rejestrowy", "ORGAN_REJESTROWY" );
   KartaPole( &karta, "Nazwa rejestru", "NAZWA_REJESTRU" );
   KartaData( &karta, "Data rejestracji", "DATA_REJESTRACJI" );
   KartaPole( &karta, "Numer w rejestrze", "NUMER_W_REJESTRZE" );

   // Rachunki bankowe
   KartaSekcja( &karta, "Rachunki bankowe" );
   KartaTakNie( &karta, "Rachunek na fakturze", "DRUKUJ_RACHUNEK" );
   KartaRachunek( &karta, "Rachunek pierwszy", 1 );
   KartaRachunek( &karta, "Rachunek drugi", 2 );

   // Dane kontaktowe
   KartaSekcja( &karta, "Dane kontaktowe" );
   KartaPole( &karta, "Telefon", "TELEFON_1" );
   KartaPole( &karta, "Telefon drugi", "TELEFON_2" );
   KartaPole( &karta, "E-mail", "EMAIL" );
   KartaPole( &karta, "Strona internetowa", "STRONA_INTERNETOWA" );

   // Dane do urzędów: urząd z kartoteki US po kodzie
   KartaSekcja( &karta, "Urząd skarbowy" );
   StringFormat( cKod, "%s", FieldGetC( FieldPos( "KOD_URZEDU" ) ) );
   AllTrim( cKod );
   if( UsZnajdz( cKod ) )
   {
      KartaPole( &karta, "Kod urzędu", "KOD_URZEDU" );
      KartaPole( &karta, "Nazwa urzędu", "NAZWA_URZEDU" );
      StringFormat( cUlica, "%s", FieldGetC( FieldPos( "ULICA" ) ) );
      StringFormat( cNr, "%s", FieldGetC( FieldPos( "NR_DOMU" ) ) );
      StringFormat( cTekst, "%s", FieldGetC( FieldPos( "NR_LOKALU" ) ) );
      AllTrim( cUlica );
      AllTrim( cNr );
      AllTrim( cTekst );
      StringFormat( cMiasto, "%s %s%s%s", cUlica, cNr, IIF( *cTekst == 0, "", "/" ), cTekst );
      KartaWiersz( &karta, "Adres", cMiasto );
      StringFormat( cUlica, "%s", FieldGetC( FieldPos( "KOD_POCZTOWY" ) ) );
      StringFormat( cMiasto, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
      StringFormat( cTekst, "%s %s", AllTrim( cUlica ), AllTrim( cMiasto ) );
      KartaWiersz( &karta, "Kod / Miejscowość", AllTrim( cTekst ) );
      DbCloseArea();
      DbSelectArea( cAlias );
   }
   else
      KartaWiersz( &karta, "Kod urzędu", cKod );

   // VAT
   KartaSekcja( &karta, "VAT" );
   KartaTakNie( &karta, "Podatnik VAT", "PODATNIK_VAT" );
   KartaPole( &karta, "Podstawa zwolnienia", "PODSTAWA_ZWOLNIENIA" );

   // Uwagi
   KartaSekcja( &karta, "Uwagi" );
   KartaPole( &karta, "Uwagi", "UWAGI" );

   for( int nStrona = 0; nStrona < karta.nStron; ++nStrona )
   {
      StringFormat( cTekst, "Strona %d z %d", nStrona + 1, karta.nStron );
      WydrukPrawo( karta.aStrony[ nStrona ], karta.pFont, 8, 555, 25, cTekst );
   }

   WydrukPlik( biuro, cNazwa, cSciezka, sizeof( cSciezka ) );

   if( HPDF_SaveToFile( karta.pPdf, cSciezka ) != HPDF_OK )
   {
      StringFormat( cTekst, "Nie można zapisać pliku %s.\nPlik może być otwarty w innym programie.", cSciezka );
      MsgError( cTekst, cTytul );
   }
   else
      FileExecute( cSciezka );

   HPDF_Free( karta.pPdf );
}

// Nowa strona A4 pionowo ze znakiem programu
// KartaStrona( karta ) --> void
static void KartaStrona( Karta *karta )
{
   karta->pPage = HPDF_AddPage( karta->pPdf );
   HPDF_Page_SetSize( karta->pPage, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT );
   WydrukUstawZnaki( karta->pPage, karta->pFont );
   if( karta->nStron < ( int )( sizeof( karta->aStrony ) / sizeof( karta->aStrony[ 0 ] ) ) )
      karta->aStrony[ karta->nStron++ ] = karta->pPage;
   karta->nY = 780;
}

// Tytuł sekcji pogrubiony, jak zakładka okna
// KartaSekcja( karta, cTytul ) --> void
static void KartaSekcja( Karta *karta, const char *cTytul )
{
   if( karta->nY < 80 )
      KartaStrona( karta );

   karta->nY -= 6;
   WydrukTekst( karta->pPage, karta->pBold, 10, 40, karta->nY, cTytul, 515 );
   karta->nY -= 15;
}

// Wiersz: opis i wartość; wartość w kilku liniach (memo) daje kilka wierszy
// KartaWiersz( karta, cOpis, cWartosc ) --> void
static void KartaWiersz( Karta *karta, const char *cOpis, const char *cWartosc )
{
   char        cLinia[ 2100 ];
   char        cBezCr[ 2100 ];
   const char *cKoniec;

   do
   {
      cKoniec = strchr( cWartosc, '\n' );
      if( cKoniec == NULL )
         StringFormat( cLinia, "%s", cWartosc );
      else
         StringFormat( cLinia, "%.*s", ( int )( cKoniec - cWartosc ), cWartosc );
      StrTran( cBezCr, sizeof( cBezCr ), cLinia, "\r", "" );

      if( karta->nY < 50 )
         KartaStrona( karta );

      HPDF_Page_SetRGBFill( karta->pPage, 0.35f, 0.35f, 0.35f );
      WydrukTekst( karta->pPage, karta->pFont, 9, 50, karta->nY, cOpis, 140 );
      HPDF_Page_SetRGBFill( karta->pPage, 0, 0, 0 );
      WydrukTekst( karta->pPage, karta->pFont, 9, 200, karta->nY, RTrim( cBezCr ), 355 );
      karta->nY -= 13;
      cOpis = "";
      cWartosc = IIF( cKoniec == NULL, "", cKoniec + 1 );
   }
   while( cKoniec != NULL );
}

// Wiersz z polem tekstowym bieżącego rekordu
// KartaPole( karta, cOpis, cPole ) --> void
static void KartaPole( Karta *karta, const char *cOpis, const char *cPole )
{
   char cWartosc[ 2100 ];

   StringFormat( cWartosc, "%s", FieldGetC( FieldPos( cPole ) ) );
   KartaWiersz( karta, cOpis, RTrim( cWartosc ) );
}

// Wiersz z datą bieżącego rekordu
// KartaData( karta, cOpis, cPole ) --> void
static void KartaData( Karta *karta, const char *cOpis, const char *cPole )
{
   char cData[ 16 ];

   KartaWiersz( karta, cOpis, DToC( FieldGetD( FieldPos( cPole ) ), cData, sizeof( cData ) ) );
}

// Wiersz Tak / Nie z polem logicznym bieżącego rekordu
// KartaTakNie( karta, cOpis, cPole ) --> void
static void KartaTakNie( Karta *karta, const char *cOpis, const char *cPole )
{
   KartaWiersz( karta, cOpis, IIF( FieldGetL( FieldPos( cPole ) ), "Tak", "Nie" ) );
}

// Rachunek bankowy nRachunek (1 albo 2): numer, bank z krajem, SWIFT; „drukowany”, gdy idzie na fakturę
// KartaRachunek( karta, cOpis, nRachunek ) --> void
static void KartaRachunek( Karta *karta, const char *cOpis, int nRachunek )
{
   char cNumer[ 64 ];
   char cBank[ 300 ];
   char cKraj[ 8 ];
   char cSwift[ 16 ];
   char cPole[ 32 ];
   char cTekst[ 400 ];

   StringFormat( cPole, "NUMER_RACHUNKU_%d", nRachunek );
   StringFormat( cNumer, "%s", FieldGetC( FieldPos( cPole ) ) );
   StringFormat( cPole, "NAZWA_BANKU_%d", nRachunek );
   StringFormat( cBank, "%s", FieldGetC( FieldPos( cPole ) ) );
   StringFormat( cPole, "KRAJ_BANKU_%d", nRachunek );
   StringFormat( cKraj, "%s", FieldGetC( FieldPos( cPole ) ) );
   StringFormat( cPole, "KOD_SWIFT_%d", nRachunek );
   StringFormat( cSwift, "%s", FieldGetC( FieldPos( cPole ) ) );
   AllTrim( cNumer );
   AllTrim( cBank );
   AllTrim( cKraj );
   AllTrim( cSwift );

   StringFormat( cTekst, "%s%s", cNumer, IIF( FieldGetN( FieldPos( "DRUKUJ_RACHUNEK_NR" ) ) == nRachunek || ( nRachunek == 1 && FieldGetN( FieldPos( "DRUKUJ_RACHUNEK_NR" ) ) != 2 ), "  (drukowany)", "" ) );
   KartaWiersz( karta, cOpis, IIF( *cNumer == 0, "", cTekst ) );
   StringFormat( cTekst, "%s%s%s", cBank, IIF( *cKraj == 0, "", ", " ), cKraj );
   KartaWiersz( karta, "Bank", cTekst );
   KartaWiersz( karta, "Kod SWIFT", cSwift );
}

// Liczba całkowita słownie, jak s_Slownie() z ntow.prg
// Slownie( nValue, cWynik, nRozmiar ) --> void
static void Slownie( long long nValue, char *cWynik, size_t nRozmiar )
{
   static const char *aJednosci[]   = { "jeden", "dwa", "trzy", "cztery", "pięć", "sześć", "siedem", "osiem", "dziewięć" };
   static const char *aNascie[]     = { "jedenaście", "dwanaście", "trzynaście", "czternaście", "piętnaście", "szesnaście", "siedemnaście", "osiemnaście", "dziewiętnaście" };
   static const char *aDziesiatki[] = { "dziesięć", "dwadzieścia", "trzydzieści", "czterdzieści", "pięćdziesiąt", "sześćdziesiąt", "siedemdziesiąt", "osiemdziesiąt", "dziewięćdziesiąt" };
   static const char *aSetki[]      = { "sto", "dwieście", "trzysta", "czterysta", "pięćset", "sześćset", "siedemset", "osiemset", "dziewięćset" };
   static const char *aGrupy[][ 3 ] = { { "tysiąc", "tysiące", "tysięcy" }, { "milion", "miliony", "milionów" }, { "miliard", "miliardy", "miliardów" }, { "bilion", "biliony", "bilionów" }, { "biliard", "biliardy", "biliardów" }, { "trylion", "tryliony", "trylionów" } };

   int         nGrupy          = 1;
   char        cSlownie[ 512 ] = "";
   const char *cZnak           = "";
   char        cWartSl[ 256 ];
   char        cReszta[ 512 ];
   int         nWart;
   int         n;

   if( nValue < 0 )
   {
      cZnak  = "minus";
      nValue = -nValue;
   }
   if( nValue == 0 )
      cZnak = "zero";

   while( nValue > 0 )
   {
      nWart = ( int )( nValue % 1000 );

      if( nWart == 1 )
         n = 0;
      else if( nWart % 10 >= 2 && nWart % 10 <= 4 && ( nWart % 100 < 12 || nWart % 100 > 14 ) )
         n = 1;
      else
         n = 2;

      *cWartSl = 0;
      if( nWart >= 100 )
      {
         Dopisz( cWartSl, sizeof( cWartSl ), aSetki[ nWart / 100 - 1 ] );
         nWart %= 100;
      }
      if( nWart >= 11 && nWart <= 19 )
      {
         Dopisz( cWartSl, sizeof( cWartSl ), aNascie[ nWart - 11 ] );
         nWart = 0;
      }
      else if( nWart >= 10 )
      {
         Dopisz( cWartSl, sizeof( cWartSl ), aDziesiatki[ nWart / 10 - 1 ] );
         nWart %= 10;
      }
      if( nWart != 0 )
         Dopisz( cWartSl, sizeof( cWartSl ), aJednosci[ nWart - 1 ] );
      if( *cWartSl != 0 && nGrupy >= 2 )
         Dopisz( cWartSl, sizeof( cWartSl ), aGrupy[ nGrupy - 2 ][ n ] );

      StringFormat( cReszta, "%s", cSlownie );
      StringFormat( cSlownie, "%s%s", cWartSl, cReszta );
      nValue /= 1000;
      ++nGrupy;
   }

   StringFormatSize( cWynik, nRozmiar, "%s%s", cZnak, cSlownie );
   AllTrim( cWynik );
}

// Dopisuje słowo po spacji na końcu tekstu
// Dopisz( cTekst, nRozmiar, cSlowo ) --> void
static void Dopisz( char *cTekst, size_t nRozmiar, const char *cSlowo )
{
   size_t nDlugosc = strlen( cTekst );

   StringFormatSize( cTekst + nDlugosc, nRozmiar - nDlugosc, " %s", cSlowo );
}
