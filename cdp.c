/*
 *  $Id: cdp.c,v 1.2 2004/01/03 20:31:01 mike Exp $
 *
 *  libnet 1.1
 *  Build an CDP packet
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "includes.h"

#ifdef OS_WINDOWS

#include <libnet.h>
#include "voiphop.h"

//int cdp_spoof_mode(char *);

struct cdpoptions{
	/*
	short deviceid_type;
	short deviceid_length;
	char deviceid[15];
	*/
	short portid_type;
	short portid_length;
	char portid[6];
	short cap_type;
	short cap_length;
	char cap[4];
	short sw_type;
	short sw_length;
	char sw[12];
	short platform_type;
	short platform_length;
	char platform[19];
	short power_type;
	short power_length;
	short power;
	short duplex_type;
	short duplex_length;
	char duplex_data;
	short vlanquery_type;
	short vlanquery_length;
	char vlanquery_data[4];
}__attribute((packed));



int
cdp_spoof_mode(char *IfName_temp)
{
    int c, len;
    libnet_t *l;
    libnet_ptag_t t;
    u_char *value;
    u_char values[100];
    u_short tmp;
    u_long tmp2;
	//char *device = "\\DEVICE\\NPF_{F1427D6F-B37D-4FE7-BDA9-8DB3A0E5E309}";
	char *device = IfName_temp;
    char errbuf[LIBNET_ERRBUF_SIZE];

    //printf("libnet 1.1 packet shaping: CDP[link]\n"); 
    /*
     *  Initialize the library.  Root priviledges are required.
     */
    l = libnet_init(
            LIBNET_LINK,                            /* injection type */
            device,                                 /* network interface */
            errbuf);                                /* errbuf */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE);
    }

	struct cdpoptions cdpopt;

	short deviceid_type;
	short deviceid_length;
	u_char deviceid[15];

	deviceid_type = htons(0x0001);
	deviceid_length = htons(0x0013);
	memset(deviceid,'\0',sizeof(deviceid));
	memcpy(deviceid,"SEP123456789123",sizeof(deviceid));


	cdpopt.portid_type = htons(0x0003);
	cdpopt.portid_length = htons(0x000a);
	memset(cdpopt.portid,'\0',sizeof(cdpopt.portid));
	strncpy(cdpopt.portid,"Port 1",sizeof(cdpopt.portid));

	cdpopt.cap_type = htons(0x0004);
	cdpopt.cap_length = htons(0x0008);
	cdpopt.cap[0] = 0x00; cdpopt.cap[1] = 0x00;
	cdpopt.cap[2] = 0x00; cdpopt.cap[3] = 0x10;

	cdpopt.sw_type = htons(0x0005);
	cdpopt.sw_length = htons(0x0010);
	memset(cdpopt.sw,'\0',sizeof(cdpopt.sw));
	strncpy(cdpopt.sw,"P003-08-8-00",sizeof(cdpopt.sw));

	cdpopt.platform_type = htons(0x0006);
	cdpopt.platform_length = htons(0x0017);
	memset(cdpopt.platform,'\0',sizeof(cdpopt.platform));
	strncpy(cdpopt.platform,"cisco WS-C3560G-4PS",sizeof(cdpopt.platform));

	/*
	cdpopt.duplex_type = htons(0x000b);
	cdpopt.duplex_length = htons(0x0005);
	cdpopt.duplex = 0x01;
	*/

	cdpopt.power_type = htons(0x0010);
	cdpopt.power_length = htons(0x0006);
	cdpopt.power = htons(0x189c);

	cdpopt.duplex_type = htons(0x000b);
	cdpopt.duplex_length = htons(0x0005);
	cdpopt.duplex_data = 0x31;

	cdpopt.vlanquery_type = htons(0x000f);
	cdpopt.vlanquery_length = htons(0x0008);
	cdpopt.vlanquery_data[0] = 0x20; cdpopt.vlanquery_data[1] = 0x02;
	cdpopt.vlanquery_data[2] = 0x00; cdpopt.vlanquery_data[3] = 0x01;
		
    value   = "1";
    len     = strlen(value); 
	//u_int8_t value;
	//value = "1";

	unsigned char cdpBuffer[sizeof(cdpopt) + 19 + 4 + 1];
	memset(cdpBuffer,'\0',sizeof(cdpBuffer));
	unsigned char *tempbuf = cdpBuffer;
	unsigned char version = 0x02;
	memcpy(tempbuf,&version,1);
	tempbuf += 1;
	unsigned char ttl = 0xb4;
	memcpy(tempbuf,&ttl,1);
	tempbuf += 1;
	unsigned short psuedochksum = 0;
	memcpy(tempbuf,&psuedochksum,2);
	tempbuf += 2;
	unsigned short tmp1 = deviceid_type;
	memcpy(tempbuf,&tmp1,2);
	tempbuf += 2;
	tmp1 = deviceid_length;
	memcpy(tempbuf,&tmp1,2);
	tempbuf += 2;
	memcpy(tempbuf,&deviceid,sizeof(deviceid));
	tempbuf += sizeof(deviceid);
	memcpy(tempbuf,&cdpopt,sizeof(cdpopt));

   u_int16_t sum = chksum(cdpBuffer,sizeof(cdpopt)+19+4);

    t = libnet_build_cdp(
            2,                                      /* version */
            180,                                     /* time to live */
            htons(sum),                                      /* checksum */
            0x0001,									/* type */
            sizeof(deviceid),                               /* length */
            deviceid,										/* value */
            (u_int8_t *)&cdpopt,                    /* payload */
            sizeof(cdpopt),                         /* payload size */
            l,                                      /* libnet handle */
            0);                                     /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build CDP header: %s\n", libnet_geterror(l));
        goto bad;
    }

	/*
    memset(values, 0, sizeof(values));
    tmp = htons(LIBNET_CDP_PORTID);
    memcpy(values, &tmp, 2);
    tmp = htons(0x0014);
    memcpy(values + 2, &tmp, 2);
    memcpy(values + 4, (u_char *)"FastEthernet0/20", 16);
    t = libnet_build_data(
            values,
            20,
            l,
            0);
    if (t == -1)
    {
        fprintf(stderr, "Can't build CDP data: %s\n", libnet_geterror(l));
        goto bad;
    }
    memset(values, 0, sizeof(values));
    tmp = htons(LIBNET_CDP_CAPABIL);
    memcpy(values, &tmp, 2);
    tmp = htons(0x0008);
    memcpy(values + 2, &tmp, 2);
    tmp2 = htonl((LIBNET_CDP_CAP_L2S | LIBNET_CDP_CAP_L2B));
    memcpy(values + 4, &tmp2, 4);
    t = libnet_build_data(
            values,
            8,
            l,
            0);
    if (t == -1)
    {
        fprintf(stderr, "Can't build CDP data: %s\n", libnet_geterror(l));
        goto bad;
    }
    memset(values, 0, sizeof(values));
    tmp = htons(LIBNET_CDP_VERSION);
    memcpy(values, &tmp, 2);
    tmp = htons(0x001f);
    memcpy(values + 2, &tmp, 2);
    memcpy(values + 4, (u_char *)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26);
    t = libnet_build_data(
            values,
            31,
            l,
            0);
    if (t == -1)
    {
        fprintf(stderr, "Can't build CDP data: %s\n", libnet_geterror(l));
        goto bad;
    }
    memset(values, 0, sizeof(values));
    tmp = htons(LIBNET_CDP_PLATFORM);
    memcpy(values, &tmp, 2);
    tmp = htons(0x0015);
    memcpy(values + 2, &tmp, 2);
    memcpy(values + 4, (u_char *)"cisco WS-C2924-XL", 17);
    t = libnet_build_data(
            values,
            21,
            l,
            0);
    if (t == -1)
    {
        fprintf(stderr, "Can't build CDP data: %s\n", libnet_geterror(l));
        goto bad;
    }
	*/

	int maclen;
	u_int8_t *enet_dst;
	u_int8_t *enet_src;

	int8_t *dst_mac = "01:00:0c:cc:cc:cc";
	int8_t *src_mac = "00:43:ff:a9:86:db";

	enet_dst = libnet_hex_aton(dst_mac,&maclen);
	enet_src = libnet_hex_aton(src_mac,&maclen);

	/*
	enet_dst[0] = 0x01; enet_dst[3] = 0xcc;
	enet_dst[1] = 0x00; enet_dst[4] = 0xcc;
	enet_dst[2] = 0x0c; enet_dst[5] = 0xcc;

	u_char *enet_src;
	enet_src[0] = 0x06;	enet_src[3] = 0xa7;
	enet_src[1] = 0x0a; enet_src[4] = 0x24;
	enet_src[2] = 0x41; enet_src[5] = 0x4c;
	*/
	
	
    //t = libnet_build_ethernet(
       //     enet_dst,                               /* ethernet destination */
       //     enet_src,                               /* ethernet source */
       //     0x2000,                                 /* protocol type */
       //     NULL,                                   /* payload */
       //     0,                                      /* payload size */
       //     l,                                      /* libnet handle */
       //     0);                                     /* libnet id */
   /* if (t == -1)
    {
        fprintf(stderr, "Can't build ethernet header: %s\n", libnet_geterror(l));
        goto bad;
    } */	
		
	u_char oui[3];
	oui[0] = 0x00; oui[1] = 0x00; oui[2] = 0x0c;
	t = libnet_build_802_2snap(
			0xaa,
			0xaa,
			0x03,
			oui,
			0x2000,
			NULL,
			0,
			l,
			0);
	if(t == -1){
		fprintf(stderr, "Can't build ethernet header: %s\n", libnet_geterror(l));
        goto bad;
    }

	int eth_len = 0x6B;
	t = libnet_build_802_3(
			enet_dst,
			enet_src,
			eth_len,
			NULL,
			0,
			l,
			0);
	if(t == -1)
	{
		fprintf(stderr, "Can't build ethernet header: %s\n", libnet_geterror(l));
        goto bad;
    }


    /*
     *  Write it to the wire.
     */
    c = libnet_write(l);

    if (c == -1)
    {
        fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
        goto bad;
    }
    else
    {
        //fprintf(stderr, "Wrote %d byte CDP packet; check the wire.\n", c);
    }
    libnet_destroy(l);
    return 0;
bad:
    libnet_destroy(l);
    return -1;
}

#endif

/* EOF */
