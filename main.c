/*
 * ACE - Automated Corporate Enumerator
 * Copyright (C) 2008 Sipera VIPER Lab
 * Author:  Jason Ostrom
 * Chief Developer:  Arjun Sambamoorthy
 *
 * This file is part of ACE.
 *
 * ACE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ACE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"
#include "tftp_client.h"
#include "pcap.h"
#include "voiphop.h"

#ifndef OS_WINDOWS
#include "net/route.h"
#include "client.h"
#endif

#ifdef OS_WINDOWS

#include <ddk/ndis.h>
#include <ddk/ntddndis.h>
#include <ddk/nuiouser.h>
#include <windows.h>
#include <winsock.h>
#include <malloc.h>
#include <ctype.h>
#include <pshpack1.h>
#include <poppack.h>

#include <winsock2.h>
#include <iphlpapi.h>
#include <tchar.h>

#include "wchar.h"
#include "dhcpcsdk.h"
#include "setupapi.h"
#include "Iprtrmib.h"

#define MAX_NDIS_DEVICE_NAME_LEN        256

#ifndef MIB_IPPROTO_NETMGMT
 #define MIB_IPPROTO_NETMGMT 3
#endif

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x)) 
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

BOOL OpenNdisDevice(HANDLE, CHAR *);
void windowsSetVLANID(int, CHAR *);
int vcRenewInterfaceIP(WCHAR *);
int getOption150AndRouterIP(CHAR *, CHAR *, CHAR *);
int addRoute(WCHAR *, CHAR *, CHAR *);
char tftp_server_ip[16];
char router_ip[16];

int tftp_ip_yes = 0;
int dont_parse_option150 = 0;
int dont_set_ip = 0;

#endif



void usage(){

        printf("ACE v1.10: Automated Corporate (Data) Enumerator\n");
	printf("Usage: ace [-i interface] [ -m mac address ] [ -t tftp server ip address | -c cdp mode | -v voice vlan id | -r vlan interface | -d verbose mode ]\n\n");
	printf("-i <interface> (Mandatory) Interface for sniffing/sending packets\n");
	printf("-m <mac address> (Mandatory) MAC address of the victim IP phone \n");
	printf("-t <tftp server ip> (Optional) tftp server ip address \n");
	printf("-c <cdp mode 0|1 > (Optional) 0 CDP sniff mode, 1 CDP spoof mode \n");
	printf("-v <voice vlan id> (Optional) Enter the voice vlan ID \n");
	printf("-r <vlan interface> (Optional) Removes the VLAN interface \n");
	printf("-d 		    (Optional) Verbose | debug mode \n\n");

	printf("Example Usages: \n");
        printf("Usage requires MAC Address of IP Phone supplied with -m option\n");
        printf("Usage:  ace -t <TFTP-Server-IP> -m <MAC-Address>\n\n");
        printf("Mode to automatically discover TFTP Server IP via DHCP Option 150 (-m)\n");
        printf("Example:  ace -i eth0 -m 00:1E:F7:28:9C:8e\n\n");
        printf("Mode to specify IP Address of TFTP Server\n");
        printf("Example:  ace -i eth0 -t 192.168.10.150 -m 00:1E:F7:28:9C:8e\n\n");
	printf("Mode to specify the Voice VLAN ID\n");
	printf("Example: ace -i eth0 -v 96 -m 00:1E:F7:28:9C:8E\n\n");
	printf("Verbose mode \n");
	printf("Example: ace -i eth0 -v 96 -m 00:1E:F7:28:9C:8E -d\n\n");
	printf("Mode to remove vlan interface\n");
	printf("Example: ace -r eth0.96 \n\n");
	printf("Mode to auto-discover voice vlan ID in the listening mode for CDP\n");
	printf("Example: ace -i eth0 -c 0 -m 00:1E:F7:28:9C:8E\n\n");
	printf("Mode to auto-discover voice vlan ID in the spoofing mode for CDP\n");
	printf("Example: ace -i eth0 -c 1 -m 00:1E:F7:28:9C:8E\n\n");
}

void remove_interface(char *RemovedInterface){

        printf("Removing interface %s\n",RemovedInterface);
#ifndef OS_WINDOWS
        struct vlan_ioctl_args if_request;
        int fd;
        int vvid;

        strcpy(if_request.device1, RemovedInterface);

        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                fprintf(stderr, "Fatal:  Couldn't open socket\n");
                        exit(2);
        }

        if_request.cmd = DEL_VLAN_CMD;
        if (ioctl(fd, SIOCSIFVLAN, &if_request) < 0) {
                fprintf(stderr, "Error trying to remove VLAN Interface %s.  Error: %s\n", RemovedInterface, strerror(errno));
        } else {
                fprintf(stdout, "Removed VLAN Interface %s\n", RemovedInterface);
        }
#else
	printf("To remove the interface in windows, please uninstall the driver\n");
#endif

}

int check_mac_format(char *mac){
	
	int nbyte = 5;

	for (nbyte=2; nbyte<16; nbyte+=3) {
                if (mac[nbyte] != ':') {
                        fprintf (stderr, "Incorrect format: %s\n",mac);
                        return -1;
                }
        }
	return 0;

}

int main(int argc, char *argv[]){

	int 		opt = 0;
	int		vlan_yes = 0;
	int		vlan_id;
	int		intfd;
	int		tftp_yes = 0;
	int		mac_yes = 0;
	int		cdp_mode_type = 0;
	int		cdp_mode_yes = 0;
	int		IfName_temp_len;
	char 		file_name[64];
	char 		*IfName_temp;
	
	http_port = HTTP_PORT;	

	while((opt = getopt(argc,argv,"i:t:v:m:c:r:dh?"))!=-1){

                switch(opt){

			case 'i':
				intfd = 1;
				IfName_temp = malloc((strlen(optarg)+1)*sizeof(char));
				memset(IfName_temp,'\0',strlen(optarg)+1);
				strcpy(IfName_temp,optarg);
				IfName_temp_len = strlen(optarg);
				break;
                        case 't':
				tftp_yes = 1;
                                memset(tftp_server_ip,'\0',sizeof(tftp_server_ip));
                                strncpy(tftp_server_ip,optarg,strlen(optarg));
                                break;
                        case 'm':
				mac_yes = 1;
                                memset(file_name,'\0',64);
                                strncpy(file_name,optarg,strlen(optarg));
				int mac_err = check_mac_format(file_name);
				if(mac_err < 0){
					usage();
					exit(1);
				}
                                break;
			case 'v':
			        vlan_yes = 1;
				vlan_id = atol(optarg);
				break;
			case 'r':
				IfName_temp = malloc((strlen(optarg)+1)*sizeof(char));
				memset(IfName_temp,'\0',strlen(optarg)+1);
				strcpy(IfName_temp,optarg);
				remove_interface(IfName_temp);
				exit(0);
			case 'c':
				cdp_mode_yes = 1;
				cdp_mode_type = atol(optarg);
				break;						
			case 'd':
				verbosity = 1;
				printf("Entered verbosity mode\n");
				break;
				
                        default:
                                usage();
                                return 0;
                }
        }
        if(argc < 2){
                usage();
                exit(1);
        }
	
        if(intfd != 1){
		printf("Interface option should be entered \n\n");
		usage();
		exit(1);
	}
        if(mac_yes != 1){
		printf("MAC address should be entered \n\n");
		usage();
		exit(1);
	}

#ifdef OS_WINDOWS
	
	char *start = strstr(IfName_temp,"{");
	char *end = strchr(IfName_temp,'\0');

	int length = end - start;

	int buf = length + 1;
	CHAR networkDevice[buf];
	memcpy(networkDevice,start,length);

	int v_netdevLength = strlen("\\DEVICE\\") + length + 1;
	CHAR *v_NetworkDevice = (CHAR *)malloc(v_netdevLength * sizeof(CHAR));
	CHAR *tempDevice = v_NetworkDevice;
	memset(tempDevice,'\0',v_netdevLength * sizeof(CHAR));
	memcpy(tempDevice,"\\DEVICE\\",strlen("\\DEVICE\\"));
	tempDevice += strlen("\\DEVICE\\");
	memcpy(tempDevice,start,length);

	int tcpNetLength = v_netdevLength + strlen("TCPIP_");
	CHAR *tcpNetworkDevice = (CHAR *) malloc(tcpNetLength * sizeof(CHAR));
	CHAR *temptcpNetworkDevice = tcpNetworkDevice;
	
	memset(temptcpNetworkDevice,'\0',tcpNetLength);

	memcpy(temptcpNetworkDevice,"\\DEVICE\\TCPIP_",strlen("\\DEVICE\\TCPIP_"));
	temptcpNetworkDevice += strlen("\\DEVICE\\TCPIP_");
	memcpy(temptcpNetworkDevice,start,length);
	WCHAR wTCPNetworkDevice[tcpNetLength+1];
	MultiByteToWideChar(CP_ACP,0,tcpNetworkDevice,strlen(tcpNetworkDevice)+1,wTCPNetworkDevice,sizeof(wTCPNetworkDevice)/sizeof(wTCPNetworkDevice[0]));

#endif

	if(tftp_yes == 1){

                /* Global variable, which tells dhcpcd not to parse for option150 */
                dont_parse_option150 = 1;
        }
	/*Sets the vlan interface and ip address */			
	if(vlan_yes == 1){
#ifndef OS_WINDOWS
		vlan_hop(vlan_id,IfName_temp);
		if(tftp_ip_yes != 1 && tftp_yes != 1){
                	printf("TFTP server ip address not received in Option 150 in DHCP messages \n");
               		printf("See Usage, to VLAN HOP and get the TFTP server ip address \n");
                	exit(1);
        	}
#endif
#ifdef OS_WINDOWS
			windowsSetVLANID(vlan_id, v_NetworkDevice);
			if(!dont_set_ip){
				printf("Please wait. It will take a few seconds for setting the IP address and querying the DHCP option 150.\n");
				int retRenew = vcRenewInterfaceIP(wTCPNetworkDevice);
				if(retRenew < 0)
					exit(1);
			}
			int retdhcp = getOption150AndRouterIP(networkDevice,tftp_server_ip,router_ip);
			if(retdhcp < 0)
				exit(1);
			int retRoute = addRoute(wTCPNetworkDevice, tftp_server_ip, router_ip);
			if(retRoute < 0)
				exit(1);
#endif

	}
	else if(cdp_mode_yes == 1){
#ifndef OS_WINDOWS
		vlan_id = 0;
		vlan_id = cdp_mode(cdp_mode_type,IfName_temp);
		if(tftp_ip_yes != 1 && tftp_yes != 1){
                	printf("TFTP server ip address not received in Option 150 in DHCP messages \n");
                	printf("See Usage, to VLAN HOP and get the TFTP server ip address \n");
                	exit(1);
        	}
#endif
#ifdef OS_WINDOWS
			vlan_id = 0;
		    vlan_id = cdp_mode(cdp_mode_type,IfName_temp);
			windowsSetVLANID(vlan_id, v_NetworkDevice);
			if(!dont_set_ip){
				printf("Please wait. It will take a few seconds for setting the IP address and querying the DHCP option 150.\n");
				int retRenew = vcRenewInterfaceIP(wTCPNetworkDevice);
				if(retRenew < 0)
					exit(1);
			}
			int retdhcp = getOption150AndRouterIP(networkDevice,tftp_server_ip,router_ip);
			if(retdhcp < 0)
				exit(1);
			int retRoute = addRoute(wTCPNetworkDevice, tftp_server_ip, router_ip);
			if(retRoute < 0)
				exit(1);
#endif
	}
	else if(tftp_yes != 1) {
		/* Global variable, which tells dhcpcd not to set the ip address */
		dont_set_ip = 1;
#ifndef OS_WINDOWS
		dhcpclientcall(IfName_temp);
		if(tftp_ip_yes != 1){
                printf("TFTP server ip address not received in Option 150 in DHCP messages \n");
                printf("See Usage, to VLAN HOP and get the TFTP server ip address \n");
                exit(1);
        }
#endif

#ifdef OS_WINDOWS
		int retdhcp = getOption150AndRouterIP(networkDevice,tftp_server_ip,router_ip);
		if(retdhcp < 0)
			exit(1);
		int retRoute = addRoute(wTCPNetworkDevice, tftp_server_ip, router_ip);
		if(retRoute < 0)
			exit(1);
#endif
		
	}

	/*	
	struct  rtentry         rtent;
	struct  sockaddr_in     *p;
	
	int sock = socket(AF_INET,SOCK_DGRAM,0);
	memset(&rtent,0,sizeof(struct rtentry));
	
        p                       =       (struct sockaddr_in *)&rtent.rt_dst;
        p->sin_family           =       AF_INET;
	p->sin_addr.s_addr	= 	inet_addr("172.16.100.1");
        p                       =       (struct sockaddr_in *)&rtent.rt_gateway;
        p->sin_family           =       AF_INET;
        p->sin_addr.s_addr      =       inet_addr("172.16.96.254");
        p                       =       (struct sockaddr_in *)&rtent.rt_genmask;
        p->sin_family           =       AF_INET;
        p->sin_addr.s_addr      =       inet_addr("255.255.255.255");

        rtent.rt_dev            =       "eth0.96";
        rtent.rt_metric     	=     	0;
        rtent.rt_flags      	=     	RTF_UP|RTF_HOST;

        if (ioctl(sock,SIOCADDRT,&rtent) == -1 ){
		perror("route host add \n");	
	}
	*/		
	
	/* Adding host route using popen, have to use SIOCADDRT commented above, but it is not working */
	
