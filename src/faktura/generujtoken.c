/*
 * Program...: xBiuro
 * Plik......: src/faktura/generujtoken.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "faktura.h"

#define UPRAWNIENIA 5

typedef struct
{
   Biuro     *biuro;
   Win       *okno;
   StatusBar *pasekStanu;
   Toolbar   *pasek;
   Win       *panel;
   Control   *NIP;
   Control   *SRODOWISKO_KSEF;
   Control   *CERTYFIKAT;
   Control   *aUprawnienia[ UPRAWNIENIA ];
   Control   *NAZWA_TOKENU;
   Control   *WAZNY_DO;
   xBool      aWybrane[ UPRAWNIENIA ];
   char       cNip[ 64 ];
   char       cNazwa[ 241 ];
   int        nSrodowisko;
   int        dWaznyDo;
} OknoTokenu;

static const char *aKody[ UPRAWNIENIA ]  = { "InvoiceWrite", "InvoiceRead", "Introspection", "CredentialsRead", "CredentialsManage" };
static const char *aOpisy[ UPRAWNIENIA ] = { "Wystawianie faktur", "Przeglądanie faktur", "Przeglądanie historii sesji i generowanie UPO", "Przeglądanie uprawnień", "Zarządzanie uprawnieniami" };

static void     Pomoc( Toolbar *toolbar, void *data );
static void     Okno( OknoTokenu *oDialog );
static void     Opis( Win *strona, int y, const char *tekst );
static Control *Pole( Win *strona, int x, int y, int dlugosc );
static void     Generuj( Toolbar *toolbar, void *data );
static xBool    Wybor( OknoTokenu *oDialog );
static void     Zamknij( Toolbar *toolbar, void *data );

void GenerujToken( Win *win, void *data )
{
   OknoTokenu  oknoTokenu = { 0 };
   OknoTokenu *oDialog    = &oknoTokenu;
   Win        *panel;
   Control    *ramka;
   char        cData[ 16 ];
   int         przycisk;

   oDialog->biuro = ( Biuro * )data;
   if( !FirmaPobierzKsef( oDialog->biuro, oDialog->cNip, sizeof( oDialog->cNip ), &oDialog->nSrodowisko ) )
      return;

   if( *oDialog->cNip == 0 )
   {
      MsgInfo( "Najpierw wpisz NIP firmy w menu Firma, Dane firmy.", "Generuj token" );

      return;
   }

   StringFormat( oDialog->cNazwa, "xBiuro %s", DToC( Date(), cData, sizeof( cData ) ) );
   oDialog->dWaznyDo      = DateAdd( Date(), 90 );
   oDialog->aWybrane[ 0 ] = T;
   oDialog->aWybrane[ 1 ] = T;

   oDialog->okno = DialogCreate( win, "Generuj token", 650, 450 );
   WinSetBackground( oDialog->okno, 0xE9EDF1 );

   oDialog->pasekStanu = StatusBarCreate( oDialog->okno, "" );
   StatusBarAdd( oDialog->pasekStanu, 0, "", F );
   StatusBarAdd( oDialog->pasekStanu, 60, "", F );
   StatusBarSetScoreboard( oDialog->pasekStanu, 2 );

   oDialog->pasek = ToolbarCreate( oDialog->okno, TOOLBAR_BOTTOM );
   przycisk = ToolbarAdd( oDialog->pasek, "F1-Pomoc", Pomoc, NULL );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "F1" );
   ToolbarAddSeparator( oDialog->pasek );
   przycisk = ToolbarAdd( oDialog->pasek, "F2-Generuj", Generuj, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "F2" );
   ToolbarAddSeparator( oDialog->pasek );
   przycisk = ToolbarAdd( oDialog->pasek, "Esc-Zamknij", Zamknij, oDialog );
   ToolbarSetShortcut( oDialog->pasek, przycisk, "Esc" );

   panel          = PanelCreate( oDialog->okno, 0, 0, 0, 0 );
   oDialog->panel = panel;

   Opis( panel, 13, "NIP / środowisko:" );
   Opis( panel, 43, "Certyfikat:" );
   oDialog->NIP             = Pole( panel, 200, 10, 20 );
   oDialog->SRODOWISKO_KSEF = Pole( panel, 398, 10, 24 );
   oDialog->CERTYFIKAT      = Pole( panel, 200, 40, 46 );

   ramka = GroupBoxCreate( panel, 10, 76, WinWidth( panel ) - 20, 180, "Uprawnienia tokenu" );
   GroupBoxSetLineColor( ramka, 0xDCDCDC );
   for( int i = 0; i < UPRAWNIENIA; ++i )
   {
      oDialog->aUprawnienia[ i ] = CheckBoxCreate( panel, 30, 101 + 30 * i, 580, 20, aOpisy[ i ] );
   }

   Opis( panel, 273, "Nazwa tokenu:" );
   Opis( panel, 303, "Ważny do:" );
   oDialog->NAZWA_TOKENU = Pole( panel, 200, 270, 46 );
   oDialog->WAZNY_DO     = Pole( panel, 200, 300, 10 );

   Okno( oDialog );

   WinCenter( oDialog->okno );
   DialogShowModal( oDialog->okno );
}

static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgHelp( "Token KSeF to klucz, którym xBiuro loguje się do KSeF. Tu xBiuro generuje go samo: loguje się do KSeF certyfikatem typu Authentication z listy certyfikatów (NIP firmy z menu Firma, Dane firmy, środowisko z menu KSeF, Środowisko KSeF), zakłada token z zaznaczonymi uprawnieniami i od razu zapisuje go w liście tokenów. KSeF pokazuje token tylko raz.\n\n"
            "F1\tpomoc\n"
            "F2\tnowy token: uprawnienia, nazwa i data ważności (najwyżej rok)\n"
            "Esc\tzamknięcie okna\n\n"
            "Wpisywanie danych\n\n"
            "Spacja\tzaznaczenie uprawnienia\n"
            "Enter\tnastępne pole, na ostatnim generowanie\n"
            "PgUp, PgDn\tgenerowanie\n"
            "Esc\tbez generowania", "Generuj token" );
}

static void Okno( OknoTokenu *oDialog )
{
   Certyfikat cert;
   char       cData[ 16 ];
   char       cTekst[ 300 ];

   if( CertyfikatyPobierz( oDialog->cNip, oDialog->nSrodowisko, "Authentication", &cert ) )
   {
      StringFormat( cTekst, "%s, ważny do %s", cert.cNazwa, DToC( cert.dWaznyDo, cData, sizeof( cData ) ) );
      CertyfikatFree( &cert );
   }
   else
      StringFormat( cTekst, "brak ważnego certyfikatu typu Authentication" );

   ControlSetText( oDialog->NIP, oDialog->cNip );
   ControlSetText( oDialog->SRODOWISKO_KSEF, KsefSrodowisko( oDialog->nSrodowisko, F ) );
   ControlSetText( oDialog->CERTYFIKAT, cTekst );
   ControlSetText( oDialog->NAZWA_TOKENU, oDialog->cNazwa );
   ControlSetText( oDialog->WAZNY_DO, DToC( oDialog->dWaznyDo, cData, sizeof( cData ) ) );
   for( int i = 0; i < UPRAWNIENIA; ++i )
   {
      CheckBoxSetChecked( oDialog->aUprawnienia[ i ], oDialog->aWybrane[ i ] );
      ControlSetEnabled( oDialog->aUprawnienia[ i ], F );
   }
}

static void Opis( Win *strona, int y, const char *tekst )
{
   SayCreate( strona, 20, y, 0, 20, tekst );
}

static Control *Pole( Win *strona, int x, int y, int dlugosc )
{
   Control *pole = SayCreate( strona, x, y, dlugosc, 24, "" );

   LabelSetBorder( pole, T );

   return pole;
}

static void Generuj( Toolbar *toolbar, void *data )
{
   OknoTokenu *oDialog = ( OknoTokenu * )data;
   Certyfikat  cert;
   TokenKsef   token;
   size_t      nDlugosc = 0;
   xBool       lJest;
   char        cUprawnienia[ 300 ];
   char        cData[ 16 ];
   char        cBlad[ 2000 ];
   char        cTekst[ 2600 ];

   UNUSED( toolbar );

   if( !CertyfikatyPobierz( oDialog->cNip, oDialog->nSrodowisko, "Authentication", &cert ) )
   {
      StringFormat( cTekst, "Brak ważnego certyfikatu KSeF typu Authentication dla NIP %s\ni środowiska %s.\nWczytaj go w menu KSeF, Tokeny / Certyfikaty, Lista certyfikatów.", oDialog->cNip, IIF( oDialog->nSrodowisko == SRODOWISKO_PRODUKCYJNE, "produkcyjnego", "testowego" ) );
      MsgInfo( cTekst, "Generuj token" );

      return;
   }

   if( !Wybor( oDialog ) )
   {
      CertyfikatFree( &cert );

      return;
   }

   StringFormat( cTekst, "Wygenerować w KSeF token „%s”\ndla NIP %s (środowisko %s), ważny do %s?", oDialog->cNazwa, oDialog->cNip, IIF( oDialog->nSrodowisko == SRODOWISKO_PRODUKCYJNE, "produkcyjne", "testowe" ), DToC( oDialog->dWaznyDo, cData, sizeof( cData ) ) );
   if( !MsgYesNo( cTekst, "Generuj token" ) )
   {
      CertyfikatFree( &cert );

      return;
   }

   *cUprawnienia = 0;
   for( int i = 0; i < UPRAWNIENIA; ++i )
   {
      if( oDialog->aWybrane[ i ] )
      {
         StringFormatSize( cUprawnienia + nDlugosc, sizeof( cUprawnienia ) - nDlugosc, "%s\"%s\"", IIF( nDlugosc == 0, "", "," ), aKody[ i ] );
         nDlugosc = strlen( cUprawnienia );
      }
   }

   StatusBarSetText( oDialog->pasekStanu, 1, " Generowanie tokenu w KSeF..." );
   lJest = KsefGenerujToken( oDialog->cNip, &cert, oDialog->nSrodowisko, oDialog->cNazwa, cUprawnienia, oDialog->dWaznyDo, &token, cBlad, sizeof( cBlad ) );
   StatusBarSetText( oDialog->pasekStanu, 1, "" );
   CertyfikatFree( &cert );
   if( !lJest )
   {
      MsgError( cBlad, "Generuj token" );

      return;
   }

   if( TokenyZapisz( oDialog->cNazwa, oDialog->cNip, oDialog->nSrodowisko, token.cToken, oDialog->dWaznyDo ) )
   {
      StringFormat( cTekst, "Token „%s” jest w KSeF i w liście tokenów.\n\nNumer referencyjny: %s\nStan: %s\nWażny do: %s", oDialog->cNazwa, token.cNumer, token.cStatus, DToC( oDialog->dWaznyDo, cData, sizeof( cData ) ) );
      MsgInfo( cTekst, "Generuj token" );
   }
   else
   {
      StringFormat( cTekst, "Token powstał w KSeF, ale nie został zapisany w liście tokenów.\nKSeF pokazuje go tylko raz, więc dodaj go teraz ręcznie (Lista tokenów, F2):\n\n%s", token.cToken );
      MsgError( cTekst, "Generuj token" );
   }
   free( token.cToken );
}

static xBool Wybor( OknoTokenu *oDialog )
{
   Control *GetList[ UPRAWNIENIA + 2 ];
   xBool    lWybrane = F;
   int      dWaznyDo = oDialog->dWaznyDo;
   int      dDzis    = Date();
   char     cNazwa[ 241 ];
   char     cData[ 16 ];
   char     cTekst[ 300 ];

   StringFormat( cNazwa, "%s", oDialog->cNazwa );
   for( int i = 0; i < UPRAWNIENIA; ++i )
   {
      ControlSetEnabled( oDialog->aUprawnienia[ i ], T );
      GetList[ i ] = oDialog->aUprawnienia[ i ];
   }
   GetList[ UPRAWNIENIA ]     = GetText( oDialog->panel, 200, 270, 46, cNazwa, sizeof( cNazwa ), "" );
   GetList[ UPRAWNIENIA + 1 ] = GetDate( oDialog->panel, 200, 300, &dWaznyDo );

   PozycjeTrybEdycji( oDialog->pasekStanu, T );
   WinReadModal( oDialog->okno, GetList, UPRAWNIENIA + 2, 1 );
   PozycjeTrybEdycji( oDialog->pasekStanu, F );
   ControlDestroy( GetList[ UPRAWNIENIA ] );
   ControlDestroy( GetList[ UPRAWNIENIA + 1 ] );

   if( ReadLastKey() == K_ESC )
   {
      Okno( oDialog );

      return F;
   }

   for( int i = 0; i < UPRAWNIENIA; ++i )
   {
      oDialog->aWybrane[ i ] = CheckBoxChecked( oDialog->aUprawnienia[ i ] );
      if( oDialog->aWybrane[ i ] )
         lWybrane = T;
   }
   AllTrim( cNazwa );
   if( ULen( cNazwa ) >= 5 )
      StringFormat( oDialog->cNazwa, "%s", cNazwa );
   if( dWaznyDo > dDzis && dWaznyDo <= DateAdd( dDzis, 364 ) )
      oDialog->dWaznyDo = dWaznyDo;
   Okno( oDialog );

   if( !lWybrane )
   {
      MsgInfo( "Zaznacz co najmniej jedno uprawnienie tokenu.", "Generuj token" );

      return F;
   }

   if( ULen( cNazwa ) < 5 )
   {
      MsgInfo( "Nazwa tokenu musi mieć co najmniej 5 znaków.", "Generuj token" );

      return F;
   }

   if( dWaznyDo <= dDzis || dWaznyDo > DateAdd( dDzis, 364 ) )
   {
      StringFormat( cTekst, "Token może być ważny od jutra najdłużej do %s.", DToC( DateAdd( dDzis, 364 ), cData, sizeof( cData ) ) );
      MsgInfo( cTekst, "Generuj token" );

      return F;
   }

   return T;
}

static void Zamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( OknoTokenu * )data )->okno );
}
