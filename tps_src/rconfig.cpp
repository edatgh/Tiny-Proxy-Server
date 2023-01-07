/*==============================================================*/
/* rconfig - Remote configuration interface via Telnet protocol */
/*                                                              */
/* Edward Gess                                                  */
/*==============================================================*/

/*=INCLUDES=*/
#include <windows.h>
#include "pmapping.h"
#include "rconfig.h"

/*=Local function prototypes=*/
int auth_client(SOCKET);
int start_shell(SOCKET);
int get_s_data(int,char *,int);
void execute_command(int,char *);

/*=Function implementation=*/
int init_rconfig(void)
{
	WSADATA WSAData;
	SOCKET ls; // listener socket handle
	SOCKET as; // accepted connection socket handle
	int err;
	sockaddr_in l_addr;
	sockaddr_in r_addr;
	int addr_len;

	/*=Before any operations with sockets, I have to start up the WSA=*/
	err=WSAStartup(MAKEWORD(0x1,0x1),&WSAData);
	if (err)
		return 0x0; // error starting up WSA	
	/*=Create a socket=*/
	ls=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (ls==INVALID_SOCKET) {
		WSACleanup();
		return 0x0; // error, unable to create a socket
	}
	/*=Fill local address information=*/
	memset(&l_addr,0x0,sizeof(sockaddr_in));
	l_addr.sin_addr.s_addr=INADDR_ANY;
	l_addr.sin_family=AF_INET;
	l_addr.sin_port=htons(RCONFIG_PORT_NUMBER);
	/*=Bind a socket to specified port=*/
	err=bind(ls,(struct sockaddr *)&l_addr,sizeof(struct sockaddr));
	if (err==SOCKET_ERROR) {
		closesocket(ls);
		WSACleanup();
		return 0x0; // error
	}
	/*=Listen a socket=*/
	err=listen(ls,0x5);
	if (err==SOCKET_ERROR) {
		closesocket(ls);
		WSACleanup();
		return 0x0; // error
	}
	for (;;) {
		addr_len=sizeof(struct sockaddr_in);
		as=accept(ls,(struct sockaddr *)&r_addr,&addr_len);
		if (as==INVALID_SOCKET)
			continue; // next cycle round
		/*=I've got connection=*/
		if (auth_client(as))
			start_shell(as);
		else
			closesocket(as);
	}
}

int auth_client(SOCKET con_s)
{
	return 0x1; // currently there is no authorization
}

int start_shell(SOCKET con_s)
{
	char command_buf[0x100];
	int res;
	int err;

	memset(command_buf,0x0,sizeof(command_buf));
	do {
		err=send(con_s,"\r\nEnter command: ",0x11,0x0);
		if (err==SOCKET_ERROR)
			return 0x0;
		res=get_s_data(con_s,command_buf,sizeof(command_buf));
		if ((res)&&(res!=-1) && (res!=-2)) {
			execute_command(con_s,command_buf);
			memset(command_buf,0x0,sizeof(command_buf));
		}
		if (res==-2)
			if (send(con_s,"\r\nCommand too long!",0x13,0x0)==SOCKET_ERROR)
				return 0x0;
	} while (res!=-1);
	closesocket(con_s);
	return 0x0;
}

/*========================== Code fetched from Network Toolkit ==============================*/
/*
 *	get_s_data() - receives chars from the given socket and stores them in to the
 *			   "buf". This function receives only (<= "m_length") number of chars.
 *			   If number of passed chars is bigger than "m_length" it will
 *			   returns zero. This feature is done here to handle buffer over-
 *			   flows and to be compatible with Telnet protocol.
 */
int get_s_data(int con_s, char *buf, int m_length)
{
	char *p_com, com;
	int res, total_rec;

	p_com = buf;
	total_rec = 0; /* NOTE: total_rec must be zero on this function call! */
	do {
		if ((res = recv(con_s, &com, 1, 0)) == SOCKET_ERROR)
			return -1;
		if (com == '\r') { /* if Enter key was presses, so first I'll get 0x0D then 0x0A */
			*p_com = '\0'; /* make a valid end of the received string, */
				       /* or do nothing if no chars was received, */
				       /* except 0x0D... */
			return (total_rec); /* return number of chars received (without last zero!), */
					    /* or zero if received only 0x0D (I don't need 0x0D) */
		}
		if ((res) && (res != -1) && (com != ('\n'))) {
		/* if client is alive, no errors and com != 0x0A, continue... */
			if (total_rec < m_length) { /* will received data holds in buf ? */
				*p_com = com; /* yes... */
				p_com++;
				total_rec++;
			} else { return -2; /* No! - protect myself from buffer-overflow */ }
		} /* do nothing else */
	} while ((res) && (res != -1));
	return (res);
}

