/*
 * Program...: xBiuro
 * Plik......: prog-zaso/wekt/logo-xbiuro.c
 *
 */

#include "xwgui.h"

int main( void )
{
   Svg *svg = SvgInit( "logo-xbiuro.svg", 512, 512, 24 );
   double romb[][ 2 ] = { { 18.513, 0.262 }, { 23.738, 5.487 }, { 18.513, 10.711 }, { 13.289, 5.487 } };

   if( !svg )
      return 1;

   SvgPen( svg, 2, "currentColor", T );

   SvgRect( svg, 1, 1.628, 10.686, 10.686, 1.781, "#E9EDF1" );
   SvgRect( svg, 1, 12.314, 10.686, 10.686, 1.781, "#E9EDF1" );
   SvgRect( svg, 11.686, 12.314, 10.686, 10.686, 1.781, "#E9EDF1" );

   SvgPolygon( svg, romb, 4, 1.781, "#E9EDF1" );

   SvgClose( svg );

   return 0;
}
