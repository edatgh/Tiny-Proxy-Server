
#include <windows.h>
#include "pmapping.h"

/*=Globals=*/
PPORT_MAPPER port_mappers=NULL; // array of port mappers
int nr_portmappers=0x0;
int kill_flag=-1; // this flag tells which thread must halt

/*=Local function prototypes=*/
DWORD WINAPI port_mapping_th(void *);
void add_portmapper(char *,int,int,HANDLE);
void del_portmapper(HANDLE);
int get_index(HANDLE);

int map_port(int src_pnum,int dst_pnum,char *ip)
{
	DWORD th_id;
	PPM_PARAM ppm_param;
	int i;

	/*=Is there any mapped port with the same number?=*/
	for (i=0x0;i<nr_portmappers;i++)
		if (port_mappers[i].pm_param.src==src_pnum)
			return 0x0; // error
	ppm_param=(PPM_PARAM)malloc(sizeof(PM_PARAM)); // allocate a memory for the thread parameter
	ppm_param->src=src_pnum; // set source port number
	ppm_param->dst=dst_pnum; // set destination port number
	lstrcpy(ppm_param->p_ip,ip); // set destination IP address
	CreateThread(NULL,0x0,port_mapping_th,ppm_param,0x0,&th_id); // create port mapping thread

	return 0x1; // success
}

int unmap_port(int port_n)
{
	int i;

	for (i=0x0;i<nr_portmappers;i++)
		if (port_mappers[i].pm_param.src==port_n) {
			kill_flag=i; // set it to the index of thread which has to halt
			break; // skip other items
		}
	if (i==nr_portmappers)
		return 0x0; // error
	else
		return 0x1; // success
}

void list_mapped(int con_s)
{
	int i;
	char msg[0x100];

	for (i=0x0;i<nr_portmappers;i++)
		if (port_mappers[i].h_portmapper!=NULL) {
			wsprintf(msg,
				"LOCAL(%d) ---> %s(%d)\r\n",
				port_mappers[i].pm_param.src,
				port_mappers[i].pm_param.p_ip,
				port_mappers[i].pm_param.dst);
			send(con_s,msg,lstrlen(msg),0x0);
		}
	wsprintf(msg,"--- Total items: [%lu] ---\r\n",nr_portmappers);
	send(con_s,msg,lstrlen(msg),0x0);
}

