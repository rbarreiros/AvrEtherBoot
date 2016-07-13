/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        24.10.2007
 Description:    Webserver uvm.

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
----------------------------------------------------------------------------*/

#include <avr/io.h>
#include "config.h"
#include "enc28j60.h"
#include "stack.h"
#include "udp_cmd.h"

//----------------------------------------------------------------------------
//Hier startet das Hauptprogramm
int main(void)
{  	
  unsigned long a;
	
  //Interrupt Vektoren verbiegen
  MCUCR = (1<<IVCE);
  MCUCR = (1<<IVSEL);
  
  /* Clear WDRF in MCUSR */
  MCUSR &= ~(1<<WDRF);
  /* Write logical one to WDCE and WDE */
  /* Keep old prescaler setting to prevent unintentional time-out */
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  /* Turn off WDT */
  WDTCSR = 0x00;
  
  //Applikationen starten
  stack_init();
  enc28j60_led_blink (1);
  
  //Ethernetcard Interrupt enable
  ETH_INT_ENABLE;
  
  //Globale Interrupts einschalten
  sei(); 
  
  a = 5000000;
  while(a--)
  {
    if(bootloader_lock) a = 5000000;
    eth_get_data();
  }

  leave();
  return(0);
}

