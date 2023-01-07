/*===========================================================================================*/
/* TPS, stands for "Tiny Proxy Server" - it's a program which maps some ports of             */
/* one networked machine to another. For example: if the TPS is running on a machine         */
/* "A", so after connection to this machine to port "N", the TPS redirects this              */
/* connection to machine "B" and port "N". Machine "B" thinks that it got the connection     */
/* from machine "A" not from that machine ("X") which was actually connected to machine "A". */
/*                                                                                           */
/*                                                                                           */
/*                                                                                           */
/*             +-------------+    +--------------------+    +-------------+                  */
/*             | machine "X" |--->| TPS on machine "A" |--->| machine "B" |                  */
/*             +-------------+    +--------------------+    +-------------+                  */
/*                    |                                             /\                       */
/*                    | Concealed connection to machine "B" via "A" |                        */
/*                    +---------------------------------------------+                        */
/*                                                                                           */
/* Copyright (C) Edward Gess 2001                                                            */
/*                                                                                           */
/* e-mail: edward_gess@hotmail.com                                                           */
/*                                                                                           */
/*===========================================================================================*/

/*=WIN32 includes=*/
#include <windows.h>
#include "rconfig.h"

/*======================*/
/* Win32 main function. */
/*======================*/
int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	init_rconfig();

	return 0x0;
}