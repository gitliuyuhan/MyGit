#include<assert.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<pthread.h>

#define  SERV_PORT   4507                 // 端口
#define  LIST        10                   // 最大连接数
#define  BUFSIZE     1024                 // 缓冲区大小
#define  LEN         sizeof(struct users) // 用户数据结构体大小

pthread_mutex_t      mutex;               // 锁

// 个人信息
struct info
{
	char             username[10];        // 用户名
};

// 群组
struct group
{
	char             gro_name[20];        // 群名称
	char             admin[10];           // 管理员名字
	int              m_num;               // 群成员个数
	struct info      member[10];          // 保存群成员
};

// 用户
struct users
{
	char             password[10];        // 用户密码
	int              f_num;               // 用户好友数
	int              g_num;               // 用户群组个数
	struct info      user;                // 用户个人信息
	struct info      friend[10];          // 好友
	struct group     group[3];            // 保存群组
	struct users*    next;
};

struct users*        head = NULL;         // 用户数据链表头指针
struct users*        p_user;              // 指向当前用户的指针

// 客户端连接队列
struct conn
{
	int              fd;                  // 连接套接字
	char             name[10];            // 所连接客户端的用户名
}conn[10];                                

// 专用于传输数据的标准结构体
struct chat
{
	int              mark;                // 消息标志
	char             from[10];            // 发起方
	char             to[10];              // 目标对象
	char             ask[10];             // 某些消息的附加标志
	char             time[30];            // 时间
	char             news[900];           // 保存聊天内容或其他字符串
};

// 保存用户名和密码，用于登陆和注册模块
struct log
{
	char         flag;                   // 区分当前操作是在登陆还是注册的标志
	char         name[10];               // 用户名
	char         pwd[10];                // 密码
}; 

//================================用户数据保存
void save()
{
	FILE *fp;		
	struct users *p=head->next;		
	if((fp=fopen("users.dat","wb"))==NULL)
	{
		printf("Cannot open userdata file!\n");
        exit(0);									
	}
    while(p!=NULL)
	{
		fwrite(p,LEN,1,fp);
		p=p->next;
	}
	fclose(fp);
}

//====================用户数据读取

struct users *readuser()              
{

	FILE *fp;		
	struct users *p1,*p2;

	// 判断是否存在该文件，否则新建文件
	if((fopen("users.dat","rb"))==NULL)
	{
		fp=fopen("users.dat","wb");
		fclose(fp);
	}
	// 打开文件
	if((fp=fopen("users.dat","rb"))==NULL)				
	{
		printf("Cannot open userdata file!\n");								
		exit(0);
	}
    head=p1=(struct users *)malloc(LEN);
    p2=(struct users *)malloc(LEN);
    fread(p2,LEN,1,fp);
    while(!feof(fp))
    {
		p1->next=p2;
		p1=p2;
        p2=(struct users *)malloc(LEN);
	    fread(p2,LEN,1,fp);
	}
    p1->next=NULL;
    free(p2);
    fclose(fp);
	return (head);
}

/* 保存私聊天记录 */
void save_record(struct chat record)
{
	FILE*                fp;		
    char                 file_from[15],file_to[15];  // 用于保存发送方和接受方的聊天记录文件名

	strcpy(file_from,record.from);
    strcat(file_from,"_record.dat");     // 使文件名都以“_record.dat”结尾
	strcpy(file_to,record.to);
	strcat(file_to,"_record.dat");

	/* 如果这两个文件不存在则新建文件 */
	if((fopen(file_from,"ab"))==NULL)
	{
		fp=fopen(file_from,"wb");
		fclose(fp);
	}
	if((fopen(file_to,"ab"))==NULL)
	{
		fp=fopen(file_to,"wb");
		fclose(fp);
	}
	
	// 重新打开文件
	if((fp=fopen(file_from,"ab"))==NULL)				
	{
		printf("Cannot open chat record file!\n");								
	}
    fwrite(&record,sizeof(struct chat),1,fp);       // 写入一条聊天记录
	fclose(fp);

    if((fp=fopen(file_to,"ab"))==NULL)				
	{
		printf("Cannot open chat record file!\n");								
	}
    fwrite(&record,sizeof(struct chat),1,fp);
	fclose(fp);
}

