#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<time.h>
#include<fcntl.h>
#include<unistd.h>

const char* ip = "127.0.0.1";
FILE *fp = NULL;

int main(int argc, char *argv[]){
	if(argc != 4){
		printf("参数不符\n");
		exit(1);
	}
	int client_socket;
	//创造套接字
	client_socket=socket(AF_INET,SOCK_STREAM,0);
	if(client_socket<0){
		perror("socket创建失败");
		exit(1);
	}
	
	//填写服务器端地址信息
	int port = atoi(argv[1]);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(argv[2]);
	socklen_t addr_len = sizeof(addr);

	//向服务器发送连接请求
	int connect_fd = connect(client_socket,(struct sockaddr*)&addr,addr_len);
	if(connect_fd<0){
		perror("connect出错");
		exit(1);
	}
	
	//创建并打开聊天历史记录文件
	char fpname[100];
	strcpy(fpname,"./history/");
	strcat(fpname,argv[3]);
	strcat(fpname,".txt");
	if((fp=fopen(fpname,"a+"))==NULL)
		perror("打开文件错误");

	send(client_socket,argv[3],strlen(argv[3]),0);//发送用户名
	
	//提示信息
	printf("您已进入聊天室\n");
	printf("发送文本即可参与聊天\n");
	printf("发送大写QUIT即可退出聊天\n");
	
	
	char msg[1024];
	pid_t pid = fork();
	if(pid<0){
		perror("fork");
		exit(1);
	}
	//创建子进程用于向服务器发送消息
	while(pid==0){
		memset(msg,'\0',sizeof(msg));
		fflush(stdout);
		fgets(msg,1024,stdin);
		msg[strlen(msg)-1] = '\0';
		//发送消息
		send(client_socket,msg,strlen(msg),0);
		
		//如果发送的消息为结束符，则结束对聊天纪录文件的读写并且结束该进程
		if(strcmp(msg,"QUIT")==0){
			fclose(fp);
			exit(1);
		}
	}
	
	//父进程用于接收服务器的消息
	pid_t wpid;
	int status,i;
	int recvbytes;
	char recvstr[1024];
	time_t timep;	
	while(pid>0){
		recvbytes = recv(client_socket,recvstr,1024,0);
		recvstr[recvbytes]='\0';
		//如果接受到终止符号，客户端退出
		if(strcmp(recvstr,"QUIT")==0)
			exit(1);
		time(&timep);
		
		//如果从服务器端接受的消息返回值为零，表示服务器已经终止服务，客户端退出
		if(recvbytes==0){
			printf("与服务器断开\n");
			fclose(fp);			
			exit(1);
		}
		
		//在屏幕上显示消息以及消息发达时间
		printf("%s %s\n",ctime(&timep),recvstr);		
		if(recvbytes==-1){
			perror("receive");
			continue;
		}
		
		//把消息写入历史记录文件
		fprintf(fp,"%s%s\n",ctime(&timep),recvstr);
		
	}
	close(client_socket);
	return 0;

}