#ifndef OS_WINDOWS
	
	FILE *fp;
	char route_buf[64] = {'\0'};
	strcpy(route_buf,"route add -host ");
	strcat(route_buf,tftp_server_ip);
	strcat(route_buf," gw ");
	strcat(route_buf,router_ip);

	fp = popen(route_buf,"r");
	if(fp == NULL){
		printf("Error: adding host route in routing table \n");
		exit(1);
	}
	
	pclose(fp);
#endif
	
	/*TFTP Handler */
	int tftp_return = tftp_handler(file_name,tftp_server_ip);
	if(tftp_return < 0){
		exit(1);
	}

	/*Parses directory url to get the ip address */
        parse_url(directory_buf);

	/* calls the http handler */
	http_handler();
    print_corporate_data();

return 0;
} // Main ends

#ifdef OS_WINDOWS

BOOL OpenNdisDevice( HANDLE  Handle, CHAR *v_NetworkDevice)
{
    WCHAR   wNdisDeviceName[MAX_NDIS_DEVICE_NAME_LEN];
    INT     wNameLength;
    INT     NameLength = strlen(v_NetworkDevice);
    DWORD   BytesReturned;
    INT     i;

    //
    // Convert to unicode string - non-localized...
    //
	//printf("v_NetworkDevice value is %s\n",v_NetworkDevice);
    wNameLength = 0;
    for (i = 0; i < NameLength && i < MAX_NDIS_DEVICE_NAME_LEN-1; i++)
    {
        wNdisDeviceName[i] = (WCHAR)v_NetworkDevice[i];
        wNameLength++;
    }
    wNdisDeviceName[i] = L'\0';

    //printf("Trying to access NDIS Device: %ws\n", wNdisDeviceName);

    return (DeviceIoControl(
                Handle,
                IOCTL_NDISPROT_OPEN_DEVICE,
                (LPVOID)&wNdisDeviceName[0],
                wNameLength*sizeof(WCHAR),
                NULL,
                0,
                &BytesReturned,
                NULL));

}

