ace: tftp_client.o http_handler.o http_packet.o directory_parser.o xml_config_parser.o dhcpconfig.o arp.o buildmsg.o cache.o client.o peekfd.o signals.o udpipgen.o voiphop.o main.o 
	gcc -g -ggdb tftp_client.o http_handler.o http_packet.o directory_parser.o xml_config_parser.o dhcpconfig.o arp.o buildmsg.o cache.o client.o peekfd.o signals.o udpipgen.o voiphop.o main.o -o ace -lpthread -lpcap
tftp_client.o: tftp_client.c 
		gcc -c tftp_client.c
http_handler.o: http_handler.c
		gcc -c http_handler.c
http_packet.o: http_packet.c
		gcc -c http_packet.c
directory_parser.o: directory_parser.c
		gcc -c directory_parser.c
xml_config_parser.o: xml_config_parser.c
		gcc -c xml_config_parser.c
dhcpconfig.o: dhcpconfig.c
		gcc -c dhcpconfig.c
arp.o: arp.c
		gcc -c arp.c
builmsg.o: buildmsg.c
		gcc -c buildmsg.c
cache.o: cache.c
		gcc -c cache.c
client.o: client.c
		gcc -c client.c
peekfd.o: peekfd.c
		gcc -c peekfd.c
signals.o: signals.c
		gcc -c signals.c
udpipgen.o: udpipgen.c
		gcc -c udpipgen.c
voiphop.o: voiphop.c voiphop.h
		gcc -c voiphop.c
main.o: main.c
		gcc -c main.c
clean:
	rm -f *.o
	rm -f ace