DWORD WINAPI port_mapping_th(void *param)
{
	int src_pnum;
	int dst_pnum;
	char *ip; // IP address of machine to which connection will be redirected
	WSADATA WSAData;
	int err; // error status
	SOCKET ms; // main socket handle
	SOCKET in_s; // incoming connection socket handle
	SOCKET r_s; // redirection socket
	sockaddr_in laddr; // local address information
	sockaddr_in raddr; // remote address information
	sockaddr_in re_addr;
	int addr_len;
	PBYTE io_buf; // socket I/O buffer
	fd_set read_fds;
	timeval tm;
	int br; // number of received bytes
	int bs; // number of sent bytes
	DWORD nonblk;

	/*=Initialize WSA=*/
	err=WSAStartup(MAKEWORD(0x1,0x1),&WSAData); // start up WSA
	if (err)
		return 0x0; // error starting up WSA
	/*=Create a socket=*/
	ms=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (ms==INVALID_SOCKET)
		return 0x0; // error creating socket
	/*=Get parameters=*/
	src_pnum=((PPM_PARAM)param)->src;
	dst_pnum=((PPM_PARAM)param)->dst;
	ip=(char *)malloc(sizeof(char)*(lstrlen(((PPM_PARAM)param)->p_ip)+0x1)); // for NULL
	lstrcpy(ip,((PPM_PARAM)param)->p_ip);
	/*=Free memory used by "param"=*/
	free(param);
	/*=Fill information for socket binding=*/
	memset(&laddr,0x0,sizeof(sockaddr_in));
	laddr.sin_port=htons(src_pnum);
	laddr.sin_family=AF_INET;
	laddr.sin_addr.s_addr=INADDR_ANY;
	/*=Bind the socket=*/
	err=bind(ms,(struct sockaddr *)&laddr,sizeof(struct sockaddr));
	if (err==SOCKET_ERROR) {
		free(ip);
		closesocket(ms);
		WSACleanup(); // clean up WSA
		return 0x0;
	}
	err=listen(ms,0x1);
	if (err==SOCKET_ERROR) {
		free(ip);
		closesocket(ms);
		WSACleanup();
		return 0x0;
	}
	/*=The port is mapped, so add new port mapper=*/
	add_portmapper(ip,src_pnum,dst_pnum,GetCurrentThread());
	/*=Conection handler=*/
	while (get_index(GetCurrentThread())!=kill_flag) {
		addr_len=sizeof(sockaddr_in);
		in_s=accept(ms,(struct sockaddr *)&raddr,&addr_len);
		if (in_s==INVALID_SOCKET)
			continue;
		/*=I've got a connection, so I have to connect to the destination server=*/
		memset(&re_addr,0x0,sizeof(sockaddr_in));
		re_addr.sin_addr.s_addr=inet_addr(ip);
		re_addr.sin_family=AF_INET;
		re_addr.sin_port=htons(dst_pnum);
		r_s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if (r_s==INVALID_SOCKET) {
			closesocket(in_s);
			continue;
		}
		/*=Connect to the destination address=*/
		err=connect(r_s,(struct sockaddr *)&re_addr,sizeof(struct sockaddr));
		if (err==SOCKET_ERROR) {
			closesocket(r_s);
			closesocket(in_s);
			continue;
		}
		/*=Allocate a memory for the "io_buf"=*/
		io_buf=(PBYTE)malloc(sizeof(BYTE)*0x200); // I'll operate 512 bytes buffers
		/*=Start I/O redirection=*/
		memset(&tm,0x0,sizeof(timeval));
		/*=Set non-blocking mode for both sockets=*/
		nonblk=0x1; // TRUE
		ioctlsocket(in_s,FIONBIO,&nonblk);
		nonblk=0x1; // TRUE
		ioctlsocket(r_s,FIONBIO,&nonblk);
		while (get_index(GetCurrentThread())!=kill_flag) {
			FD_ZERO(&read_fds); // remove all the read fd's
			FD_SET(in_s,&read_fds); // add "in_s" to "read_fd0"
			FD_SET(r_s,&read_fds); // and "r_s" to "read_fd1"
			err=select(0x0,&read_fds,NULL,NULL,&tm);
			if (err>0x0) {
				if (FD_ISSET(in_s,&read_fds))
					do {
						br=recv(in_s,(char *)io_buf,0x200,0x0);
						if (br>0x0)
							bs=send(r_s,(char *)io_buf,br,0x0);
					} while (br>0x0);
				if (FD_ISSET(r_s,&read_fds))
					do {
						br=recv(r_s,(char *)io_buf,0x200,0x0);
						if (br>0x0)
							bs=send(in_s,(char *)io_buf,br,0x0);
					} while (br>0x0);
			} else
				if (err==SOCKET_ERROR) {
					free(io_buf);
					closesocket(r_s);
					closesocket(in_s);
					break;
				}
		} // while(...) {...}
	} // while (...) {...}
	del_portmapper(GetCurrentThread());
	free(ip);
	closesocket(ms);
	WSACleanup();
	return 0x0;
}

void add_portmapper(char *ip,int src_pnum,int dst_pnum,HANDLE hpm)
{
	nr_portmappers++; // increment number of port mappers
	kill_flag=-1;
	port_mappers=(PPORT_MAPPER)realloc(port_mappers,sizeof(PORT_MAPPER)*nr_portmappers);
	port_mappers[nr_portmappers-0x1].h_portmapper=hpm;
	port_mappers[nr_portmappers-0x1].pm_param.src=src_pnum;
	port_mappers[nr_portmappers-0x1].pm_param.dst=dst_pnum;
	lstrcpy(port_mappers[nr_portmappers-0x1].pm_param.p_ip,ip);
}

void del_portmapper(HANDLE hpm)
{
	int i;

	/*=Search for corresponding port mapper=*/
	for (i=0x0;i<nr_portmappers;i++)
		/*=Remove it (if found)=*/
		if (port_mappers[i].h_portmapper==hpm) {
			port_mappers[i].h_portmapper=NULL;
			port_mappers[i].pm_param.src=0x0;
			port_mappers[i].pm_param.dst=0x0;
			memset(port_mappers[i].pm_param.p_ip,0x0,sizeof(port_mappers[i].pm_param.p_ip));
			break;
		}
}

int get_index(HANDLE hpm)
{
	int i;

	for (i=0x0;i<nr_portmappers;i++)
		if (port_mappers[i].h_portmapper==hpm)
			return i;
	return 0x0;
}