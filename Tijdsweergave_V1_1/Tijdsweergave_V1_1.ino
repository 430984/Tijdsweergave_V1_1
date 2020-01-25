#include "src/MP3.h"

// StartKnop
#define KNOP_DEBOUNCE 50
#define KNOP_LANGDRUKKEN 2000

#define TIJD_MS_SCHERMUIT 30000

#define TIJD_MINUTEN 45
#define TIJD_SECONDEN 0

// Let op: Alles wat in een ISR gebruikt wordt, moet volatile zijn
// Dubbele punt om uren en minuten te scheiden op het scherm
volatile bool xDp = 1, xIndicatorStartKnop = 0, xReset = 0;
// Huidige waarde voor timer gestart
volatile bool xTimerGestart = 0;
// Huidige waarde op het display (0000..9999)
volatile int iDisplayWaarde;

volatile static unsigned long ulVorigeMillisDisplayUit = 0;

int iTijdInSeconden = 0;

// Nummer definities op het scherm
const unsigned char ucNummers[12] = 
{
  // PB3, PB2, PB1, PB0, PD7, PD6, PD5, PD4
  //  E    F    G    A    B    C    D    dP
  // 
  0xDE,
  0xC,
  0xBA,
  0x3E,
  0x6C,
  0x76,
  0xF6,
  0x1C,
  0xFE,
  0x7E,
  0x20,
  0xFF  // 8. <- Alleen test! 
};

//Instantie voor de MP3 speler
MP3 mp3;

// Inputs en outputs instellen
void setupIO()
{
  
  // Knoppen
  // Ingangen
  DDRD &= ~((1 << PD2) | (1 << PD3));
  // Pullup-weerstanden aanzetten
  PORTD |= (1 << PD2) | (1 << PD3);

  // Stopknop interrupt (PD3 = INT1)
  // Dalende flank op INT1 genereert een interrupt
  EICRA = (1 << ISC11);
  // Interrupt inschakelen
  EIMSK = (1 << INT1);

  // Lamp onder startknop uit
  PORTB &= ~(1 << 4);
}

// Instellen van de IO voor het display
void setupDisplay()
{
  // Uitgangen
  // Digits 
  DDRB |= 0x1F;
  DDRD |= 0xF0;
  
  // Segmenten
  DDRC |= 0x0F; // PC 3..0 -> D, E, DP, B


  // Timed interrupt voor multiplexen van display
  // Timer resetten
  TCCR2B = TCCR2A = TIMSK2 = TCNT2 = 0;
  // Alle segmenten en digits uitzetten
  displayWissen();
  // De tijd resetten naar de vaste waarde
  resetTijd();
  // Display multiplexing inschakelen
  //displayAan();
}

// Instellen en starten van de MP3 speler
void setupMP3()
{
  mp3.begin();
  delay(3000);
}

// Startknop
void startKnop(bool xIngang, bool &xKortGedrukt, bool &xLangGedrukt)
{
  static unsigned long ulStartMillis = 0;
  static bool xOudKnopStatus = 0;
  static bool xEersteKeerFlag = 0;
  
  unsigned long ulMillis = millis();
  bool xKnopStatus = ~(PIND >> PD2) & 0x01;

  xKortGedrukt = 0;
  xLangGedrukt = 0;
  
  if(xKnopStatus != xOudKnopStatus)
  {
    unsigned long ulDelta = ulMillis - ulStartMillis;
    
    xOudKnopStatus = xKnopStatus;
    if(xKnopStatus)
      ulStartMillis = ulMillis;
    else{
      if(ulDelta >= KNOP_DEBOUNCE && ulDelta < KNOP_LANGDRUKKEN)
      {
        xKortGedrukt = 1;          
      }
      xEersteKeerFlag = 0;
    }
  }

  if(xKnopStatus && ulMillis - ulStartMillis >= KNOP_LANGDRUKKEN && xEersteKeerFlag == 0)
  {
    xEersteKeerFlag = 1;
    xLangGedrukt = 1;
  }
}

// Tijd weer terugzetten op standaardinstellingen
void resetTijd()
{
  xTimerGestart = 0;
  iTijdInSeconden = TIJD_SECONDEN + TIJD_MINUTEN * 60;
  ulVorigeMillisDisplayUit = millis();
}

// Alles resetten
void resetAlles()
{
  displayAan();
  resetTijd();
  if(mp3.getPlayState() != 0) mp3.stop();
  xReset = 1;
}

// Indicator startknop
void indicatorStart()
{
  if(xIndicatorStartKnop)
    PORTB |= (1 << 4);
  else
    PORTB &= ~(1 << 4);
}

