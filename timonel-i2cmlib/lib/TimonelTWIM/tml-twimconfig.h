/*
  tml-twimconfig.h
  ================
  Timonel I2C Master Configuration
  ---------------------------
  2019-02-07 Gustavo Casanova
  ---------------------------
*/

#ifndef _TML_TWIMCONFIG_H_
#define _TML_TWIMCONFIG_H_

#define USE_SERIAL Serial
#define V_CMD_LENGTH 9    /* Timonel version reply lenght */
#define T_SIGNATURE 84    /* Timonel signature (expected "T") */
#define V_SIGNATURE 0     /* Ver cmd reply: signature position */
#define V_MAJOR 1         /* Ver cmd reply: major number position */
#define V_MINOR 2         /* Ver cmd reply: minor number position */
#define V_FEATURES 3      /* Ver cmd reply: available features code position */
#define V_BOOT_ADDR_MSB 4 /* Ver cmd reply: Timonel start address MSB position */
#define V_BOOT_ADDR_LSB 5 /* Ver cmd reply: Timonel start address LSB position */
#define V_TMPL_ADDR_MSB 6 /* Ver cmd reply: Trampoline address MSB position */
#define V_TMPL_ADDR_LSB 7 /* Ver cmd reply: Trampoline address LSB position */

// Error GetTmlID (0) 1: No conn
#define ERR_01 0x0        /* Error GetTmlID (0) 1:   */

#endif /* _TML_TWIMCONFIG_H_ */