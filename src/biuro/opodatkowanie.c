/*
 * Program...: xBiuro
 * Plik......: src/biuro/opodatkowanie.c
 *
 */

#include <string.h>

#include "biuro.h"

/* Pola tabeli opodatkowanie */
static const DbField aDbf[] = {
   { "FORMA_OPODATKOWANIA", 'N', 1, 0 },   /* FormaOpodatkowania z biuro.h, 0 przed wyborem */
   { "EWIDENCJA",           'N', 1, 0 }    /* RodzajEwidencji z biuro.h, 0 przed wyborem */
};

#define POLA_OPODATKOWANIA ( ( int )( sizeof( aDbf ) / sizeof( aDbf[ 0 ] ) ) )

/* Okno formy opodatkowania; zmienna lokalna Opodatkowanie(), przekazywana parametrem */
typedef struct
{
   Biuro     *biuro;
   Win       *okno;
   StatusBar *pasekStanu;
   Control   *aPola[ 6 ];   /* przyciski opcji: 3 formy w kolejności FormaOpodatkowania, potem 3 ewidencje w kolejności RodzajEwidencji */
} OknoOpodatkowania;

static xBool DbOpen( const char *cBaza );
static void  Przebudowa( const char *cBaza );
static int   Ewidencja( Biuro *biuro );
static void  Okno( OknoOpodatkowania *oOkno );
static xBool Zgodna( OknoOpodatkowania *oOkno );
static int   Wybrany( Control *aOpcje[] );
static void  Pomoc( Toolbar *toolbar, void *data );
static void  Nowy( Toolbar *toolbar, void *data );
static void  Zamknij( Toolbar *toolbar, void *data );

// Okno formy opodatkowania i ewidencji
// Opodatkowanie( win, data ) --> void
void Opodatkowanie( Win *win, void *data )
{
   static const char *aOpis[] = { "Zasady ogólne – skala podatkowa (art. 27)", "Podatek liniowy (art. 30c)", "Ryczałt od przychodów ewidencjonowanych", "Podatkowa księga przychodów i rozchodów", "Księgi rachunkowe", "Ewidencja przychodów" };

   OknoOpodatkowania  oknoOpodatkowania = { 0 };
   OknoOpodatkowania *oOkno             = &oknoOpodatkowania;
   Toolbar           *pasek;
   Win               *panel;
   char               cKatalog[ 64 ];
   char               cBaza[ 80 ];
   int                przycisk;

   oOkno->biuro = ( Biuro * )data;
   StringFormat( cBaza, "%sopodatkowanie", RokFirma( oOkno->biuro, oOkno->biuro->firma, cKatalog, sizeof( cKatalog ) ) );
   if( !DbOpen( cBaza ) )
      return;

   oOkno->okno = DialogCreate( win, "Forma opodatkowania", 620, 400 );
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
   przycisk = ToolbarAdd( pasek, "Esc-Zamknij", Zamknij, oOkno );
   ToolbarSetShortcut( pasek, przycisk, "Esc" );

   /* Ramka przed każdą grupą przycisków opcji, więc formy i ewidencje wybiera się osobno */
   panel = PanelCreate( oOkno->okno, 0, 0, 0, 0 );
   for( int i = 0; i < 6; ++i )
   {
      if( i == 0 )
         GroupBoxSetLineColor( GroupBoxCreate( panel, 10, 10, WinWidth( panel ) - 20, 120, "Forma opodatkowania (art. 9a ustawy o PIT)" ), 0xDCDCDC );

      if( i == 3 )
         GroupBoxSetLineColor( GroupBoxCreate( panel, 10, 140, WinWidth( panel ) - 20, 120, "Ewidencja (art. 24a ustawy o PIT, art. 15 ustawy o zryczałtowanym podatku)" ), 0xDCDCDC );

      oOkno->aPola[ i ] = RadioButtonCreate( panel, 30, 40 + 30 * i + 40 * ( i / 3 ), 400, 20, aOpis[ i ] );
      ControlSetEnabled( oOkno->aPola[ i ], F );
   }

   Okno( oOkno );

   WinCenter( oOkno->okno );
   DialogShowModal( oOkno->okno );

   DbSelectArea( "OPODATKOWANIE" );
   DbCloseArea();
}

// Czy moduł jest dostępny według ewidencji
// OpodatkowanieModul( biuro, cModul ) --> lDostepny
xBool OpodatkowanieModul( Biuro *biuro, const char *cModul )
{
   int nEwidencja = Ewidencja( biuro );

   if( strcmp( cModul, "PKPiR" ) == 0 )
      return IIF( nEwidencja == KSIEGA_PRZYCHODOW_I_ROZCHODOW, T, F );

   if( strcmp( cModul, "EP" ) == 0 )
      return IIF( nEwidencja == EWIDENCJA_PRZYCHODOW, T, F );

   if( strcmp( cModul, "KR" ) == 0 )
      return IIF( nEwidencja == KSIEGI_RACHUNKOWE, T, F );

   if( strcmp( cModul, "WY" ) == 0 )
      return F;

   return T;
}