/* 保存群聊天记录 */
void gro_record(struct chat record)
{
	FILE*             fp;
	char              file_gro[25];

	strcpy(file_gro,record.to);
    strcat(file_gro,"_record.dat");

	// 文件不存在时新建文件
	if((fopen(file_gro,"ab"))==NULL)
	{
		fp=fopen(file_gro,"wb");
		fclose(fp);
	}
	
	if((fp=fopen(file_gro,"ab"))==NULL)				
	{
		printf("Cannot open chat record file!\n");								
	}
    fwrite(&record,sizeof(struct chat),1,fp);
	fclose(fp);
}
/* 获取聊天记录 */
void get_record(struct chat chat,int i)
{
	FILE*             fp;
	int               k=0;              // 传输标志
	char              file_record[25];
	char              reco_buf[1024];
    struct chat       reco;             // 用于传输聊天记录的结构体

	memcpy(reco_buf,&chat,sizeof(chat));
	send(conn[i].fd,reco_buf,1024,0);

	strcpy(file_record,chat.from);
	strcat(file_record,"_record.dat");

	fp=fopen(file_record,"rb");
	// 如果文件不存在发送传输结束标志空字符串
	if(fp==NULL)
	{
		strcpy(chat.news,"");
        memcpy(reco_buf,&chat,sizeof(chat));
		send(conn[i].fd,reco_buf,1024,0);
		fclose(fp);
		return;                         // 终止，无返回值
	}

	// 私聊天记录请求
	if(strcmp(conn[i].name,chat.from)==0)
	{
		do
		{
			if(k == 1)
			{
				if(strcmp(chat.to,reco.from)==0||strcmp(chat.to,reco.to)==0)
				{
					memset(reco_buf,0,sizeof(reco_buf));
					memcpy(reco_buf,&reco,sizeof(reco));
				//	printf("%s and %s\n",reco.from,reco.to);
				//	printf("%s\n",reco.news);
					send(conn[i].fd,reco_buf,1024,0);
				}
			}
			fread(&reco,sizeof(struct chat),1,fp);
			k = 1;             // 使得第一次循环之后开始传输
		}while(!feof(fp));
	}
	// 群聊天记录请求
	else
	{
		do
		{
			 if(k==1)
			 {
				 memset(reco_buf,0,sizeof(reco_buf));
				 memcpy(reco_buf,&reco,sizeof(reco));
		//		 printf("%s and %s\n",reco.from,reco.to);
		//		 printf("%s\n",reco.news);
				 send(conn[i].fd,reco_buf,1024,0);
			 }
			 fread(&reco,sizeof(struct chat),1,fp);
			 k = 1;
		}while(!feof(fp));
	}
	// 文件读取结束，发送传输结束标志
	strcpy(chat.news,"");
    memcpy(reco_buf,&chat,sizeof(chat));
	send(conn[i].fd,reco_buf,1024,0);
	fclose(fp);
}
/* 错误处理函数 */
void my_err(const char * err_string, int line)
{
	fprintf(stderr,"line:%d ",line);
	perror(err_string);
}

/* 查找指定的用户名 */
struct users* found(char* str_name)
{
	struct users*    p;
	p = head->next;
	while(p!=NULL)
	{
		if(strcmp((p->user).username,str_name)==0)
		  return(p);
		p = p->next;
	}
	return NULL;
}

/* 注册 */
struct users*  apply(struct log log,int i)
{
	char             plybuf[BUFSIZE];
	struct log       ply;
    struct users     *p = head,*p1;

	ply = log;
	
	while(p->next!=NULL)
	{
		if(strcmp((p->next->user).username,ply.name)==0)
		{
		//	printf("n\n");
			int ret=send(conn[i].fd,"n",1024,0); assert(ret==1024);  // 申请注册的用户名已被占用，返回n
			return NULL;
		}
		p = p->next;
	}

	p1 =(struct users*)malloc(LEN);
	strcpy((p1->user).username,ply.name);
	strcpy(p1->password,ply.pwd);
	p->next = p1;
	p1->next = NULL;
	save();
	memset(plybuf,0,sizeof(plybuf));
	memcpy(plybuf,&(p1->user),sizeof(p1->user));
	int ret=send(conn[i].fd,plybuf,1024,0); assert(ret==1024);
	strcpy(conn[i].name,ply.name);
	printf("apply successfully\n");
	return (p1);
}

/* 登陆 */
struct users* login(struct log log,int i)
{
	char             logbuf[BUFSIZE];
	struct users*    p = head->next;
	struct users     user_temp;
	
