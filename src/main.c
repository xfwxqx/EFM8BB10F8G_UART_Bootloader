/******************************************************************************
 * Copyright (c) 2015 by Silicon Laboratories Inc. All rights reserved.
 *
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *****************************************************************************/

#include "efm8_device.h"
#include "boot.h"
#include "flash.h"
// Converts command byte into zero based opcode (makes code smaller)
#define OPCODE(cmd) ((cmd) - BOOT_CMD_IDENT)
#define AppEnterAddr 0x0400

// Holds the current command opcode
static uint8_t opcode;

// Holds reply to the current command
static uint8_t reply;
// ----------------------------------------------------------------------------
// Perform the bootloader erase or write commands.
// ----------------------------------------------------------------------------
void doEraseWriteCmd(void)
{
  // Get the starting address from the boot record
  uint16_t address = boot_getWord();

  // Check if bootloader is allowed to modify this address range
  if (flash_isValidRange(address, boot_hasRemaining()))
  {
    // Erase the flash page first if this was the erase command
    if (opcode == OPCODE(BOOT_CMD_ERASE))
    {
      flash_erasePage(address);
    }
    // Write data from boot record to flash one byte at a time
    while (boot_hasRemaining())
    {
      flash_writeByte(address, boot_getByte());
      address++;
    }
  }
  else
  {
    // Return an error if the address range was restricted
    reply = BOOT_ERR_RANGE;
  }
}

// ----------------------------------------------------------------------------
// Perform the bootloader verify command.
// ----------------------------------------------------------------------------
void doVerifyCmd(void)
{
  // Get the starting and ending addresses from the boot record
  uint16_t address = boot_getWord();
  uint16_t limit = boot_getWord();

  // Compute an Xmodem CRC16 over the indicated flash range
  flash_initCRC();
  while (address <= limit)
  {
    flash_updateCRC(flash_readByte(address));
    address++;
  }
  // Compare with the expected result
  if (flash_readCRC() != boot_getWord())
  {
    // Return an error if the CRC did not match
    reply = BOOT_ERR_CRC;
  }
}

void WDT0_stop()
{
  bool ea = IE_EA;
  IE_EA = 0;
  WDTCN = 0xDE;
  WDTCN = 0xAD;
  IE_EA = ea;
}

void WDT0_feed()
{
  WDTCN = 0xA5;
}
//void IsEnterBootloader(void)
//{
//	}
// ----------------------------------------------------------------------------
// Bootloader Mainloop
// ----------------------------------------------------------------------------
void main(void)
{
	uint8_t i,j,k;
	bit StartBLFlag=0,FrameStart=0;
	uint8_t RecvTemp[6] = {0};
	uint8_t RecvTempLen = 0;
  // Initialize the communication channel and clear the flash keys
  boot_initDevice();
  flash_setKeys(0, 0);
  flash_setBank(0);

  // Loop until a run application command is received

/*  *((uint8_t SI_SEG_DATA *)0x00) = 0xA5;
  RSTSRC = RSTSRC_SWRSF__SET | RSTSRC_PORSF__SET;*/
  while(!StartBLFlag)
  	  {
  	 	  for(k=0;k<5;k++){
  	 		  for(i=0;i<200;i++){
  	 			  for(j=0;j<200;j++){
  	 				  if(SCON0_RI==1){
  	 					  SCON0_RI = 0;
  	 					  if(BOOT_FRAME_START == SBUF0){
  	 						 //SCON0_RI = 0;
  	 						 FrameStart=1;
  	 					  }
  	 					  if(FrameStart)
  	 					  {
  	 						 RecvTemp[RecvTempLen++] = SBUF0;
  	 					  }
  	 					  if(RecvTempLen == 6)
  	 					  {
  	 						 RecvTempLen=0;
  	 						 FrameStart=0;
  	 						 if((RecvTemp[3]==0xa5)&&(RecvTemp[4]==0xf1)){
  	 							StartBLFlag=1;
  	 							break;
  	 						}
  	 					  }
  	 					 //StartBLFlag=1;
  	 					//break;
  	 				  }
  	 				  else
  	 				  {
  	 					  WDT0_feed();
  	 				  }
  	 			  }
  	 			  if(StartBLFlag)
  	 				  break;
  	 		  }
  	 		  if(StartBLFlag)
  	 			  break;
  	 	  }

  	 	  if(!StartBLFlag){

  	 		  //WDT0_stop();
  	 		  //
  	 		  //超时时判断应用程序是否存在，不存在进入BL;若在，则关闭看门狗并进入应用程序
  	 		  #pragma ASM
  	 		  MOV     DPTR,#1FFDH
  	 		  CLR     A
  	 		  MOVC    A,@A+DPTR
  	 		  ;CPL     A
  	 		  ;JZ	LOOP
  	 		  MOV 	B,#055H
  	 		  CLR 	C
  	 		  SUBB 	A,B
  	 		  JNZ	LOOP
  	 		  MOV	97H,#0DEH
  	 		  MOV	97H,#0ADH
  	 		  LJMP AppEnterAddr
  	 		  LOOP:CLR     A
  	          #pragma ENDASM

  	 		StartBLFlag =1;

  	 	  }//;LJMP AppEnterAddr;JNZ	AppEnterAddr
  	}
  while (true)
  {

    // Wait for a valid boot record to arrive
	WDT0_feed();

    boot_nextRecord();
    // Receive the command byte and convert to opcode
    opcode = OPCODE(boot_getByte());
    
    // Assume success - handlers will modify if there is an error
    reply = BOOT_ACK_REPLY;

    // Interpret the command opcode
    switch (opcode)
    {
      case OPCODE(BOOT_CMD_IDENT):
        // Return an error if bootloader derivative ID does not match
        if (BL_DERIVATIVE_ID != boot_getWord())
        {
          reply = BOOT_ERR_BADID;
        }
        break;

      case OPCODE(BOOT_CMD_SETUP):
        // Save flash keys and select the requested flash bank
        flash_setKeys(boot_getByte(), boot_getByte());
        flash_setBank(boot_getByte());
        break;

      case OPCODE(BOOT_CMD_ERASE):
      case OPCODE(BOOT_CMD_WRITE):
        doEraseWriteCmd();
        break;

      case OPCODE(BOOT_CMD_VERIFY):
        doVerifyCmd();
        break;
      case OPCODE(BOOT_CMD_LOCK):
        // Write the boot signature and flash lock bytes
        flash_setBank(0);
        flash_writeByte((uint16_t)&boot_otp[0], boot_getByte());
        flash_writeByte((uint16_t)&boot_otp[1], boot_getByte());
        break;

      case OPCODE(BOOT_CMD_RUNAPP):
        // Acknowledge the command, then reset to start the user application
        //boot_sendReply(BOOT_ACK_REPLY);
        //boot_runApp();
		SendAck(reply);
      	RSTSRC = RSTSRC_SWRSF__SET | RSTSRC_PORSF__SET;
//        WDT0_stop();
//		#pragma ASM
//
//		  LJMP AppEnterAddr
//
//      	#pragma ENDASM
        break;
//      case OPCODE(BOOT_CMD_ERASEAPPFLAG):
//		flash_writeByte(boot_getWord(), boot_getByte());
//              break;
      default:
        // Return bootloader revision for any unrecognized command
        reply = BL_HARDWAREVERSION;
        break;
    }

    SendAck(reply);
    // Reply with the results of the command
    //boot_sendReply(reply);
  }
}