int get_com_param(
				  int con_s, /* connected socket */
				  char *question, /* question for client */
				  char *response, /* where to store the response */
				  int resp_length, /* maximum length of client's response */
				  BOOL r_mandatory /* is this response mandatory ? */
				  ) 
{
	int res;

	if (send(con_s, question, lstrlen(question), 0) == SOCKET_ERROR)
		return (-1);
	res = get_s_data(con_s, response, resp_length);
	if ((res == -1) || ((r_mandatory) ? (!res) : (res == -1)))
		return (-1);
	return (0x0);
}
/*============================ End of code from Network Toolkit =============================*/

void execute_command(int con_s,char *c_buf)
{
	/*=Define some usable macroses=*/
#define SEND_RESPONSE() send(con_s,response,lstrlen(response),0x0)
#define CHECK_ERROR_RETURN() if (res==-1) return
	/*=Local variables=*/
	char response[0x100]; // 256 byte buffer
	char p0[0x100]; // parameter 0
	char p1[0x100]; // parameter 1
	char p2[0x100]; // ...
	int res;

	/*=Clean up all the parameters=*/
	memset(p0,0x0,sizeof(p0));
	memset(p1,0x0,sizeof(p1));
	memset(p2,0x0,sizeof(p2));
	/*=MAPPORT=*/
	if (lstrcmpi(c_buf,"mapport")==0x0) {
		int port1;
		int port2;

		res=get_com_param(
			con_s,
			"Enter port number: ",
			p0,
			0x6,
			TRUE);
		CHECK_ERROR_RETURN();
		res=get_com_param(
			con_s,
			"Enter IP address of remote machine: ",
			p1,
			0x20,
			TRUE);
		CHECK_ERROR_RETURN();
		res=get_com_param(
			con_s,
			"Enter port number on the remote machine: ",
			p2,
			0x6,
			TRUE);
		CHECK_ERROR_RETURN();
		port1=atol(p0);
		if (port1<0x0||port1>0xFFFF)
			return;
		port2=atol(p2);
		if (port2<0x0||port2>0xFFFF)
			return;
		if (map_port(port1,port2,p1))
			wsprintf(response,"Port mapped successfully.\r\n");
		else
			wsprintf(response,"Unable to map the port!\r\n");
		SEND_RESPONSE();
		return; // skip other functions
	}
	/*=UNMAPPORT=*/
	if (lstrcmpi(c_buf,"unmapport")==0x0) {
		int portn;

		res=get_com_param(
			con_s,
			"Enter port number: ",
			p0,
			0x6,
			TRUE);
		CHECK_ERROR_RETURN();
		portn=atol(p0);
		if (portn<0x0||portn>0xFFFF)
			return;
		if (unmap_port(portn))
			wsprintf(response,"Port unmapped successfully.\r\n");
		else
			wsprintf(response,"Unable to unmap the port!\r\n");
		SEND_RESPONSE();
		return; // skip other functions
	}
	/*=LISTMAPPED=*/
	if (lstrcmpi(c_buf,"listmapped")==0x0) {
		list_mapped(con_s);
		return; // skip other functions
	}
	/*=CLOSEPROG=*/
	if (lstrcmpi(c_buf,"closeprog")==0x0) {
		res=get_com_param(
			con_s,
			"Are you sure want to close the TPS? ['y', 'n']: ",
			p0,
			0x1,
			TRUE);
		CHECK_ERROR_RETURN();
		if (*p0=='y')
			ExitProcess(0x0);
		else
			if (*p0!='n') {
				wsprintf(response,"Incorrect input!\r\n");
				SEND_RESPONSE();
			}
		return; // skip other functions
	}
	/*=HELP=*/
	if (lstrcmpi(c_buf,"help")==0x0) {
		wsprintf(response,"\r\n--- TPS HELP ---\r\n");
		SEND_RESPONSE();
		wsprintf(response,"[mapport] - Map specified port\r\n");
		SEND_RESPONSE();
		wsprintf(response,"[unmapport] - Unmap specified port\r\n");
		SEND_RESPONSE();
		wsprintf(response,"[listmapped] - List currently mapped ports\r\n");
		SEND_RESPONSE();
		wsprintf(response,"[closeprog] - Close program\r\n\r\n");
		SEND_RESPONSE();
		return; // skip other functions
	}
	/*=BAD COMMAND HANDLER=*/
	wsprintf(response,"Bad command!\r\n");
	SEND_RESPONSE();
#undef CHECK_ERROR_RETURN()
#undef SEND_RESPONSE()
}