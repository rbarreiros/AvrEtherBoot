/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        03.11.2007
 Description:    Webserver Config-File

 Dieses Programm ist freie Software. Sie k�nnen es unter den Bedingungen der 
 GNU General Public License, wie von der Free Software Foundation ver�ffentlicht, 
 weitergeben und/oder modifizieren, entweder gem�� Version 2 der Lizenz oder 
 (nach Ihrer Option) jeder sp�teren Version. 

 Die Ver�ffentlichung dieses Programms erfolgt in der Hoffnung, 
 da� es Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, 
 sogar ohne die implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT 
 F�R EINEN BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License. 

 Sie sollten eine Kopie der GNU General Public License zusammen mit diesem 
 Programm erhalten haben. 
 Falls nicht, schreiben Sie an die Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA. 
------------------------------------------------------------------------------*/

#ifndef _CONFIG_H_
#define _CONFIG_H_	

//Umrechnung von IP zu unsigned long
#define IP(a,b,c,d) ((unsigned long)(d)<<24)+((unsigned long)(c)<<16)+((unsigned long)(b)<<8)+a

//IP des Webservers
#define MYIP		IP(192,168,1,99)

//Netzwerkmaske
#define NETMASK		IP(255,255,255,0)

//MAC Adresse des Webservers	
#define MYMAC1	0x00
#define MYMAC2	0x22
#define MYMAC3	0xF9
#define MYMAC4	0x01	
#define MYMAC5	0x1D
#define MYMAC6	0xA6

#endif //_CONFIG_H


