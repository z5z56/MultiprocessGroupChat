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
#include <vector>
#include <errno.h>
using namespace std;
/*
	今天作业：
	    上课代码，同时为作业一
		 1.实现一个tcp服务器和N个tcp客户端，每个客户端发送字符串数据给服务器，服务器接收并显示。
		  
		  
		  本工程：（只完成群聊）
		 2.这是个聊天程序雏形。如何实现群聊，如何实现私聊呢？
		   
                注意点：1.群发，父进程要保存所有的accept_fd，可以存到向量中
            
			 聊天就不能直接发字符串了。而是要发结构体
			 
			 struct  chat
			 {
			     int chat_type; //群聊 还是私聊
				 int user_id;
				 int private_id;//想和谁私聊。
				 char user_name[30];
				 char chat_info[300];
			 }
			 //终端字符界面聊天

*/

//聊天信息包,用于子进程写入共享内存
typedef struct  chat
{
	int chat_type; //群聊0 还是私聊1
	//int private_id;//想和谁私聊。	
	int user_id;
	char user_name[30];
	char chat_msg[300];
}CHAT_PACKET;
/*
typedef struct  chat
{
	int chat_type; //群聊0 还是私聊1
	int user_id;
	int private_id;//想和谁私聊。
	char user_name[30];
	char chat_msg[300];
}CHAT;
*/

vector<int> accfdVector;//全局，存储已连接套接字描述符
void sigaction_init();//信号初始化绑定
void shm_init();//共享内存初始化
void* shm_addr=NULL;//全局，共享内存地址


void msg_handle(int num, siginfo_t *info, void *d)
{
	cout << "msg_handle" << info->si_int << endl;
	
	int r_size = 0;
	//shm_init();//接收端共享内存初始化,注意，只有在不同工程才需要两次，同一个工程内初始化一次就好
	CHAT_PACKET recv_client;
	memcpy(&recv_client, shm_addr, sizeof(CHAT_PACKET));//从共享内存读出消息
	
	cout << "shm recv: " << recv_client.chat_msg << endl;
	vector<int>::iterator it;//迭代器
	for (it = accfdVector.begin(); it != accfdVector.end(); it++)//迭代发送给所有客户端
	{
		if (*it != info->si_int)//不给自己发送自己发的消息
		{
			r_size = write(*it, &recv_client, sizeof(CHAT_PACKET));//注意，accept_fd才是向客户写
			if (r_size < 0)
			{
				cout << "server write err" << *it << endl;
			}
		}

	}
	//shmdt(shm_addr);//脱钩函数,一直在读不能脱钩！
	
	
}


int main(int argc, char *argv[])
{
	pid_t father_pid = getpid();
	//shm_addr = (CHAT_PACKET*)malloc(5000);
	//==========================================socket
	int server_socket_fd;
	server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket_fd < 0)
	{
		perror("create socket err:");
		return -1;
	}
	
	//使得不必等待TIME_WAIT状态就可以重启服务器
	int mw_optval = 1;
	setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&mw_optval, sizeof(mw_optval));

	//==========================================bind
	//socket和socketaddr_in是兼容的，socketaddr_in程序员友好，包含头文件#include <netinet/in.h>
	struct sockaddr_in ser_addr;

	int addr_len = sizeof(ser_addr);
	ser_addr.sin_family = AF_INET;//地址的协议家族是ipv4协议族
	
	//使用字节转换函数htons主机字节顺序转换为网络字节顺序
	ser_addr.sin_port = htons(8888);//注意，linux操作系统预留1-1024的端口号，所以我们不要使用这一部分
	ser_addr.sin_addr.s_addr =  htonl(INADDR_ANY);//INADDR_ANY自动绑定ip地址（192.168.xxx）
	if (bind(server_socket_fd, (struct sockaddr*)&ser_addr, addr_len) < 0)//将sockaddr_in类型转换为sockaddr类型
	{
		perror("bind err:");
		return -1;
	}
	//=================================listen
	if (listen(server_socket_fd, 10) < 0)//参数2表示同一时间能否支持几个客户端同时连接
	{
		perror("listen err:");
		return -1;
	}
	
	
	
	//================================accept
	int accept_fd;
	
	//安装信号先于发送信号
	sigaction_init();


	shm_init();//发送端共享内存初始化
	
	while (1)
	{
		//当有一个客户端连接到服务器的时候，accept会返回一个新的套接字描述符
		accept_fd = accept(server_socket_fd, NULL, NULL);
		 if (accept_fd == -1&&errno == EINTR)
		{
			continue;
		}	
		//cout << "new accept_fd is" << accept_fd<<endl;
		accfdVector.push_back(accept_fd);
	//==============================read
		int r_size = 0;
		cout << "server start to read..." << endl;
		pid_t pid;
		pid = fork();
		if (pid == 0)
		{
			while (1)
			{
				CHAT_PACKET client1;
				//从客户端读
				cout << "child ready to read" << endl;
				r_size = read(accept_fd, &client1, sizeof(client1));//阻塞这里等待从客户机接收到数据,read字符串会增加\n
				if (r_size < 0)
				{
					cout << "read err" << endl;
				}
						
				//子进程把信息包写入共享内存
				memset(shm_addr, 0, sizeof(shm_addr));//把共享内存清零
				memcpy(shm_addr, &client1, sizeof(CHAT_PACKET));//地址，内容，大小
				//然后发送信息accept_fd通知父进程读并群发

				union sigval value;//新建携带数据联合体value
				value.sival_int = accept_fd;
				cout << "send signal" << endl;
				sigqueue(father_pid, SIGUSR1, value);//携带value值发送信号SIGUSR1给进程号为pid的进程
				
				memset(&client1, 0, sizeof(client1));
				
				//exit(0);//just for test
			}
		}
		//close(accept_fd);//会关闭和客户端的连接

	}
	close(server_socket_fd);


	
	return 0;
}

void sigaction_init()
{
	//安装信号先于发送信号
	struct sigaction sig_act;//新建信号安装结构体
	sig_act.sa_sigaction = msg_handle;//指定信号关联函数
	sig_act.sa_flags = SA_SIGINFO;//声明信号是携带数据的
	//将信号和安装信号结构体关联
	sigaction(SIGUSR1, &sig_act, NULL);//信号num，新信号结构体指针，旧信号结构体指针
}

void shm_init()//共享内存初始化，创建共享内存，把共享内存连接到进程,并返回shm_addr
{
	//创建共享内存
	int shm_id;
	shm_id = shmget((key_t)6677, 4096, IPC_CREAT | 0666);//666可读可写
	if (shm_id == -1)
	{
		perror("shm create error");
		return ;
	}

	//把共享内存连接到进程
	shm_addr = shmat(shm_id, NULL, 0);
	//要对内存清空一下
	memset(shm_addr, 0, 4096);

}