	while(p!=NULL)
	{
		if(strcmp((p->user).username,log.name)==0)
		{
			if(strcmp(p->password,log.pwd)==0)
			{
				user_temp.user = p->user;
				memset(logbuf,0,sizeof(logbuf));
			    memcpy(logbuf,&user_temp.user,sizeof(user_temp.user));
			    int ret=send(conn[i].fd,logbuf,1024,0); assert(ret==1024);
			    strcpy(conn[i].name,log.name);
				return (p);
			}
		}
		p = p->next;
	}
	int ret=send(conn[i].fd,"n",1024,0); assert(ret==1024);// 登陆的用户名和密码错误，返回n
    return NULL;
}

/* 好友添加处理 */
void addfriend(struct chat chat,int i)
{
	int              j;
	char             addbuf[1024];
	struct users*    p;

	memset(addbuf,0,sizeof(addbuf));
	// 好友请求
	if(strcmp(chat.ask,"?")==0)
	{
		for(j=0;j<LIST;j++)
	    {
			if(strcmp(chat.to,conn[j].name)==0)
		    {
				memcpy(addbuf,&chat,sizeof(chat));
				int ret=send(conn[j].fd,addbuf,1024,0); assert(ret==1024); // 发给对方
				memset(addbuf,0,sizeof(addbuf));
                strcpy(chat.ask,"?y");
                memcpy(addbuf,&chat,sizeof(chat));
				ret=send(conn[i].fd,addbuf,1024,0); assert(ret==1024);// 返回信号给请求方
			    break;
		    }
		}
		if(j==LIST)
		{
			memset(addbuf,0,sizeof(addbuf));
			strcpy(chat.ask,"?n");                        // 申请添加的好友不在线
			memcpy(addbuf,&chat,sizeof(chat));
		    int ret=send(conn[i].fd,addbuf,1024,0); assert(ret==1024); 
		}
	}
	// 对方拒绝
	if(strcmp(chat.ask,"n")==0)
	{
	    for(j=0;j<LIST;j++)
	    {
			if(strcmp(chat.from,conn[j].name)==0)
		    {
				memcpy(addbuf,&chat,sizeof(chat));
				int ret=send(conn[j].fd,addbuf,1024,0); assert(ret==1024);
			    break;
		    }
		}
	}
    // 对方同意
	if(strcmp(chat.ask,"y")==0)
	{
		pthread_mutex_lock(&mutex);    // 加锁
	  
		head = readuser();
        p = head->next;

		p_user = found(conn[i].name);
        while(p!=NULL)
		{
			if(strcmp((p->user).username,chat.from)==0)
				break;
			p = p->next;
		}
		p->friend[(p->f_num)++]=p_user->user;
		p_user->friend[(p_user->f_num)++]=p->user;
		save();
		pthread_mutex_unlock(&mutex);  // 解锁
		for(j=0;j<LIST;j++)
	    {
			if(strcmp(chat.from,conn[j].name)==0)
		    {
				memcpy(addbuf,&chat,sizeof(chat));
				int ret=send(conn[j].fd,addbuf,1024,0); assert(ret==1024);
			    break;
		    }
		}
	}
}

/* 删除好友*/
void delfriend(struct chat chat,int i)
{
	    int                  j;
		char                 delbuf[1024];
		struct info          s;
	    struct users*        p;
		pthread_mutex_lock(&mutex);    // 加锁
		head = readuser();
        p = head->next;
		p_user = found(conn[i].name);
        while(p!=NULL)
		{
			if(strcmp((p->user).username,chat.to)==0)
				break;
			p = p->next;
		}
		for(j=0;j<(p->f_num);j++)
		{
			if(strcmp((p->friend[j]).username,chat.from)==0)
			{
				p->friend[j]=p->friend[(p->f_num)-1];
				memset(&(p->friend[(p->f_num)-1]),0,sizeof(p->friend[(p->f_num)-1]));
				(p->f_num)--;
		        break;
			}
		}
		for(j=0;j<(p_user->f_num);j++)
		{
			if(strcmp((p_user->friend[j]).username,chat.to)==0)
			{
				p_user->friend[j]=p_user->friend[(p_user->f_num)-1];
				memset(&(p_user->friend[(p_user->f_num)-1]),0,sizeof(p_user->friend[(p_user->f_num)-1]));

				(p_user->f_num)--;
		        break;
			}
		}
		save();
		pthread_mutex_unlock(&mutex);  // 解锁
		memset(delbuf,0,sizeof(delbuf));
        strcpy(chat.ask,"y");
		memcpy(delbuf,&chat,sizeof(chat));
		int ret=send(conn[i].fd,delbuf,1024,0); assert(ret==1024);  // 返回信号
}