// Otwiera tabelę formy opodatkowania
// DbOpen( cBaza ) --> lOtwarta
static xBool DbOpen( const char *cBaza )
{
   char cPlik[ 96 ];

   StringFormat( cPlik, "%s.dbf", cBaza );
   if( !FileExists( cPlik ) )
      DbCreate( cBaza, aDbf, POLA_OPODATKOWANIA );

   if( !NetUse( cBaza, F, NET_WAIT, "OPODATKOWANIE", F ) )
      return F;

   if( FCount() != POLA_OPODATKOWANIA )
   {
      DbCloseArea();
      MsgInfo( "Dodanie pola w bazie", "Forma opodatkowania" );
      Przebudowa( cBaza );
      if( !NetUse( cBaza, F, NET_WAIT, "OPODATKOWANIE", F ) )
         return F;
   }

   if( LastRec() == 0 && NetAppBlank( NET_WAIT, T ) )
   {
      DbCommit();
      DbUnlock();
   }

   DbGoTop();

   return T;
}

// Przebudowuje tabelę do pól aDbf
// Przebudowa( cBaza ) --> void
static void Przebudowa( const char *cBaza )
{
   static const int aEwidencja[] = { 0, KSIEGA_PRZYCHODOW_I_ROZCHODOW, KSIEGA_PRZYCHODOW_I_ROZCHODOW, EWIDENCJA_PRZYCHODOW, KSIEGI_RACHUNKOWE };

   char cDbf[ 96 ];
   char cTmp[ 96 ];
   char cTmpDbf[ 96 ];
   char cStara[ 96 ];
   int  nForma;

   StringFormat( cDbf, "%s.dbf", cBaza );
   StringFormat( cTmp, "%s-tmp", cBaza );
   StringFormat( cTmpDbf, "%s.dbf", cTmp );
   StringFormat( cStara, "%s.tmp", cBaza );

   if( FileExists( cDbf ) )
   {
      FileCopy( cDbf, cTmpDbf );
      FRename( cDbf, cStara );
      DbCreate( cBaza, aDbf, POLA_OPODATKOWANIA );
      if( NetUse( cBaza, F, NET_WAIT, "OPODATKOWANIE", F ) )
      {
         DbAppendFrom( cTmp, NULL, NULL );
         DbGoTop();
         nForma = ( int )FieldGetN( FieldPos( "FORMA_OPODATKOWANIA" ) );
         if( LastRec() > 0 && NetRLock( NET_WAIT ) )
         {
            FieldPutN( FieldPos( "EWIDENCJA" ), aEwidencja[ nForma ] );
            FieldPutN( FieldPos( "FORMA_OPODATKOWANIA" ), IIF( nForma == 4, 0, nForma ) );
            DbCommit();
            DbUnlock();
         }
         DbCloseArea();
      }
      FErase( cTmpDbf );
      FErase( cStara );
   }
}

// Ewidencja wybranej firmy
// Ewidencja( biuro ) --> nEwidencja
static int Ewidencja( Biuro *biuro )
{
   char cKatalog[ 64 ];
   char cBaza[ 80 ];
   char cPlik[ 96 ];
   int  nEwidencja = 0;

   StringFormat( cBaza, "%sopodatkowanie", RokFirma( biuro, biuro->firma, cKatalog, sizeof( cKatalog ) ) );
   StringFormat( cPlik, "%s.dbf", cBaza );
   if( FileExists( cPlik ) && NetUse( cBaza, F, NET_WAIT, "FORMA", T ) )
   {
      if( FieldPos( "EWIDENCJA" ) > 0 )
         nEwidencja = ( int )FieldGetN( FieldPos( "EWIDENCJA" ) );
      DbCloseArea();
   }

   return nEwidencja;
}

// Pokazuje wybraną formę i ewidencję
// Okno( oOkno ) --> void
static void Okno( OknoOpodatkowania *oOkno )
{
   int nForma;
   int nEwidencja;

   DbSelectArea( "OPODATKOWANIE" );
   nForma     = ( int )FieldGetN( FieldPos( "FORMA_OPODATKOWANIA" ) );
   nEwidencja = ( int )FieldGetN( FieldPos( "EWIDENCJA" ) );
   for( int i = 0; i < 3; ++i )
   {
      CheckBoxSetChecked( oOkno->aPola[ i ], IIF( nForma == i + 1, T, F ) );
      CheckBoxSetChecked( oOkno->aPola[ i + 3 ], IIF( nEwidencja == i + 1, T, F ) );
   }
}

