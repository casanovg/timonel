/*
  timonel-twim-ms.h
  ==================
  Timonel library test header (Multi Slave) v1.5
  ----------------------------------------------------------------------------
  2020-04-29 Gustavo Casanova
  ----------------------------------------------------------------------------
*/

#ifndef _TIMONEL_TWIM_MS_H_
#define _TIMONEL_TWIM_MS_H_

#include <NbMicro.h>
#include <TimonelTwiM.h>
#include <TwiBus.h>

// Prototypes
void setup(void);
void loop(void);
bool CheckApplUpdate(void);
void PrintStatus(Timonel timonel);
void ThreeStarDelay(void);
void ShowHeader(void);
void PrintLogo(void);
void ClrScr(void);

#endif  // _TIMONEL_TWIM_MS_H_