/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        24.10.2007
 Description:    Ethernet Stack

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
#include "stack.h"

const UDP_PORT_ITEM UDP_PORT_TABLE[MAX_APP_ENTRY] = // Port-Tabelle
{
	{7602,(void(*)(unsigned char))udp_cmd_get},
};

unsigned char myip[4];
unsigned char netmask[4];
unsigned char broadcast_ip[4];
unsigned char eth_buffer[MTU_SIZE+1];

struct arp_table arp_entry[MAX_ARP_ENTRY];

//----------------------------------------------------------------------------
//Converts integer variables to network Byte order
unsigned int htons(unsigned int val)
{
  return HTONS(val);
}
//----------------------------------------------------------------------------
//Converts integer variables to network Byte order
unsigned long htons32(unsigned long val)
{
  return HTONS32(val);
}

//----------------------------------------------------------------------------
//Trägt Anwendung in Anwendungsliste ein
void stack_init (void)
{
	//IP, NETMASK und ROUTER_IP aus EEPROM auslesen	
    (*((unsigned long*)&myip[0])) = get_eeprom_value(IP_EEPROM_STORE,MYIP);
	(*((unsigned long*)&netmask[0])) = get_eeprom_value(NETMASK_EEPROM_STORE,NETMASK);
  
	//Broadcast-Adresse berechnen
	(*((unsigned long*)&broadcast_ip[0])) = (((*((unsigned long*)&myip[0])) & (*((unsigned long*)&netmask[0]))) | (~(*((unsigned long*)&netmask[0]))));

	//MAC Adresse setzen
	mymac[0] = MYMAC1;
    mymac[1] = MYMAC2;
    mymac[2] = MYMAC3;
    mymac[3] = MYMAC4;
    mymac[4] = MYMAC5;
    mymac[5] = MYMAC6;
	
	/*NIC Initialisieren*/
	ETH_INIT();
}

//----------------------------------------------------------------------------
//
unsigned long get_eeprom_value (unsigned int eeprom_adresse,unsigned long default_value)
{
	unsigned char value[4];
	
	for (unsigned char count = 0; count<4;count++)
	{
		eeprom_busy_wait ();	
		value[count] = eeprom_read_byte((unsigned char *)(eeprom_adresse + count));
	}

	//Ist der EEPROM Inhalt leer?
	if ((*((unsigned long*)&value[0])) == 0xFFFFFFFF)
	{
		return(default_value);
	}
	return((*((unsigned long*)&value[0])));
}

//----------------------------------------------------------------------------
//Interrupt von der Netzwerkkarte
ISR (ETH_INTERRUPT)
{
	eth.data_present = 1;
	ETH_INT_DISABLE;
}

//----------------------------------------------------------------------------
//ETH get data
void eth_get_data (void)
{ 		
	if(eth.data_present)
	{
		while( (PIND &(1<<3)) == 0)
		{	
			unsigned int packet_lenght;
				  
			packet_lenght = ETH_PACKET_RECEIVE(MTU_SIZE,eth_buffer);
			/*Wenn ein Packet angekommen ist, ist packet_lenght =! 0*/
			eth_buffer[packet_lenght+1] = 0;
			check_packet();
		}
		eth.data_present = 0;
		ETH_INT_ENABLE;
	}
	return;
}
//----------------------------------------------------------------------------
//Check Packet and call Stack for TCP or UDP
void check_packet (void)
{
    struct Ethernet_Header *ethernet;    //Pointer auf Ethernet_Header
    struct IP_Header       *ip;          //Pointer auf IP_Header

    ethernet = (struct Ethernet_Header *)&eth_buffer[ETHER_OFFSET];
    ip       = (struct IP_Header       *)&eth_buffer[IP_OFFSET];
    
    if(ethernet->EnetPacketType == HTONS(0x0806) )     //ARP
    {
        arp_reply(); // check arp packet request/reply
    }
    else
    {
        if( ethernet->EnetPacketType == HTONS(0x0800) )  // if IP
        {              
            if( ip->IP_Destaddr == *((unsigned long*)&myip[0]) )  // if my IP address 
            {
                arp_entry_add();  ///Refresh des ARP Eintrages
                if( ip->IP_Proto == PROT_UDP ) udp_socket_process();
            }
            else
			{
				if (ip->IP_Destaddr == (unsigned long)0xffffffff || ip->IP_Destaddr == *((unsigned long*)&broadcast_ip[0]) ) // if broadcast
				{
					if( ip->IP_Proto == PROT_UDP ) udp_socket_process();
				}
			}			
        }
    }
    return;
}

