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

#ifndef OS_WINDOWS

int dhcpclientcall(char *);
void create_vlan_interface(char *,int);
unsigned int mk_spoof_cdp(char *,char *,char *,char *,char *,char *);

#endif

int cdp_mode(int,char*);
int get_cdp(u_char *, const struct pcap_pkthdr *, const u_char *);
int vlan_hop(int,char*);
int cdp_spoof_mode(char *);