void windowsSetVLANID(int vlan_id, CHAR *v_NetworkDevice){

	DWORD byteReturned;
	HANDLE v_FileHandle;
	v_FileHandle = INVALID_HANDLE_VALUE;
	CHAR *ndisProto = "\\\\.\\\\NdisProt";

	v_FileHandle = CreateFile(ndisProto,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,(HANDLE)INVALID_HANDLE_VALUE);
	if(v_FileHandle == INVALID_HANDLE_VALUE){
		printf("Error: Creating File Handle %02x\n",GetLastError());
		CloseHandle(v_FileHandle);
		v_FileHandle = INVALID_HANDLE_VALUE;
		exit(1);
	}

     if (!DeviceIoControl(v_FileHandle,
						 IOCTL_NDISPROT_BIND_WAIT,
						 NULL,
						 0,
						 NULL,
						 0,
						 &byteReturned,
						 NULL))
	{
		printf("IOCTL_NDISPROT_BIND_WAIT failed %02x\n", GetLastError());
		CloseHandle(v_FileHandle);
		v_FileHandle = INVALID_HANDLE_VALUE;
		exit(1);
	}

	if (!OpenNdisDevice(v_FileHandle, v_NetworkDevice))
        {
            printf("Failed to access %s\n", v_NetworkDevice);
			if(v_FileHandle)
				CloseHandle(v_FileHandle);
			exit(1);

        }

	
	UCHAR	QueryBuffer[1024];
	PNDISPROT_QUERY_OID pQueryOid;
	ULONG vvid;

	pQueryOid = (PNDISPROT_QUERY_OID) &QueryBuffer[0];
	pQueryOid->Oid = OID_GEN_VLAN_ID;
	//pQueryOid->Oid = OID_802_11_SSID;

	if (DeviceIoControl(v_FileHandle,
							IOCTL_NDISPROT_QUERY_OID_VALUE,
							(LPVOID) &QueryBuffer[0],
							sizeof(QueryBuffer),
							(LPVOID) &QueryBuffer[0],
							sizeof(QueryBuffer),
							&byteReturned,
							NULL))
		{
			//printf(("Vlan Query Success\n"));
			//printf("The sizeo of ULONG is %d\n",sizeof(ULONG));
			memcpy(&vvid,pQueryOid->Data,sizeof(ULONG));
			//printf("The VLAN ID is %d \n", vlan_id);

		}	
	else{
		printf("DeviceIoControl Failed:%d\n",GetLastError());
		if(v_FileHandle)
			CloseHandle(v_FileHandle);
		exit(1);
	}

	if(vvid == vlan_id){
		printf("The same 802.1Q tag is already set on the selected network device\n");
		dont_set_ip = 1;
		if(v_FileHandle)
			CloseHandle(v_FileHandle);
		return;
	}
	
	UCHAR SetBuffer[1024];
	NDISPROT_SET_OID pSetOid;
	ULONG set_vlan = vlan_id;
	memset(SetBuffer,'\0',sizeof(SetBuffer));

	//pSetOid = (PNDISPROT_SET_OID) &SetBuffer[0];
	pSetOid.Oid = OID_GEN_VLAN_ID;

	memcpy(&pSetOid.Data[0],&set_vlan,sizeof(ULONG));

	if(DeviceIoControl(v_FileHandle,IOCTL_NDISPROT_SET_OID_VALUE,(LPVOID)&pSetOid, sizeof(pSetOid),
		(LPVOID) &pSetOid, sizeof(pSetOid), &byteReturned, NULL)){

			printf("802.1Q tag successfully set on the selected network device \n");
	}
	else{
		printf("DeviceIoControl Failed:%d \n", GetLastError());
		
		if(v_FileHandle)
			CloseHandle(v_FileHandle);
		
		exit(1);
	}
	
	if(v_FileHandle){
		CloseHandle(v_FileHandle);
	}

}

