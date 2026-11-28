/*
 * Program...: xBiuro
 * Plik......: src/faktura/faktura.h
 *
 */

#ifndef FAKTURA_H
#define FAKTURA_H

#include "biuro.h"
#include "xsiec.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define KSEF_NIEWYSLANA 0
#define KSEF_W_TOKU     1
#define KSEF_PRZYJETA   2
#define KSEF_ODRZUCONA  3

#define DNI_OSTRZEZENIA 30

#define PLATNOSC_GOTOWKA 1
#define PLATNOSC_KARTA   2
#define PLATNOSC_PRZELEW 6

typedef void ( *PozycjeOdswiez )( void *data );

typedef struct
{
   Win           *okno;
   Toolbar       *pasek;
   int            aPrzyciski[ 16 ];
   int            nPrzyciskow;
   StatusBar     *pasekStanu;
   Control       *BRW;
   PozycjeOdswiez odswiez;
   void          *data;
} OknoPozycji;

typedef struct
{
   char NUMER_FAKTURY[ 255 ];
   int  DATA_WYSTAWIENIA;
   int  DATA_DOSTAWY;
   int  FORMA_PLATNOSCI;
   int  TERMIN_PLATNOSCI;
   char KOD_KRAJU_NABYWCY[ 3 ];
   char NIP_NABYWCY[ 51 ];
   char NAZWA_NABYWCY[ 2049 ];
   char ULICA_NABYWCY[ 255 ];
   char KOD_POCZTOWY_NABYWCY[ 9 ];
   char MIEJSCOWOSC_NABYWCY[ 225 ];
   char NAZWA_BANKU[ 255 ];
   char NUMER_RACHUNKU[ 35 ];
   char PODSTAWA_ZWOLNIENIA[ 2049 ];
} Faktura;

void ModulOtworz( Biuro *biuro );
void ModulZamknij( Biuro *biuro );
void ModulNtx( Biuro *biuro );
int  RokLista( Biuro *biuro, int *aLata, int nMaks );
int  RokWybierz( Win *win, Biuro *biuro, const char *cTytul );

void           Faktury( Win *win, void *data );
xBool          FakturyOtworz( Biuro *biuro );
void           FakturyZamknij( void );
void           FakturyZakres( const char *cRodzaj );
int            FakturyNastepnyNumer( const char *cRodzaj );
void           FakturyNtx( Biuro *biuro );
const char    *FakturyWKsef( void );
xBool          FakturyJest( void );
void           FakturyPodglad( Win *win );
void           FakturyDrukuj( Biuro *biuro );
const DbField *PozycjeStruct( int *nPol );
char          *FakturyKatalog( Biuro *biuro, int nRok, char *cKatalog, int nRozmiar );
char          *FakturyKlucz( char *cKlucz, int nRozmiar );
char          *FakturyPlik( Biuro *biuro, const char *cKatalog, const char *cRozszerzenie, char *cPlik, int nRozmiar );
char          *FakturyTymczasowa( const char *cNazwa, char *cBaza, int nRozmiar );
int            FakturySrodowiskoKsef( int nSrodowiskoFirmy );
xBool          FakturySrodowisko( int nSrodowiskoFirmy, const char *cTytul );
char          *FakturyBase64Url( const void *data, size_t nRozmiarDanych, char *cWynik, int nRozmiar );

void FakturaVat( Win *win, Biuro *biuro );

void FakturyKor( Win *win, void *data );

char *Fa3GenerujXml( const char *cSprzedawca );

typedef struct
{
   Json  *json;
   char   cSesja[ 80 ];
   char   cFaktura[ 80 ];
   char  *cUpo;
   size_t nUpo;
} WynikKsef;

typedef void ( *KsefPrzyjeta )( const char *cSesja, const char *cFaktura, void *data );

