#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<assert.h>

#define   BUFSIZE    1024

int                  conn_fd;              // 套接字
char                 talk_to[10];          // 正在交互对象
char                 talk_menu[10];        // 正在主界面
int                  key = 0,r_key=0;

// 个人信息
struct info
{
	char             username[10];
};
 
// 群组
struct group
{
	char             gro_name[20];
    char             admin[10];
	struct info      member[10];
};
 
// 用户
struct users
{
	char             password[10];
	struct info      user;
	struct info      friend[10];    // 好友
	struct group     group[3];
	struct users*    next;
};

int                  f_num;
int                  g_num;
int                  m_num;
char                 password[10];
struct info          user;
struct info          friend[10];
struct info          member[10];
struct group         group[3];

// 聊天消息
struct chat
{
	int              mark;
	char             from[10];
	char             to[10];
	char             ask[10];
	char             time[30];
	char             news[900];
};

int                  mes_num = 0;   // 消息计数
struct chat          message[100];  // 消息队列
struct chat          sear_key;

void remenu();
// 错误处理函数
void my_err(const char *err_string,int line)
{
	fprintf(stderr,"line:%d ",line);
	perror(err_string);
	exit(1);
}

/* 时间 */
void get_time(char *p_time)
{
	time_t      now;
    struct tm   *timenow; 
	time(&now);
	timenow = localtime(&now); 
	strcpy(p_time,asctime(timenow));
}

/* 自定义输入函数 */
void mygets(char * str,int max)
{
	int       k=0;
	char      c;

	while((c=getchar())!='\n')
	{
		str[k++]=c;
		if(k==max-1)
		{
			while((c=getchar())!=EOF&&c!='\n');
			break;
		}
	}
	str[k]='\0';
}

/* 注册 */
int apply()
{
	int              i;
	char             plybuf[BUFSIZE];
	struct info      user_temp;
	struct log
	{
        char         flag;
		char         name[10];
		char         pwd[10];
	}ply;

	memset(&ply,0,sizeof(ply));
	while(1)
	{
		printf("输入用户名： ");
	    mygets(ply.name,10);
		if(strcmp(ply.name,"")==0)
		  printf("输入不能为空\n");
		else
		  break;
	}
	while(1)
	{
		printf("输入密码：   ");
		system("stty -echo");  
	    mygets(ply.pwd,10);
		system("stty echo");  
		if(strcmp(ply.pwd,"")==0)
		  printf("输入不能为空\n");
		else
		  break;
	}

	ply.flag = 'a';
	memset(plybuf,0,sizeof(plybuf));
	memcpy(plybuf,&ply,sizeof(ply));
	send(conn_fd,plybuf,1024,0);

	memset(plybuf,0,sizeof(plybuf));
	recv(conn_fd,plybuf,1024,0);
	if(strcmp(plybuf,"n")==0)
	{
		printf("\n注册失败，用户名已被使用，任意键继续。。。\n");
		getchar();
		return 0;
	}
	memcpy(&user_temp,plybuf,sizeof(plybuf));
	printf("UU: %s\n",user_temp.username);
	strcpy(password,ply.pwd);
	user = user_temp;

	return 1;	  
}

/* 登陆 */
int login()
{
	int              i;
	char             logbuf[BUFSIZE];
	struct info      user_temp;
	struct log
	{
		char         flag;
		char         name[10];
		char         pwd[10];
	}log;

	while(1)
	{
		printf("输入用户名： ");
	    mygets(log.name,10);
		if(strcmp(log.name,"")==0)
		  printf("输入不能为空\n");
		else
		  break;
	}
	while(1)
	{
		printf("输入密码：   ");
		system("stty -echo");  
	    mygets(log.pwd,10);
		system("stty echo");  
		if(strcmp(log.pwd,"")==0)
		  printf("输入不能为空\n");
		else
		  break;
	}
    log.flag = 'b';
	memcpy(logbuf,&log,sizeof(log));
	send(conn_fd,logbuf,1024,0);
    printf("send ok\n");

	memset(logbuf,0,sizeof(logbuf));
	recv(conn_fd,logbuf,1024,0);
	if(strcmp(logbuf,"n")==0)
	{
		printf("\n用户名或密码错误,任意键继续。。。\n");
		getchar();
		return 0;
	}
	printf("logbuf:%s\n",logbuf);
	memset(&user_temp,0,sizeof(user_temp));
	memcpy(&user_temp,logbuf,sizeof(logbuf));
	printf("temp: %s \n",user_temp.username);
	strcpy(password,log.pwd);
	user = user_temp;
	printf("%s\n",user.username);

	return 1;	  
}

