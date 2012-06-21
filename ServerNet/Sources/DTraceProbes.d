/*

Generer le header :

> dtrace -C -h -s DTraceProbes.d -o DTraceProbes.h

Lister les sondes dispo après lancement de Wakanda :

> dtrace -l -P 'Wakanda*'

*/

/*Typedef pour Mac64*/

#define sLONG		signed int
#define uLONG		unsigned int
#define PortNumber	signed int


provider Wakanda
{
	
	probe read__connection(sLONG Socket, const char* ServerAddr, sLONG ServerPort, const char* ClientAddr, sLONG CientPort);
	
	probe read__start(sLONG Socket, uLONG MaxLen);
	
	probe read__done(sLONG Socket, uLONG ReadLen);
	
	probe read__dump(sLONG Socket, sLONG Offset, const char* Payload);
	
	probe write__connection(sLONG Socket, const char* ServerAddr, sLONG ServerPort, const char* ClientAddr, sLONG CientPort);
	
	probe write__start(sLONG Socket, uLONG MaxLen);
	
	probe write__done(sLONG Socket, uLONG ReadLen);
	
	probe write__dump(sLONG Socket, sLONG Offset, const char* Payload);
};