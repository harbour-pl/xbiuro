/*
 * Program...: xBiuro
 * Plik......: src/faktura/certyfikaty.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "faktura.h"

#define CERTYFIKATY      DANE_BAZA "ksef/certyfikaty"

static const DbField aDbf[] = {
   { "NAZWA_CERTYFIKATU",   'C', 240, 0 },
   { "NIP",                 'C',  50, 0 },
   { "SRODOWISKO_KSEF",     'N',   1, 0 },
   { "TYP_CERTYFIKATU",     'C',  14, 0 },
   { "NUMER_SERYJNY",       'C',  40, 0 },
   { "PODMIOT",             'M',  10, 0 },
   { "WAZNY_OD",            'D',   8, 0 },
   { "WAZNY_DO",            'D',   8, 0 },
   { "CERTYFIKAT",          'M',  10, 0 },
   { "KLUCZ_ZASZYFROWANY",  'M',  10, 0 },
   { "HASLO_ZASZYFROWANE",  'M',  10, 0 },
   { "DATA_DODANIA",        'D',   8, 0 }
};

#define POLA_CERTYFIKATOW ( ( int )( sizeof( aDbf ) / sizeof( aDbf[ 0 ] ) ) )

typedef struct
{
   Biuro     *biuro;
   Win       *okno;
   StatusBar *pasekStanu;
   Toolbar   *pasek;
   Win       *panel;
   Control   *BRW;
   int        aZamroz[ 2 ];
   Control   *NAZWA_CERTYFIKATU;
   Control   *TYP_CERTYFIKATU;
   Control   *NUMER_SERYJNY;
   Control   *OPIS_PODMIOT;
   Control   *PODMIOT;
   Control   *WAZNOSC;
   Control   *STAN;
   char       cNazwa[ 64 ];
   char       cStan[ 64 ];
   char       cData[ 16 ];
} OknoCertyfikatow;

static xBool       DbOpen( void );
static void        Przebudowa( void );
static void        Stan( OknoCertyfikatow *oDialog );
static Control    *Opis( Win *strona, int y, const char *tekst );
static Control    *Pole( Win *strona, int x, int y, int dlugosc );
static void        Pomoc( Toolbar *toolbar, void *data );
static void        Okno( OknoCertyfikatow *oDialog );
static void        Zmiana( Control *browse, void *data );
static const char *Typ( const char *cTyp );
static void        Ostrzez( void );
static const char *KolumnaNazwa( Control *browse, void *data );
static const char *KolumnaTyp( Control *browse, void *data );
static const char *KolumnaPole( Control *browse, void *data );
static const char *KolumnaSrodowisko( Control *browse, void *data );
static const char *KolumnaWazny( Control *browse, void *data );
static const char *KolumnaStan( Control *browse, void *data );
static double      Numer( Control *browse, void *data );
static void        Dodaj( Toolbar *toolbar, void *data );
static void        Wstaw( Win *win, void *data );
static void        Aplikacja( Toolbar *toolbar, void *data );
static xBool       Jest( const char *cNumer, int nSrodowisko );
static int         Data( const char *cCzas );
static int         Der( const unsigned char *cDer, int nPoz, int *nTresc, int *nDlugosc );
static char       *NumerZDer( Bytes der, char *cNumer, int nRozmiar );
static const char *TypZDer( Bytes der );
static void        Sprawdz( Toolbar *toolbar, void *data );
static void        Usun( Toolbar *toolbar, void *data );
static void        Zamknij( Toolbar *toolbar, void *data );

void Certyfikaty( Win *win, void *data )
{
   OknoCertyfikatow  oknoCertyfikatow = { 0 };
   OknoCertyfikatow *oDialog          = &oknoCertyfikatow;
   Win              *panel;
   int               przycisk;

   oDialog->biuro = ( Biuro * )data;
   if( !DbOpen() )
      return;

   oDialog->okno = DialogCreate( win, "Lista certyfikatów KSeF", 900, 600 );
   WinSetBackground( oDialog->okno, 0xE9EDF1 );
   WinSetSizable( oDialog->okno, T );
   WinSetKey( oDialog->okno, "Ins", Wstaw, oDialog );

   oDialog->pasekStanu = StatusBarCreate( oDialog->okno, "" );
   StatusBarAdd( oDialog->pasekStanu, 0, "", F );
   StatusBarAdd( oDialog->pasekStanu, 60, "", F );
   StatusBarSetScoreboard( oDialog->pasekStanu, 2 );

   oDialog->pasek = ToolbarCreate( oDialog->okno, TOOLBAR_BOTTOM );
   przycisk = ToolbarAdd( oDialog->pasek, "F1-Pomoc", Pomoc, NULL );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "F1" );
   ToolbarAddSeparator( oDialog->pasek );
   przycisk = ToolbarAdd( oDialog->pasek, "F2-Wczytaj", Dodaj, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "F2" );
   ToolbarAddSeparator( oDialog->pasek );
   przycisk = ToolbarAdd( oDialog->pasek, "F3-Aplikacja KSeF", Aplikacja, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "F3" );
   ToolbarAddSeparator( oDialog->pasek );
   oDialog->aZamroz[ 0 ] = ToolbarAdd( oDialog->pasek, "F5-Sprawdź", Sprawdz, oDialog );
   ToolbarSetShortcut( oDialog->pasek, oDialog->aZamroz[ 0 ], "F5" );
   ToolbarAddSeparator( oDialog->pasek );
   oDialog->aZamroz[ 1 ] = ToolbarAdd( oDialog->pasek, "F8-Usuń", Usun, oDialog );
   ToolbarSetShortcut( oDialog->pasek, oDialog->aZamroz[ 1 ], "F8" );
   ToolbarAddSeparator( oDialog->pasek );
   przycisk = ToolbarAdd( oDialog->pasek, "Esc-Zamknij", Zamknij, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "Esc" );

   panel          = PanelCreate( oDialog->okno, 0, 0, 0, 0 );
   oDialog->panel = panel;

   Opis( panel, 13, "Nazwa:" );
   Opis( panel, 43, "Typ / numer:" );
   oDialog->OPIS_PODMIOT = Opis( panel, 73, "Właściciel:" );
   Opis( panel, 103, "Ważność:" );
   oDialog->NAZWA_CERTYFIKATU = Pole( panel, 175,  10, 60 );
   oDialog->TYP_CERTYFIKATU   = Pole( panel, 175,  40, 23 );
   oDialog->NUMER_SERYJNY     = Pole( panel, 400,  40, 35 );
   oDialog->PODMIOT           = Pole( panel, 175,  70, 60 );
   oDialog->WAZNOSC           = Pole( panel, 175, 100, 23 );
   oDialog->STAN              = Pole( panel, 400, 100, 35 );

   oDialog->BRW = BrowseCreate( panel, 10, 140, WinWidth( panel ) - 20, WinHeight( panel ) - 150 );
   ControlSetAnchor( oDialog->BRW, "LTRB" );
   BrowseSetColor( oDialog->BRW, BROWSE_HEADBACKGROUND, 0x4D7326 );
   DbBrowse( oDialog->BRW, "CERTYFIKATY" );
   BrowseAddNumber( oDialog->BRW, "Poz.", 4, "9999", Numer, NULL );
   BrowseAddColumn( oDialog->BRW, "Nazwa", 30, BROWSE_LEFT, KolumnaNazwa, oDialog );
   BrowseAddColumn( oDialog->BRW, "Typ", 14, BROWSE_LEFT, KolumnaTyp, NULL );
   BrowseAddColumn( oDialog->BRW, "NIP", 12, BROWSE_LEFT, KolumnaPole, "NIP" );
   BrowseAddColumn( oDialog->BRW, "Środowisko", 12, BROWSE_LEFT, KolumnaSrodowisko, NULL );
   BrowseAddColumn( oDialog->BRW, "Ważny do", 10, BROWSE_LEFT, KolumnaWazny, oDialog );
   BrowseAddColumn( oDialog->BRW, "Stan", 20, BROWSE_LEFT, KolumnaStan, oDialog );
   ControlSetAction( oDialog->BRW, Zmiana, oDialog );

   DbSelectArea( "CERTYFIKATY" );
   DbGoTop();
   Okno( oDialog );
   Stan( oDialog );

   Ostrzez();

   WinCenter( oDialog->okno );
   DialogShowModal( oDialog->okno );

   DbSelectArea( "CERTYFIKATY" );
   DbCloseArea();
}

xBool CertyfikatyPobierz( const char *cNipFirmy, int nSrodowiskoKsef, const char *cTyp, Certyfikat *cert )
{
   static const unsigned char aRsa[] = { 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01 };

   char  cNip[ 64 ];
   char  cJest[ 64 ];
   char  cTypJest[ 16 ];
   int   dDzis = Date();
   xBool lJest = F;

   memset( cert, 0, sizeof( *cert ) );
   if( !DbOpen() )
      return F;

   StringFormat( cNip, "%s", cNipFirmy );
   AllTrim( cNip );
   for( DbGoBottom(); !Bof() && !Eof(); DbSkip( -1 ) )
   {
      StringFormat( cJest, "%s", FieldGetC( FieldPos( "NIP" ) ) );
      StringFormat( cTypJest, "%s", FieldGetC( FieldPos( "TYP_CERTYFIKATU" ) ) );
      if( strcmp( AllTrim( cJest ), cNip ) != 0 || FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ) != nSrodowiskoKsef || strcmp( AllTrim( cTypJest ), cTyp ) != 0 || FieldGetD( FieldPos( "WAZNY_OD" ) ) > dDzis || FieldGetD( FieldPos( "WAZNY_DO" ) ) < dDzis )
         continue;

      cert->cKlucz = KsefOdszyfruj( FieldGetC( FieldPos( "KLUCZ_ZASZYFROWANY" ) ) );
      if( *cert->cKlucz == 0 )
      {
         free( cert->cKlucz );
         cert->cKlucz = NULL;
         continue;
      }

      StringFormat( cert->cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_CERTYFIKATU" ) ) );
      StringFormat( cert->cNumer, "%s", FieldGetC( FieldPos( "NUMER_SERYJNY" ) ) );
      AllTrim( cert->cNazwa );
      AllTrim( cert->cNumer );
      cert->cHaslo   = KsefOdszyfruj( FieldGetC( FieldPos( "HASLO_ZASZYFROWANE" ) ) );
      cert->der      = Base64Decode( FieldGetC( FieldPos( "CERTYFIKAT" ) ) );
      cert->dWaznyDo = FieldGetD( FieldPos( "WAZNY_DO" ) );
      cert->lRsa     = F;
      for( size_t i = 0; cert->der.data != NULL && i + sizeof( aRsa ) <= cert->der.size; ++i )
      {
         if( memcmp( cert->der.data + i, aRsa, sizeof( aRsa ) ) == 0 )
         {
            cert->lRsa = T;
            break;
         }
      }
      lJest = T;
      break;
   }
   DbCloseArea();

   return lJest;
}

void CertyfikatFree( Certyfikat *cert )
{
   free( cert->cKlucz );
   free( cert->cHaslo );
   BytesFree( &cert->der );
   memset( cert, 0, sizeof( *cert ) );
}

void CertyfikatyNtx( void )
{
   char cPlik[ 64 ];

   StringFormat( cPlik, "%s.dbf", CERTYFIKATY );
   if( FileExists( cPlik ) && NetUse( CERTYFIKATY, T, NET_WAIT, "CERTYFIKATY", F ) )
   {
      NetPack();
      DbCloseArea();
   }
}

static xBool DbOpen( void )
{
   char cPlik[ 64 ];

   DirCreate( DANE_BAZA "ksef" );
   StringFormat( cPlik, "%s.dbf", CERTYFIKATY );
   if( !FileExists( cPlik ) )
      DbCreate( CERTYFIKATY, aDbf, POLA_CERTYFIKATOW );

   if( !NetUse( CERTYFIKATY, F, NET_WAIT, "CERTYFIKATY", F ) )
      return F;

   if( FCount() != POLA_CERTYFIKATOW )
   {
      DbCloseArea();
      MsgInfo( "Dodanie pola w bazie", "Certyfikaty" );
      Przebudowa();
      if( !NetUse( CERTYFIKATY, F, NET_WAIT, "CERTYFIKATY", F ) )
         return F;
   }

   return T;
}

static void Przebudowa( void )
{
   char cDbf[ 64 ];
   char cTmp[ 64 ];
   char cTmpDbf[ 64 ];
   char cStara[ 64 ];

   StringFormat( cDbf, "%s.dbf", CERTYFIKATY );
   StringFormat( cTmp, "%s-tmp", CERTYFIKATY );
   StringFormat( cTmpDbf, "%s.dbf", cTmp );
   StringFormat( cStara, "%s.tmp", CERTYFIKATY );

   if( FileExists( cDbf ) )
   {
      FileCopy( cDbf, cTmpDbf );
      FRename( cDbf, cStara );
      DbCreate( CERTYFIKATY, aDbf, POLA_CERTYFIKATOW );
      if( NetUse( CERTYFIKATY, F, NET_WAIT, "CERTYFIKATY", F ) )
      {
         DbAppendFrom( cTmp, NULL, NULL );
         DbCloseArea();
      }
      FErase( cTmpDbf );
      FErase( cStara );
   }
}

static void Stan( OknoCertyfikatow *oDialog )
{
   xBool lJest = !DbPusta( "CERTYFIKATY" );

   ToolbarEnable( oDialog->pasek, oDialog->aZamroz[ 0 ], lJest );
   ToolbarEnable( oDialog->pasek, oDialog->aZamroz[ 1 ], lJest );
   ControlSetEnabled( oDialog->BRW, lJest );
   BrowseRefresh( oDialog->BRW );

   if( lJest )
      ControlSetFocus( oDialog->BRW );
}

static Control *Opis( Win *strona, int y, const char *tekst )
{
   return SayCreate( strona, 20, y, 15, 20, tekst );
}

static Control *Pole( Win *strona, int x, int y, int dlugosc )
{
   Control *pole = SayCreate( strona, x, y, dlugosc, 24, "" );

   LabelSetBorder( pole, T );

   return pole;
}

static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgHelp( "Certyfikat KSeF wydaje Aplikacja Podatnika KSeF (Certyfikaty - Wnioskuj). Przy wniosku zapisuje ona klucz prywatny (plik .key) chroniony hasłem, a po wydaniu certyfikat pobiera się z listy certyfikatów (plik .crt, PEM). Oba pliki wskazuje się w oknie wyboru pliku po F2.\n\n"
            "Typ Offline służy do kodu QR II na fakturach wystawionych w trybie offline, typ Authentication do logowania w KSeF.\n\n"
            "F1\tpomoc\n"
            "F2, Ins\twczytanie certyfikatu: plik .crt, plik .key i hasło klucza\n"
            "F3\tAplikacja Podatnika KSeF w przeglądarce (środowisko firmy)\n"
            "F5\tpróbne logowanie do KSeF certyfikatem (podpis XAdES)\n"
            "F8\tusunięcie certyfikatu z listy\n"
            "Strzałki\tpoprzedni / następny certyfikat\n"
            "Esc\tzamknięcie okna\n\n"
            "Wpisywanie danych\n\n"
            "Ctrl+V\twklejenie hasła\n"
            "Enter\tnastępne pole, na ostatnim zapis\n"
            "PgUp, PgDn\tzapis\n"
            "Esc\tbez zapisu\n\n"
            "Klucz prywatny i hasło są zaszyfrowane kluczem tego stanowiska (profil Windows). Na 30 dni przed końcem ważności xBiuro ostrzega przy otwarciu listy.", "Lista certyfikatów KSeF" );
}

static void Okno( OknoCertyfikatow *oDialog )
{
   char  cOd[ 16 ];
   char  cDo[ 16 ];
   char  cTekst[ 64 ];
   xBool lEof;

   DbSelectArea( "CERTYFIKATY" );
   lEof = Eof();
   StringFormat( cTekst, "%s - %s", DToC( FieldGetD( FieldPos( "WAZNY_OD" ) ), cOd, sizeof( cOd ) ), DToC( FieldGetD( FieldPos( "WAZNY_DO" ) ), cDo, sizeof( cDo ) ) );
   ControlSetText( oDialog->NAZWA_CERTYFIKATU, FieldGetC( FieldPos( "NAZWA_CERTYFIKATU" ) ) );
   ControlSetText( oDialog->TYP_CERTYFIKATU, IIF( lEof, "", Typ( FieldGetC( FieldPos( "TYP_CERTYFIKATU" ) ) ) ) );
   ControlSetText( oDialog->NUMER_SERYJNY, FieldGetC( FieldPos( "NUMER_SERYJNY" ) ) );
   ControlSetText( oDialog->PODMIOT, FieldGetC( FieldPos( "PODMIOT" ) ) );
   ControlSetText( oDialog->WAZNOSC, IIF( lEof, "", cTekst ) );
   ControlSetText( oDialog->STAN, IIF( lEof, "", KsefWaznosc( FieldGetD( FieldPos( "WAZNY_DO" ) ), oDialog->cStan, sizeof( oDialog->cStan ) ) ) );
}

static void Zmiana( Control *browse, void *data )
{
   UNUSED( browse );

   Okno( ( OknoCertyfikatow * )data );
}

static const char *Typ( const char *cTyp )
{
   if( strncmp( cTyp, "Offline", 7 ) == 0 )
      return "Offline";

   if( strncmp( cTyp, "Authentication", 14 ) == 0 )
      return "Authentication";

   return "Nieznany";
}

static void Ostrzez( void )
{
   char   cTekst[ 4000 ] = "";
   char   cNazwa[ 241 ];
   char   cStan[ 64 ];
   size_t nDlugosc;
   long   nRecNo;
   int    dDo;

   DbSelectArea( "CERTYFIKATY" );
   nRecNo = RecNo();
   for( DbGoTop(); !Eof(); DbSkip( 1 ) )
   {
      dDo = FieldGetD( FieldPos( "WAZNY_DO" ) );
      if( dDo != 0 && DateDiff( dDo, Date() ) <= DNI_OSTRZEZENIA )
      {
         StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_CERTYFIKATU" ) ) );
         nDlugosc = strlen( cTekst );
         StringFormatSize( cTekst + nDlugosc, sizeof( cTekst ) - nDlugosc, "\n%s (%s): %s", AllTrim( cNazwa ), Typ( FieldGetC( FieldPos( "TYP_CERTYFIKATU" ) ) ), KsefWaznosc( dDo, cStan, sizeof( cStan ) ) );
      }
   }
   DbGoto( nRecNo );

   if( *cTekst != 0 )
   {
      nDlugosc = strlen( cTekst );
      StringFormatSize( cTekst + nDlugosc, sizeof( cTekst ) - nDlugosc, "\n\nNowy certyfikat wnioskuje się w Aplikacji Podatnika KSeF (F3)." );
      MsgInfo( cTekst + 1, "Certyfikaty KSeF do odnowienia" );
   }
}

static const char *KolumnaNazwa( Control *browse, void *data )
{
   OknoCertyfikatow *oDialog = ( OknoCertyfikatow * )data;

   UNUSED( browse );

   DbSelectArea( "CERTYFIKATY" );
   StringFormat( oDialog->cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_CERTYFIKATU" ) ) );

   return ULeft( oDialog->cNazwa, 30 );
}

static const char *KolumnaTyp( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "CERTYFIKATY" );

   return Typ( FieldGetC( FieldPos( "TYP_CERTYFIKATU" ) ) );
}

static const char *KolumnaPole( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( "CERTYFIKATY" );

   return FieldGetC( FieldPos( ( const char * )data ) );
}

static const char *KolumnaSrodowisko( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "CERTYFIKATY" );

   return KsefSrodowisko( ( int )FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ), F );
}

static const char *KolumnaWazny( Control *browse, void *data )
{
   OknoCertyfikatow *oDialog = ( OknoCertyfikatow * )data;

   UNUSED( browse );

   DbSelectArea( "CERTYFIKATY" );

   return DToC( FieldGetD( FieldPos( "WAZNY_DO" ) ), oDialog->cData, sizeof( oDialog->cData ) );
}

static const char *KolumnaStan( Control *browse, void *data )
{
   OknoCertyfikatow *oDialog = ( OknoCertyfikatow * )data;

   UNUSED( browse );

   DbSelectArea( "CERTYFIKATY" );

   return KsefWaznosc( FieldGetD( FieldPos( "WAZNY_DO" ) ), oDialog->cStan, sizeof( oDialog->cStan ) );
}

static double Numer( Control *browse, void *data )
{
   UNUSED( browse );
   UNUSED( data );

   DbSelectArea( "CERTYFIKATY" );

   return ( double )RecNo();
}

static void Dodaj( Toolbar *toolbar, void *data )
{
   OknoCertyfikatow *oDialog = ( OknoCertyfikatow * )data;
   CertDetails       info;
   Control          *GetList[ 2 ];
   Bytes             cert;
   Bytes             klucz;
   Bytes             podpis;
   const char       *cTyp;
   const char       *cKoniec;
   char             *cSzyfrKlucza;
   char             *cSzyfrHasla;
   char             *cDer;
   long              nRecNo;
   int               nSrodowisko;
   int               dOd;
   int               dDo;
   xBool             lZgodny;
   char              cNip[ 64 ];
   char              cPlikCert[ 600 ]   = "";
   char              cPlikKlucza[ 600 ] = "";
   char              cNumer[ 128 ];
   char              cNazwa[ 241 ];
   char              cHaslo[ 257 ]      = "";
   char              cTekst[ 1000 ];
   char              cOd[ 16 ];
   char              cDo[ 16 ];

   UNUSED( toolbar );

   if( !FirmaPobierzKsef( oDialog->biuro, cNip, sizeof( cNip ), &nSrodowisko ) || *cNip == 0 )
   {
      MsgInfo( "Najpierw wpisz NIP firmy w menu Firma, Dane firmy.", "Certyfikaty" );

      return;
   }

   if( !FileOpenDialog( "Wskaż plik certyfikatu KSeF (.crt)", "Certyfikat KSeF|*.crt;*.pem;*.cer|Wszystkie pliki|*.*", cPlikCert, sizeof( cPlikCert ) ) )
      return;

   cert.data = ( unsigned char * )MemoRead( cPlikCert, &cert.size );
   if( cert.data == NULL || !CertInfo( cert, &info ) )
   {
      StringFormat( cTekst, "Plik nie zawiera certyfikatu:\n%s\n%s", cPlikCert, CryptoError() );
      MsgError( cTekst, "Certyfikaty" );
      BytesFree( &cert );

      return;
   }

   cTyp = TypZDer( info.der );
   NumerZDer( info.der, cNumer, sizeof( cNumer ) );
   if( Jest( cNumer, nSrodowisko ) )
   {
      StringFormat( cTekst, "Ten certyfikat jest już na liście:\n%s", cNumer );
      MsgInfo( cTekst, "Certyfikaty" );
      BytesFree( &cert );
      BytesFree( &info.der );

      return;
   }

   if( !FileOpenDialog( "Wskaż plik klucza prywatnego certyfikatu (.key)", "Klucz prywatny|*.key;*.pem|Wszystkie pliki|*.*", cPlikKlucza, sizeof( cPlikKlucza ) ) )
   {
      BytesFree( &cert );
      BytesFree( &info.der );

      return;
   }
   klucz.data = ( unsigned char * )MemoRead( cPlikKlucza, &klucz.size );

   cKoniec = cPlikCert + strlen( cPlikCert );
   while( cKoniec > cPlikCert && cKoniec[ -1 ] != '/' && cKoniec[ -1 ] != '\\' )
      --cKoniec;
   StringFormat( cNazwa, "%s", cKoniec );
   if( strrchr( cNazwa, '.' ) != NULL )
      *strrchr( cNazwa, '.' ) = 0;

   dOd = Data( info.notBefore );
   dDo = Data( info.notAfter );

   DbSelectArea( "CERTYFIKATY" );
   nRecNo = RecNo();
   DbGoBottom();
   DbSkip( 1 );
   Okno( oDialog );
   StringFormat( cTekst, "%s - %s", DToC( dOd, cOd, sizeof( cOd ) ), DToC( dDo, cDo, sizeof( cDo ) ) );
   ControlSetText( oDialog->TYP_CERTYFIKATU, Typ( cTyp ) );
   ControlSetText( oDialog->NUMER_SERYJNY, cNumer );
   ControlSetText( oDialog->WAZNOSC, cTekst );
   ControlSetText( oDialog->STAN, KsefWaznosc( dDo, oDialog->cStan, sizeof( oDialog->cStan ) ) );
   ControlSetText( oDialog->PODMIOT, "" );
   ControlSetText( oDialog->OPIS_PODMIOT, "Hasło klucza:" );

   GetList[ 0 ] = GetText( oDialog->panel, 175, 10, 60, cNazwa, sizeof( cNazwa ), "" );
   GetList[ 1 ] = GetText( oDialog->panel, 175, 70, 60, cHaslo, sizeof( cHaslo ), "" );

   PozycjeTrybEdycji( oDialog->pasekStanu, T );
   WinReadModal( oDialog->okno, GetList, 2, 1 );
   PozycjeTrybEdycji( oDialog->pasekStanu, F );
   ControlDestroy( GetList[ 0 ] );
   ControlDestroy( GetList[ 1 ] );
   ControlSetText( oDialog->OPIS_PODMIOT, "Właściciel:" );

   DbSelectArea( "CERTYFIKATY" );
   if( ReadLastKey() == K_ESC )
   {
      DbGoto( nRecNo );
      Okno( oDialog );
      BytesFree( &cert );
      BytesFree( &klucz );
      BytesFree( &info.der );

      return;
   }

   AllTrim( cHaslo );
   podpis = SignData( klucz, "xBiuro", 6, "SHA256", cHaslo, RSA_PADDING_PKCS1 );
   if( podpis.data == NULL )
   {
      StringFormat( cTekst, "Nie można otworzyć klucza prywatnego - złe hasło albo zły plik:\n%s\n%s", cPlikKlucza, CryptoError() );
      MsgError( cTekst, "Certyfikaty" );
      lZgodny = F;
   }
   else
   {
      lZgodny = VerifyData( cert, "xBiuro", 6, podpis, "SHA256", RSA_PADDING_PKCS1 );
      if( !lZgodny )
      {
         StringFormat( cTekst, "Klucz prywatny nie pasuje do certyfikatu:\n%s\n%s", cPlikKlucza, cPlikCert );
         MsgError( cTekst, "Certyfikaty" );
      }
   }
   BytesFree( &podpis );

   cSzyfrKlucza = IIF( lZgodny, KsefZaszyfruj( ( const char * )klucz.data ), NULL );
   cSzyfrHasla  = IIF( lZgodny, KsefZaszyfruj( cHaslo ), NULL );
   BytesFree( &cert );
   BytesFree( &klucz );
   if( lZgodny && ( cSzyfrKlucza == NULL || cSzyfrHasla == NULL ) )
   {
      StringFormat( cTekst, "Nie można utworzyć klucza szyfrowania w katalogu\n%s/xBiuro.\nCertyfikat nie został zapisany.", GetEnv( "APPDATA" ) );
      MsgError( cTekst, "Certyfikaty" );
      lZgodny = F;
   }

   DbSelectArea( "CERTYFIKATY" );
   if( !lZgodny || !NetAppBlank( NET_WAIT, T ) )
   {
      free( cSzyfrKlucza );
      free( cSzyfrHasla );
      BytesFree( &info.der );
      DbSelectArea( "CERTYFIKATY" );
      DbGoto( nRecNo );
      Okno( oDialog );

      return;
   }

   cDer = Base64Encode( info.der.data, info.der.size );
   if( *AllTrim( cNazwa ) == 0 )
      StringFormat( cNazwa, "Certyfikat %s", DToC( Date(), cOd, sizeof( cOd ) ) );
   FieldPutC( FieldPos( "NAZWA_CERTYFIKATU" ), cNazwa );
   FieldPutC( FieldPos( "NIP" ), cNip );
   FieldPutN( FieldPos( "SRODOWISKO_KSEF" ), nSrodowisko );
   FieldPutC( FieldPos( "TYP_CERTYFIKATU" ), cTyp );
   FieldPutC( FieldPos( "NUMER_SERYJNY" ), cNumer );
   FieldPutC( FieldPos( "PODMIOT" ), info.subject );
   FieldPutD( FieldPos( "WAZNY_OD" ), dOd );
   FieldPutD( FieldPos( "WAZNY_DO" ), dDo );
   FieldPutC( FieldPos( "CERTYFIKAT" ), cDer );
   FieldPutC( FieldPos( "KLUCZ_ZASZYFROWANY" ), cSzyfrKlucza );
   FieldPutC( FieldPos( "HASLO_ZASZYFROWANE" ), cSzyfrHasla );
   FieldPutD( FieldPos( "DATA_DODANIA" ), Date() );
   DbCommit();
   DbUnlock();
   free( cDer );
   free( cSzyfrKlucza );
   free( cSzyfrHasla );
   BytesFree( &info.der );

   Okno( oDialog );
   Stan( oDialog );

   if( *cTyp == 0 )
      MsgInfo( "Nie rozpoznano typu certyfikatu (Offline / Authentication).", "Certyfikaty" );
}

static void Wstaw( Win *win, void *data )
{
   UNUSED( win );

   Dodaj( NULL, data );
}

static void Aplikacja( Toolbar *toolbar, void *data )
{
   OknoCertyfikatow *oDialog     = ( OknoCertyfikatow * )data;
   int               nSrodowisko = SRODOWISKO_TESTOWE;
   char              cNip[ 64 ];

   UNUSED( toolbar );

   FirmaPobierzKsef( oDialog->biuro, cNip, sizeof( cNip ), &nSrodowisko );
   DbSelectArea( "CERTYFIKATY" );
   KsefAplikacja( nSrodowisko, "Certyfikaty" );
}

static xBool Jest( const char *cNumer, int nSrodowisko )
{
   char  cJest[ 64 ];
   long  nRecNo;
   xBool lJest = F;

   DbSelectArea( "CERTYFIKATY" );
   nRecNo = RecNo();
   for( DbGoTop(); !Eof(); DbSkip( 1 ) )
   {
      StringFormat( cJest, "%s", FieldGetC( FieldPos( "NUMER_SERYJNY" ) ) );
      if( strcmp( AllTrim( cJest ), cNumer ) == 0 && FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ) == nSrodowisko )
      {
         lJest = T;
         break;
      }
   }
   DbGoto( nRecNo );

   return lJest;
}

static int Data( const char *cCzas )
{
   if( strlen( cCzas ) < 10 )
      return 0;

   return atoi( cCzas ) * 10000 + atoi( cCzas + 5 ) * 100 + atoi( cCzas + 8 );
}

static int Der( const unsigned char *cDer, int nPoz, int *nTresc, int *nDlugosc )
{
   int nBajtow;

   *nDlugosc = cDer[ nPoz + 1 ];
   *nTresc   = nPoz + 2;
   if( *nDlugosc > 127 )
   {
      nBajtow   = *nDlugosc - 128;
      *nDlugosc = 0;
      while( nBajtow-- > 0 )
         *nDlugosc = *nDlugosc * 256 + cDer[ ( *nTresc )++ ];
   }

   return cDer[ nPoz ];
}

static char *NumerZDer( Bytes der, char *cNumer, int nRozmiar )
{
   int nTresc;
   int nDlugosc;
   int nZnacznik;
   int nDlugoscTekstu = 0;

   *cNumer = 0;
   if( der.size < 16 )
      return cNumer;

   Der( der.data, 0, &nTresc, &nDlugosc );
   Der( der.data, nTresc, &nTresc, &nDlugosc );
   nZnacznik = Der( der.data, nTresc, &nTresc, &nDlugosc );
   if( nZnacznik == 0xA0 )
      nZnacznik = Der( der.data, nTresc + nDlugosc, &nTresc, &nDlugosc );
   if( nZnacznik != 0x02 )
      return cNumer;

   for( int i = 0; i < nDlugosc && nDlugoscTekstu < nRozmiar - 3; ++i )
   {
      StringFormatSize( cNumer + nDlugoscTekstu, ( size_t )( nRozmiar - nDlugoscTekstu ), "%02X", der.data[ nTresc + i ] );
      nDlugoscTekstu += 2;
   }

   return cNumer;
}

static const char *TypZDer( Bytes der )
{
   static const unsigned char aUzycie[] = { 0x06, 0x03, 0x55, 0x1D, 0x0F };

   size_t nPoz;
   int    nUzycie;

   for( nPoz = 0; nPoz + sizeof( aUzycie ) <= der.size && memcmp( der.data + nPoz, aUzycie, sizeof( aUzycie ) ) != 0; ++nPoz )
   {
   }
   if( nPoz + sizeof( aUzycie ) > der.size )
      return "";

   nPoz += sizeof( aUzycie );
   if( der.data[ nPoz ] == 0x01 )
      nPoz += 2 + der.data[ nPoz + 1 ];
   if( nPoz + 6 > der.size || der.data[ nPoz ] != 0x04 || der.data[ nPoz + 2 ] != 0x03 )
      return "";

   nUzycie = der.data[ nPoz + 5 ];
   if( nUzycie >= 0x80 )
      return "Authentication";

   if( nUzycie >= 0x40 )
      return "Offline";

   return "";
}

static void Sprawdz( Toolbar *toolbar, void *data )
{
   OknoCertyfikatow *oDialog = ( OknoCertyfikatow * )data;
   Json             *bilety;
   Bytes             klucz;
   Bytes             cert;
   char             *cHaslo;
   const char       *cWazny;
   int               nSrodowisko;
   char              cNip[ 64 ];
   char              cBlad[ 2000 ];
   char              cTekst[ 2200 ];

   UNUSED( toolbar );

   DbSelectArea( "CERTYFIKATY" );
   if( Eof() )
      return;

   if( strncmp( FieldGetC( FieldPos( "TYP_CERTYFIKATU" ) ), "Offline", 7 ) == 0 )
   {
      MsgInfo( "Certyfikat typu Offline służy tylko do kodu QR na fakturach offline.\nDo logowania w KSeF potrzebny jest certyfikat typu Authentication.", "Certyfikaty" );

      return;
   }

   nSrodowisko = ( int )FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) );
   StringFormat( cNip, "%s", FieldGetC( FieldPos( "NIP" ) ) );
   klucz.data = ( unsigned char * )KsefOdszyfruj( FieldGetC( FieldPos( "KLUCZ_ZASZYFROWANY" ) ) );
   klucz.size = strlen( ( const char * )klucz.data );
   cHaslo     = KsefOdszyfruj( FieldGetC( FieldPos( "HASLO_ZASZYFROWANE" ) ) );
   cert       = Base64Decode( FieldGetC( FieldPos( "CERTYFIKAT" ) ) );

   StringFormat( cTekst, " Logowanie do KSeF certyfikatem (%s)...", KsefSrodowisko( nSrodowisko, F ) );
   StatusBarSetText( oDialog->pasekStanu, 1, cTekst );
   bilety = KsefZalogujXades( AllTrim( cNip ), klucz, cert, cHaslo, nSrodowisko, cBlad, sizeof( cBlad ) );
   StatusBarSetText( oDialog->pasekStanu, 1, "" );
   BytesFree( &klucz );
   BytesFree( &cert );
   free( cHaslo );

   if( bilety == NULL )
      MsgError( cBlad, "Połączenie z KSeF" );
   else
   {
      cWazny = JsonString( JsonGet( JsonGet( bilety, "accessToken" ), "validUntil" ) );
      StringFormat( cTekst, "Zalogowano do KSeF certyfikatem (%s).\nBilet dostępu ważny do: %s", KsefSrodowisko( nSrodowisko, F ), IIF( cWazny == NULL, "", cWazny ) );
      KsefWyloguj( nSrodowisko, bilety );
      JsonFree( bilety );
      MsgInfo( cTekst, "Połączenie z KSeF" );
   }
   DbSelectArea( "CERTYFIKATY" );
}

static void Usun( Toolbar *toolbar, void *data )
{
   OknoCertyfikatow *oDialog = ( OknoCertyfikatow * )data;
   char              cNazwa[ 241 ];
   char              cPytanie[ 600 ];

   UNUSED( toolbar );

   DbSelectArea( "CERTYFIKATY" );
   if( Eof() )
      return;

   StringFormat( cNazwa, "%s", FieldGetC( FieldPos( "NAZWA_CERTYFIKATU" ) ) );
   StringFormat( cPytanie, "Czy na pewno usunąć certyfikat\n%s?\n\nZaszyfrowany klucz prywatny zostanie usunięty z xBiura.\nZachowaj pliki .crt i .key, jeśli certyfikat jest jeszcze potrzebny.", AllTrim( cNazwa ) );
   if( !MsgYesNo( cPytanie, "Certyfikaty" ) )
      return;

   DbSelectArea( "CERTYFIKATY" );
   if( NetRLock( NET_WAIT ) )
   {
      DbDelete();
      DbCommit();
      DbUnlock();
      DbSkip( 1 );
      if( Eof() )
         DbGoBottom();
   }
   Okno( oDialog );
   Stan( oDialog );
}

static void Zamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( OknoCertyfikatow * )data )->okno );
}