/* 退出 */
void quit()
{
	char               quitbuf[1024];
	send(conn_fd,"q",1024,0);
	recv(conn_fd,quitbuf,1024,0);
	if(strcmp(quitbuf,"y")==0)
	{
		close(conn_fd);
		system("clear");
		exit(1);
	}
	else
	  printf("quit fail.\n");
}

/* 获取聊天记录 */
void get_record()
{
	char                      reco_buf[1024];
    struct chat               reco;

	printf("==================================================\n\n");
    while(1)
	{
		recv(conn_fd,reco_buf,1024,0);
		memcpy(&reco,reco_buf,sizeof(reco_buf));
		if(strcmp(reco.news,"")==0)
		{
			printf("==================================================\n");
			printf("\n任意键返回。。。");
			break;
		}
		else
		{
			printf("$ %s $: %s",reco.from,reco.time);
			printf("%s\n",reco.news);
			printf("-------------------------------------------------\n");
		}
	}
}

/* 查找 */
void search()
{
	char                      ch[10];
	char                      sch[1024];
    struct chat               sear;
	printf("\n");
	system("clear");
	printf("\n");
	printf("                   $ 查找用户 $\n");
	printf("=================================================\n");
	printf("输入用户名：");
	mygets(ch,10);
	printf("=================================================\n\n");
	printf("                   $ 用户资料 $\n\n");
	printf("-------------------------------------------------\n\n");
	sear.mark = 3;
	strcpy(sear.to,ch);
	strcpy(sear.from,user.username);
    memset(sch,0,sizeof(sch));
	memcpy(sch,&sear,sizeof(sear));
	send(conn_fd,sch,1024,0);
}

/* 私聊 */
void private_chat(char* from,char* to)
{
	char                  s_buf[1024];
	struct chat           rs;
	while(1)
	{
		char              str_chat[900];
		while(1)
		{
			mygets(str_chat,900);
			if(strcmp(str_chat,"")==0)
			  printf("输入不能为空\n");
			else
			  break;
		}
		if(strcmp(str_chat,"/quit")==0)
		{
			break;
		}
		else
		{
			rs.mark = 6;
			strcpy(rs.to,to);
			strcpy(rs.from,from);
			strcpy(rs.news,str_chat);
			get_time(rs.time);
			memset(s_buf,0,sizeof(s_buf));
			memcpy(s_buf,&rs,sizeof(rs));
			send(conn_fd,s_buf,1024,0);
			printf("    <---- 您说 | %s",rs.time);
			printf("------------------------------------\n");
		}
	}
}


/* 群聊 */
void gro_chat()
{
	char                  s_buf[1024];
	struct chat           rs;
	while(1)
	{
		char              str_chat[900];
		while(1)
		{
			mygets(str_chat,900);
			if(strcmp(str_chat,"")==0)
			  printf("输入不能为空\n");
			else
			  break;
		}
		if(strcmp(str_chat,"/quit")==0)
		{
			memset(talk_to,0,sizeof(talk_to));
			break;
		}
		else
		{
			rs.mark = 7;
			strcpy(rs.from,user.username);
			strcpy(rs.news,str_chat);
			strcpy(rs.to,talk_to);
			get_time(rs.time);
			memset(s_buf,0,sizeof(s_buf));
			memcpy(s_buf,&rs,sizeof(rs));
			send(conn_fd,s_buf,1024,0);
			printf("        <--- 您说 | %s",rs.time);
			printf("------------------------------------------\n");
		}
	}
}