//----------------------------------------------------------------------------
//erzeugt einen ARP - Eintrag wenn noch nicht vorhanden 
void arp_entry_add (void)
{
    struct Ethernet_Header *ethernet;
    struct ARP_Header      *arp;
    struct IP_Header       *ip;
      
    ethernet = (struct Ethernet_Header *)&eth_buffer[ETHER_OFFSET];
    arp      = (struct ARP_Header      *)&eth_buffer[ARP_OFFSET];
    ip       = (struct IP_Header       *)&eth_buffer[IP_OFFSET];
        
    //Eintrag schon vorhanden?
    for (unsigned char a = 0; a<MAX_ARP_ENTRY; a++)
    {
        if( ethernet->EnetPacketType == HTONS(0x0806) ) //If ARP
        {
            if(arp_entry[a].arp_t_ip == arp->ARP_SIPAddr)
            {
                //Eintrag gefunden Time refresh
                arp_entry[a].arp_t_time = ARP_MAX_ENTRY_TIME;
                return;
            }
        } 
        if( ethernet->EnetPacketType == HTONS(0x0800) ) //If IP
        {
            if(arp_entry[a].arp_t_ip == ip->IP_Srcaddr)
            {
                //Eintrag gefunden Time refresh
                arp_entry[a].arp_t_time = ARP_MAX_ENTRY_TIME;
                return;
            }
        }
    }
  
    //Freien Eintrag finden
    for (unsigned char b = 0; b<MAX_ARP_ENTRY; b++)
    {
        if(arp_entry[b].arp_t_ip == 0)
        {
            if( ethernet->EnetPacketType == HTONS(0x0806) ) //if ARP
            {
                for(unsigned char a = 0; a < 6; a++)
                {
                    arp_entry[b].arp_t_mac[a] = ethernet->EnetPacketSrc[a]; 
                }
                arp_entry[b].arp_t_ip   = arp->ARP_SIPAddr;
                arp_entry[b].arp_t_time = ARP_MAX_ENTRY_TIME;
                return;
            }
            if( ethernet->EnetPacketType == HTONS(0x0800) ) //if IP
            {
                for(unsigned char a = 0; a < 6; a++)
                {
                    arp_entry[b].arp_t_mac[a] = ethernet->EnetPacketSrc[a]; 
                }
                arp_entry[b].arp_t_ip   = ip->IP_Srcaddr;
                arp_entry[b].arp_t_time = ARP_MAX_ENTRY_TIME;
                return;
            }
            DEBUG("Kein ARP oder IP Packet!\r\n");
            return;
        }
    }
    //Eintrag konnte nicht mehr aufgenommen werden
    DEBUG("ARP entry tabelle voll!\r\n");
    return;
}

//----------------------------------------------------------------------------
//Diese Routine such anhand der IP den ARP Eintrag
char arp_entry_search (unsigned long dest_ip)
{
	for (unsigned char b = 0;b<MAX_ARP_ENTRY;b++)
	{
		if(arp_entry[b].arp_t_ip == dest_ip)
		{
			return(b);
		}
	}
	return (MAX_ARP_ENTRY);
}

//----------------------------------------------------------------------------
//Diese Routine Erzeugt ein neuen Ethernetheader
void new_eth_header (unsigned char *buffer,unsigned long dest_ip)
{
    unsigned char b;
    unsigned char a;
	struct Ethernet_Header *ethernet;
	ethernet = (struct Ethernet_Header *)&buffer[ETHER_OFFSET];
	
	b = arp_entry_search (dest_ip);
	if (b != MAX_ARP_ENTRY) //Eintrag gefunden wenn ungleich
	{
		for(unsigned char a = 0; a < 6; a++)
		{
			//MAC Destadresse wird geschrieben mit MAC Sourceadresse
			ethernet->EnetPacketDest[a] = arp_entry[b].arp_t_mac[a];
			//Meine MAC Adresse wird in Sourceadresse geschrieben
			ethernet->EnetPacketSrc[a] = mymac[a];
		}
		return;
	}
    
	DEBUG("ARP Eintrag nicht gefunden*\r\n");
	for(a = 0; a < 6; a++)
	{
		//MAC Destadresse wird geschrieben mit MAC Sourceadresse
		ethernet->EnetPacketDest[a] = 0xFF;
		//Meine MAC Adresse wird in Sourceadresse geschrieben
		ethernet->EnetPacketSrc[a] = mymac[a];
	}
	return;

}

