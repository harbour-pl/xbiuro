/*
 * Program...: xBiuro
 * Plik......: src/dbbrowse.c
 *
 * Browse na tabeli DBF, jak bloki guiBrowse w kartotekach xDev (us.prg): początek, koniec, przesunięcie, miejsce na
 * pasku przewijania i pusta tabela. Alias tabeli przychodzi jako data; akcje wybierają jej obszar i go zostawiają
 *
 */

#include "biuro.h"

static void   GoTop( Control *browse, void *data );
static void   GoBottom( Control *browse, void *data );
static int    Skip( Control *browse, int count, void *data );
static double Pozycja( Control *browse, void *data );
static xBool  Pusta( Control *browse, void *data );

// Browse czyta tabelę o aliasie alias
// DbBrowse( browse, alias ) --> void
void DbBrowse( Control *browse, const char *alias )
{
   BrowseSetSource( browse, GoTop, GoBottom, Skip, ( void * )alias );
   BrowseSetPosition( browse, Pozycja );
   BrowseSetEmpty( browse, Pusta );
}

// Tabela o aliasie alias bez rekordów (z filtrem i SetDeleted()); wybiera jej obszar, rekord zostaje ten sam, jak
// Pusta() w us.prg
// DbPusta( alias ) --> lPusta
xBool DbPusta( const char *alias )
{
   long  nRecNo;
   xBool lPusta;

   DbSelectArea( alias );
   nRecNo = RecNo();
   DbGoTop();
   lPusta = Eof();
   DbGoto( nRecNo );

   return lPusta;
}

// Pierwszy rekord
// GoTop( browse, data ) --> void
static void GoTop( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( ( const char * )data );
   DbGoTop();
}

// Ostatni rekord
// GoBottom( browse, data ) --> void
static void GoBottom( Control *browse, void *data )
{
   UNUSED( browse );

   DbSelectArea( ( const char * )data );
   DbGoBottom();
}

// Przesuwa o count rekordów, nie dalej niż do pierwszego i ostatniego
// Skip( browse, count, data ) --> nSkipped
static int Skip( Control *browse, int count, void *data )
{
   UNUSED( browse );

   DbSelectArea( ( const char * )data );

   return ( int )DbSkipper( count );
}

// Miejsce rekordu 0..1 według indeksu, a bez indeksu według numeru rekordu
// Pozycja( browse, data ) --> nPozycja
static double Pozycja( Control *browse, void *data )
{
   long nKluczy;

   UNUSED( browse );

   DbSelectArea( ( const char * )data );
   nKluczy = OrdKeyCount();

   return IIF( nKluczy > 0, ( double )OrdKeyNo() / ( double )nKluczy, 0 );
}

// Tabela bez rekordów dla Browse
// Pusta( browse, data ) --> lPusta
static xBool Pusta( Control *browse, void *data )
{
   UNUSED( browse );

   return DbPusta( ( const char * )data );
}