/* 聊天框 */
void chat_menu(char* to)
{
	printf("\n");
	system("clear");
	printf("\n");
	printf("-----------------------------------------------\n");
	printf("help: 直接输入聊天内容回车发送，输入“/quit”退出\n");
	printf("===============================================\n");
	printf("正在与 %s 聊天中～\n",to);
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

// 聊天线程
void thread(void *arg)
{
    private_chat(user.username,talk_to);
	key = 1;
	memset(talk_to,0,sizeof(talk_to));
	pthread_exit(0);
}

/* 接收查找消息 */
void recv_search(struct chat chat)
{
	int                    c;
	struct chat            rs;
	if(chat.ask[0]=='y')
	{
		char               s_buf[1024];// 接收二次消息待测
		struct info        s_user;
		pthread_t          thid1;
		recv(conn_fd,s_buf,1024,0);
		memcpy(&s_user,s_buf,sizeof(s_buf));
		printf("\n用户信息\n");
		printf("%s\n",s_user.username);
		printf("==================================\n");
		printf(" 1.与他私聊           2.加为好友\n");
		printf("----------------------------------\n");
		printf("请选择<1/2>:");
		scanf("%d",&c);
		getchar();
		switch(c)
		{
			case 1:// 私聊
				{
					strcpy(talk_to,s_user.username);
					chat_menu(s_user.username);
                    pthread_create(&thid1,NULL,(void*)thread,NULL);
			    }
			    break;
			case 2:// 添加好友
		        {
					rs.mark = 1;
			        strcpy(rs.to,s_user.username);
			        strcpy(rs.from,user.username);
			        strcpy(rs.ask,"?");
			        memset(s_buf,0,sizeof(s_buf));
			        memcpy(s_buf,&rs,sizeof(rs));
			        printf("正在发送。。。");
			        send(conn_fd,s_buf,1024,0);
			        sleep(2);
					key = 1;
				}
				break;
		}
	}
	else
	{
		printf("\n没有找到该用户\n");
		getchar();
		key = 1;
	}
}

/* 接收私聊消息处理 */
void recv_pchat(struct chat chat)
{
	if(strcmp(talk_to,chat.from)==0)
	{
		printf("\n$ %s: %s",chat.from,chat.time);
		printf("%s\n",chat.news);
		printf("---------------------------------\n");
	}
	else
	{
		message[mes_num] = chat;
		mes_num++;
		remenu();
	}
}

/* 接收群聊消息处理 */
void recv_gchat(struct chat chat)
{
	if(strcmp(talk_to,chat.to)==0)
	{
		printf("\n$ %s: %s",chat.from,chat.time);
		printf("%s\n",chat.news);
		printf("---------------------------------\n");
	}
	else
	{
		message[mes_num] = chat;
		mes_num++;
		remenu();
	}
}

/* 获取好友列表 */
void friendlist()
{
	char              listbuf[1024];
	struct chat       fri;
    int               i;
	int ret;
	int               status[10];         // 记录在线状态
	for(i=0;i<10;i++)
	{
		recv(conn_fd,listbuf,1024,0);
		memcpy(&fri,listbuf,sizeof(listbuf));
		if(strcmp(fri.news,"")==0)
		    break;
		strcpy(friend[i].username,fri.news);
		status[i] = fri.mark;
	}
	f_num = i;
	printf("\n我的好友\n");
	printf("-------------------------\n\n");
	for(i=0;i<f_num;i++)
	{
		printf("[%2d]   %-10s",i,friend[i].username);
		if(status[i]==1)
		  printf("  在线\n");
		if(status[i]==0)
		  printf("  离线\n");
	}
	printf("-------------------------\n");
}

/* 获取群列表 */
void grouplist()
{
	char              listbuf[1024];
	struct chat       gro;
    int               i;
	int ret;

	for(i=0;i<3;i++)
	{
		recv(conn_fd,listbuf,1024,0);
		memcpy(&gro,listbuf,sizeof(listbuf));
		if(strcmp(gro.news,"")==0)
		    break;
		strcpy(group[i].gro_name,gro.news);
	}
	g_num = i;
	printf("\n我的群组\n");
	printf("--------------------\n");
	for(i=0;i<g_num;i++)
	{
		printf("[%2d] %s\n",i,group[i].gro_name);
	}
	printf("--------------------\n");
}

/* 获取群成员列表 */
void memberlist()
{
	char              listbuf[1024];
	struct chat       gro;
    int               i;
	int ret;
	int               status[10];        // 记录在线状态
	for(i=0;i<10;i++)
	{
		recv(conn_fd,listbuf,1024,0);
		memcpy(&gro,listbuf,sizeof(listbuf));
		if(strcmp(gro.news,"")==0)
		    break;
		strcpy(member[i].username,gro.news);
		status[i] = gro.mark;
	}
	m_num = i;
	printf("\n群组成员\n");
	printf("------------------------\n");
	for(i=0;i<m_num;i++)
	{
		printf("[%2d]  %-10s",i,member[i].username);
		if(status[i]==1)
		  printf("  在线\n");
		if(status[i]==0)
		  printf("  离线\n");
	}
	printf("------------------------\n");
}

void remenu()
{
	if(strcmp(talk_menu,"menu")==0)
	{
		printf("\n");
		system("clear");
		strcpy(talk_menu,"menu");
		printf("\n\n");
		printf("                %s\n",user.username);
		printf("               ===========================\n\n");
        printf("                     1. 我的好友\n\n");
		printf("                     2. 查找用户\n\n");
		printf("                     3. 我的群组\n\n");
		printf("                     4. 新建群组\n\n");
		printf("                     5. 消息管理 [%d]\n\n",mes_num);
		printf("                     0. 退    出\n\n");
		printf("               ===========================\n\n");
		printf("               请输入选项<0~4>:");
	}
}

/* 消息处理线程 */
void sign_pthread()
{
	char                sign_buf[BUFSIZE];
	struct chat         chat;
	int ret;

	while(1)
	{
		memset(sign_buf,0,sizeof(sign_buf));
		memset(&chat,0,sizeof(chat));
		recv(conn_fd,sign_buf,1024,0);
	
		memcpy(&chat,sign_buf,sizeof(sign_buf));
		switch(chat.mark)
		{
			case 1:// 添加好友
				{
					if(strcmp(chat.ask,"?y")==0)
					  printf("\n已发送请求\n");
					if(strcmp(chat.ask,"?n")==0)
					  printf("\n对方不在线\n");
					if(strcmp(chat.ask,"?")==0)
					{
						message[mes_num] = chat;
						mes_num++;
                        remenu();
					}
					if(strcmp(chat.ask,"y")==0)
					{ 
						message[mes_num] = chat;
						mes_num++;
						remenu();
					}
					if(strcmp(chat.ask,"n")==0)
					{
						message[mes_num] = chat;
						mes_num++;
						remenu();
					}
				}
				break;
			case 2:// 删除好友
				if(chat.ask[0]=='y')
				  printf("\n已删除\n");
				break;
			case 3:// 查找用户
				recv_search(chat);
				break;
			case 4:// 创建群
				if(chat.ask[0]=='y')
				  printf("\n恭喜您成功建群\n");
				else
				  printf("\nfail\n");
				break;
			case 5:// 申请入群
				if(chat.ask[0]=='y')
				  printf("\n恭喜您成功入群\n");
				else
				  printf("\n没有找到该群\n");
				break;
			case 6:// 私聊
			    recv_pchat(chat);	
				break;
			case 7:// 群聊
				recv_gchat(chat);
				break;
			case 11:
				friendlist();
				break;
			case 12:
				grouplist();
				break;
			case 13:
				memberlist();
				break;
			case 14:
				get_record();
				break;
		}
	}
}

/* 消息管理 */
void do_news()
{
	int                    i = 0;
	int                    c;
	char                   cy;
	char                   do_buf[1024];

	printf("\n");
	system("clear");
	printf("\n消息管理\n");
	printf("========================\n");
	printf("您收到[%d条]消息\n",mes_num);
	printf("------------------------\n");
	printf("[标号]        内容\n");
	for(i=0;i<mes_num;i++)
	{
		printf("[%3d]",i);
		if(message[i].mark==1)
		  printf("        好友请求\n");
		if(message[i].mark==6)
		  printf("        私聊消息\n");
		if(message[i].mark==7)
		  printf("        群聊消息\n");
	}
    if(mes_num==0)
	  printf("无消息\n");
	else
	{
		printf("请选择标号查看消息内容:");
		scanf("%d",&c);
		getchar();
		switch(message[c].mark)
		{
			case 6:
				strcpy(talk_to,message[c].from);
				chat_menu(talk_to);
				printf("\n$ %s: %s",message[c].from,message[c].time);
				printf("%s\n",message[c].news);
				printf("---------------------------------\n");
				message[c] = message[mes_num-1];
				memset(&message[mes_num-1],0,sizeof(message[mes_num-1]));
				mes_num--;
                private_chat(user.username,talk_to);
				memset(talk_to,0,sizeof(talk_to));
				break;
			case 7:
	        	strcpy(talk_to,message[c].to);
				chat_menu(talk_to);
				printf("\n$ %s: %s",message[c].from,message[c].time);
				printf("%s\n",message[c].news);
				printf("---------------------------------\n");
				message[c] = message[mes_num-1];
				memset(&message[mes_num-1],0,sizeof(message[mes_num-1]));
				mes_num--;
                gro_chat(user.username,talk_to);
				break;
			case 1:
				printf("\n");
				if(strcmp(message[c].ask,"?")==0)
				{
					strcpy(talk_to,message[c].from);
				printf("[%s ]请求添加您为好友，是否同意(y/n)？:",talk_to);
				scanf("%c",&cy);
				getchar();
				if(cy=='y')
				{
					strcpy(message[c].ask,"y");
					memcpy(do_buf,&message[c],sizeof(message[c]));
					send(conn_fd,do_buf,1024,0);
                    memset(&message[c],0,sizeof(message[c]));
					printf("已同意\n");
					getchar();
				}
				else
				{
                    strcpy(message[c].ask,"n");
					memcpy(do_buf,&message[c],sizeof(message[c]));
					send(conn_fd,do_buf,1024,0);
					memset(&message[c],0,sizeof(message[c]));
				    printf("已拒绝");
					getchar();
				}
				}
                if(strcmp(message[c].ask,"y")==0)
				{
					strcpy(talk_to,message[c].to);
					printf("[%s]同意加您为好友\n",talk_to);
				}
				if(strcmp(message[c].ask,"n")==0)
				{
					strcpy(talk_to,message[c].to);
					printf("[%s]拒绝加您为好友\n",talk_to);
				}
				message[c] = message[mes_num-1];
				memset(&message[mes_num-1],0,sizeof(message[mes_num-1]));
				memset(talk_to,0,sizeof(talk_to));
                mes_num--;
				getchar();
				break;
		}
	}
}

/* 好友选项 */
void friendmenu()
{
	int                   c,c1;
	char                  fribuf[1024];
	struct chat           chat;
	printf("输入 -1 返回,序号选择对应好友:");
	while(1)
	{
		scanf("%d",&c);
	    getchar();
		if(c==-1)
		  break;
		else if(c>=0&&c<f_num)
		{
			printf("\n");
			system("clear");
			printf("\n\n");
			printf("                 好友 %s\n",friend[c].username);
			printf("            -----------------\n");
			printf("               1. 删除\n");
			printf("               2. 私聊\n");
			printf("               3. 记录\n");
			printf("               0. 返回\n");
			printf("            -----------------\n");
			printf("            请选择:");
			scanf("%d",&c1);
			getchar();
			switch(c1)
			{
				case 1:
					chat.mark = 2;
					strcpy(chat.to,friend[c].username);
					strcpy(chat.from,user.username);
					memcpy(fribuf,&chat,sizeof(chat));
					send(conn_fd,fribuf,1024,0);
					break;
				case 2:
					strcpy(talk_to,friend[c].username);
					printf("%s",friend[c].username);
                    chat_menu(friend[c].username);
                    private_chat(user.username,talk_to);
                    memset(talk_to,0,sizeof(talk_to));
					break;
				case 3:
					printf("\n");
					system("clear");
					printf("\n\n");
					printf("与 %s 的聊天记录\n",friend[c].username);
					chat.mark = 14;
					strcpy(chat.from,user.username);
					strcpy(chat.to,friend[c].username);
					memcpy(fribuf,&chat,sizeof(chat));
					send(conn_fd,fribuf,1024,0);
					usleep(500);
					getchar();
			}

			break;
		}
	}
}

/* 群选项 */
void groupmenu()
{
	int                   c,c1;
	char                  grobuf[1024];
	struct chat           chat;
	printf("输入 -1 返回,序号选择对应群组:");
	while(1)
    {
		scanf("%d",&c);
		getchar();
        if(c == -1)
		  break;
		else if(c>=0&&c<g_num)
		{
            while(1)
			{
				printf("\n");
				system("clear");
				printf("\n\n");
				printf("                     群 %s\n",group[c].gro_name);
			    printf("                 ---------------------\n\n");
			    printf("                     1. 进入聊天\n\n");
			    printf("                     2. 群组成员\n\n");
				printf("                     3. 聊天记录\n\n");
			    printf("                     0. 回主菜单\n\n");
			    printf("                 ---------------------\n\n");
			    printf("                 请选择：");
			    scanf("%d",&c1);
			    getchar();
			    switch(c1)
			    {
					case 1:
						strcpy(talk_to,group[c].gro_name);
                        chat_menu(group[c].gro_name);
						gro_chat();
						break;
				    case 2:
						chat.mark = 13;
					    strcpy(chat.to,group[c].gro_name);
					    strcpy(chat.from,user.username);
					    memcpy(grobuf,&chat,sizeof(chat));
					    send(conn_fd,grobuf,1024,0);
						usleep(500);
						getchar();
					    break;
					case 3:
						chat.mark = 14;
						strcpy(chat.from,group[c].gro_name);
						memcpy(grobuf,&chat,sizeof(chat));
						printf("\n\n");
						printf("群 %s 的聊天记录\n",group[c].gro_name);
						send(conn_fd,grobuf,1024,0);
						usleep(500);
						getchar();
						break;
				}
				if(c1==0)
				  break;
			}
			break;
		}
	}
}

/* 新建群组 */
void buildgro()
{
	int                          c;
	char                         grobuf[1024];
	struct chat                  chat;
	printf("\n");
	system("clear");
	printf("\n\n");
	printf("                  -----------------------\n\n");
	printf("                        1. 创建群组\n\n");
	printf("                        2. 加入群组\n\n");
	printf("                        0. 回主菜单\n\n");
	printf("                  -----------------------\n");
	printf("                  请选择<0~2>：");
    scanf("%d",&c);
	getchar();
	switch(c)
	{
		case 1:
			printf("输入新建群名字：");
	        mygets(chat.to,10);
			chat.mark = 4;
	        strcpy(chat.from,user.username);
	        memcpy(grobuf,&chat,sizeof(chat));
	        send(conn_fd,grobuf,1024,0);
	        getchar();
			break;
		case 2:
            printf("输入群组名字：");
			mygets(chat.to,10);
			chat.mark = 5;
			strcpy(chat.from,user.username);
			memcpy(grobuf,&chat,sizeof(chat));
			send(conn_fd,grobuf,1024,0);
			getchar();
			break;
	}
}

/* 登陆菜单 */
void login_menu_()
{
	while(1)
	{
		int                c;
		char               lgbuf[1024];
		struct  chat       lg_user;
		int ret;
		printf("\n");
		system("clear");
		strcpy(talk_menu,"menu");
		printf("\n\n");
		printf("                %s\n",user.username);
		printf("               ===========================\n\n");
        printf("                     1. 我的好友\n\n");
		printf("                     2. 查找用户\n\n");
		printf("                     3. 我的群组\n\n");
		printf("                     4. 新建群组\n\n");
		printf("                     5. 消息管理 [%d]\n\n",mes_num);
		printf("                     0. 退    出\n\n");
		printf("               ===========================\n\n");
		printf("               请输入选项<0~4>:");
	    scanf("%d",&c);
		getchar();
		memset(talk_menu,0,sizeof(talk_menu));
		switch(c)
		{
			case 1:
				
				 memset(lgbuf,0,sizeof(lgbuf));
				 memset(&lg_user,0,sizeof(lg_user));
				 lg_user.mark = 11;
				 memcpy(lgbuf,&lg_user,sizeof(lg_user));
				 if((ret = send(conn_fd,lgbuf,1024,0))<0)
				 {
					 my_err("send",__LINE__);
				 }
	             usleep(500);
				 friendmenu();
				 break;
			case 2:// 查找
                 search();
				 while(key == 0)
				 {}
				 key = 0;
				 break;
			case 3:
				 memset(lgbuf,0,sizeof(lgbuf));
				 memset(&lg_user,0,sizeof(lg_user));
				 lg_user.mark = 12;
				 memcpy(lgbuf,&lg_user,sizeof(lg_user));
				 if((ret = send(conn_fd,lgbuf,1024,0))<0)
				 {
					 my_err("send",__LINE__);
				 }
	             usleep(500);
				 groupmenu();
				 break;
			case 4:
				 buildgro();
				 break;
			case 5:
				 do_news();
				 break;
			case 0:
				{
					memset(lgbuf,0,sizeof(lgbuf));
				    lg_user.mark = 9;
				    memcpy(lgbuf,&lg_user,sizeof(lg_user));
				    send(conn_fd,lgbuf,1024,0);
				    sleep(1);
					close(conn_fd);
				    exit(1);
				}
		}
	}
}

/* 主界面 */
void menu()
{
	char                 c;
	while(1)
	{
		printf("\n");
		system("clear");
	    printf("\n\n\n");
	    printf("                ========================\n\n");
	    printf("                      1.  注册\n\n");
	    printf("                      2.  登陆\n\n");
	    printf("                      0.  退出\n\n");
	    printf("                ========================\n\n");
	    printf("                请输入选项<0 ~ 2>：");
	    c = getchar();
		getchar();
		printf("\n");
		if(c=='1')
		{
			if(apply())
			  break;
		}
		else if(c=='2')
		{
			if(login())
			  break;
		}
		else if(c=='0')
		{
			quit();
		}
	}
}

/* 主函数 */
int main(int argc,char **argv)
{
	int                  i;
	int                  serv_port;
	struct sockaddr_in   serv_addr;
	pthread_t            thid,thid1;

	// 检查参数个数
	if(argc!=5)
	{
		printf("Usage: [-p] [serv_port] [-a] [serv_address]\n");
		exit(1);
	}

	// 初始化服务器端地址结构
	memset(&serv_addr,0,sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;

	// 从命令行获取服务器端的端口与地址
	for(i=1;i<argc;i++)
	{
		if(strcmp("-p",argv[i])==0)
		{
			serv_port = atoi(argv[i+1]);
			if(serv_port<0||serv_port>65535)
			{
				printf("invalid serv_addr.sin_port\n");
				exit(1);
			}
			else
			{
				serv_addr.sin_port = htons(serv_port);
			}
			continue;
		}

		if(strcmp("-a",argv[i])==0)
		{
			if(inet_aton(argv[i+1],&serv_addr.sin_addr)==0)
			{
				printf("invalid server ip address\n");
				exit(1);
			}
			continue;
		}
	}

	// 检查参数
	if(serv_addr.sin_port ==0||serv_addr.sin_addr.s_addr==0)
	{
		printf("Usage: [-p] [serv_addr.sin_port] [-a] [serv_address]\n");
		exit(1);
	}

	// 创建一个TCP套接字
	conn_fd = socket(AF_INET,SOCK_STREAM,0);
	if(conn_fd<0)
	{
		my_err("socket",__LINE__);
		exit(1);
	}

	// 向服务器发送连接请求
	if(connect(conn_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr))<0)
	{
		my_err("connect",__LINE__);
		exit(1);
	}
    
	menu();

	pthread_create(&thid,NULL,(void*)sign_pthread,NULL);
	login_menu_();
	
	
	return 0;
}