int vcRenewInterfaceIP(WCHAR *wTCPNetworkDevice)
{
    ULONG ulOutBufLen = 0;
    DWORD dwRetVal = 0;
    PIP_INTERFACE_INFO pInfo;
    pInfo = (IP_INTERFACE_INFO *) MALLOC(sizeof(IP_INTERFACE_INFO));

    // Make an initial call to GetInterfaceInfo to get
    // the necessary size into the ulOutBufLen variable
    if (GetInterfaceInfo(pInfo, &ulOutBufLen) == ERROR_INSUFFICIENT_BUFFER) {
        FREE(pInfo);
        pInfo = (IP_INTERFACE_INFO *) MALLOC (ulOutBufLen);
    }

    // Make a second call to GetInterfaceInfo to get the
    // actual data we want
	int i = 0, adapterFound = 0;
    if ((dwRetVal = GetInterfaceInfo(pInfo, &ulOutBufLen)) == NO_ERROR ) {
        //printf("\tAdapter Name: %ws\n", pInfo->Adapter[0].Name);
        //printf("\tAdapter Index: %d\n", pInfo->Adapter[0].Index);
        //printf("\tNum Adapters: %d\n", pInfo->NumAdapters);

		for(i = 0; i < pInfo->NumAdapters; i++){
			if(!wcsicmp(wTCPNetworkDevice,pInfo->Adapter[i].Name)){
				adapterFound = 1;
				break;
			}	
		}
		if(adapterFound == 0)
			return -1;

		//printf("Adapter Name: %ws\n", pInfo->Adapter[i].Name);
		//printf("\tAdapter Name2: %ws\n", pInfo->Adapter[2].Name);
		//printf("\tAdapter Name3: %ws\n", pInfo->Adapter[3].Name);
		
    }
    else if (dwRetVal == ERROR_NO_DATA) {
        printf("There are no network adapters with IPv4 enabled on the local system\n");
        return -1;
    }
    else {
        LPVOID lpMsgBuf;
        printf("GetInterfaceInfo failed.\n");
			
        if (FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwRetVal,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL )) {
            printf("\tError: %s", lpMsgBuf);
        }
        LocalFree( lpMsgBuf );
        return -1;
    }

