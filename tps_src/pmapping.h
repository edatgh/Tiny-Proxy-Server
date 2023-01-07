
#ifndef _TPS_SRC__PMAPPING_H_
#define _TPS_SRC__PMAPPING_H_

/*=Types=*/
typedef struct {
	char p_ip[0x20]; // IP address
	int src; // source port number
	int dst; // destination port number
} PM_PARAM, *PPM_PARAM; // port mapping thread param

typedef struct {
	PM_PARAM pm_param;
	HANDLE h_portmapper; // handle of thread which is mapping a port
} PORT_MAPPER, *PPORT_MAPPER;

/*=Function prototypes=*/
int map_port(int,int,char *);
int unmap_port(int);
void list_mapped(int);

#endif