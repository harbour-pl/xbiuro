/*
 * Program...: xBiuro
 * Plik......: src/firma.c
 *
 */

#include <ctype.h>
#include <string.h>

#include "biuro.h"

/* Okno danych firmy: pola i zakładki podmiotu z podmiot.c; zmienna lokalna Firma(), przekazywana parametrem jak aSet
   w firma.prg */
typedef struct
{
   Biuro     *biuro;
   Win       *okno;
   StatusBar *pasekStanu;
   Podmiot    podmiot;
} OknoFirmy;

static void Pomoc( Toolbar *toolbar, void *data );
static void Nowy( Toolbar *toolbar, void *data );
static void Drukuj( Win *win, void *data );
static void Zamknij( Toolbar *toolbar, void *data );

// Okno danych firmy
// Firma( win, data ) --> void
void Firma( Win *win, void *data )
{
   OknoFirmy  oknoFirmy = { 0 };
   OknoFirmy *oOkno     = &oknoFirmy;
   Toolbar   *pasek;
   Menu      *oDrukuj;
   Win       *panel;
   Control   *zakladka;
   char       cBaza[ 64 ];
   char       cKatalog[ 64 ];
   int        przycisk;

   oOkno->biuro = ( Biuro * )data;
   StringFormat( cBaza, "%sfirma", RokFirma( oOkno->biuro, oOkno->biuro->firma, cKatalog, sizeof( cKatalog ) ) );
   if( !FirmaDbOpen( cBaza ) )
      return;

   oOkno->okno = DialogCreate( win, "Dane firmy", 900, 590 );
   WinSetBackground( oOkno->okno, 0xE9EDF1 );

   oOkno->pasekStanu = StatusBarCreate( oOkno->okno, "" );
   StatusBarAdd( oOkno->pasekStanu, 0, "", F );
   StatusBarAdd( oOkno->pasekStanu, 60, "", F );
   StatusBarSetScoreboard( oOkno->pasekStanu, 2 );

   pasek = ToolbarCreate( oOkno->okno, TOOLBAR_BOTTOM );
   przycisk = ToolbarAdd( pasek, "F1-Pomoc", Pomoc, NULL );
   ToolbarSetShortcut( pasek, przycisk, "F1" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "F4-Popraw", Nowy, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "F4" );
   ToolbarAddSeparator( pasek );
   oDrukuj = MenuCreatePopup();
   MenuAdd( oDrukuj, "Karta firmy", Drukuj, oOkno, "" );
   przycisk = ToolbarAddMenu( pasek, "F9-Drukuj", oDrukuj );
   ToolbarSetShortcut( pasek, przycisk, "F9" );
   ToolbarAddSeparator( pasek );
   przycisk = ToolbarAdd( pasek, "Esc-Zamknij", Zamknij, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "Esc" );

   panel = PanelCreate( oOkno->okno, 0, 0, 0, 0 );
   zakladka = TabControlCreate( panel, 10, 10, WinWidth( panel ) - 20, WinHeight( panel ) - 20 );
   ControlSetAnchor( zakladka, "LTRB" );
   oOkno->podmiot.cAlias = "FIRMA";
   PodmiotZakladki( &oOkno->podmiot, zakladka, "Dane firmy" );

   PodmiotOkno( &oOkno->podmiot );

   WinCenter( oOkno->okno );
   DialogShowModal( oOkno->okno );

   DbSelectArea( "FIRMA" );
   DbCloseArea();
}

// Otwiera tabelę firmy (wzór z podmiot.c); pusta tabela dostaje rekord z krajem PL, pierwszym rachunkiem i VAT
// FirmaDbOpen( cBaza ) --> lOtwarta
xBool FirmaDbOpen( const char *cBaza )
{
   if( !PodmiotDbOpen( cBaza, "FIRMA" ) )
      return F;

   if( LastRec() == 0 && NetAppBlank( NET_WAIT, T ) )
   {
      FieldPutC( FieldPos( "KOD_KRAJU" ), "PL" );
      FieldPutN( FieldPos( "DRUKUJ_RACHUNEK_NR" ), 1 );
      FieldPutL( FieldPos( "PODATNIK_VAT" ), T );
      FieldPutN( FieldPos( "SRODOWISKO_KSEF" ), SRODOWISKO_TESTOWE );
      DbCommit();
      DbUnlock();
   }

   DbGoTop();

   return T;
}

