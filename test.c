#include <stdio.h>
#include <signal.h>
#include <pcap.h>
#include <stdlib.h>
#include <string.h>


const u_char broadcastaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

pcap_t *pcap_handle;

void exit_signal(int id)
{
    printf("Exit signal\n");
    if (pcap_handle != NULL) {
	pcap_breakloop(pcap_handle);
	pcap_close(pcap_handle);
    }
    exit(0);
}


void bssid_found(const u_char * bssid, int8_t power)
{
    if (strncmp(bssid, broadcastaddr, 6) != 0)
	printf("%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
	       bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

}


void process_packet(u_char * useless, const struct pcap_pkthdr *header,
		    const u_char * packet)
{
    unsigned char bssid[6];
    int8_t power;

    //printf("Packet with length %d received con %hhx:%hhx\n", header->len,packet[18],packet[19]);
    /* First 18 bytes are radiotap header. We don't care about that */
    /* The byte after the radiotap header is the 802.11 type or subtype */
    /* We have the station address on:
       pkg of type 0x08 (beacons)
       or of type 0x04 (probe request)
       or of type 0x24 (null function)
       or of type 0x05 (probe response)
       or of type 0x28 (QoS)
     */
    if (header->len > 18) {
	power = packet[14];


	switch (packet[18]) {

	case 0x40:		//destination,source,bss
	case 0x50:		//destination,source,bss
	case 0x80:		//destination,source,bss 
	case 0x48:		//bss,source,destination
	case 0x88:		//bss,source,destination
	    if (header->len >= 40) {
		bssid_found(packet + 22, power);
		bssid_found(packet + 28, power);
		bssid_found(packet + 34, power);
	    }
	    break;

	case 0xd4:		//destination
	case 0xc4:		//destination
	    if (header->len == 28) {
		bssid_found(packet + 22, power);
	    }
	    break;

	default:
	    break;

	}
    }

}

void dump_pcap_error(pcap_t *pcap_handle) 
{
	fprintf(stderr, "Error: %s\n", pcap_geterr(pcap_handle));

}

int main(int argc, char **argv)
{
    char error_buffer[100];
    int status = 0;

    signal(SIGINT, exit_signal);	/* Ctrl-C */
    signal(SIGQUIT, exit_signal);	/* Ctrl-\ */

    pcap_handle = pcap_create(argv[argc - 1], error_buffer);
    if (pcap_handle == NULL) {
	fprintf(stderr, "Error while creating handle on interface %s\n",
		argv[argc - 1]);
    }

    status = pcap_set_rfmon(pcap_handle, 1);
    if (status != 0) {
	dump_pcap_error(pcap_handle) ;
	//fprintf(stderr, "Error while opening monitor mode\n");
    }

    status = pcap_set_promisc(pcap_handle, 0);
    if (status != 0) {
	//fprintf(stderr, "Error while setting no-promisc\n");
	dump_pcap_error(pcap_handle) ;
    }

    status = pcap_set_timeout(pcap_handle, 0);
    if (status != 0) {
	//fprintf(stderr, "Error while setting timeout on pcap\n");
	dump_pcap_error(pcap_handle) ;
    }

    status = pcap_activate(pcap_handle);
    if (status != 0) {
	dump_pcap_error(pcap_handle) ;
	
	/* 
	fprintf(stderr, "Error while activating pcap number %d\n", status);
	errorstring = pcap_geterr(pcap_handle);
	fprintf(stderr, "Error: %s\n", errorstring);
	*/
    }

    pcap_loop(pcap_handle, -1, process_packet, NULL);


    pcap_close(pcap_handle);
    return 0;
}