// Wissen van het display
void displayWissen()
{
  // Digits  
  PORTC &= ~0x0F;
  // Segmenten
  PORTB &= ~0x0F;
  PORTD &= ~0xF0;
  // Indicator onder startknop uit
  PORTB &= ~(1 << 4);
}

// Aanzetten van het display
void displayAan()
{
  //xDisplayIsAan = true;
  // Prescaler instellen op clkio/128 (mogelijk: uit, 1, 8, 32, 64, 128, 256, 1024)
  TCCR2B |= 0x5;
  // Timer overflow interrupt inschakelen
  // Dus multiplexen starten
  TIMSK2 |= (1 << TOIE2);
}

// Uitzetten van het display
void displayUit()
{
  //xDisplayIsAan = false;
  // Prescaler instellen op 0 (no clock source)
  TCCR2B &= ~0x7;
  // Timer overflow interrupt uitschakelen
  // Dus multiplexen stoppen
  TIMSK2 &= ~(1 << TOIE2);
  // Timer wissen
  TCNT2 = 0;
  // display uitzetten
  displayWissen();  
}

// MP3 speler
void handleMP3()
{
  mp3.handle();
}

void setup()
{ 
  setupIO();
  setupDisplay();
  setupMP3();
    
  // Alle (ingestelde) interrupts inschakelen
  sei();
}

void loop()
{ 
  // Lokale variabelen declareren
  // Statische variabelen (blijven behouden)
  static unsigned long ulVorigeMillis = 0;

  // Tijdelijke variabelen (worden steeds opnieuw geinitialiseerd)
  unsigned long ulNuMillis = millis();
  bool xStartKnopKort, xStartKnopLang, xTijdIsNieuw, xDisplayIsAan;

  xTijdIsNieuw = iTijdInSeconden == (TIJD_SECONDEN + TIJD_MINUTEN * 60);
  xDisplayIsAan = (TIMSK2 & (1 << TOIE2)) != 0;

  // MP3 module afhandelen.
  handleMP3();
  
  // Functie van de startknop
  startKnop(~(PIND >> PD2) & 0x01, xStartKnopKort, xStartKnopLang);

  if(xStartKnopKort)
  {
    if(xDisplayIsAan == 0)
      displayAan();
    else if(xTijdIsNieuw)
      // Inverteer startsignaal
      // xTimerGestart ^= 1;
      xTimerGestart = 1;
  }

  if(xStartKnopLang)
    resetAlles();
    
  // Indien de timer gestart is en het verschil tussen de actuele tijd en de vorige tijd
  // is hoger dan of gelijk aan 1000 (één seconde)
  if(xTimerGestart && ulNuMillis - ulVorigeMillis >= 500)
  {
    if(xDp == 1)
      xDp = 0;
      
    else
    {
      xDp = 1;
      
      if(iTijdInSeconden == 0)
        xTimerGestart = 0;
      else
        // Dan een seconde aftellen
        iTijdInSeconden--;
    }

    // Dit tijdstip onthouden voor de volgende keer
    ulVorigeMillis = ulNuMillis;
  }
  else if(xTimerGestart == 0)
    xDp = 1;

  // Bepalen wanneer scherm uit moet (energiebesparing)
  if((xTimerGestart == 0) && (xDisplayIsAan == 1) && xTijdIsNieuw)
  {
    if (ulNuMillis - ulVorigeMillisDisplayUit >= TIJD_MS_SCHERMUIT)
      displayUit();
  }
  else
    ulVorigeMillisDisplayUit = ulNuMillis;
    
//---------------------------------------------------------------------
// Indicator startknop
  // Als het display aan is, dan knippert de indicator onder startknop één keer in de 5 seconden (Standby)
  if(xDisplayIsAan == 0)
  {
    if(ulNuMillis % 5000 <= 500)
      xIndicatorStartKnop = 1;
    else
      xIndicatorStartKnop = 0;
  }
  // Anders, als het display aan is
  else
  {
    // Als de tijd loopt dan knippert de indicator hetzelfde als de dubbele punten op het display (tijd loopt)
    if(xTimerGestart)
      xIndicatorStartKnop = xDp;
      
    // Anders, als de tijd niet loopt
    else
    {
      // Als de tijd niet gelijk is aan de beginwaarde (30:00, of 60:00, net wat ingesteld is)
      if(xTijdIsNieuw == 0)
      {
        // dan knippert de indicator onder startknop twee keer in 5 seconden (moet gereset worden)
        if((ulNuMillis % 5000 <= 500) || ((ulNuMillis % 5000 >= 1000) && (ulNuMillis % 5000 <= 1500)))
          xIndicatorStartKnop = 1;
        else
          xIndicatorStartKnop = 0;
      }
      // Anders brandt deze continu (kan gestart worden)
      else
        xIndicatorStartKnop = 1;
    }
  }

  // Als het 
  if(xDisplayIsAan == 0)
    indicatorStart();
  
//---------------------------------------------------------------------
// MP3 speler
  static int iMP3Stap = 0;

  if(xReset)
    iMP3Stap = 0;

  switch(iMP3Stap)
  {
//---------------------------------------------------------------------
// Wachten tot iets gebeurt
    case 0:
      // Als tijd start en de speler is niet aan het spelen
      if(mp3.getPlayState() == 0 && xTimerGestart)
        iMP3Stap = 1;

      if(mp3.getPlayState() != 0 && xTimerGestart == 0)
        iMP3Stap = 5;
      break;
//---------------------------------------------------------------------
// Afspelen van muziek starten
    // Commando afspelen geven
    case 1:
      if(mp3.playMP3(1) != -1)
        iMP3Stap = 2;
      break;
      
    // Wachten tot MP3 speler gaat spelen
    case 2:
      if(mp3.getPlayState() != 0)
        iMP3Stap = 10;
      break;

//---------------------------------------------------------------------
// Afspelen stoppen
    case 5:
      if(mp3.stop() != -1)
        iMP3Stap = 0;
      break;

//---------------------------------------------------------------------
// Muziek speelt
      
    // MP3 speler speelt. 
    // Meerdere dingen in de gaten houden.
    case 10:
      // Als de muziek gestopt is
      if(mp3.getPlayState() == 0)
        iMP3Stap = 0;
        
      // Tijd is gestopt -> Muziek stoppen en geluid afspelen
      else if(xTimerGestart == 0)
        iMP3Stap = 20;
      break;
//---------------------------------------------------------------------
// Muziek stoppen en geluid afspelen (adhv gewonnen of verloren)

    case 20:
      if(mp3.getPlayState() != 0)
        iMP3Stap = 21;
      else
        iMP3Stap = 22;
      break;
      
    case 21:
      if(mp3.stop() != -1)
        iMP3Stap = 22;
      break;

    case 22:
      if(mp3.playSound(1, (iTijdInSeconden > 0 ? 1 : 2)) != -1)
        iMP3Stap = 23;
      break;

    case 23:
      if(mp3.getPlayState() != 0)
        iMP3Stap = 24;
      break;

    case 24:
      if(mp3.getPlayState() == 0)
        iMP3Stap = 0;
      break;

    default:
      iMP3Stap = 0;
      break;
  }
  
  iDisplayWaarde = (iTijdInSeconden / 60) * 100 + iTijdInSeconden % 60;
  xReset = 0;
}