//----------------------------------------------------------------------------
//Diese Routine Antwortet auf ein ARP Paket
void arp_reply (void)
{
    unsigned char b;
    unsigned char a;
    struct Ethernet_Header *ethernet;
    struct ARP_Header      *arp;

    ethernet = (struct Ethernet_Header *)&eth_buffer[ETHER_OFFSET];
    arp      = (struct ARP_Header      *)&eth_buffer[ARP_OFFSET];


    if( arp->ARP_HWType  == HTONS(0x0001)  &&             // Hardware Typ:   Ethernet
        arp->ARP_PRType  == HTONS(0x0800)  &&             // Protokoll Typ:  IP
        arp->ARP_HWLen   == 0x06           &&             // Länge der Hardwareadresse: 6
        arp->ARP_PRLen   == 0x04           &&             // Länge der Protokolladresse: 4 
        arp->ARP_TIPAddr == *((unsigned long*)&myip[0])) // Für uns?
    {
        if (arp->ARP_Op == HTONS(0x0001) )                  // Request?
        {
            arp_entry_add(); 
            new_eth_header (eth_buffer, arp->ARP_SIPAddr); // Erzeugt ein neuen Ethernetheader
            ethernet->EnetPacketType = HTONS(0x0806);      // Nutzlast 0x0800=IP Datagramm;0x0806 = ARP
      
            b = arp_entry_search (arp->ARP_SIPAddr);
            if (b < MAX_ARP_ENTRY)                         // Eintrag gefunden wenn ungleich
            {
                for(a = 0; a < 6; a++)
                {
                    arp->ARP_THAddr[a] = arp_entry[b].arp_t_mac[a];
                    arp->ARP_SHAddr[a] = mymac[a];
                }
            }
            else
            {
                DEBUG("ARP Eintrag nicht gefunden\r\n");        // Unwarscheinlich
            }
      
            arp->ARP_Op      = HTONS(0x0002);                   // ARP op = ECHO  
            arp->ARP_TIPAddr = arp->ARP_SIPAddr;                // ARP Target IP Adresse 
            arp->ARP_SIPAddr = *((unsigned long *)&myip[0]);   // Meine IP Adresse = ARP Source
      
            ETH_PACKET_SEND(ARP_REPLY_LEN,eth_buffer);          // ARP Reply senden...
            return;
        }

        if ( arp->ARP_Op == HTONS(0x0002) )                    // REPLY von einem anderen Client
        {
            arp_entry_add();
            DEBUG("ARP REPLY EMPFANGEN!\r\n");
        }
    }
    return;
}

//----------------------------------------------------------------------------
//Diese Routine erzeugt eine Cecksumme
unsigned int checksum (unsigned char *pointer,unsigned int result16,unsigned long result32)
{
	unsigned int result16_1 = 0x0000;
	unsigned char DataH;
	unsigned char DataL;
	
	//Jetzt werden alle Packete in einer While Schleife addiert
	while(result16 > 1)
	{
		//schreibt Inhalt Pointer nach DATAH danach inc Pointer
		DataH=*pointer++;

		//schreibt Inhalt Pointer nach DATAL danach inc Pointer
		DataL=*pointer++;

		//erzeugt Int aus Data L und Data H
		result16_1 = ((DataH << 8)+DataL);
		//Addiert packet mit vorherigen
		result32 = result32 + result16_1;
		//decrimiert Länge von TCP Headerschleife um 2
		result16 -=2;
	}

	//Ist der Wert result16 ungerade ist DataL = 0
	if(result16 > 0)
	{
		//schreibt Inhalt Pointer nach DATAH danach inc Pointer
		DataH=*pointer;
		//erzeugt Int aus Data L ist 0 (ist nicht in der Berechnung) und Data H
		result16_1 = (DataH << 8);
		//Addiert packet mit vorherigen
		result32 = result32 + result16_1;
	}
	
	//Komplementbildung (addiert Long INT_H Byte mit Long INT L Byte)
	result32 = ((result32 & 0x0000FFFF)+ ((result32 & 0xFFFF0000) >> 16));
	result32 = ((result32 & 0x0000FFFF)+ ((result32 & 0xFFFF0000) >> 16));	
	result16 =~(result32 & 0x0000FFFF);
	
	return (result16);
}