// Nazwa skrócona i NIP firmy
// FirmaOpis( biuro, firma, nazwa, rozmiarNazwy, nip, rozmiarNip ) --> void
void FirmaOpis( Biuro *biuro, int firma, char *nazwa, int rozmiarNazwy, char *nip, int rozmiarNip )
{
   char cBaza[ 64 ];
   char cKatalog[ 64 ];
   char cPlik[ 64 ];

   *nazwa = 0;
   *nip = 0;

   StringFormat( cBaza, "%sfirma", RokFirma( biuro, firma, cKatalog, sizeof( cKatalog ) ) );
   StringFormat( cPlik, "%s.dbf", cBaza );
   if( !FileExists( cPlik ) || !NetUse( cBaza, F, NET_WAIT, "OPIS", T ) )
      return;

   StringFormatSize( nazwa, ( size_t )rozmiarNazwy, "%s", FieldGetC( FieldPos( "SKROCONA_NAZWA" ) ) );
   StringFormatSize( nip, ( size_t )rozmiarNip, "%s", FieldGetC( FieldPos( "NIP" ) ) );
   RTrim( nazwa );
   RTrim( nip );

   DbCloseArea();
}

// Nazwa pełna firmy w jednej linii, do tytułu okna
// FirmaNazwa( biuro, firma, nazwa, rozmiar ) --> void
void FirmaNazwa( Biuro *biuro, int firma, char *nazwa, int rozmiar )
{
   char cBaza[ 64 ];
   char cKatalog[ 64 ];
   char cPlik[ 64 ];
   char cTekst[ 2049 ];

   *nazwa = 0;

   StringFormat( cBaza, "%sfirma", RokFirma( biuro, firma, cKatalog, sizeof( cKatalog ) ) );
   StringFormat( cPlik, "%s.dbf", cBaza );
   if( !FileExists( cPlik ) || !NetUse( cBaza, F, NET_WAIT, "OPIS", T ) )
      return;

   StrTran( cTekst, sizeof( cTekst ), FieldGetC( FieldPos( "PELNA_NAZWA" ) ), "\r\n", " " );
   StrTran( nazwa, ( size_t )rozmiar, cTekst, "\n", " " );
   AllTrim( nazwa );

   DbCloseArea();
}

// Sprawdza NIP
// FirmaNipPoprawny( cNip ) --> lPoprawny
xBool FirmaNipPoprawny( const char *cNip )
{
   static const int aWaga[] = { 6, 5, 7, 2, 3, 4, 5, 6, 7 };
   char cTekst[ 64 ];
   char cCyfry[ 64 ];
   char cBezKresek[ 64 ];
   char cBezSpacji[ 64 ];
   int  nSuma = 0;

   StringFormat( cTekst, "%s", cNip );
   AllTrim( cTekst );
   if( toupper( ( unsigned char )cTekst[ 0 ] ) == 'P' && toupper( ( unsigned char )cTekst[ 1 ] ) == 'L' )
      memmove( cTekst, cTekst + 2, strlen( cTekst + 2 ) + 1 );

   FirmaCyfry( cTekst, cCyfry, sizeof( cCyfry ) );
   StrTran( cBezKresek, sizeof( cBezKresek ), cTekst, "-", "" );
   StrTran( cBezSpacji, sizeof( cBezSpacji ), cBezKresek, " ", "" );
   if( strcmp( cCyfry, cBezSpacji ) != 0 )
      return F;

   if( strlen( cCyfry ) != 10 )
      return F;

   for( int i = 0; i < 9; ++i )
      nSuma += ( cCyfry[ i ] - '0' ) * aWaga[ i ];

   return IIF( nSuma % 11 == cCyfry[ 9 ] - '0', T, F );
}

// Same cyfry tekstu
// FirmaCyfry( cTekst, cWynik, nRozmiar ) --> void
void FirmaCyfry( const char *cTekst, char *cWynik, int nRozmiar )
{
   int nDlugosc = 0;

   for( ; *cTekst != 0 && nDlugosc < nRozmiar - 1; ++cTekst )
   {
      if( isdigit( ( unsigned char )*cTekst ) )
         cWynik[ nDlugosc++ ] = *cTekst;
   }

   cWynik[ nDlugosc ] = 0;
}