/* 查找用户信号 */
void search(struct chat chat,int i)
{
	char                 searchbuf[BUFSIZE];
	struct users*        p;
	struct info          s;

	pthread_mutex_lock(&mutex);        // 加锁
	head = readuser();
	p = head->next;
	while(p!=NULL)
	{
		if(strcmp((p->user).username,chat.to)==0)
		{
			memset(searchbuf,0,sizeof(searchbuf));
			strcpy(chat.ask,"y");
			memcpy(searchbuf,&chat,sizeof(chat));
			int ret=send(conn[i].fd,searchbuf,1024,0); assert(ret==1024);
	        memset(searchbuf,0,sizeof(searchbuf));
			memset(&s,0,sizeof(s));
			s = p->user;
			memcpy(searchbuf,&s,sizeof(s));
			ret=send(conn[i].fd,searchbuf,1024,0); assert(ret==1024);
			break;
		}
		p = p->next;
	}
	pthread_mutex_unlock(&mutex);      // 解锁
	if(p==NULL)
	{
	   		memset(searchbuf,0,sizeof(searchbuf));
			strcpy(chat.ask,"n");
			memcpy(searchbuf,&chat,sizeof(chat));
			int ret=send(conn[i].fd,searchbuf,1024,0); assert(ret==1024);
	}
}

/* 申请建群 */
void buildgro(struct chat chat,int i)
{
	int              j;
    char             buildbuf[1024];

	p_user = found(conn[i].name);
	j = p_user->g_num;

	strcpy((p_user->group[j]).gro_name,chat.to);
	strcpy((p_user->group[j]).admin,chat.from);
	(p_user->group[j]).member[0] = p_user->user;
	(p_user->group[j]).m_num = 1;
	(p_user->g_num)++;
	pthread_mutex_lock(&mutex);          // 加锁
	save();
	pthread_mutex_unlock(&mutex);        // 解锁
	memset(buildbuf,0,sizeof(buildbuf));
	strcpy(chat.ask,"y");
	memcpy(buildbuf,&chat,sizeof(chat));
	int ret=send(conn[i].fd,buildbuf,1024,0); assert(ret==1024);
}

/* 请求入群 */
void joingro(struct chat chat,int i)
{
	int                     j,g,m,join_flag;
	char                    joinbuf[1024];
	struct users*           p;

	pthread_mutex_lock(&mutex);        // 加锁
	head = readuser();
	join_flag = 0;
	p = head->next;
	p_user = found(conn[i].name);

	while(p!=NULL)
	{
		j = p->g_num;
		for(g=0;g<j;g++)
		{
			if(strcmp((p->group[g]).gro_name,chat.to)==0)
			{
				join_flag = 1;
				m = (p->group[g]).m_num;
				(p->group[g]).member[m] = p_user->user;
				(p->group[g]).m_num++;
				m = p_user->g_num;
				p_user->group[m] = p->group[g];
			}  
		}
		p = p->next;
	}
	if(join_flag==1)
	{
		(p_user->g_num)++;
		memset(joinbuf,0,sizeof(joinbuf));
		strcpy(chat.ask,"y");
		memcpy(joinbuf,&chat,sizeof(chat));
		int ret=send(conn[i].fd,joinbuf,1024,0); assert(ret==1024);  // 返回信号
		save();
	}
	else
	{
		memset(joinbuf,0,sizeof(joinbuf));
		strcpy(chat.ask,"n");
		memcpy(joinbuf,&chat,sizeof(chat));
		int ret=send(conn[i].fd,joinbuf,1024,0); assert(ret==1024);  // 返回信号
	}
	pthread_mutex_unlock(&mutex);        // 解锁
}

/* 私聊 */
void pri_chat(struct chat chat,int i)
{
	int                  j;
	char                 chat_buf[BUFSIZE];

	memset(chat_buf,0,sizeof(chat_buf));

	for(j=0;j<LIST;j++)
	{
		if(strcmp(conn[j].name,chat.to)==0)
		{
			memcpy(chat_buf,&chat,1024);
			int ret=send(conn[j].fd,chat_buf,1024,0); assert(ret==1024);
			save_record(chat);
			break;
		}
	}
	if(j==LIST)
	{
		strcpy(chat.ask,"haveno");
		memcpy(chat_buf,&chat,1024);
	    int ret=send(conn[i].fd,chat_buf,1024,0); assert(ret==1024);
	}
}

