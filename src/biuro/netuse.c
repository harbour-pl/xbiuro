/*
 * Program...: xBiuro
 * Plik......: src/biuro/netuse.c
 *
 */

#include "biuro.h"

static xBool NetWait( int *czekanie, int sekundy, const char *tekst, xBool bezPytania );
static xBool NetMsgYesNo( const char *tekst );
static void NetMsgError( const char *tekst, const char *powod );

// Otwiera tabelę w sieci
// NetUse( baza, wylacznie, sekundy, alias, bezPytania ) --> lOtwarta
xBool NetUse( const char *baza, xBool wylacznie, int sekundy, const char *alias, xBool bezPytania )
{
   char tekst[ 300 ];
   int  czekanie = sekundy * 1000;

   StringFormat( tekst, "Nie można otworzyć pliku %s w trybie %s", baza, IIF( wylacznie, "wyłącznym", "współdzielonym" ) );

   do
   {
      if( DbUseArea( T, baza, alias, !wylacznie, F ) )
         return T;

      if( !NetErr() )
      {
         if( !bezPytania )
            NetMsgError( tekst, DbError() );

         return F;
      }
   }
   while( NetWait( &czekanie, sekundy, tekst, bezPytania ) );

   return F;
}

// PACK tabeli otwartej wyłącznie
// NetPack() --> lSpakowana
xBool NetPack( void )
{
   if( !Used() )
   {
      NetMsgError( "PACK nieudany", "Żadna baza nie jest otwarta" );
      return F;
   }

   if( !DbPack() )
   {
      NetMsgError( "PACK nieudany", DbError() );
      return F;
   }

   return T;
}

// ZAP tabeli otwartej wyłącznie
// NetZap() --> lWyczyszczona
xBool NetZap( void )
{
   if( !Used() )
   {
      NetMsgError( "ZAP nieudany", "Żadna baza nie jest otwarta" );
      return F;
   }

   if( !DbZap() )
   {
      NetMsgError( "ZAP nieudany", DbError() );
      return F;
   }

   return T;
}

// Dopisuje i blokuje pusty rekord
// NetAppBlank( sekundy, zdejmijBlokady ) --> lDopisany
xBool NetAppBlank( int sekundy, xBool zdejmijBlokady )
{
   int czekanie = sekundy * 1000;

   if( !Used() )
   {
      NetMsgError( "Nie można dopisać rekordu", "Żadna baza nie jest otwarta" );
      return F;
   }

   do
   {
      if( DbAppend( zdejmijBlokady ) )
         return T;

      if( !NetErr() )
      {
         NetMsgError( "Nie można dopisać rekordu", DbError() );
         return F;
      }
   }
   while( NetWait( &czekanie, sekundy, "Nie można dopisać rekordu", F ) );

   return F;
}

// Blokuje bieżący rekord
// NetRLock( sekundy ) --> lZablokowany
xBool NetRLock( int sekundy )
{
   int czekanie = sekundy * 1000;

   if( !Used() )
   {
      NetMsgError( "Nie można zablokować rekordu", "Żadna baza nie jest otwarta" );
      return F;
   }

   do
   {
      if( RLock() )
         return T;
   }
   while( NetWait( &czekanie, sekundy, "Nie można zablokować rekordu", F ) );

   return F;
}

// Czeka na następną próbę
// NetWait( czekanie, sekundy, tekst, bezPytania ) --> lDalej
static xBool NetWait( int *czekanie, int sekundy, const char *tekst, xBool bezPytania )
{
   if( *czekanie > 0 )
   {
      IdleSleep( 0.1 );
      *czekanie -= 100;
      return T;
   }

   if( bezPytania || !NetMsgYesNo( tekst ) )
      return F;

   *czekanie = sekundy * 1000;

   return T;
}

// Pytanie o następną próbę
// NetMsgYesNo( tekst ) --> lTak
static xBool NetMsgYesNo( const char *tekst )
{
   char pytanie[ 400 ];

   StringFormat( pytanie, "%s.\nSpróbować ponownie?", tekst );

   return MsgYesNo( pytanie, "Baza danych" );
}

// Komunikat błędu tabeli
// NetMsgError( tekst, powod ) --> void
static void NetMsgError( const char *tekst, const char *powod )
{
   char komunikat[ 600 ];

   StringFormat( komunikat, "%s.\n%s.", tekst, powod );
   MsgError( komunikat, "Baza danych" );
}