Json       *KsefZaloguj( const char *cNip, const char *cToken, int nSrodowisko, char *cBlad, int nRozmiar );
void        KsefWyloguj( int nSrodowisko, const Json *bilety );
xBool       KsefWyslij( const char *cNip, const char *cToken, int nSrodowisko, const char *cXml, size_t nXml, xBool lOffline, KsefPrzyjeta przyjeta, void *data, WynikKsef *wynik, char *cBlad, int nRozmiar );
xBool       KsefSprawdz( const char *cNip, const char *cToken, int nSrodowisko, const char *cSesja, const char *cFaktura, WynikKsef *wynik, char *cBlad, int nRozmiar );
void        KsefWynikFree( WynikKsef *wynik );
int         KsefKod( const Json *odp );
char       *KsefOpisStatusu( const Json *odp, char *cOpis, int nRozmiar );
const char *KsefSrodowisko( int nSrodowisko, xBool lPelna );
char       *KsefWaznosc( int dDo, char *cWynik, int nRozmiar );
void        KsefAplikacja( int nSrodowisko, const char *cTytul );
char       *KsefZaszyfruj( const char *cDane );
char       *KsefOdszyfruj( const char *cZapis );

typedef struct
{
   char  cNazwa[ 241 ];
   char  cNumer[ 64 ];
   char *cKlucz;
   char *cHaslo;
   Bytes der;
   int   dWaznyDo;
   xBool lRsa;
} Certyfikat;

typedef struct
{
   char *cToken;
   char  cNumer[ 80 ];
   char  cStatus[ 400 ];
} TokenKsef;

Json *KsefZalogujXades( const char *cNip, Bytes klucz, Bytes cert, const char *cHaslo, int nSrodowisko, char *cBlad, int nRozmiar );
xBool KsefGenerujToken( const char *cNip, const Certyfikat *cert, int nSrodowisko, const char *cOpis, const char *cUprawnienia, int dWaznyDo, TokenKsef *token, char *cBlad, int nRozmiar );

void  Certyfikaty( Win *win, void *data );
xBool CertyfikatyPobierz( const char *cNipFirmy, int nSrodowiskoKsef, const char *cTyp, Certyfikat *cert );
void  CertyfikatFree( Certyfikat *cert );
void  CertyfikatyNtx( void );

void  Tokeny( Win *win, void *data );
char *TokenyPobierz( const char *cNipFirmy, int nSrodowiskoKsef );
xBool TokenyZapisz( const char *cNazwa, const char *cNip, int nSrodowisko, const char *cToken, int dWaznyDo );
void  TokenyNtx( void );
xBool FirmaPobierzKsef( Biuro *biuro, char *cNip, int nRozmiar, int *nSrodowisko );

void GenerujToken( Win *win, void *data );

void SrodowiskoKsef( Win *win, void *data );

void FakturyWyslij( Biuro *biuro, StatusBar *pasekStanu, Control *browse );
void FakturySprawdz( Biuro *biuro, StatusBar *pasekStanu, Control *browse );

void        Offline( Win *okno, Biuro *biuro, Control *browse );
const char *OfflineTryb( int nTryb );
void        OfflinePrzypomnij( void );
char       *OfflineKodII( Biuro *biuro, char *cAdres, int nRozmiar );

void        PozycjeEdytuj( OknoPozycji *oDialog, const char *cAlias, xBool lNowa, const char *cTytul, const char *cKomunikat, xBool lZero );
void        PozycjeUsun( OknoPozycji *oDialog, const char *cAlias, const char *cKomunikat );
void        PozycjeTrybEdycji( StatusBar *pasekStanu, xBool lWlacz );
const char *PozycjeStawka( const char *cStawka );
xBool       PozycjeSuma( const char *cAlias, double *nNetto, double *nVat, xBool *lZw );
xBool       PozycjeZapisz( const char *cAlias, int nNr, const char *cRodzaj, int nLpOd, xBool lPrzed, long *aDodane, int *nDodane );
void        PozycjeWycofaj( long *aDodane, int nDodane );

#ifdef __cplusplus
}
#endif

#endif
