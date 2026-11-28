/*
 * Program...: xBiuro
 * Plik......: src/faktura/offline.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "faktura.h"

typedef struct
{
   Win     *okno;
   Control *aTryb[ 3 ];
   Control *KONIEC;
   int      nTryb;
   int      dKoniec;
} OknoTrybu;

static xBool CertyfikatOffline( const char *cNip, int nSrodowisko, Certyfikat *cert );
static xBool Warunki( const char *cNip, int nSrodowisko );
static void  P1363( Bytes *podpis );
static int   WybierzTryb( Win *okno, const char *cNumer, int *dKoniec );
static void  TrybZmiana( Control *radio, void *data );
static void  TrybOk( Control *button, void *data );
static void  TrybAnuluj( Control *button, void *data );
static int   Termin( int nTryb, int dWystawienia, int dKoniec );
static int   DodajRobocze( int dData, int nDni );
static xBool Roboczy( int dData );
static int   Wielkanoc( int nRok );

void Offline( Win *okno, Biuro *biuro, Control *browse )
{
   char *cXml    = NULL;
   int   nTryb   = 0;
   int   dKoniec = 0;
   int   nSrodowisko;
   int   dTermin;
   xBool lZapis;
   char  cNip[ 64 ];
   char  cNumer[ 300 ];
   char  cData[ 16 ];
   char  cPlik[ 400 ];
   char  cKoniec[ 200 ];
   char  cTekst[ 1000 ];

   DbSelectArea( "FKTHEAD" );
   if( Eof() )
      return;

   StringFormat( cNumer, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY" ) ) );
   AllTrim( cNumer );
   if( FieldGetN( FieldPos( "STATUS_KSEF" ) ) == KSEF_PRZYJETA )
   {
      StringFormat( cNip, "%s", FieldGetC( FieldPos( "NUMER_KSEF" ) ) );
      StringFormat( cTekst, "Faktura jest już w KSeF.\nNumer KSeF: %s", AllTrim( cNip ) );
      MsgInfo( cTekst, "Offline" );

      return;
   }

   if( FieldGetN( FieldPos( "TRYB_WYSTAWIENIA" ) ) > 0 )
   {
      StringFormat( cTekst, "Faktura jest już wystawiona w trybie %s.\nWyślij ją do KSeF (F6) do dnia %s.", OfflineTryb( ( int )FieldGetN( FieldPos( "TRYB_WYSTAWIENIA" ) ) ), DToC( FieldGetD( FieldPos( "TERMIN_PRZESLANIA_KSEF" ) ), cData, sizeof( cData ) ) );
      MsgInfo( cTekst, "Offline" );

      return;
   }

   if( FieldGetN( FieldPos( "STATUS_KSEF" ) ) == KSEF_W_TOKU )
   {
      MsgInfo( "KSeF przyjmuje już tę fakturę.\nSprawdź ją klawiszem F7.", "Offline" );

      return;
   }

   if( !FirmaPobierzDane( biuro, "SPRZEDAWCA" ) )
      return;

   StringFormat( cNip, "%s", FieldGetC( FieldPos( "NIP" ) ) );
   AllTrim( cNip );
   nSrodowisko = IIF( FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ) == SRODOWISKO_PRODUKCYJNE, SRODOWISKO_PRODUKCYJNE, SRODOWISKO_TESTOWE );

   if( Warunki( cNip, nSrodowisko ) )
      nTryb = WybierzTryb( okno, cNumer, &dKoniec );

   if( nTryb > 0 )
      cXml = Fa3GenerujXml( "SPRZEDAWCA" );

   DbSelectArea( "SPRZEDAWCA" );
   DbCloseArea();
   DbSelectArea( "FKTHEAD" );
   if( cXml == NULL )
      return;

   FakturyPlik( biuro, "xml", ".xml", cPlik, sizeof( cPlik ) );
   lZapis = MemoWrit( cPlik, cXml, strlen( cXml ) );
   free( cXml );
   if( !lZapis )
   {
      StringFormat( cTekst, "Nie można zapisać pliku %s.", cPlik );
      MsgError( cTekst, "Offline" );

      return;
   }

   dTermin = Termin( nTryb, FieldGetD( FieldPos( "DATA_WYSTAWIENIA" ) ), dKoniec );

   DbSelectArea( "FKTHEAD" );
   if( !NetRLock( NET_WAIT ) )
      return;

   StringFormat( cTekst, "Wystawiona w trybie %s %s", OfflineTryb( nTryb ), DToC( Date(), cData, sizeof( cData ) ) );
   FieldPutN( FieldPos( "TRYB_WYSTAWIENIA" ), nTryb );
   FieldPutD( FieldPos( "TERMIN_PRZESLANIA_KSEF" ), dTermin );
   FieldPutN( FieldPos( "SRODOWISKO_KSEF" ), nSrodowisko );
   FieldPutN( FieldPos( "STATUS_KSEF" ), KSEF_NIEWYSLANA );
   FieldPutC( FieldPos( "OPIS_STATUSU_KSEF" ), cTekst );
   DbCommit();
   DbUnlock();
   BrowseRefresh( browse );

   *cKoniec = 0;
   if( nTryb > 1 )
      StringFormat( cKoniec, "\nTermin liczy się od dnia zakończenia niedostępności lub awarii KSeF (%s).", DToC( dKoniec, cData, sizeof( cData ) ) );

   StringFormat( cTekst, "Faktura %s wystawiona w trybie %s.\n\nWydruk (F9) ma kod QR faktury (OFFLINE) i kod QR certyfikatu.\nWyślij fakturę do KSeF (F6) do dnia %s.%s", cNumer, OfflineTryb( nTryb ), DToC( dTermin, cData, sizeof( cData ) ), cKoniec );
   MsgInfo( cTekst, "Offline" );
}

const char *OfflineTryb( int nTryb )
{
   if( nTryb == 1 )
      return "offline24";

   if( nTryb == 2 )
      return "offline (niedostępność KSeF)";

   if( nTryb == 3 )
      return "awaryjnym (awaria KSeF)";

   return "";
}

void OfflinePrzypomnij( void )
{
   long   nRecNo;
   int    dTermin;
   int    dDzis    = Date();
   int    nLiczba  = 0;
   size_t nDlugosc;
   char   cNumer[ 300 ];
   char   cRodzaj[ 16 ];
   char   cData[ 16 ];
   char   cLinia[ 400 ];
   char   cTekst[ 4000 ];

   StringFormat( cTekst, "Faktury offline do wysłania do KSeF (F6):\n" );
   nDlugosc = strlen( cTekst );

   DbSelectArea( "FKTHEAD" );
   nRecNo = RecNo();
   for( DbGoTop(); !Eof(); DbSkip( 1 ) )
   {
      if( FieldGetN( FieldPos( "TRYB_WYSTAWIENIA" ) ) == 0 || FieldGetN( FieldPos( "STATUS_KSEF" ) ) == KSEF_PRZYJETA )
         continue;

      StringFormat( cNumer, "%s", FieldGetC( FieldPos( "NUMER_FAKTURY" ) ) );
      StringFormat( cRodzaj, "%s", FieldGetC( FieldPos( "RODZAJ_FAKTURY" ) ) );
      dTermin = FieldGetD( FieldPos( "TERMIN_PRZESLANIA_KSEF" ) );
      StringFormat( cLinia, "\n%s %s: do %s%s", IIF( strcmp( AllTrim( cRodzaj ), "KOR" ) == 0, "Korekta", "Faktura" ), AllTrim( cNumer ), DToC( dTermin, cData, sizeof( cData ) ), IIF( dTermin < dDzis, " - termin minął!", IIF( dTermin == dDzis, " - dzisiaj!", "" ) ) );
      if( nDlugosc + strlen( cLinia ) + 5 >= sizeof( cTekst ) )
      {
         StringFormatSize( cTekst + nDlugosc, sizeof( cTekst ) - nDlugosc, "\n..." );
         ++nLiczba;
         break;
      }

      StringFormatSize( cTekst + nDlugosc, sizeof( cTekst ) - nDlugosc, "%s", cLinia );
      nDlugosc += strlen( cLinia );
      ++nLiczba;
   }
   DbGoto( nRecNo );

   if( nLiczba > 0 )
      MsgWarning( cTekst, "Offline" );
}

char *OfflineKodII( Biuro *biuro, char *cAdres, int nRozmiar )
{
   Certyfikat cert;
   Bytes      klucz;
   Bytes      skrot;
   Bytes      podpis;
   char      *cXml;
   size_t     nXml;
   int        nSrodowisko;
   char       cNip[ 64 ];
   char       cCyfry[ 64 ];
   char       cSkrot[ 64 ];
   char       cPlik[ 400 ];
   char       cDoPodpisu[ 400 ];
   char       cPodpis[ 1100 ];
   char       cTekst[ 900 ];

   *cAdres = 0;
   DbSelectArea( "FKTHEAD" );
   if( FieldGetN( FieldPos( "TRYB_WYSTAWIENIA" ) ) == 0 )
      return cAdres;

   DbSelectArea( "SPRZEDAWCA" );
   StringFormat( cNip, "%s", FieldGetC( FieldPos( "NIP" ) ) );
   AllTrim( cNip );
   FirmaCyfry( cNip, cCyfry, sizeof( cCyfry ) );
   nSrodowisko = FakturySrodowiskoKsef( IIF( FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ) == SRODOWISKO_PRODUKCYJNE, SRODOWISKO_PRODUKCYJNE, SRODOWISKO_TESTOWE ) );
   if( !CertyfikatOffline( cNip, nSrodowisko, &cert ) )
   {
      DbSelectArea( "FKTHEAD" );

      return cAdres;
   }

   DbSelectArea( "FKTHEAD" );
   cXml = MemoRead( FakturyPlik( biuro, "xml", ".xml", cPlik, sizeof( cPlik ) ), &nXml );
   if( cXml == NULL || nXml == 0 )
   {
      free( cXml );
      CertyfikatFree( &cert );
      StringFormat( cTekst, "Brak pliku XML faktury offline:\n%s\nWydruk bez kodu QR certyfikatu.", cPlik );
      MsgWarning( cTekst, "Offline" );

      return cAdres;
   }

   skrot = HashData( "SHA256", cXml, nXml );
   free( cXml );
   FakturyBase64Url( skrot.data, skrot.size, cSkrot, sizeof( cSkrot ) );
   BytesFree( &skrot );

   StringFormat( cDoPodpisu, "%s/certificate/Nip/%s/%s/%s/%s", IIF( nSrodowisko == SRODOWISKO_PRODUKCYJNE, "qr.ksef.mf.gov.pl", "qr-test.ksef.mf.gov.pl" ), cCyfry, cCyfry, cert.cNumer, cSkrot );
   klucz.data = ( unsigned char * )cert.cKlucz;
   klucz.size = strlen( cert.cKlucz );
   podpis     = SignData( klucz, cDoPodpisu, strlen( cDoPodpisu ), "SHA256", cert.cHaslo, RSA_PADDING_PSS );
   if( podpis.data != NULL && !cert.lRsa )
      P1363( &podpis );

   CertyfikatFree( &cert );
   if( podpis.data == NULL )
   {
      StringFormat( cTekst, "Nie można podpisać kodu QR certyfikatem KSeF.\n%s\nWydruk bez kodu QR certyfikatu.", CryptoError() );
      MsgWarning( cTekst, "Offline" );

      return cAdres;
   }

   FakturyBase64Url( podpis.data, podpis.size, cPodpis, sizeof( cPodpis ) );
   BytesFree( &podpis );
   StringFormatSize( cAdres, ( size_t )nRozmiar, "https://%s/%s", cDoPodpisu, cPodpis );

   return cAdres;
}

static xBool CertyfikatOffline( const char *cNip, int nSrodowisko, Certyfikat *cert )
{
   char  cTekst[ 600 ];
   xBool lJest = CertyfikatyPobierz( cNip, nSrodowisko, "Offline", cert );

   if( !lJest )
   {
      StringFormat( cTekst, "Brak ważnego certyfikatu KSeF typu Offline dla NIP %s\ni środowiska %s.\nWczytaj go w menu KSeF, Tokeny / Certyfikaty, Lista certyfikatów.", cNip, IIF( nSrodowisko == SRODOWISKO_PRODUKCYJNE, "produkcyjnego", "testowego" ) );
      MsgInfo( cTekst, "Offline" );
   }

   return lJest;
}

static xBool Warunki( const char *cNip, int nSrodowisko )
{
   Certyfikat cert;
   char       cNazwa[ 2100 ];
   char       cUlica[ 300 ];
   char       cMiasto[ 230 ];

   if( !FakturySrodowisko( nSrodowisko, "Offline" ) )
      return F;

   DbSelectArea( "SPRZEDAWCA" );
   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "PELNA_NAZWA" ) ) );
   StringFormat( cUlica, "%s", FieldGetC( FieldPos( "ULICA" ) ) );
   StringFormat( cMiasto, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC" ) ) );
   if( *cNip == 0 || *AllTrim( cNazwa ) == 0 || ( *AllTrim( cUlica ) == 0 && *AllTrim( cMiasto ) == 0 ) )
   {
      MsgInfo( "Uzupełnij NIP, nazwę i adres firmy:\nmenu Firma, Dane firmy.", "Offline" );

      return F;
   }

   DbSelectArea( "FKTHEAD" );
   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_NABYWCY" ) ) );
   StringFormat( cUlica, "%s", FieldGetC( FieldPos( "ULICA_NABYWCY" ) ) );
   StringFormat( cMiasto, "%s", FieldGetC( FieldPos( "MIEJSCOWOSC_NABYWCY" ) ) );
   if( *AllTrim( cNazwa ) == 0 || ( *AllTrim( cUlica ) == 0 && *AllTrim( cMiasto ) == 0 ) )
   {
      MsgInfo( "Faktura nie ma nazwy albo adresu nabywcy.", "Offline" );

      return F;
   }

   if( !CertyfikatOffline( cNip, nSrodowisko, &cert ) )
      return F;

   CertyfikatFree( &cert );

   return T;
}

static void P1363( Bytes *podpis )
{
   unsigned char       *cWynik;
   const unsigned char *cLiczba;
   size_t               nPoz;
   size_t               nDlugosc;

   if( podpis->size < 8 || podpis->data[ 0 ] != 0x30 )
      return;

   cWynik = calloc( 1, 64 );
   if( cWynik == NULL )
      return;

   nPoz = IIF( podpis->data[ 1 ] > 127, 3, 2 );
   for( int nLiczba = 0; nLiczba < 2; ++nLiczba )
   {
      if( nPoz + 2 > podpis->size || podpis->data[ nPoz ] != 0x02 || nPoz + 2 + podpis->data[ nPoz + 1 ] > podpis->size )
      {
         free( cWynik );

         return;
      }

      nDlugosc = podpis->data[ nPoz + 1 ];
      cLiczba  = podpis->data + nPoz + 2;
      nPoz    += 2 + nDlugosc;
      while( nDlugosc > 32 && *cLiczba == 0 )
      {
         ++cLiczba;
         --nDlugosc;
      }
      if( nDlugosc > 32 )
      {
         free( cWynik );

         return;
      }

      memcpy( cWynik + nLiczba * 32 + 32 - nDlugosc, cLiczba, nDlugosc );
   }

   free( podpis->data );
   podpis->data = cWynik;
   podpis->size = 64;
}

static int WybierzTryb( Win *okno, const char *cNumer, int *dKoniec )
{
   OknoTrybu oTryb = { 0 };
   Control  *oOk;
   Control  *oAnuluj;
   char      cTekst[ 400 ];

   oTryb.okno = DialogCreate( okno, "Wystaw w trybie offline", 520, 240 );
   StringFormat( cTekst, "Faktura %s - tryb wystawienia:", cNumer );
   SayCreate( oTryb.okno, 20, 15, 54, 20, cTekst );
   oTryb.aTryb[ 0 ] = RadioButtonCreate( oTryb.okno, 20,  45, 480, 20, "offline24 - z wyboru, wysyłka do następnego dnia roboczego" );
   oTryb.aTryb[ 1 ] = RadioButtonCreate( oTryb.okno, 20,  75, 480, 20, "offline - ogłoszona niedostępność KSeF" );
   oTryb.aTryb[ 2 ] = RadioButtonCreate( oTryb.okno, 20, 105, 480, 20, "awaryjny - ogłoszona awaria KSeF (7 dni roboczych)" );
   CheckBoxSetChecked( oTryb.aTryb[ 0 ], T );
   for( int i = 0; i < 3; ++i )
   {
      ControlSetAction( oTryb.aTryb[ i ], TrybZmiana, &oTryb );
   }

   SayCreate( oTryb.okno, 40, 140, 0, 20, "Koniec niedostępności / awarii:" );
   oTryb.KONIEC = EditCreate( oTryb.okno, 335, 137, 110, 24, "" );
   EditSetDate( oTryb.KONIEC, Date() );
   ControlSetEnabled( oTryb.KONIEC, F );

   oOk     = ButtonCreate( oTryb.okno, 310, 192, 90, 28, "OK" );
   oAnuluj = ButtonCreate( oTryb.okno, 410, 192, 90, 28, "Anuluj" );
   ButtonSetDefault( oOk, T );
   ControlSetAction( oOk, TrybOk, &oTryb );
   ButtonSetCancel( oAnuluj, T );
   ControlSetAction( oAnuluj, TrybAnuluj, &oTryb );
   WinCenter( oTryb.okno );
   DialogShowModal( oTryb.okno );

   *dKoniec = oTryb.dKoniec;

   return oTryb.nTryb;
}

static void TrybZmiana( Control *radio, void *data )
{
   OknoTrybu *oTryb = ( OknoTrybu * )data;

   UNUSED( radio );

   ControlSetEnabled( oTryb->KONIEC, !CheckBoxChecked( oTryb->aTryb[ 0 ] ) );
}

static void TrybOk( Control *button, void *data )
{
   OknoTrybu *oTryb = ( OknoTrybu * )data;

   UNUSED( button );

   for( int i = 0; i < 3; ++i )
   {
      if( CheckBoxChecked( oTryb->aTryb[ i ] ) )
         oTryb->nTryb = i + 1;
   }
   oTryb->dKoniec = IIF( EditDate( oTryb->KONIEC ) == 0, Date(), EditDate( oTryb->KONIEC ) );
   DialogEnd( oTryb->okno, 1 );
}

static void TrybAnuluj( Control *button, void *data )
{
   UNUSED( button );

   DialogEnd( ( ( OknoTrybu * )data )->okno, 0 );
}

static int Termin( int nTryb, int dWystawienia, int dKoniec )
{
   int dOd = IIF( dKoniec > dWystawienia, dKoniec, dWystawienia );

   if( nTryb == 1 )
      return DodajRobocze( dWystawienia, 1 );

   if( nTryb == 2 )
      return DodajRobocze( dOd, 1 );

   return DodajRobocze( dOd, 7 );
}

static int DodajRobocze( int dData, int nDni )
{
   while( nDni > 0 )
   {
      dData = DateAdd( dData, 1 );
      if( Roboczy( dData ) )
         --nDni;
   }

   return dData;
}

static xBool Roboczy( int dData )
{
   static const int aSwieta[] = { 101, 106, 501, 503, 815, 1101, 1111, 1224, 1225, 1226 };

   int dWielkanoc;

   if( DOW( dData ) == 1 || DOW( dData ) == 7 )
      return F;

   for( size_t i = 0; i < sizeof( aSwieta ) / sizeof( aSwieta[ 0 ] ); ++i )
   {
      if( dData % 10000 == aSwieta[ i ] )
         return F;
   }

   dWielkanoc = Wielkanoc( dData / 10000 );

   return IIF( dData != DateAdd( dWielkanoc, 1 ) && dData != DateAdd( dWielkanoc, 60 ), T, F );
}

static int Wielkanoc( int nRok )
{
   int a        = nRok % 19;
   int b        = nRok / 100;
   int c        = nRok % 100;
   int d        = b / 4;
   int e        = b % 4;
   int f        = ( b + 8 ) / 25;
   int g        = ( b - f + 1 ) / 3;
   int h        = ( 19 * a + b - d - g + 15 ) % 30;
   int i        = c / 4;
   int k        = c % 4;
   int l        = ( 32 + 2 * e + 2 * i - h - k ) % 7;
   int m        = ( a + 11 * h + 22 * l ) / 451;
   int nMiesiac = ( h + l - 7 * m + 114 ) / 31;
   int nDzien   = ( h + l - 7 * m + 114 ) % 31 + 1;

   return nRok * 10000 + nMiesiac * 100 + nDzien;
}