/* 群聊 */
void gro_chat(struct chat chat,int i)
{
	int                     j,k,m;
    char                    chatbuf[1024];

    for(k=0;k<3;k++)
	{
		if(strcmp((p_user->group[k]).gro_name,chat.to)==0)
		  break;
	}
	memset(chatbuf,0,sizeof(chatbuf));
	memcpy(chatbuf,&chat,sizeof(chat));

	for(j=0;j<LIST;j++)
	{
		if(j!=i&&conn[j].fd>0)
		{
			for(m=0;m<(p_user->group[k]).m_num;m++)
            {
				if(strcmp(conn[j].name,(p_user->group[k]).member[m].username)==0)
				  send(conn[j].fd,chatbuf,1024,0);
			}
		}
    }
	gro_record(chat);
}

/* 好友列表 */
void friendlist(struct chat chat,int i)
{
	int                  j;
	char                 listbuf[1024];
    
	p_user = found(conn[i].name);

    memcpy(listbuf,&chat,sizeof(chat));
	send(conn[i].fd,listbuf,1024,0);
	for(j=0;j<10;j++)
	{
		int t;
		usleep(10);
	    memset(listbuf,0,sizeof(listbuf));
	    strcpy(chat.news,(p_user->friend[j]).username);
		for(t=0;t<LIST;t++)
		{
			if(strcmp(chat.news,conn[t].name)==0)
			{
				chat.mark = 1;
				break;
			}
		}
		if(t==LIST)
		  chat.mark = 0;
	    memcpy(listbuf,&chat,sizeof(chat));
	    send(conn[i].fd,listbuf,1024,0);
		if(strcmp(chat.news,"")==0)
		  break;
	}
	memset(&chat,0,sizeof(chat));
}

/* 获取群列表 */
void grouplist(struct chat chat,int i)
{
	int                  j;
	char                 listbuf[1024];
    
	p_user = found(conn[i].name);

    memcpy(listbuf,&chat,sizeof(chat));
	send(conn[i].fd,listbuf,1024,0);
	for(j=0;j<3;j++)
	{
		usleep(10);
	    memset(listbuf,0,sizeof(listbuf));
	    strcpy(chat.news,(p_user->group[j]).gro_name);
	    memcpy(listbuf,&chat,sizeof(chat));
	    send(conn[i].fd,listbuf,1024,0);
		if(strcmp(chat.news,"")==0)
		  break;
	}
	memset(&chat,0,sizeof(chat));
}

/* 获取群成员列表 */
void memberlist(struct chat chat,int i)
{
	int                  j,k;
	char                 listbuf[1024];
    
	p_user = found(conn[i].name);

    memcpy(listbuf,&chat,sizeof(chat));
	send(conn[i].fd,listbuf,1024,0);

	for(k=0;k<3;k++)
	{
		if(strcmp((p_user->group[k]).gro_name,chat.to)==0)
		  break;
	}
	for(j=0;j<10;j++)
	{
		int t;
		usleep(10);
	    memset(listbuf,0,sizeof(listbuf));
	    strcpy(chat.news,(p_user->group[k]).member[j].username);
		for(t=0;t<LIST;t++)
		{
			if(strcmp(conn[t].name,chat.news)==0)
			{
				chat.mark = 1;
				break;
			}
		}
		if(t==LIST)
		  chat.mark = 0;
	    memcpy(listbuf,&chat,sizeof(chat));
	    send(conn[i].fd,listbuf,1024,0);
		if(strcmp(chat.news,"")==0)
		  break;
	}
	memset(&chat,0,sizeof(chat));
}