// Call IpReleaseAddress and IpRenewAddress to release and renew
// the IP address on the first network adapter returned 
// by the call to GetInterfaceInfo.
	
	if ((dwRetVal = IpReleaseAddress(&pInfo->Adapter[i])) == NO_ERROR) {
        //printf("IP release succeeded.\n");
    }
    else {
        printf("Setting the IP address(release) failed: %ld\n", dwRetVal);
		return -1;
    }
	

    if ((dwRetVal = IpRenewAddress(&pInfo->Adapter[i])) == NO_ERROR) {
        //printf("IP renew succeeded.\n");
    }
    else {
        printf("Setting the IP address(renew) failed: %ld\n", dwRetVal);
		return -1;
    }

    // Free memory for IP_INTERFACE_INFO 
    if (pInfo != NULL) {
        //FREE(AdapterAddresses);
    }
    return 0;
}

int getOption150AndRouterIP(CHAR *networkDevice,CHAR *Dhcp150Option, CHAR *RouterIP){


	DWORD bSize, dhcpError;
	CHAR Buffer[96] = {'\0'};
	DWORD dhcpInitRet;
	
	int length = strlen(networkDevice);
	WCHAR waName[length + 1];

	MultiByteToWideChar(CP_ACP,0,networkDevice,length+1,waName,sizeof(waName)/sizeof(waName[0]));
	LPWSTR adapterName = (LPWSTR) &waName;
    
	if(!dont_parse_option150){

		DHCPCAPI_PARAMS DhcpOption150Params = {

			0, // Reserved, Set to 0
			150, // DHCP Cisco Option ID
			FALSE, // It is a vendor specific option ID
			NULL,
			0 
		};

		DHCPCAPI_PARAMS_ARRAY RequestParams = {
 
			1,
			&DhcpOption150Params
		};

		DHCPCAPI_PARAMS_ARRAY SendParams = {

			0,
			NULL
		};

		bSize = sizeof(Buffer);

		dhcpError = DhcpRequestParams(
				
					DHCPCAPI_REQUEST_SYNCHRONOUS,
					NULL,
					adapterName,
					NULL,
					SendParams,
					RequestParams,
					(PBYTE)Buffer,
					&bSize,
					NULL );


		if(dhcpError != ERROR_SUCCESS ){

			printf(" Error at DhcpRequestParams: %d \n", GetLastError() );
			return -1;
		}

		if( dhcpError == ERROR_MORE_DATA ){

			printf("Error: Buffer Size not enough \n");
			return -1;
		}

		if(dhcpError == ERROR_INVALID_PARAMETER){
			printf("Adapter name is invalid \n");
			return -1;
		}	

		if(dhcpError != NO_ERROR){
			return -1;
		}

		if(DhcpOption150Params.nBytesData){

			//memcpy(Option150Value,DhcpOption150Params.Data,DhcpOption150Params.nBytesData);
			//printf("Option150Value is %d.%d.%d.%d \n", Option150Value[0] + 256, Option150Value[1] + 256, Option150Value[2], Option150Value[3] + 256);
			//printf("Value is %d.%d.%d.%d \n", DhcpOption150Params.Data[0],DhcpOption150Params.Data[1],DhcpOption150Params.Data[2],DhcpOption150Params.Data[3]);
			//printf("Length is %d \n", DhcpOption150Params.nBytesData);
			sprintf(Dhcp150Option,"%d.%d.%d.%d",DhcpOption150Params.Data[0],DhcpOption150Params.Data[1],DhcpOption150Params.Data[2],DhcpOption150Params.Data[3]);
		}
		else{
			/* Windows DHCP API sucks, it is too slow and sometimes it returns option 150 is not received */
			printf("DHCP Option150 not found in DHCP Server. If you are using windows :-(, please try again.\n");
			return -1;
		}
	}


	
	DHCPCAPI_PARAMS DhcpRouterAddress = {

		0,
		OPTION_ROUTER_ADDRESS,
		FALSE,
		NULL,
		0
	};

	;

	DHCPCAPI_PARAMS_ARRAY RouterRequestParams = {

		1,
		&DhcpRouterAddress

	};

	
	DHCPCAPI_PARAMS_ARRAY RouterSendParams = {

			0,
			NULL
	};

	memset(Buffer,'\0',sizeof(Buffer));
	bSize = sizeof(Buffer);

	dhcpError = DhcpRequestParams(
				
					DHCPCAPI_REQUEST_SYNCHRONOUS,
					NULL,
					adapterName,
					NULL,
					RouterSendParams,
					RouterRequestParams,
					(PBYTE)Buffer,
					&bSize,
					NULL );

	if(dhcpError != ERROR_SUCCESS ){

		printf(" Error at DhcpRequestParams: %d \n", GetLastError() );
		return -1;
	}

	if(dhcpError != NO_ERROR){
		return -1;
	}

	if(DhcpRouterAddress.nBytesData){

		//printf("Value is %d.%d.%d.%d \n", DhcpRouterAddress.Data[0],DhcpRouterAddress.Data[1],DhcpRouterAddress.Data[2],DhcpRouterAddress.Data[3]);
		//printf("Length is %d \n", DhcpRouterAddress.nBytesData);
		sprintf(RouterIP,"%d.%d.%d.%d",DhcpRouterAddress.Data[0],DhcpRouterAddress.Data[1],DhcpRouterAddress.Data[2],DhcpRouterAddress.Data[3]);
		//printf("Router Ip is %s\n",RouterIP);
	}
	else{
		printf("Default Gateway IP not found in DHCP Server.\n");
		return -1;
	}

	return 0;

}

