/*
 * Program...: xBiuro
 * Plik......: src/faktura/ksef.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "faktura.h"

#define BILET_SEKUND 600
#define KLUCZE_KSEF  8
#define BILETY_KSEF  8

typedef struct
{
   long  nStatus;
   Json *json;
   Bytes tresc;
} Odpowiedz;

typedef struct
{
   char  cKlucz[ 160 ];
   Bytes cert;
} KluczKsef;

typedef struct
{
   char   cKlucz[ 200 ];
   char  *cBilet;
   double nWazny;
} BiletKsef;

typedef struct
{
   const char *cNip;
   const char *cToken;
   int         nSrodowisko;
   const char *cBilet;
} DostepKsef;

static KluczKsef aKlucze[ KLUCZE_KSEF ];
static int       nKluczy;
static BiletKsef aBilety[ BILETY_KSEF ];
static int       nBiletow;
static Bytes     kluczInstalacji;

static Json       *Bilet( const char *cUrl, const Json *odp, char *cBlad, int nRozmiar );
static Bytes      *KluczInstalacji( void );
static const char *BiletDostepu( const char *cNip, const char *cToken, int nSrodowisko, char *cBlad, int nRozmiar );
static void        ZapomnijBilet( const char *cNip, const char *cToken, int nSrodowisko );
static char       *KluczBiletu( const char *cNip, const char *cToken, int nSrodowisko, char *cKlucz, int nRozmiar );
static double      Teraz( void );
static xBool       Dostep( const char *cNip, const char *cToken, int nSrodowisko, DostepKsef *dostep, char *cBlad, int nRozmiar );
static xBool       ZapytanieDostep( const char *cMetoda, const char *cUrl, const char *cTresc, DostepKsef *dostep, xBool lXml, Odpowiedz *odp, char *cBlad, int nRozmiar );
static void        ZamknijSesje( const char *cUrl, const char *cSesja, DostepKsef *dostep );
static xBool       Status( const char *cUrl, DostepKsef *dostep, const char *cSesja, const char *cFaktura, int nProb, WynikKsef *wynik, char *cBlad, int nRozmiar );
static void        Nieoczekiwana( char *cBlad, int nRozmiar );
static const char *Adres( int nSrodowisko );
static Bytes      *Klucz( const char *cUrl, const char *cUzycie, char *cBlad, int nRozmiar );
static char       *Rsa( Bytes cert, const void *dane, size_t nDane, char *cBlad, int nRozmiar );
static xBool       Zapytanie( const char *cMetoda, const char *cUrl, const char *cTresc, const char *cBilet, xBool lXml, const char *cTypTresci, Odpowiedz *odp, char *cBlad, int nRozmiar );
static void        OdpowiedzFree( Odpowiedz *odp );
static char       *OpisBledu( const Odpowiedz *odp, char *cOpis, int nRozmiar );
static char       *Kopia( const char *cTekst );

Json *KsefZaloguj( const char *cNip, const char *cToken, int nSrodowisko, char *cBlad, int nRozmiar )
{
   const char *cUrl = Adres( nSrodowisko );
   Odpowiedz   wyzwanie;
   Odpowiedz   odp;
   Bytes      *cert;
   Json       *bilety;
   char       *cSzyfr;
   char       *cChallenge;
   char       *cWartosc;
   char       *cTresc;
   char        cAdres[ 200 ];
   char        cDane[ 700 ];
   char        cTokenBezSpacji[ 600 ];
   char        cNipBezSpacji[ 64 ];
   size_t      nTresc;
   xBool       lJest;

   *cBlad = 0;
   StringFormat( cAdres, "%s/auth/challenge", cUrl );
   if( !Zapytanie( "POST", cAdres, "{}", NULL, F, NULL, &wyzwanie, cBlad, nRozmiar ) )
      return NULL;

   if( JsonString( JsonGet( wyzwanie.json, "challenge" ) ) == NULL || JsonType( JsonGet( wyzwanie.json, "timestampMs" ) ) != JSON_NUMBER )
   {
      Nieoczekiwana( cBlad, nRozmiar );
      OdpowiedzFree( &wyzwanie );

      return NULL;
   }

   cert = Klucz( cUrl, "KsefTokenEncryption", cBlad, nRozmiar );
   if( cert == NULL )
   {
      OdpowiedzFree( &wyzwanie );

      return NULL;
   }

   StringFormat( cTokenBezSpacji, "%s", cToken );
   StringFormat( cDane, "%s|%.0f", AllTrim( cTokenBezSpacji ), JsonNumber( JsonGet( wyzwanie.json, "timestampMs" ) ) );
   cSzyfr = Rsa( *cert, cDane, strlen( cDane ), cBlad, nRozmiar );
   if( cSzyfr == NULL )
   {
      OdpowiedzFree( &wyzwanie );

      return NULL;
   }

   StringFormat( cNipBezSpacji, "%s", cNip );
   cChallenge = JsonQuote( JsonString( JsonGet( wyzwanie.json, "challenge" ) ) );
   cWartosc   = JsonQuote( AllTrim( cNipBezSpacji ) );
   nTresc     = strlen( cChallenge ) + strlen( cWartosc ) + strlen( cSzyfr ) + 200;
   cTresc     = ( char * )malloc( nTresc );
   StringFormatSize( cTresc, nTresc, "{\"challenge\":%s,\"contextIdentifier\":{\"type\":\"Nip\",\"value\":%s},\"encryptedToken\":\"%s\"}", cChallenge, cWartosc, cSzyfr );
   free( cChallenge );
   free( cWartosc );
   free( cSzyfr );
   OdpowiedzFree( &wyzwanie );

   StringFormat( cAdres, "%s/auth/ksef-token", cUrl );
   lJest = Zapytanie( "POST", cAdres, cTresc, NULL, F, NULL, &odp, cBlad, nRozmiar );
   free( cTresc );
   if( !lJest )
      return NULL;

   bilety = Bilet( cUrl, odp.json, cBlad, nRozmiar );
   OdpowiedzFree( &odp );

   return bilety;
}

Json *KsefZalogujXades( const char *cNip, Bytes klucz, Bytes cert, const char *cHaslo, int nSrodowisko, char *cBlad, int nRozmiar )
{
   const char *cUrl = Adres( nSrodowisko );
   Odpowiedz   wyzwanie;
   Odpowiedz   odp;
   Json       *bilety;
   char       *cPodpisany;
   char        cAdres[ 200 ];
   char        cXml[ 2000 ];
   xBool       lJest;

   *cBlad = 0;
   StringFormat( cAdres, "%s/auth/challenge", cUrl );
   if( !Zapytanie( "POST", cAdres, "{}", NULL, F, NULL, &wyzwanie, cBlad, nRozmiar ) )
      return NULL;

   if( JsonString( JsonGet( wyzwanie.json, "challenge" ) ) == NULL )
   {
      Nieoczekiwana( cBlad, nRozmiar );
      OdpowiedzFree( &wyzwanie );

      return NULL;
   }

   StringFormat( cXml, "<?xml version=\"1.0\" encoding=\"utf-8\"?><AuthTokenRequest xmlns=\"http://ksef.mf.gov.pl/auth/token/2.0\"><Challenge>%s</Challenge><ContextIdentifier><Nip>%s</Nip></ContextIdentifier><SubjectIdentifierType>certificateSubject</SubjectIdentifierType></AuthTokenRequest>", JsonString( JsonGet( wyzwanie.json, "challenge" ) ), cNip );
   OdpowiedzFree( &wyzwanie );

   cPodpisany = XadesSign( cXml, strlen( cXml ), klucz, cert, cHaslo );
   if( cPodpisany == NULL )
   {
      StringFormatSize( cBlad, ( size_t )nRozmiar, "Nie można podpisać żądania certyfikatem:\n%s", XadesError() );

      return NULL;
   }

   StringFormat( cAdres, "%s/auth/xades-signature", cUrl );
   lJest = Zapytanie( "POST", cAdres, cPodpisany, NULL, F, "application/xml", &odp, cBlad, nRozmiar );
   free( cPodpisany );
   if( !lJest )
   {
      OdpowiedzFree( &odp );

      return NULL;
   }

   bilety = Bilet( cUrl, odp.json, cBlad, nRozmiar );
   OdpowiedzFree( &odp );

   return bilety;
}

xBool KsefGenerujToken( const char *cNip, const Certyfikat *cert, int nSrodowisko, const char *cOpis, const char *cUprawnienia, int dWaznyDo, TokenKsef *token, char *cBlad, int nRozmiar )
{
   const char *cUrl = Adres( nSrodowisko );
   const char *cBilet;
   const char *cKod;
   const char *cNazwa;
   Odpowiedz   odp;
   Json       *bilety;
   Bytes       klucz;
   char       *cOpisJson;
   char       *cTresc;
   size_t      nTresc;
   char        cAdres[ 300 ];
   xBool       lJest;

   memset( token, 0, sizeof( *token ) );
   klucz.data = ( unsigned char * )cert->cKlucz;
   klucz.size = strlen( cert->cKlucz );
   bilety     = KsefZalogujXades( cNip, klucz, cert->der, cert->cHaslo, nSrodowisko, cBlad, nRozmiar );
   if( bilety == NULL )
      return F;

   cBilet    = JsonString( JsonGet( JsonGet( bilety, "accessToken" ), "token" ) );
   cOpisJson = JsonQuote( cOpis );
   nTresc    = strlen( cOpisJson ) + strlen( cUprawnienia ) + 200;
   cTresc    = ( char * )malloc( nTresc );
   StringFormatSize( cTresc, nTresc, "{\"permissions\":[%s],\"description\":%s,\"validTo\":\"%04d-%02d-%02dT23:59:59Z\"}", cUprawnienia, cOpisJson, dWaznyDo / 10000, dWaznyDo / 100 % 100, dWaznyDo % 100 );
   free( cOpisJson );

   StringFormat( cAdres, "%s/tokens/rev2", cUrl );
   lJest = Zapytanie( "POST", cAdres, cTresc, cBilet, F, NULL, &odp, cBlad, nRozmiar );
   free( cTresc );
   if( lJest && ( JsonString( JsonGet( odp.json, "token" ) ) == NULL || JsonString( JsonGet( odp.json, "referenceNumber" ) ) == NULL ) )
   {
      Nieoczekiwana( cBlad, nRozmiar );
      lJest = F;
   }

   if( !lJest )
   {
      OdpowiedzFree( &odp );
      KsefWyloguj( nSrodowisko, bilety );
      JsonFree( bilety );

      return F;
   }

   token->cToken = Kopia( JsonString( JsonGet( odp.json, "token" ) ) );
   StringFormat( token->cNumer, "%s", JsonString( JsonGet( odp.json, "referenceNumber" ) ) );
   OdpowiedzFree( &odp );

   StringFormat( cAdres, "%s/tokens/rev2/%s", cUrl, token->cNumer );
   for( int nProba = 0; nProba < 10; ++nProba )
   {
      IdleSleep( 1 );
      if( !Zapytanie( "GET", cAdres, NULL, cBilet, F, NULL, &odp, token->cStatus, sizeof( token->cStatus ) ) )
      {
         OdpowiedzFree( &odp );
         break;
      }

      cKod   = JsonString( JsonGet( JsonGet( odp.json, "status" ), "code" ) );
      cNazwa = JsonString( JsonGet( JsonGet( odp.json, "status" ), "displayName" ) );
      StringFormat( token->cStatus, "%s", IIF( cNazwa == NULL, IIF( cKod == NULL, "", cKod ), cNazwa ) );
      lJest = IIF( cKod != NULL && strcmp( cKod, "Pending" ) == 0, T, F );
      OdpowiedzFree( &odp );
      if( !lJest )
         break;
   }

   KsefWyloguj( nSrodowisko, bilety );
   JsonFree( bilety );

   return T;
}

void KsefWyloguj( int nSrodowisko, const Json *bilety )
{
   const char *cBilet = JsonString( JsonGet( JsonGet( bilety, "accessToken" ), "token" ) );
   Odpowiedz   odp;
   char        cAdres[ 200 ];
   char        cBlad[ 300 ];

   if( cBilet == NULL )
      return;

   StringFormat( cAdres, "%s/auth/sessions/current", Adres( nSrodowisko ) );
   Zapytanie( "DELETE", cAdres, NULL, cBilet, F, NULL, &odp, cBlad, sizeof( cBlad ) );
   OdpowiedzFree( &odp );
}

xBool KsefWyslij( const char *cNip, const char *cToken, int nSrodowisko, const char *cXml, size_t nXml, xBool lOffline, KsefPrzyjeta przyjeta, void *data, WynikKsef *wynik, char *cBlad, int nRozmiar )
{
   const char *cUrl = Adres( nSrodowisko );
   DostepKsef  dostep;
   Odpowiedz   odp;
   Bytes      *cert;
   Bytes       klucz;
   Bytes       iv;
   Bytes       zaszyfrowana;
   Bytes       skrot;
   Bytes       skrotZaszyfrowanej;
   char       *cSzyfrKlucza;
   char       *cIv;
   char       *cSkrot;
   char       *cSkrotZaszyfrowanej;
   char       *cZawartosc;
   char       *cTresc;
   char        cNaglowek[ 2000 ];
   char        cAdres[ 300 ];
   char        cSesja[ 80 ];
   char        cFaktura[ 80 ];
   size_t      nTresc;
   xBool       lJest;

   memset( wynik, 0, sizeof( *wynik ) );
   *cBlad = 0;
   if( !Dostep( cNip, cToken, nSrodowisko, &dostep, cBlad, nRozmiar ) )
      return F;

   cert = Klucz( cUrl, "SymmetricKeyEncryption", cBlad, nRozmiar );
   if( cert == NULL )
      return F;

   klucz        = RandomBytes( 32 );
   iv           = RandomBytes( 16 );
   cSzyfrKlucza = Rsa( *cert, klucz.data, klucz.size, cBlad, nRozmiar );
   if( cSzyfrKlucza == NULL )
   {
      BytesFree( &klucz );
      BytesFree( &iv );

      return F;
   }

   cIv = Base64Encode( iv.data, iv.size );
   StringFormat( cNaglowek, "{\"formCode\":{\"systemCode\":\"FA (3)\",\"schemaVersion\":\"1-0E\",\"value\":\"FA\"},\"encryption\":{\"encryptedSymmetricKey\":\"%s\",\"initializationVector\":\"%s\"}}", cSzyfrKlucza, cIv );
   free( cSzyfrKlucza );
   free( cIv );

   StringFormat( cAdres, "%s/sessions/online", cUrl );
   if( !ZapytanieDostep( "POST", cAdres, cNaglowek, &dostep, F, &odp, cBlad, nRozmiar ) )
   {
      OdpowiedzFree( &odp );
      BytesFree( &klucz );
      BytesFree( &iv );

      return F;
   }

   if( JsonString( JsonGet( odp.json, "referenceNumber" ) ) == NULL )
   {
      Nieoczekiwana( cBlad, nRozmiar );
      OdpowiedzFree( &odp );
      BytesFree( &klucz );
      BytesFree( &iv );

      return F;
   }
   StringFormat( cSesja, "%s", JsonString( JsonGet( odp.json, "referenceNumber" ) ) );
   OdpowiedzFree( &odp );

   zaszyfrowana = AesEncrypt( klucz, iv, cXml, nXml, AES_MODE_CBC );
   BytesFree( &klucz );
   BytesFree( &iv );
   if( zaszyfrowana.data == NULL )
   {
      StringFormatSize( cBlad, ( size_t )nRozmiar, "Nie można zaszyfrować faktury:\n%s", CryptoError() );
      ZamknijSesje( cUrl, cSesja, &dostep );

      return F;
   }

   skrot               = HashData( "SHA256", cXml, nXml );
   skrotZaszyfrowanej  = HashData( "SHA256", zaszyfrowana.data, zaszyfrowana.size );
   cSkrot              = Base64Encode( skrot.data, skrot.size );
   cSkrotZaszyfrowanej = Base64Encode( skrotZaszyfrowanej.data, skrotZaszyfrowanej.size );
   cZawartosc          = Base64Encode( zaszyfrowana.data, zaszyfrowana.size );
   nTresc              = strlen( cZawartosc ) + 500;
   cTresc              = ( char * )malloc( nTresc );
   StringFormatSize( cTresc, nTresc, "{\"invoiceHash\":\"%s\",\"invoiceSize\":%zu,\"encryptedInvoiceHash\":\"%s\",\"encryptedInvoiceSize\":%zu,\"encryptedInvoiceContent\":\"%s\",\"offlineMode\":%s}", cSkrot, nXml, cSkrotZaszyfrowanej, zaszyfrowana.size, cZawartosc, IIF( lOffline, "true", "false" ) );
   free( cSkrot );
   free( cSkrotZaszyfrowanej );
   free( cZawartosc );
   BytesFree( &skrot );
   BytesFree( &skrotZaszyfrowanej );
   BytesFree( &zaszyfrowana );

   StringFormat( cAdres, "%s/sessions/online/%s/invoices", cUrl, cSesja );
   lJest = ZapytanieDostep( "POST", cAdres, cTresc, &dostep, F, &odp, cBlad, nRozmiar );
   free( cTresc );
   if( !lJest || JsonString( JsonGet( odp.json, "referenceNumber" ) ) == NULL )
   {
      if( lJest )
         Nieoczekiwana( cBlad, nRozmiar );
      OdpowiedzFree( &odp );
      ZamknijSesje( cUrl, cSesja, &dostep );

      return F;
   }
   StringFormat( cFaktura, "%s", JsonString( JsonGet( odp.json, "referenceNumber" ) ) );
   OdpowiedzFree( &odp );

   przyjeta( cSesja, cFaktura, data );
   ZamknijSesje( cUrl, cSesja, &dostep );

   return Status( cUrl, &dostep, cSesja, cFaktura, 40, wynik, cBlad, nRozmiar );
}

xBool KsefSprawdz( const char *cNip, const char *cToken, int nSrodowisko, const char *cSesja, const char *cFaktura, WynikKsef *wynik, char *cBlad, int nRozmiar )
{
   DostepKsef dostep;

   memset( wynik, 0, sizeof( *wynik ) );
   *cBlad = 0;
   if( !Dostep( cNip, cToken, nSrodowisko, &dostep, cBlad, nRozmiar ) )
      return F;

   return Status( Adres( nSrodowisko ), &dostep, cSesja, cFaktura, 5, wynik, cBlad, nRozmiar );
}

void KsefWynikFree( WynikKsef *wynik )
{
   JsonFree( wynik->json );
   free( wynik->cUpo );
   memset( wynik, 0, sizeof( *wynik ) );
}

int KsefKod( const Json *odp )
{
   const Json *kod = JsonGet( JsonGet( odp, "status" ), "code" );

   return IIF( JsonType( kod ) == JSON_NUMBER, ( int )JsonNumber( kod ), -1 );
}

char *KsefOpisStatusu( const Json *odp, char *cOpis, int nRozmiar )
{
   const Json *status    = JsonGet( odp, "status" );
   const Json *szczegoly = JsonGet( status, "details" );
   const char *cTekst;
   size_t      nDlugosc;

   StringFormatSize( cOpis, ( size_t )nRozmiar, "%d", KsefKod( odp ) );
   if( JsonString( JsonGet( status, "description" ) ) != NULL )
   {
      nDlugosc = strlen( cOpis );
      StringFormatSize( cOpis + nDlugosc, ( size_t )nRozmiar - nDlugosc, " %s", JsonString( JsonGet( status, "description" ) ) );
   }

   for( int i = 1; i <= JsonCount( szczegoly ); ++i )
   {
      cTekst = JsonString( JsonItem( szczegoly, i ) );
      if( cTekst != NULL )
      {
         nDlugosc = strlen( cOpis );
         StringFormatSize( cOpis + nDlugosc, ( size_t )nRozmiar - nDlugosc, "\n%s", cTekst );
      }
   }

   return cOpis;
}

const char *KsefSrodowisko( int nSrodowisko, xBool lPelna )
{
   if( nSrodowisko == SRODOWISKO_PRODUKCYJNE )
      return IIF( lPelna, "Produkcyjne (prawdziwe faktury)", "Produkcyjne" );

   return IIF( lPelna, "Testowe (próbny, do testów)", "Testowe" );
}

char *KsefWaznosc( int dDo, char *cWynik, int nRozmiar )
{
   int  nDni = DateDiff( dDo, Date() );
   char cData[ 16 ];

   if( dDo == 0 )
      *cWynik = 0;
   else if( nDni < 0 )
      StringFormatSize( cWynik, ( size_t )nRozmiar, "Wygasł %s", DToC( dDo, cData, sizeof( cData ) ) );
   else if( nDni == 0 )
      StringFormatSize( cWynik, ( size_t )nRozmiar, "Wygasa dzisiaj" );
   else if( nDni <= DNI_OSTRZEZENIA )
      StringFormatSize( cWynik, ( size_t )nRozmiar, "Wygasa za %d %s", nDni, IIF( nDni == 1, "dzień", "dni" ) );
   else
      StringFormatSize( cWynik, ( size_t )nRozmiar, "Ważny" );

   return cWynik;
}

void KsefAplikacja( int nSrodowisko, const char *cTytul )
{
   const char *cAdres = IIF( nSrodowisko == SRODOWISKO_PRODUKCYJNE, "https://ap.ksef.mf.gov.pl", "https://ap-test.ksef.mf.gov.pl" );
   char        cTekst[ 200 ];

   if( !FileExecute( cAdres ) )
   {
      StringFormat( cTekst, "Otwórz w przeglądarce:\n%s", cAdres );
      MsgInfo( cTekst, cTytul );
   }
}

char *KsefZaszyfruj( const char *cDane )
{
   Bytes *klucz = KluczInstalacji();
   Bytes  iv;
   Bytes  szyfr;
   Bytes  razem;
   char  *cWynik;

   if( klucz == NULL )
      return NULL;

   iv    = RandomBytes( 12 );
   szyfr = AesEncrypt( *klucz, iv, cDane, strlen( cDane ), AES_MODE_GCM );
   if( szyfr.data == NULL )
   {
      BytesFree( &iv );

      return NULL;
   }

   razem.size = iv.size + szyfr.size;
   razem.data = ( unsigned char * )malloc( razem.size );
   memcpy( razem.data, iv.data, iv.size );
   memcpy( razem.data + iv.size, szyfr.data, szyfr.size );
   cWynik = Base64Encode( razem.data, razem.size );
   BytesFree( &iv );
   BytesFree( &szyfr );
   BytesFree( &razem );

   return cWynik;
}

char *KsefOdszyfruj( const char *cZapis )
{
   Bytes *klucz = KluczInstalacji();
   Bytes  dane;
   Bytes  iv;
   Bytes  tekst;
   char  *cTekst = Kopia( cZapis );

   dane = Base64Decode( AllTrim( cTekst ) );
   free( cTekst );
   if( klucz == NULL || dane.size <= 12 + 16 )
   {
      BytesFree( &dane );

      return Kopia( "" );
   }

   iv.data = dane.data;
   iv.size = 12;
   tekst   = AesDecrypt( *klucz, iv, dane.data + 12, dane.size - 12, AES_MODE_GCM );
   BytesFree( &dane );

   return IIF( tekst.data == NULL, Kopia( "" ), ( char * )tekst.data );
}

static Json *Bilet( const char *cUrl, const Json *odp, char *cBlad, int nRozmiar )
{
   const char *cNumer = JsonString( JsonGet( odp, "referenceNumber" ) );
   const char *cAuth  = JsonString( JsonGet( JsonGet( odp, "authenticationToken" ), "token" ) );
   Odpowiedz   status;
   Json       *bilety;
   char        cAdres[ 300 ];
   int         nKod   = -1;

   if( cNumer == NULL || cAuth == NULL )
   {
      Nieoczekiwana( cBlad, nRozmiar );

      return NULL;
   }

   StringFormat( cAdres, "%s/auth/%s", cUrl, cNumer );
   for( int nProba = 0; nProba < 20; ++nProba )
   {
      if( !Zapytanie( "GET", cAdres, NULL, cAuth, F, NULL, &status, cBlad, nRozmiar ) )
      {
         OdpowiedzFree( &status );

         return NULL;
      }

      nKod = KsefKod( status.json );
      if( nKod != 100 )
      {
         if( nKod != 200 )
            KsefOpisStatusu( status.json, cBlad, nRozmiar );
         OdpowiedzFree( &status );
         break;
      }
      OdpowiedzFree( &status );
      IdleSleep( 0.5 );
   }

   if( nKod == 100 )
      StringFormatSize( cBlad, ( size_t )nRozmiar, "KSeF nie odpowiedział w czasie, spróbuj ponownie." );
   if( nKod != 200 )
      return NULL;

   StringFormat( cAdres, "%s/auth/token/redeem", cUrl );
   if( !Zapytanie( "POST", cAdres, "", cAuth, F, NULL, &status, cBlad, nRozmiar ) )
   {
      OdpowiedzFree( &status );

      return NULL;
   }

   if( JsonString( JsonGet( JsonGet( status.json, "accessToken" ), "token" ) ) == NULL )
   {
      Nieoczekiwana( cBlad, nRozmiar );
      OdpowiedzFree( &status );

      return NULL;
   }

   bilety      = status.json;
   status.json = NULL;
   OdpowiedzFree( &status );

   return bilety;
}

static Bytes *KluczInstalacji( void )
{
   char   cKatalog[ 300 ];
   char   cPlik[ 320 ];
   char  *cKlucz;
   size_t nKlucz;

   if( kluczInstalacji.data != NULL )
      return &kluczInstalacji;

   if( *GetEnv( "APPDATA" ) == 0 )
      return NULL;

   StringFormat( cKatalog, "%s/xBiuro", GetEnv( "APPDATA" ) );
   StringFormat( cPlik, "%s/klucz.bin", cKatalog );
   cKlucz = MemoRead( cPlik, &nKlucz );
   if( cKlucz != NULL && nKlucz == 32 )
   {
      kluczInstalacji.data = ( unsigned char * )cKlucz;
      kluczInstalacji.size = 32;

      return &kluczInstalacji;
   }
   free( cKlucz );

   if( FileExists( cPlik ) )
      return NULL;

   kluczInstalacji = RandomBytes( 32 );
   if( !DirExists( cKatalog ) )
      DirCreate( cKatalog );

   if( !MemoWrit( cPlik, kluczInstalacji.data, kluczInstalacji.size ) )
   {
      FErase( cPlik );
      BytesFree( &kluczInstalacji );

      return NULL;
   }

   return &kluczInstalacji;
}

static const char *BiletDostepu( const char *cNip, const char *cToken, int nSrodowisko, char *cBlad, int nRozmiar )
{
   Json *bilety;
   char  cKlucz[ 200 ];
   int   nBilet;

   KluczBiletu( cNip, cToken, nSrodowisko, cKlucz, sizeof( cKlucz ) );
   for( nBilet = 0; nBilet < nBiletow && strcmp( aBilety[ nBilet ].cKlucz, cKlucz ) != 0; ++nBilet )
   {
   }

   if( nBilet < nBiletow && aBilety[ nBilet ].nWazny > Teraz() )
      return aBilety[ nBilet ].cBilet;

   bilety = KsefZaloguj( cNip, cToken, nSrodowisko, cBlad, nRozmiar );
   if( bilety == NULL )
      return NULL;

   if( nBilet == nBiletow )
   {
      if( nBiletow < BILETY_KSEF )
         ++nBiletow;
      else
         nBilet = 0;
   }

   free( aBilety[ nBilet ].cBilet );
   StringFormat( aBilety[ nBilet ].cKlucz, "%s", cKlucz );
   aBilety[ nBilet ].cBilet = Kopia( JsonString( JsonGet( JsonGet( bilety, "accessToken" ), "token" ) ) );
   aBilety[ nBilet ].nWazny = Teraz() + BILET_SEKUND;
   JsonFree( bilety );

   return aBilety[ nBilet ].cBilet;
}

static void ZapomnijBilet( const char *cNip, const char *cToken, int nSrodowisko )
{
   char cKlucz[ 200 ];

   KluczBiletu( cNip, cToken, nSrodowisko, cKlucz, sizeof( cKlucz ) );
   for( int nBilet = 0; nBilet < nBiletow; ++nBilet )
   {
      if( strcmp( aBilety[ nBilet ].cKlucz, cKlucz ) == 0 )
         aBilety[ nBilet ].nWazny = 0;
   }
}

static char *KluczBiletu( const char *cNip, const char *cToken, int nSrodowisko, char *cKlucz, int nRozmiar )
{
   Bytes skrot;
   char *cSkrot;
   char *cTekst = Kopia( cToken );
   char  cNipBezSpacji[ 64 ];

   AllTrim( cTekst );
   skrot  = HashData( "SHA256", cTekst, strlen( cTekst ) );
   cSkrot = Base64Encode( skrot.data, skrot.size );
   StringFormat( cNipBezSpacji, "%s", cNip );
   StringFormatSize( cKlucz, ( size_t )nRozmiar, "%s|%d|%s", AllTrim( cNipBezSpacji ), nSrodowisko, cSkrot );
   free( cSkrot );
   free( cTekst );
   BytesFree( &skrot );

   return cKlucz;
}

static double Teraz( void )
{
   return ( double )Date() * 86400 + Seconds();
}

static xBool Dostep( const char *cNip, const char *cToken, int nSrodowisko, DostepKsef *dostep, char *cBlad, int nRozmiar )
{
   dostep->cNip        = cNip;
   dostep->cToken      = cToken;
   dostep->nSrodowisko = nSrodowisko;
   dostep->cBilet      = BiletDostepu( cNip, cToken, nSrodowisko, cBlad, nRozmiar );

   return IIF( dostep->cBilet == NULL, F, T );
}

static xBool ZapytanieDostep( const char *cMetoda, const char *cUrl, const char *cTresc, DostepKsef *dostep, xBool lXml, Odpowiedz *odp, char *cBlad, int nRozmiar )
{
   if( Zapytanie( cMetoda, cUrl, cTresc, dostep->cBilet, lXml, NULL, odp, cBlad, nRozmiar ) )
      return T;

   if( odp->nStatus != 401 )
      return F;

   OdpowiedzFree( odp );
   ZapomnijBilet( dostep->cNip, dostep->cToken, dostep->nSrodowisko );
   dostep->cBilet = BiletDostepu( dostep->cNip, dostep->cToken, dostep->nSrodowisko, cBlad, nRozmiar );
   if( dostep->cBilet == NULL )
      return F;

   return Zapytanie( cMetoda, cUrl, cTresc, dostep->cBilet, lXml, NULL, odp, cBlad, nRozmiar );
}

static void ZamknijSesje( const char *cUrl, const char *cSesja, DostepKsef *dostep )
{
   Odpowiedz odp;
   char      cAdres[ 300 ];
   char      cBlad[ 300 ];

   StringFormat( cAdres, "%s/sessions/online/%s/close", cUrl, cSesja );
   ZapytanieDostep( "POST", cAdres, "", dostep, F, &odp, cBlad, sizeof( cBlad ) );
   OdpowiedzFree( &odp );
}

static xBool Status( const char *cUrl, DostepKsef *dostep, const char *cSesja, const char *cFaktura, int nProb, WynikKsef *wynik, char *cBlad, int nRozmiar )
{
   Odpowiedz odp;
   char      cAdres[ 300 ];
   int       nKod = -1;

   memset( &odp, 0, sizeof( odp ) );
   StringFormat( cAdres, "%s/sessions/%s/invoices/%s", cUrl, cSesja, cFaktura );
   for( int nProba = 0; nProba < nProb; ++nProba )
   {
      OdpowiedzFree( &odp );
      if( !ZapytanieDostep( "GET", cAdres, NULL, dostep, F, &odp, cBlad, nRozmiar ) )
      {
         OdpowiedzFree( &odp );

         return F;
      }

      nKod = KsefKod( odp.json );
      if( nKod == -1 )
      {
         Nieoczekiwana( cBlad, nRozmiar );
         OdpowiedzFree( &odp );

         return F;
      }

      if( nKod != 100 && nKod != 150 )
         break;

      IdleSleep( 1 );
   }

   wynik->json = odp.json;
   odp.json    = NULL;
   OdpowiedzFree( &odp );
   StringFormat( wynik->cSesja, "%s", cSesja );
   StringFormat( wynik->cFaktura, "%s", cFaktura );

   if( nKod == 200 )
   {
      if( JsonString( JsonGet( wynik->json, "ksefNumber" ) ) == NULL )
      {
         Nieoczekiwana( cBlad, nRozmiar );
         KsefWynikFree( wynik );

         return F;
      }

      StringFormat( cAdres, "%s/sessions/%s/invoices/%s/upo", cUrl, cSesja, cFaktura );
      for( int nProba = 0; nProba < 10; ++nProba )
      {
         if( ZapytanieDostep( "GET", cAdres, NULL, dostep, T, &odp, cBlad, nRozmiar ) )
         {
            wynik->cUpo    = ( char * )odp.tresc.data;
            wynik->nUpo    = odp.tresc.size;
            odp.tresc.data = NULL;
            OdpowiedzFree( &odp );
            break;
         }
         OdpowiedzFree( &odp );
         IdleSleep( 1 );
      }
      *cBlad = 0;
   }

   return T;
}

static void Nieoczekiwana( char *cBlad, int nRozmiar )
{
   StringFormatSize( cBlad, ( size_t )nRozmiar, "Nieoczekiwana odpowiedź KSeF, spróbuj ponownie później." );
}

static const char *Adres( int nSrodowisko )
{
   return IIF( nSrodowisko == SRODOWISKO_PRODUKCYJNE, "https://api.ksef.mf.gov.pl/v2", "https://api-test.ksef.mf.gov.pl/v2" );
}

static Bytes *Klucz( const char *cUrl, const char *cUzycie, char *cBlad, int nRozmiar )
{
   Odpowiedz   odp;
   const Json *cert;
   const Json *uzycie;
   char        cKlucz[ 160 ];
   char        cAdres[ 200 ];

   StringFormat( cKlucz, "%s|%s", cUrl, cUzycie );
   for( int i = 0; i < nKluczy; ++i )
   {
      if( strcmp( aKlucze[ i ].cKlucz, cKlucz ) == 0 )
         return &aKlucze[ i ].cert;
   }

   StringFormat( cAdres, "%s/security/public-key-certificates", cUrl );
   if( !Zapytanie( "GET", cAdres, NULL, NULL, F, NULL, &odp, cBlad, nRozmiar ) )
   {
      OdpowiedzFree( &odp );

      return NULL;
   }

   if( JsonType( odp.json ) != JSON_ARRAY )
   {
      Nieoczekiwana( cBlad, nRozmiar );
      OdpowiedzFree( &odp );

      return NULL;
   }

   for( int i = 1; i <= JsonCount( odp.json ) && nKluczy < KLUCZE_KSEF; ++i )
   {
      cert   = JsonItem( odp.json, i );
      uzycie = JsonGet( cert, "usage" );
      for( int j = 1; j <= JsonCount( uzycie ); ++j )
      {
         if( JsonString( JsonItem( uzycie, j ) ) != NULL && strcmp( JsonString( JsonItem( uzycie, j ) ), cUzycie ) == 0 && JsonString( JsonGet( cert, "certificate" ) ) != NULL )
         {
            StringFormat( aKlucze[ nKluczy ].cKlucz, "%s", cKlucz );
            aKlucze[ nKluczy ].cert = Base64Decode( JsonString( JsonGet( cert, "certificate" ) ) );
            OdpowiedzFree( &odp );

            return &aKlucze[ nKluczy++ ].cert;
         }
      }
   }

   OdpowiedzFree( &odp );
   StringFormatSize( cBlad, ( size_t )nRozmiar, "Brak certyfikatu KSeF: %s", cUzycie );

   return NULL;
}

static char *Rsa( Bytes cert, const void *dane, size_t nDane, char *cBlad, int nRozmiar )
{
   Bytes szyfr = RsaEncrypt( cert, dane, nDane, RSA_PADDING_OAEP_SHA256 );
   char *cWynik;

   if( szyfr.data == NULL )
   {
      StringFormatSize( cBlad, ( size_t )nRozmiar, "Nie można zaszyfrować danych certyfikatem KSeF:\n%s", CryptoError() );

      return NULL;
   }

   cWynik = Base64Encode( szyfr.data, szyfr.size );
   BytesFree( &szyfr );

   return cWynik;
}

static xBool Zapytanie( const char *cMetoda, const char *cUrl, const char *cTresc, const char *cBilet, xBool lXml, const char *cTypTresci, Odpowiedz *odp, char *cBlad, int nRozmiar )
{
   const char *aNaglowki[ 4 ];
   char       *cAutoryzacja = NULL;
   char        cTyp[ 100 ];
   char        cOpis[ 1000 ];
   int         nNaglowkow   = 0;
   size_t      nAutoryzacja;
   xBool       lJest;

   memset( odp, 0, sizeof( *odp ) );
   aNaglowki[ nNaglowkow++ ] = IIF( lXml, "Accept: application/xml", "Accept: application/json" );
   if( cTresc != NULL )
   {
      StringFormat( cTyp, "Content-Type: %s", IIF( cTypTresci == NULL, "application/json", cTypTresci ) );
      aNaglowki[ nNaglowkow++ ] = cTyp;
   }
   if( cBilet != NULL )
   {
      nAutoryzacja = strlen( cBilet ) + 32;
      cAutoryzacja = ( char * )malloc( nAutoryzacja );
      StringFormatSize( cAutoryzacja, nAutoryzacja, "Authorization: Bearer %s", cBilet );
      aNaglowki[ nNaglowkow++ ] = cAutoryzacja;
   }
   aNaglowki[ nNaglowkow ] = NULL;

   lJest = HttpRequest( cMetoda, cUrl, aNaglowki, cTresc, IIF( cTresc == NULL, 0, strlen( cTresc ) ), 30, &odp->tresc, &odp->nStatus );
   free( cAutoryzacja );
   if( !lJest )
   {
      StringFormatSize( cBlad, ( size_t )nRozmiar, "Brak połączenia z KSeF: %s", HttpError() );

      return F;
   }

   if( lXml )
      return IIF( odp->nStatus == 200 && odp->tresc.size > 0, T, F );

   if( strcmp( cMetoda, "DELETE" ) == 0 )
      return IIF( odp->nStatus < 400, T, F );

   odp->json = JsonDecode( ( const char * )odp->tresc.data, odp->tresc.size );
   if( odp->nStatus >= 400 || odp->json == NULL )
   {
      StringFormatSize( cBlad, ( size_t )nRozmiar, "KSeF %ld: %s", odp->nStatus, OpisBledu( odp, cOpis, sizeof( cOpis ) ) );

      return F;
   }

   return T;
}

static void OdpowiedzFree( Odpowiedz *odp )
{
   JsonFree( odp->json );
   BytesFree( &odp->tresc );
   odp->json    = NULL;
   odp->nStatus = 0;
}

static char *OpisBledu( const Odpowiedz *odp, char *cOpis, int nRozmiar )
{
   const Json *lista = JsonGet( JsonGet( odp->json, "exception" ), "exceptionDetailList" );
   const Json *szczegol;
   size_t      nDlugosc;

   *cOpis = 0;
   if( JsonType( lista ) == JSON_ARRAY )
   {
      for( int i = 1; i <= JsonCount( lista ); ++i )
      {
         szczegol = JsonItem( lista, i );
         nDlugosc = strlen( cOpis );
         StringFormatSize( cOpis + nDlugosc, ( size_t )nRozmiar - nDlugosc, "%s", IIF( nDlugosc == 0, "", "\n" ) );
         nDlugosc = strlen( cOpis );
         if( JsonType( JsonGet( szczegol, "exceptionCode" ) ) == JSON_NUMBER )
            StringFormatSize( cOpis + nDlugosc, ( size_t )nRozmiar - nDlugosc, "%.0f ", JsonNumber( JsonGet( szczegol, "exceptionCode" ) ) );
         nDlugosc = strlen( cOpis );
         if( JsonString( JsonGet( szczegol, "exceptionDescription" ) ) != NULL )
            StringFormatSize( cOpis + nDlugosc, ( size_t )nRozmiar - nDlugosc, "%s", JsonString( JsonGet( szczegol, "exceptionDescription" ) ) );
      }

      return cOpis;
   }

   if( JsonString( JsonGet( odp->json, "title" ) ) != NULL )
      StringFormatSize( cOpis, ( size_t )nRozmiar, "%s", JsonString( JsonGet( odp->json, "title" ) ) );
   else
      StringFormatSize( cOpis, ( size_t )nRozmiar, "%.300s", IIF( odp->tresc.data == NULL, "", ( const char * )odp->tresc.data ) );

   return cOpis;
}

static char *Kopia( const char *cTekst )
{
   size_t nDlugosc = strlen( cTekst ) + 1;
   char  *cWynik   = ( char * )malloc( nDlugosc );

   memcpy( cWynik, cTekst, nDlugosc );

   return cWynik;
}
