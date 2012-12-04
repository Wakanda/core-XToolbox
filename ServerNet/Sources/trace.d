#!/usr/sbin/dtrace -s

#pragma D option quiet

/* OK !
Wakanda*:ServerNet*::
{

}

Wakanda*:ServerNet*:_ZN4xbox9NetProbes9ReadStartEPKNS_13XBsdTCPSocketEj:read-start
{
	printf("Start read on sock %d, ask for %d bytes(s)\n", arg0, arg1);
}
*/

Wakanda*:::read-connection
{
	printf("Reading Connection  socket=%d, server=%s:%d, client=%s:%d\n",
			arg0, copyinstr(arg1), arg2, copyinstr(arg3), arg4);
			
	self->port=arg2;
}

Wakanda*:::read-start
{
	printf("Start read on sock %d, ask for %d bytes(s)\n", arg0, arg1);
}

Wakanda*:::read-done
{
	printf("Done read on sock %d, get %d bytes(s)\n", arg0, arg1);
}

Wakanda*:::read-dump
{
	printf("Dump read on sock %d, offset %d : %s\n", arg0, arg1, copyinstr(arg2));
}

Wakanda*:::write-connection
{
	printf("Writing Connection  socket=%d, server=%s:%d, client=%s:%d\n",
			arg0, copyinstr(arg1), arg2, copyinstr(arg3), arg4);
			
	self->port=arg2;
}

Wakanda*:::write-start
{
	printf("Start write on sock %d, ask for %d bytes(s)\n", arg0, arg1);
}

Wakanda*:::write-done
{
	printf("Done write on sock %d, get %d bytes(s)\n", arg0, arg1);
}

Wakanda*:::write-dump
{
	printf("Dump write on sock %d, offset %d : %s\n", arg0, arg1, copyinstr(arg2));
}


END
{
	printf("FINI !\n");
}