xBool FirmaPobierzDane( Biuro *biuro, const char *cAlias )
{
   char cKatalog[ 64 ];
   char cBaza[ 80 ];

   StringFormat( cBaza, "%sfirma", RokFirma( biuro, biuro->firma, cKatalog, sizeof( cKatalog ) ) );

   return PodmiotDbOpen( cBaza, cAlias );
}

// Katalog firmy na rok danych, jak aSet[ _RokFirma ] w programie taty: dane-RRRR/firmaN/, a dla firmy 0 (administratora)
// dane-RRRR/admin/
// RokFirma( biuro, firma, cKatalog, nRozmiar ) --> cKatalog
char *RokFirma( Biuro *biuro, int firma, char *cKatalog, int nRozmiar )
{
   return KatalogFirmy( biuro->rok, firma, cKatalog, nRozmiar );
}

char *KatalogFirmy( int nRok, int firma, char *cKatalog, int nRozmiar )
{
   if( firma == 0 )
      StringFormatSize( cKatalog, ( size_t )nRozmiar, "dane-%d/admin/", nRok );
   else
      StringFormatSize( cKatalog, ( size_t )nRozmiar, "dane-%d/firma%d/", nRok, firma );

   return cKatalog;
}

void FirmaZmieniona( Biuro *biuro )
{
   biuro->zmianaFirmy( biuro );
}

// F1: pomoc
// Pomoc( toolbar, data ) --> void
static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgHelp( POMOC_PODMIOT
            "Oglądanie\n\n"
            "F1\tpomoc\n"
            "F4\tpoprawianie danych firmy\n"
            "F9\twydruk: karta firmy\n"
            "Esc\tzamknięcie okna\n\n"
            "Wpisywanie danych\n\n"
            "Enter\tnastępne pole, na ostatnim polu ostatniej zakładki zapis\n"
            "Tab\tnastępne pole, bez zapisu\n"
            "Shift+Tab\tpoprzednie pole\n"
            "Ctrl+Tab\tnastępna zakładka danych\n"
            "Ins\twstawianie / zastępowanie znaków, Ins na pasku stanu\n"
            "Spacja\tzaznaczenie pola wyboru albo opcji\n"
            "Shift+Enter\tnowa linia w nazwie pełnej i uwagach\n"
            "PgUp, PgDn\tzapis\n"
            "Esc\tbez zapisu\n\n"
            "Zakładki\n\n"
            "Dane firmy\tNIP, nazwa pełna i skrócona, REGON, KRS\n"
            "Adres siedziby firmy\tkraj, województwo, powiat, gmina, ulica, kod, miejscowość\n"
            "Dane działalności\tPKD, BDO i wpis do ewidencji\n"
            "Rachunki bankowe\trachunki firmy i który drukować na fakturze\n"
            "Dane kontaktowe\ttelefony, e-mail i strona internetowa\n"
            "Dane do urzędów\turząd skarbowy firmy\n"
            "VAT\tpodatnik VAT, podstawa prawna zwolnienia z VAT\n"
            "Uwagi\tdowolne uwagi", "Dane firmy" );
}

// F4: poprawianie danych firmy; po zapisie lista firm i tytuł okna pokazują nowe dane
// Nowy( toolbar, data ) --> void
static void Nowy( Toolbar *toolbar, void *data )
{
   OknoFirmy *oOkno = ( OknoFirmy * )data;

   UNUSED( toolbar );

   if( PodmiotEdycja( &oOkno->podmiot, oOkno->okno, oOkno->pasekStanu, POPRAW ) )
      FirmaZmieniona( oOkno->biuro );
}

// F9: karta firmy ze wszystkimi danymi (wydruk.c)
// Drukuj( win, data ) --> void
static void Drukuj( Win *win, void *data )
{
   OknoFirmy *oOkno = ( OknoFirmy * )data;

   UNUSED( win );

   WydrukKarta( oOkno->biuro, "FIRMA", "Karta firmy", "karta-firmy.pdf" );
}

// Esc: zamyka okno
// Zamknij( toolbar, data ) --> void
static void Zamknij( Toolbar *toolbar, void *data )
{
   OknoFirmy *oOkno = ( OknoFirmy * )data;

   UNUSED( toolbar );

   WinClose( oOkno->okno );
}
