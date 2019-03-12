#include "server.h"

int online_cnt;//在线人数
int user[BACKLOG];
int server_socket;
int msgqid;

void fun_ctrl_c(){
	msgctl(msgqid,IPC_RMID,0);
	close(server_socket);
	printf("服务器已关闭\n");
	kill(0,SIGKILL);
}

void fun_ctrl_z(int accept_fd){
	send(accept_fd,"QUIT",strlen("QUIT"),0);
}

int main(int argc,char *argv[]){
	online_cnt=0;
	if(argc!=2){
		printf("参数不符\n");
		exit(1);
	}

	//网络连接准备
	server_socket = socket(AF_INET,SOCK_STREAM,0);
	if(server_socket < 0){
		perror("socket创建失败");
		exit(1);
	}
	printf("socket创建成功\n");

	struct sockaddr_in my_addr;
	getAddr(argv[1],&my_addr);

	if(bind(server_socket,(struct sockaddr*)&my_addr,sizeof(my_addr))<0){
		perror("bind创建失败");
		exit(1);
	}
	printf("bind创建成功\n");

	if(listen(server_socket,BACKLOG)==-1){
		perror("listen出错");
		exit(1);
	}
	printf("listen创建成功\n");

	//创建消息队列
	msgqid = create_IPCqueue(".",'a');

	(void)signal(SIGINT,fun_ctrl_c);	
	
	while(1){
		pid_t check_user;
		check_user = fork();
		if(check_user<0){
			perror("fork出错");
			continue;
		}
		int msg_recv;
		
		//接受子进程消息并且转发
		//即从消息队列中读取消息，并将该消息按每个子进程的进程号作为mtype各自存进消息队列供相应子进程读取
		struct msgbuf msg_rqueue;//用于读取消息队列的消息结构体		
		while(check_user>0){
			
			read_queue(msgqid,&msg_rqueue,1,0);	
			
			int i;
			if(msg_rqueue.mclass==LOGIN){
				user[online_cnt++] = msg_rqueue.user_pid;	
			}

			
			for(i=0;i<online_cnt;i++){			
				msg_rqueue.mtype = user[i];
				write_queue(msgqid,&msg_rqueue,0);			
			}		
				
			//删除用户
			if(msg_rqueue.mclass==LOGOUT){
				for(i=0;i<online_cnt;i++){	
					if(user[i]==msg_rqueue.user_pid){	
							//printf("delete:%s %d\n",msg_rqueue.username,msg_rqueue.user_pid);						
							while(i<online_cnt-1){
							user[i] = user[i+1];
							i++;						
						}
						online_cnt--;
						break;					
					}				
				}			
			}	
		}

		if(check_user==0){
			//printf("a:%d\n",getpid());
			//监听是否有客户端登陆
			while(1){
				struct sockaddr_in remote_addr;
				socklen_t remote_len = 1;
				int accept_fd = accept(server_socket,(struct sockaddr*)&remote_addr,&remote_len);
				if(accept_fd<0){
					perror("accept出错");
					continue;
				}
				printf("accept成功\n");
				//创建子进程
				pid_t pid;
				pid = fork();
				
				if(pid<0){
					perror("fork出错");
					continue;
				}

				//子进程用于与客户端会话
				if(pid==0){
					pid_t send_msg = fork();
					if(send_msg<0){
						perror("fork失败");
						continue;
					}

					//int recvbytes;
					char recvstr[1024];
					struct msgbuf msg_in;
					while(send_msg>0){
						//接受客户端消息
						receive_from_client(msgqid,accept_fd,recvstr,1024,&msg_in);
					}

					struct msgbuf msg_out;
					while(send_msg==0){
						//向客户端发送信息

						send_to_client(msgqid,accept_fd,&msg_out);
					}
				}
			}
		}
	}
	return 0;
}