//----------------------------------------------------------------------------
//Diese Routine erzeugt ein IP Packet
void make_ip_header (unsigned char *buffer,unsigned long dest_ip)
{
    unsigned int result16;  //Checksum
    struct Ethernet_Header *ethernet;
    struct IP_Header       *ip;

    ethernet = (struct Ethernet_Header *)&buffer[ETHER_OFFSET];
    ip       = (struct IP_Header       *)&buffer[IP_OFFSET];

    new_eth_header (buffer, dest_ip);         //Erzeugt einen neuen Ethernetheader
    ethernet->EnetPacketType = HTONS(0x0800); //Nutzlast 0x0800=IP

    IP_id_counter++;

    ip->IP_Frag_Offset = 0x0040;  //don't fragment
    ip->IP_ttl         = 128;      //max. hops
    ip->IP_Id          = htons(IP_id_counter);
    ip->IP_Vers_Len    = 0x45;  //4 BIT Die Versionsnummer von IP, 
    ip->IP_Tos         = 0;
    ip->IP_Destaddr     = dest_ip;
    ip->IP_Srcaddr     = *((unsigned long *)&myip[0]);
    ip->IP_Hdr_Cksum   = 0;
  
    //Berechnung der IP Header länge  
    result16 = (ip->IP_Vers_Len & 0x0F) << 2;

    //jetzt wird die Checksumme berechnet
    result16 = checksum (&ip->IP_Vers_Len, result16, 0);

    //schreibt Checksumme ins Packet
    ip->IP_Hdr_Cksum = htons(result16);
    return;
}

//----------------------------------------------------------------------------
//Diese Routine verwaltet die UDP Ports
void udp_socket_process(void)
{
	struct UDP_Header *udp;
	udp = (struct UDP_Header *)&eth_buffer[UDP_OFFSET];

	//UDP DestPort mit Portanwendungsliste durchführen
	if (UDP_PORT_TABLE[0].port == (htons(udp->udp_DestPort)))
	{
		//zugehörige Anwendung ausführen
		UDP_PORT_TABLE[0].fp(0);
	}
	return;
}

//----------------------------------------------------------------------------
//Diese Routine Erzeugt ein neues UDP Packet
void create_new_udp_packet( unsigned int  data_length,
                            unsigned int  src_port,
                            unsigned int  dest_port,
                            unsigned long dest_ip)
{
    unsigned int  result16;
    unsigned long result32;

    struct UDP_Header *udp;
    struct IP_Header  *ip;

    udp = (struct UDP_Header *)&eth_buffer[UDP_OFFSET];
    ip  = (struct IP_Header  *)&eth_buffer[IP_OFFSET];
  
    udp->udp_SrcPort  = htons(src_port);
    udp->udp_DestPort = htons(dest_port);

    data_length     += UDP_HDR_LEN;                //UDP Packetlength
    udp->udp_Hdrlen = htons(data_length);

    data_length     += IP_VERS_LEN;                //IP Headerlänge + UDP Headerlänge
    ip->IP_Pktlen = htons(data_length);
    data_length += ETH_HDR_LEN;
    ip->IP_Proto = PROT_UDP;
    make_ip_header (eth_buffer,dest_ip);

    udp->udp_Chksum = 0;
  
    //Berechnet Headerlänge und Addiert Pseudoheaderlänge 2XIP = 8
    result16 = htons(ip->IP_Pktlen) + 8;
    result16 = result16 - ((ip->IP_Vers_Len & 0x0F) << 2);
    result32 = result16 + 0x09;
  
    //Routine berechnet die Checksumme
    result16 = checksum ((&ip->IP_Vers_Len+12), result16, result32);
    udp->udp_Chksum = htons(result16);

    ETH_PACKET_SEND(data_length,eth_buffer); //send...
    return;
}

//----------------------------------------------------------------------------
//End of file: stack.c







