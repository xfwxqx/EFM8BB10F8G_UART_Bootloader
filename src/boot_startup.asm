$NOMOD51
;
; Copyright (c) 2015 by Silicon Laboratories Inc. All rights reserved.
;
; http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
;

#include "efm8_device.h"

#define AppStartAddress 0x400
#define BL_SIGNATURE 0xA5

    NAME    BOOT_STARTUP

    PUBLIC  boot_otp
    PUBLIC  ?C_STARTUP
    EXTRN   CODE (?C_START)

; Declare and locate all memory segments used by the bootloader

; ?BL_EXTRA   SEGMENT CODE AT BL_LIMIT_ADDRESS

?Reset_Vector   				SEGMENT CODE AT 0x0
?ExternalInterrupt0_Vector		SEGMENT CODE AT 0x03
?Timer0Overflow_Vector			SEGMENT CODE AT 0x0B
?ExternalInterrupt1_Vector		SEGMENT CODE AT 0x13
?Timer1Overflow_Vector			SEGMENT CODE AT 0x1B
?UART0_Vector					SEGMENT CODE AT 0x23
?Timer2OverflowOrCapture_Vector	SEGMENT CODE AT 0x2B
?SPI0_Vector					SEGMENT CODE AT 0x33
?SMBus0_Vector					SEGMENT CODE AT 0x3B
?PortMatch_Vector				SEGMENT CODE AT 0x43
?ADC0WindowCompare_Vector		SEGMENT CODE AT 0x4B
?ADC0EndofConversion_Vector		SEGMENT CODE AT 0x53
?PCA0_Vector					SEGMENT CODE AT 0x5B
?Comparator0_Vector				SEGMENT CODE AT 0x63
?Comparator1_Vector				SEGMENT CODE AT 0x6B
?Timer3OverflowOrCapture_Vector	SEGMENT CODE AT 0x73

?BL_START   SEGMENT CODE AT BL_START_ADDRESS
?BL_RSVD    SEGMENT CODE AT BL_STOP_ADDRESS-2
?BL_STACK   SEGMENT IDATA

; Create idata segment for stack
    RSEG    ?BL_STACK
    DS      16

;#if (BL_LIMIT_ADDRESS != BL_START_ADDRESS)
; Create code segment for firmware that doesn't fit in security page
;    RSEG    ?BL_EXTRA
;boot_extra:
;    LJMP    ?C_STARTUP
;#endif
;应用程序中断向量表软链接，应用程序中触发中断调到相应中断向量表中，再从该表跳到应用程序中断服务函数
	RSEG    ?Reset_Vector
	LJMP	?C_STARTUP

	RSEG    ?ExternalInterrupt0_Vector
	LJMP	AppStartAddress+0x03

	RSEG    ?Timer0Overflow_Vector
	LJMP	AppStartAddress+0x0B

	RSEG    ?ExternalInterrupt1_Vector
	LJMP	AppStartAddress+0x13

	RSEG    ?Timer1Overflow_Vector
	LJMP	AppStartAddress+0x1B

	RSEG    ?UART0_Vector
	LJMP	AppStartAddress+0x23

	RSEG    ?Timer2OverflowOrCapture_Vector
	LJMP	AppStartAddress+0x2B

	RSEG    ?SPI0_Vector
	LJMP	AppStartAddress+0x33

	RSEG    ?SMBus0_Vector
	LJMP	AppStartAddress+0x3B

	RSEG    ?PortMatch_Vector
	LJMP	AppStartAddress+0x43

	RSEG    ?ADC0WindowCompare_Vector
	LJMP	AppStartAddress+0x4B

	RSEG    ?ADC0EndofConversion_Vector
	LJMP	AppStartAddress+0x53

	RSEG    ?PCA0_Vector
	LJMP	AppStartAddress+0x5B

	RSEG    ?Comparator0_Vector
	LJMP	AppStartAddress+0x63

	RSEG    ?Comparator1_Vector
	LJMP	AppStartAddress+0x6B

	RSEG    ?Timer3OverflowOrCapture_Vector
	LJMP	AppStartAddress+0x73

; Bootloader entry point (boot_vector)
    RSEG    ?BL_START
?C_STARTUP:
    USING   0

; Start bootloader if reset vector is not programmed
;    MOV     DPTR,#00H
;    CLR     A
;    MOVC    A,@A+DPTR
;    CPL     A
;    JZ      boot_start

; Start bootloader if software reset and R0 == signature
    ;MOV     A,RSTSRC
    ;CJNE    A,#010H,pin_test
;    MOV     A,R0
;    XRL     A,#BL_SIGNATURE
;    JZ      boot_start

; Start the application by jumping to the reset vector
;app_start:
;    LJMP    00H

; Start bootloader if POR|Pin reset and boot pin held low
;pin_test:
;    ANL     A,#03H                  ; A = RSTSRC
;    JZ      app_start               ; POR or PINR only
;    MOV     R0,#(BL_PIN_LOW_CYCLES / 7)
;?C0001:                             ; deglitch loop
;    JB      BL_START_PIN,app_start  ; +3
;    DJNZ    R0,?C0001               ; +4 = 7 cycles per loop

; Setup the stack and jump to the bootloader
boot_start:
    MOV     SP, #?BL_STACK-1
    LJMP    ?C_START

; Reserved Bytes (bl_revision, bl_signature, lock_byte)
    RSEG    ?BL_RSVD
boot_rev:
    DB      BL_HARDWAREVERSION
boot_otp:
    DB      BL_SIGNATURE
lock_byte:
    DB      0xFF

    END