// Multiplex ISR
ISR(TIMER2_OVF_vect)
{
  static volatile int iDigitTeller = 0;
  unsigned char ucHuidigeNummerData;
  ucHuidigeNummerData = 0;
  iDigitTeller++;
  if(iDigitTeller >= 4) iDigitTeller = 0;

  // Digits eerst uitzetten (om ghosting te voorkomen)
  displayWissen();

  switch(iDigitTeller)
  {
    case 0:
      if(iDisplayWaarde >= 1000)
        ucHuidigeNummerData = ucNummers[(iDisplayWaarde / 1000) % 10];
      else if(iDisplayWaarde < 0)
        ucHuidigeNummerData = ucNummers[10];
      else
        ucHuidigeNummerData = 0;
      break;
    case 1:
      ucHuidigeNummerData = ucNummers[(iDisplayWaarde / 100) % 10];
      break;
    case 2:
      ucHuidigeNummerData = ucNummers[(iDisplayWaarde / 10) % 10];
      break;
    case 3:
      ucHuidigeNummerData = ucNummers[(iDisplayWaarde / 1) % 10];
      break;
  }

  if(iDigitTeller >= 1 && iDigitTeller <= 2)
    ucHuidigeNummerData |= (xDp ? 0x01 : 0x00);
    
  // Segmenten klaarzetten voor het volgende digit
  PORTB |= (ucHuidigeNummerData >> 4);
  PORTD |= (ucHuidigeNummerData << 4);

  PORTC |= (1 << (3-iDigitTeller));

  // Indicator onder startknop
  if(xIndicatorStartKnop)
  //if(xDp)
    PORTB |= (1 << 4);
  else
    PORTB &= ~(1 << 4);
}

ISR(INT1_vect)
{
  if(xTimerGestart){
    xTimerGestart = 0;
  }
}
