//Nils Wei� 
//05.09.2011
//Compiler CC5x/
//HELLO GIT 
#pragma sharedAllocation

//Enumerationen definieren
#define TRUE  1
#define FALSE 0

#define STX 0xff
#define SET_COLOR 0xfd
#define SET_FADE 0xfc
#define SET_RUN 0xfb

#define FRAMELENGTH 16			// *** max length of one commandframe
#define CmdPointerAddr 0xff		// *** Address at EERPOM, where the Commandpointer is saved

//*********************** INCLUDEDATEIEN *********************************************
#pragma codepage 1
#include "inline.h"
#include "include_files\Ringbuf.h"
#include "include_files\usart.h"
#include "include_files\eeprom_nt.c"        // 2do* Check EEPROM routines for failure, I use new routines now
#include "include_files\crc.c"
#include "include_files\spi.h"
#include "include_files\ledstrip.h"

//*********************** GLOBAL VARIABLES *******************************************
struct CommandBuffer{
    char cmd_counter;
    char frame_counter;
    char cmd_buf[FRAMELENGTH];
    char crcH;
    char crcL;
};
static struct CommandBuffer gCmdBuf;	

//*********************** INTERRUPTSERVICEROUTINE ************************************
#pragma origin 4					//Adresse des Interrupts	
interrupt InterruptRoutine(void)
{
	if (RCIF)
	{
		if(!RingBufHasError) RingBufPut(RCREG);
		else 
		{
			//Register lesen um Schnittstellen Fehler zu vermeiden
			char temp = RCREG;
		}
	}
}

//*********************** FUNKTIONSPROTOTYPEN ****************************************
void init_all();
void throw_errors();
void read_commands();
void execute_commands();

//*********************** HAUPTPROGRAMM **********************************************
void main(void)
{
	init_all();
	
    while(1)
    {	
        throw_errors();
		read_commands();
    }
}
//*********************** UNTERPROGRAMME **********************************************

void init_all()
{
	//OSZILLATOR initialisieren: 4xPLL deactivated;INTOSC 16MHz
	OSCCON = 0b01111010;		
	RingBufInit();
	//initialise UART interface
	USARTinit();
	spi_init();
	ledstrip_init();
	
	//Ausgang f�r FET initalisieren
	TRISC.0 = 0;
	//Spannungsversorgung f�r LED's einschalten
	PORTC.0 = 0;

	//To Factory Restore WLAN Modul
	//TRISA.0=0;
	//PORTA.0 = 1;
    
    // *** load globals variables
    
    gCmdBuf.cmd_counter = 0;
    gCmdBuf.frame_counter = 0;
	
	char i;
	for(i=0;i<FRAMELENGTH;i++)
	{
        gCmdBuf.cmd_buf[i] = 0;
	}
    
	// *** allow interrupts
	RCIE=1;
	PEIE=1;
	GIE=1;
	
}

void throw_errors()
{
	if(RingBufHasError) 
	{
		USARTsend_str("ERROR:Receivebuffer full");
	}
}

/** This function reads one byte from the ringbuffer and check
*** for framestart, framelength, or databyte 
*** if a frame is complete, the function save the frame as a new
*** command in the internal EEPROM and calculate the Pointer for the next Command
**/
void read_commands()
{	
	if(RingBufIsNotEmpty)
	{
		// *** preload variables and 
		// *** get new_byte from ringbuffer
		char new_byte, temp, j;
		// *** get new byte
		new_byte = RingBufGet();	
		// *** do I wait for databytes?
		if(gCmdBuf.frame_counter == 0)
		{
			// *** I don't wait for databytes
			// *** Do I receive a Start_of_Text sign
			if(new_byte == STX)
			{
				// *** Do some cleaning
				gCmdBuf.cmd_counter = 1;
				// *** Write the startsign at the begin of the buffer
				gCmdBuf.cmd_buf[0] = new_byte;
                // *** Reset crc Variables
                newCRC(&gCmdBuf.crcH, &gCmdBuf.crcL);
                // *** add new_byte to crc checksum
                addCRC(new_byte, &gCmdBuf.crcH, &gCmdBuf.crcL);
			}
			else
			{	
				// *** to avoid arrayoverflow
				temp = FRAMELENGTH - 2;
				// *** check if I get the framelength byte
				if((new_byte < temp) && (gCmdBuf.cmd_counter == 1))
				{
					gCmdBuf.frame_counter = new_byte;
					gCmdBuf.cmd_buf[1] = new_byte;
					gCmdBuf.cmd_counter = 2;
                    // *** add new_byte to crc checksum
                    addCRC(new_byte, &gCmdBuf.crcH, &gCmdBuf.crcL);
				}
			}
		}
		else
		{
			// *** I wait for Databytes, so I save all bytes 
			// *** that I get until my framecounter is > 0
			gCmdBuf.cmd_buf[gCmdBuf.cmd_counter] = new_byte;
			gCmdBuf.cmd_counter++;
			gCmdBuf.frame_counter--;
            // *** add new_byte to crc checksum
            addCRC(new_byte, &gCmdBuf.crcH, &gCmdBuf.crcL);
			// *** now I have to check if my framecounter is null.
			// *** If it's null my string is complete 
			// *** and I can give the string to the crc check function.
			if(gCmdBuf.frame_counter == 0)
			{
                // *** verify crc checksum
                if( (gCmdBuf.crcL == gCmdBuf.cmd_buf[gCmdBuf.cmd_counter - 1]) &&
                    (gCmdBuf.crcH == gCmdBuf.cmd_buf[gCmdBuf.cmd_counter - 2]) )
                {
                    // *** copy new command             
                    // !!!*** ATTENTION check value of cmd_counter after if statement. 
                    // *** cmd_counter should point to crcL to copy only the command 
                    // *** whitout crc, STX and framelength
                    gCmdBuf.cmd_counter =- 2;
                    
                    char CmdPointer = EEPROM_RD(CmdPointerAddr);
                    if(CmdPointer < 241)
                    {
                        // *** calculate the next address for EEPROM write
                        EEPROM_WR(CmdPointerAddr,CmdPointer + 10);
                    }
                    else 
                    {
                        // *** EEPROM is full with commands
                        // *** Some errorhandling should be here
                        return;
                    }
                        
                    
                    for(j = 2;j < gCmdBuf.cmd_counter; j++)
                    {	
                        EEPROM_WR(CmdPointer, gCmdBuf.cmd_buf[j]);
                        CmdPointer ++;
                    }
                }
                else
                {
                    // *** Do some error handling in case of an CRC failure here
                    return;
                }
			}
		}
	}
}

/** This function reads the pointer for commands in the EEPROM from a defined address 
*** in the EEPROM. After this one by one command is executed by this function. 
**/ 
void execute_commands();