int addRoute(WCHAR *wTCPNetworkDevice, CHAR *Host, CHAR *RouterIP){

	MIB_IPFORWARDROW route;

	//LPWSTR adapterName = L"\\Device\\TCPIP_{4979EAF3-119B-4B26-94E4-4B0F52F6FD1C}";
	LPWSTR adapterName = (LPWSTR) wTCPNetworkDevice;
	ULONG pindex;
	
	DWORD err = GetAdapterIndex(adapterName,&pindex);
	DWORD res;
	
	if(err != NO_ERROR){
		printf("Get Adapter Index Failed, %d \n", GetLastError());
		return -1;
	}
	
	//printf("Index is %ld \n", pindex);

	memset(&route, 0, sizeof(route));
	route.dwForwardDest = inet_addr(Host);
	route.dwForwardMask = -1;
	route.dwForwardNextHop = inet_addr(RouterIP);
	route.dwForwardIfIndex = pindex;
	route.dwForwardProto = MIB_IPPROTO_NETMGMT ;
	route.dwForwardMetric1 = 1;

	res = CreateIpForwardEntry(&route);
	switch (res) {
		case NO_ERROR:
			break;
		case ERROR_INVALID_PARAMETER:
			printf("Error: CreateIpForwardEntry, %d\n", GetLastError());
			return -1;
	}

	return 0;
}

#endif
