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

/* Header files to be included */

#ifndef OS_WINDOWS
#undef OS_WINDOWS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>

#ifndef OS_WINDOWS

#include <netinet/in.h>
#include <rpc/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <errno.h>

#include <pcap.h>
#include <syslog.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/if_vlan.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <pthread.h>

#endif
