; Timonel startup
; file: crt1.S
; ................................
; 2020-06-06

#if (__GNUC__ < 3) || (__GNUC__ == 3 && __GNUC_MINOR__ < 3)
    #error "GCC version >= 3.3 required"
#endif

#include <avr/io.h>

#ifdef __AVR_HAVE_JMP_CALL__
    #define XJMP jmp
#else
    #define XJMP rjmp
#endif

.section .init9, "ax", @progbits
.org 0x0000
__vectors:
XJMP main
