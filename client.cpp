#include <iostream>
#include<unistd.h>//unix stand lib
#include<sys/types.h>
#include<sys/fcntl.h>
#include<sys/stat.h>
#include<stdio.h>
#include<fcntl.h>
#include<string.h>
#include<dirent.h>//file dir
#include <sys/wait.h>//wait func
#include <stdlib.h>//ststem
#include <signal.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/socket.h>//socket
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

typedef struct  chat
{
	int chat_type; //群聊0 还是私聊1
	//int private_id;//想和谁私聊。	
	int user_id;
	char user_name[30];
	char chat_msg[300];
}CHAT_PACKET;


int main(int argc, char *argv[])
{
	//==========================================socket
	int socket_fd;
	struct sockaddr_in ser_addr;
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		perror("create socket err:");
		return -1;
	}
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(8888);
	ser_addr.sin_addr.s_addr = inet_addr("127.0.0.1");//该函数把ip转换为网络语言#include <arpa/inet.h>
	//或者127.0.0.1表示本机
	//======================================connect
	int addr_len = sizeof(ser_addr);
	if (connect(socket_fd, (struct sockaddr*)&ser_addr, addr_len) < 0)
	{
		perror("connect err:");
		return -1;
	}
	

	int r_size = 0;
	//不断获取从键盘的输入
//	while (fgets(buffer, sizeof(buffer), stdin) != NULL)//阻塞这里等待用户输入
//	{
	
	CHAT_PACKET client = { 0, 1001, "name", "0" };
	char info[50];
	


	cout << "welcome!please input your name:" << endl;
	//	scanf(info);
	if (fgets(info, sizeof(info), stdin) != NULL)//新来的客户端信息包赋值用户名
	{
		
	//这里的目的是fgets接收的字符串，如果字符串结束字符\0前面,如果有个'\n'那么就替换为'\0'
	//实测如果没有这个判断，输入名字会带换行符使得名字和消息不在同一行，不好看~
		if (info[strlen(info) - 1] == '\n')
		{
			info[strlen(info) - 1] = 0;	
		}

		memset(client.user_name, 0, sizeof(client.user_name));
		strcpy(client.user_name, info);
		memset(info, 0, sizeof(info));
	}//没有做输入错误处理
	cout << "now you can chat with your friends!" << endl;


	pid_t pid;
	while (1)
	{
		pid = fork();
		if (pid == 0)//这个子进程负责不断读取并显示
		{
			while (1)
			{
				//客户端从服务器接收,自己发送完了也要不断准备接收,在子进程里面不断read
				r_size = read(socket_fd, &client, sizeof(client));//阻塞这里等待读
				if (r_size > 0)
				{
					cout << client.user_name << " : " << client.chat_msg << endl;
				}
			}
		}
		else if (pid > 0)
		{
			//不断接收用户输入
			if (fgets(info, sizeof(info), stdin) != NULL)//用户输入消息info然后放入结构体
			{
				//这里的目的是fgets接收的字符串，如果字符串结束字符\0前面,如果有个'\n'那么就替换为'\0'
				//实测如果没有这个判断，输入字符串带换行符，不好看~
				if (info[strlen(info) - 1] == '\n')
				{
					info[strlen(info) - 1] = 0;	
				}
				memset(client.chat_msg, 0, sizeof(client.chat_msg));//把消息清零，再把新消息拷贝入结构体
				strcpy(client.chat_msg, info);
			}
				//客户端发送给服务器，字符串strlen结构体sizeof
			r_size = write(socket_fd, &client, sizeof(client));
			if (r_size < 0)
			{
				perror("send meg err");
			}
			//不要清name
			memset(client.chat_msg, 0, sizeof(client.chat_msg));//把消息清零，再把新消息拷贝入结构体
			
		}


	}


		

	
	return 0;
}