/*
addby:yanghx
*/
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/in.h>
#include <signal.h>
extern void process_conn_server(int s);
extern void sig_proccess(int signo);
extern int net_routine();
#define PORT 10881		/*port name*/
#define BACKLOG 2		/* len*/
int net_routine()
{
	int ss,sc;
	const char localIP[]="192.168.127.151";
	struct sockaddr_in server_addr;  /*server_addr_struct*/
	struct sockaddr_in client_addr;	/* client_addr_struct */
	int err;
	pid_t pid;	/* id */

	/* set a stream scocket */
	ss = socket(AF_INET, SOCK_STREAM, 0);
	if(ss < 0){/*err*/
		printf("socket error\n");
		return -1;
	}

	/* set server_addr */
	bzero(&server_addr, sizeof(server_addr));	/* clean */
	server_addr.sin_family = AF_INET;			/* protool */
	server_addr.sin_addr.s_addr = inet_addr(localIP);/* local_addr-alladdr*/
	server_addr.sin_port = htons(PORT);			/* server_port */

	/* bind   addr_struct to socket */
	err = bind(ss, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(err < 0){/* err */
		printf("bind error\n");
		return -1;
	}
	/* listen*/
	err = listen(ss, BACKLOG);
	if(err < 0){/* err */
		printf("listen error\n");
		return -1;
	}

	for(;;)	{
		int addrlen = sizeof(struct sockaddr);

		/*accept the client*/
		sc = accept(ss, (struct sockaddr*)&client_addr, &addrlen);

		if(sc < 0){		/* err */
			continue;   //no link ,return accept again  else   build new  process_conn_server
		}
		//multiple processes can be receive
		//connect one,produce a process(except the first)
		//
		/* new process */
		pid = fork();		/* */
		if( pid == 0 ){
			close(ss);
			//printf("the sc is %d /n",sc);
			process_conn_server(sc);/* child_process */
		}else{
			close(sc);		/* close_father_process*/
		}
	}

}