// Czy ewidencja pasuje do formy opodatkowania
// Zgodna( oOkno ) --> lZgodna
static xBool Zgodna( OknoOpodatkowania *oOkno )
{
   int nForma     = Wybrany( oOkno->aPola );
   int nEwidencja = Wybrany( oOkno->aPola + 3 );

   if( nForma == RYCZALT && nEwidencja != EWIDENCJA_PRZYCHODOW )
   {
      MsgWarning( "Przy ryczałcie od przychodów ewidencjonowanych prowadzi się ewidencję przychodów\n(art. 15 ustawy o zryczałtowanym podatku dochodowym).", "Forma opodatkowania" );

      return F;
   }

   if( ( nForma == ZASADY_OGOLNE || nForma == PODATEK_LINIOWY ) && nEwidencja != KSIEGA_PRZYCHODOW_I_ROZCHODOW && nEwidencja != KSIEGI_RACHUNKOWE )
   {
      MsgWarning( "Przy zasadach ogólnych i podatku liniowym prowadzi się podatkową księgę przychodów i rozchodów\nalbo księgi rachunkowe (art. 24a ustawy o PIT).", "Forma opodatkowania" );

      return F;
   }

   return T;
}

// Numer zaznaczonej opcji
// Wybrany( aOpcje ) --> nWybrany
static int Wybrany( Control *aOpcje[] )
{
   for( int i = 0; i < 3; ++i )
   {
      if( CheckBoxChecked( aOpcje[ i ] ) )
         return i + 1;
   }

   return 0;
}

// F1: pomoc
// Pomoc( toolbar, data ) --> void
static void Pomoc( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );
   UNUSED( data );

   MsgHelp( "<a href=\"" USTAWA_PIT "\">Ustawa o podatku dochodowym od osób fizycznych</a> (art. 9a – forma opodatkowania, art. 24a – ewidencja)\n"
            "<a href=\"" USTAWA_RYCZALT "\">Ustawa o zryczałtowanym podatku dochodowym</a> (art. 9 – wybór ryczałtu, art. 15 – ewidencja przychodów)\n\n"
            "F1\tpomoc\n"
            "F4\tzmiana formy opodatkowania i ewidencji\n"
            "Esc\tzamknięcie okna\n\n"
            "Wpisywanie danych\n\n"
            "Strzałki\twybór formy i ewidencji\n"
            "Enter, PgUp, PgDn\tzapis\n"
            "Esc\tbez zapisu\n\n"
            "Ewidencja według formy opodatkowania\n\n"
            "Zasady ogólne, podatek liniowy\tPKPiR albo księgi rachunkowe\n"
            "Ryczałt\tewidencja przychodów\n\n"
            "Moduły według ewidencji\n\n"
            "Podatkowa księga przychodów i rozchodów\tPKPiR\n"
            "Księgi rachunkowe\tKsięgi rachunkowe (KR)\n"
            "Ewidencja przychodów\tEwidencja przychodów (EP)\n"
            "Zawsze\tFaktury (FV), Wynagrodzenia (WY)", "Forma opodatkowania" );
}

// F4: zmiana formy opodatkowania i ewidencji
// Nowy( toolbar, data ) --> void
static void Nowy( Toolbar *toolbar, void *data )
{
   OknoOpodatkowania *oOkno = ( OknoOpodatkowania * )data;

   UNUSED( toolbar );

   for( int i = 0; i < 6; ++i )
      ControlSetEnabled( oOkno->aPola[ i ], T );

   StatusBarSetText( oOkno->pasekStanu, 1, " PgUp PgDn Enter - zapisz, Esc - bez zapisu" );
   StatusBarSetBackground( oOkno->pasekStanu, 0x73264D );
   StatusBarSetForeground( oOkno->pasekStanu, 0xFFFFFF );

   do
   {
      WinReadModal( oOkno->okno, oOkno->aPola, 6, 0 );
   } while( ReadLastKey() != K_ESC && !Zgodna( oOkno ) );

   StatusBarSetText( oOkno->pasekStanu, 1, "" );
   StatusBarSetBackground( oOkno->pasekStanu, -1 );
   StatusBarSetForeground( oOkno->pasekStanu, -1 );

   for( int i = 0; i < 6; ++i )
      ControlSetEnabled( oOkno->aPola[ i ], F );

   DbSelectArea( "OPODATKOWANIE" );
   if( ReadLastKey() == K_ESC || !NetRLock( NET_WAIT ) )
   {
      Okno( oOkno );

      return;
   }

   FieldPutN( FieldPos( "FORMA_OPODATKOWANIA" ), Wybrany( oOkno->aPola ) );
   FieldPutN( FieldPos( "EWIDENCJA" ), Wybrany( oOkno->aPola + 3 ) );

   DbCommit();
   DbUnlock();

   Okno( oOkno );
   FirmaZmieniona( oOkno->biuro );
}

// Esc: zamyka okno
// Zamknij( toolbar, data ) --> void
static void Zamknij( Toolbar *toolbar, void *data )
{
   UNUSED( toolbar );

   WinClose( ( ( OknoOpodatkowania * )data )->okno );
}