/* 处理客户端线程 */
void* client(int i)
{	

	char                 flag[1024];
    char                 recv_buf[BUFSIZE];
	struct users*        p_user;
	int ret;
	printf("conn %d\n",i);
	while(1)
	{
		struct log       log;
		memset(flag,0,sizeof(flag));
		// 接收消息标志
	    if((ret = recv(conn[i].fd,flag,1024,0))<0)
	    {
			my_err("apply",__LINE__);
		    pthread_exit(0);
	    }

		pthread_mutex_lock (&mutex);    // 加锁
		memcpy(&log,flag,1024);

		head = readuser();
		
	    // 注册请求
	    if(log.flag=='a')
	    {
			p_user=apply(log,i);
			pthread_mutex_unlock(&mutex);  // 解锁
			if(p_user!=NULL)
			  break;
		}
		// 登陆请求
        if(log.flag=='b')
		{
			p_user=login(log,i);
			pthread_mutex_unlock(&mutex);  // 解锁
			if(p_user!=NULL)
			  break;
		}
	
		// 退出信号
		if(log.flag=='q')
		{   
			pthread_mutex_unlock(&mutex);  // 解锁
			int ret=send(conn[i].fd,"y",1024,0); assert(ret==1024);
			close(conn[i].fd);
			conn[i].fd = -1;
			pthread_exit(0);
		}
	}
	while(1)
	{
		struct chat      chat;
		memset(&chat,0,sizeof(chat));
		memset(recv_buf,0,sizeof(recv_buf));
		int ret=recv(conn[i].fd,recv_buf,1024,0);
	//	printf("assert here, %d\n", ret);
        memcpy(&chat,recv_buf,sizeof(recv_buf));
	//	printf("%d===%d\n",__LINE__, chat.mark);
		switch(chat.mark)
		{
			case 1:// 添加好友
				addfriend(chat,i);
				break;
			case 2:// 删除好友
				delfriend(chat,i);
				break;
			case 3:// 查找用户
				search(chat,i);
				break;
            case 4:// 创建群
				buildgro(chat,i);
				break;
			case 5:// 申请入群
				joingro(chat,i);
				break;
			case 6:// 私聊
				pri_chat(chat,i);
				break;
			case 7:// 群聊
				gro_chat(chat,i);
				break;
			case 9:// 请求退出	
		        {   
			        close(conn[i].fd);
			        conn[i].fd = -1;
			        pthread_exit(0);
		        }
			case 11:// 获取好友列表
				{
					friendlist(chat,i);
	//				printf("%d mark  %d\n",__LINE__, chat.mark);
				}
				break;
			case 12:
				grouplist(chat,i);
				break;
			case 13:
				memberlist(chat,i);
				break;
			case 14:
				get_record(chat,i);
				break;
		}
	}
}

/* 关闭服务器操作 */
void quit(void* arg)
{
	while(1)
	{
		char    shut[10];
		int     k;
		printf("[ 输入 “quit” 关闭服务器 ]\n");
		scanf("%s",shut);
		if(strcmp(shut,"quit")==0)
		{
			for(k=0;k<LIST;k++)
			if(conn[k].fd!=-1)
			{
				close(conn[k].fd);
				conn[k].fd = -1;
				strcpy(conn[k].name," ");
			}
			exit(1);
		}   
	}   
}

/* 主函数 */
int main()
{
	int                  i;
	int                  sock_fd,conn_fd;
	int                  optval;
	pthread_t            thid,quit_thid;
	socklen_t            cli_len;
    struct sockaddr_in   cli_addr,serv_addr;

	pthread_mutex_init(&mutex,NULL);    // 初始化互斥锁

	// 创建一个TCP套接字
	sock_fd = socket(AF_INET,SOCK_STREAM,0);
	if(sock_fd<0)
	{
		my_err("socket",__LINE__);
		exit(0);
	}

	// 设置该套接字使之可以重新绑定端口
	optval = 1;
	if(setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,(void *)&optval,sizeof(int))<0)
	{
		my_err("setsockopt",__LINE__);
		exit(0);
	}

	// 初始化服务器端地址结构
	memset(&serv_addr,0,sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// 将套接字绑定到本地端口
	if(bind(sock_fd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr_in))<0)
	{
		my_err("bind",__LINE__);
		exit(0);
	}

	// 将套接字转化为监听套接字
	if(listen(sock_fd,LIST)<0)
	{
		my_err("listen",__LINE__);
		exit(0);
	}

	// 初始化连接套接字队列
	for(i=0;i<LIST;i++)
	{
		conn[i].fd = -1;
		strcpy(conn[i].name," ");
	}

	pthread_create(&quit_thid,NULL,(void *)quit,NULL);
	cli_len = sizeof(struct sockaddr_in);
	while(1)
	{	
		// 找出空闲位置
		for(i=0;i<LIST;i++)
		{
			if(conn[i].fd==-1)
			  break;
		}
		// 通过accept 接收客户端的连接请求，并返回连接套接字用于收发数据
		conn_fd = accept(sock_fd,(struct sockaddr *)&cli_addr,&cli_len);
		if(conn_fd<0)
		{
			my_err("accept",__LINE__);
			exit(0);
		}
        conn[i].fd=conn_fd;
		printf("accept a new client, ip:%s\n",inet_ntoa(cli_addr.sin_addr));

		// 创建线程处理客户端
		pthread_create(&thid,NULL,(void *)client,(void *)i);
	}

	return 0;
}
