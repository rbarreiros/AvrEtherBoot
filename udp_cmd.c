/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        09.01.2012
 Description:    empfangene UDP Daten auf Port 7600

 Dieses Programm ist freie Software. Sie können es unter den Bedingungen der 
 GNU General Public License, wie von der Free Software Foundation veröffentlicht, 
 weitergeben und/oder modifizieren, entweder gemäß Version 2 der Lizenz oder 
 (nach Ihrer Option) jeder späteren Version. 

 Die Veröffentlichung dieses Programms erfolgt in der Hoffnung, 
 daß es Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, 
 sogar ohne die implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT 
 FÜR EINEN BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License. 

 Sie sollten eine Kopie der GNU General Public License zusammen mit diesem 
 Programm erhalten haben. 
 Falls nicht, schreiben Sie an die Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA. 
------------------------------------------------------------------------------*/
#include "config.h"
#include "udp_cmd.h"
#include "stack.h"
	
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>

unsigned char bootloader_lock = 0;

//----------------------------------------------------------------------------
//Programm Page
void boot_program_page (uint32_t page, uint8_t *buf)
{
	uint16_t i;
	uint8_t sreg;

	// Disable interrupts.

	sreg = SREG;
	cli();
	
	eeprom_busy_wait ();

	boot_page_erase (page);
	boot_spm_busy_wait ();      // Wait until the memory is erased.

	for (i=0; i<SPM_PAGESIZE; i+=2)
	{
		// Set up little-endian word.

		uint16_t w = *buf++;
		w += (*buf++) << 8;
		
		boot_page_fill (page + i, w);
	}

	boot_page_write (page);     // Store buffer in flash page.
	boot_spm_busy_wait();       // Wait until the memory is written.

	// Reenable RWW-section again. We need this if we want to jump back
	// to the application after bootloading.

	boot_rww_enable ();

	// Re-enable interrupts (if they were ever enabled).

	SREG = sreg;
}

//----------------------------------------------------------------------------
//Empfang der IP
void udp_cmd_get (unsigned char index)
{
	unsigned long page = 0;
	struct IP_Header *ip;
	
	if ((eth_buffer[UDP_DATA_START] == myip[0] &&
	eth_buffer[UDP_DATA_START+1] == myip[1] &&
	eth_buffer[UDP_DATA_START+2] == myip[2] &&
	eth_buffer[UDP_DATA_START+3] == myip[3]))
	{
		switch (eth_buffer[UDP_DATA_START+4])
		{
			case 'P':
				//Page Adresse aus UDP Packet auslesen
				page = eth_buffer[UDP_DATA_START+8] << 8;
				page = page + eth_buffer[UDP_DATA_START+7] << 8;  
				page = page + eth_buffer[UDP_DATA_START+6] << 8;
				page = page + eth_buffer[UDP_DATA_START+5];
				
				//überschreiben des Bootloaders verhindern
				if(page < 0x7000)
				{
					boot_program_page (page,&eth_buffer[UDP_DATA_START+9]);
					//send data back
				}
				create_new_udp_packet(137,UDP_CMD_PORT_TX,UDP_CMD_PORT_TX,(unsigned long)0xffffffff);
				break;
				
			case 'H':
				//Verlassen des Bootloaders verhindern
				bootloader_lock = 1;
				//send OK
				eth_buffer[UDP_DATA_START] = 'O';
				eth_buffer[UDP_DATA_START+1] = 'K';
				create_new_udp_packet(2,UDP_CMD_PORT_TX,UDP_CMD_PORT_TX,(unsigned long)0xffffffff);
				break;
			
			case 'E':
				exit();
		}
	}
}

//----------------------------------------------------------------------------
//Empfang der IP
void exit (void)
{
	unsigned char temp;
	//Hauptprogramm Startadresse
	void (*start)( void ) = 0x0000;
	
	//Ethernet Interrupt deaktivieren
	ETH_INT_DISABLE;
	
	//Ethernet LEDs blinken aus
	enc28j60_led_blink (0);
	
    // Interrupt Vektoren zurück setzen
    temp = MCUCR;
    MCUCR = temp | (1<<IVCE);
    MCUCR = temp & ~(1<<IVSEL);

	
	//Start Adresse 0x0000
	start();
}



