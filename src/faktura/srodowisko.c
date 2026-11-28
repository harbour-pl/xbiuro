/*
 * Program...: xBiuro
 * Plik......: src/faktura/srodowisko.c
 *
 */

#include "faktura.h"

typedef struct
{
   Win     *okno;
   Control *TESTOWE;
   Control *PRODUKCYJNE;
   int      nSrodowisko;
} OknoSrodowiska;

static void Ok( Control *button, void *data );
static void Anuluj( Control *button, void *data );

void SrodowiskoKsef( Win *win, void *data )
{
   Biuro          *biuro = ( Biuro * )data;
   OknoSrodowiska  oSrod = { 0 };
   Control        *ramka;
   Control        *oOk;
   Control        *oAnuluj;
   int             nJest;

   if( !FirmaPobierzDane( biuro, "FIRMA_KSEF" ) )
      return;

   nJest = IIF( FieldGetN( FieldPos( "SRODOWISKO_KSEF" ) ) == SRODOWISKO_PRODUKCYJNE, SRODOWISKO_PRODUKCYJNE, SRODOWISKO_TESTOWE );

   oSrod.okno = DialogCreate( win, "Środowisko KSeF", 420, 160 );
   ramka      = GroupBoxCreate( oSrod.okno, 10, 10, 400, 90, biuro->tytul );
   GroupBoxSetLineColor( ramka, 0xDCDCDC );
   oSrod.TESTOWE     = RadioButtonCreate( oSrod.okno, 30, 35, 360, 20, KsefSrodowisko( SRODOWISKO_TESTOWE, T ) );
   oSrod.PRODUKCYJNE = RadioButtonCreate( oSrod.okno, 30, 65, 360, 20, KsefSrodowisko( SRODOWISKO_PRODUKCYJNE, T ) );
   CheckBoxSetChecked( IIF( nJest == SRODOWISKO_PRODUKCYJNE, oSrod.PRODUKCYJNE, oSrod.TESTOWE ), T );

   oOk     = ButtonCreate( oSrod.okno, 220, 118, 90, 28, "OK" );
   oAnuluj = ButtonCreate( oSrod.okno, 320, 118, 90, 28, "Anuluj" );
   ButtonSetDefault( oOk, T );
   ControlSetAction( oOk, Ok, &oSrod );
   ButtonSetCancel( oAnuluj, T );
   ControlSetAction( oAnuluj, Anuluj, &oSrod );
   WinCenter( oSrod.okno );
   DialogShowModal( oSrod.okno );

   DbSelectArea( "FIRMA_KSEF" );
   if( oSrod.nSrodowisko != 0 && oSrod.nSrodowisko != nJest && NetRLock( NET_WAIT ) )
   {
      FieldPutN( FieldPos( "SRODOWISKO_KSEF" ), oSrod.nSrodowisko );
      DbCommit();
      DbUnlock();
   }
   DbCloseArea();
}

static void Ok( Control *button, void *data )
{
   OknoSrodowiska *oSrod = ( OknoSrodowiska * )data;

   UNUSED( button );

   oSrod->nSrodowisko = IIF( CheckBoxChecked( oSrod->PRODUKCYJNE ), SRODOWISKO_PRODUKCYJNE, SRODOWISKO_TESTOWE );
   DialogEnd( oSrod->okno, 1 );
}

static void Anuluj( Control *button, void *data )
{
   UNUSED( button );

   DialogEnd( ( ( OknoSrodowiska * )data )->okno, 0 );
}
