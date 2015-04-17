/*======================================================
    > File Name: sudoku.c
    > Author: lyh
    > E-mail:  
    > Other :  
    > Created Time: 2015年04月12日 星期日 20时55分29秒
 =======================================================*/

#include<stdio.h>
#include<stdlib.h>

#define     MAX          81       //格子总数
#define     KEEPODD      341      //101010101 保留奇数
#define     KEEPEVEN     170      //010101010 保留偶数

//状态值
const int status[10] = {0,1,2,4,8,16,32,64,128,256};
                        //000000000~100000000
typedef struct
{
	int  x,y;           //横，纵坐标
	int  select;        //可取值
	int  n;             //可取值个数
	int  value;         //已取值
	char ckey;          //字符标志
}MapType;

typedef struct
{
	MapType  data[MAX];
	int      top;
}Stack;

typedef struct
{
	MapType  data[MAX];
	int      top;
	int      charbase;   
}ReadyQ;                 //候选队

Stack    pass;                         //栈
ReadyQ   ready;                        //候选队
int      Pos[9][9][2] = {};            //数独棋盘
int      C[9]={0},R[9]={0},F[9]={0};   //行，列，宫

//计算该位置取值个数
int Selectnum(int binary)
{
	int    count = 0;
	while(binary)
	{
		count++;
		binary = binary & (binary-1);
	}
	return count;
}

//获取该位置可取值
int Getvalue(int binary,int key)
{
	int i;
	for(i=key;i<=9;i++)
	{
		if(binary & status[i]>0)
			return i;
	}
	return 0;
}

//判断是否同列或同行或同宫
int Samekind(MapType elem1,MapType elem2)
{
	if(elem1.x==elem2.x||elem1.y==elem2.y||((elem1.x*3+elem1.y)/3)==((elem2.x*3+elem2.y)/3))
		return 1;
	return 0;
}

//更新同行列宫的状态
int UpdateCRF(MapType elem,int v)
{
	C[elem.x] = C[elem.x] | status[v];  //更新行列宫的状态标志
	R[elem.y] = R[elem.y] | status[v];
	F[(elem.x*3+elem.y)/3] = F[(elem.x*3+elem.y)/3] | status[v];
}

//判断和更新
int JudgeUpdate(MapType elem,int v)
{
	int     i,j,err=1;
	if(elem.ckey=='e'||elem.ckey=='o'||elem.ckey=='0')  //数字或奇偶数
	{
		UpdateCRF(elem,v);
		for(i=0;i<=ready.top;i++)
			if(Samekind(elem,ready.data[i]))
			{
				ready.data[i].select = ready.data[i].select & (~status[v]);   //与反码取&消去已尝试的数
				ready.data[i].n--;
				if(ready.select==0)
				{
					for(j=i;j>=0;j--)
						if(Samekind(elem,ready.data[j]))
						{
							ready.data[j].select = ready.data[j] | (~status[v]);
							ready.data[i].n++;
						}
					return 0;
				}
			}
		return 1;
	}
	else           //字母
	{
		for(i=0;i<=ready.top;i++)
			if(elem.ckey==ready.data[i].ckey)
			{
				if(ready.data[i].select & status[v]==0)
				{
					for(j=ready.charbase;j<MAX;j++)
						if(elem.ckey==ready.data[j].ckey)
						{
							ready.data[j].select = ready.data[j].select | (~status[v]);
							ready.data[j].n++;
							ready.data[j].value = 0;
							ready.top++;
							ready.data[ready.top] = ready.data[j];
						    ready.charbase++;	
						}
					return 0;
				}
				else
				{
					ready.data[i].select = ready.data[i].select & (~status[v]);
					ready.charbase--;
					ready.data[ready.charbase] = ready.data[i];
					ready.data[ready.charbase].value = v;
					ready.data[ready.charbase].n--;
					ready.data[i] = ready.data[ready.top];     //用队顶填补空缺
					ready.top--;
					UpdateCRF(ready.data[ready.charbase],v);
					Pos[ready.data[ready.charbase].x][ready.data[ready.charbase].y] = v;
				}
			}
		for(i=0;i<=ready.top&&err!=0;i++)
		{
			if(Samekind(elem,ready.data[i]))
			{
				ready.data[i].select = ready.data[i].select & (~status[v]);
				ready.data[i].n--;
				if(ready,data[i].select==0)
				{
					err = 0;
					break;
				}
				continue;
			}
			for(j=ready.charbase;j<MAX;j++)
			{
				if(Samekind(ready.data[j],ready.data[i]))
				{
					ready.data[i].select = ready.data[i].select & (~status[v]);
					ready.data[i].n--;
					if(ready.data[i].select==0)
					{
						err = 0;
						break;
					}
					break;
				}
			}
		}
		if(err==0)
		{
			for(j=i;j>=0;j--)
				if(Samekind(elem,ready.data[j]))
				{
					ready.data[j].select = ready.data[j] | (~status[v]);
					ready.data[i].n++;
				}
			for(j=ready.charbase;j<MAX;j++)
				if(elem.ckey==ready.data[j].ckey)
				{
					ready.data[j].select = ready.data[j].select | (~status[v]);
					ready.data[j].n++;
					ready.data[j].value = 0;
					ready.top++;
					ready.data[ready.top] = ready.data[j];
					ready.charbase++;	
				}
			return 0;

		}
		return 1;
	}
}

//初始化栈
void Init_stack()
{
	pass.top = -1; 
}

//初始化候选队
void Init_Ready()
{
	ready.top = -1;
	ready.charbase = MAX;
}

//入栈
void PushS(MapType elem)
{
	pass.top++;
	pass.data[pass.top] = elem;
}
//出栈
MapType PopS()
{
	MapType   elem;
	elem = pass.data[pass.top];
	pass.top--;
	return elem;
}

//放入候选队
void PushQ(MapType elem)
{
	ready.top++;
	ready.data[ready.top] = elem;
}

//出候选队
MapType  PopQ()
{
	MapType  elem;
	int         i,k=0;
	for(i=1;i<=ready.top;i++)
	{
		if(ready.data[i].n>ready.data[k])
			k = i;
	}
	elem = ready.data[k];
	ready.data[k] = ready.data[ready.top];
	ready.top--;
	elem.value = Getvalue(elem,1);
	elem.n--;
	elem.select = elem.select & (~status[elem.value]);
	return elem;
}

//初始化棋盘
int  Init_Map(int OrgMap[9][9])
{
	int  i,j,k;
	for(i=0;i<9;i++)
	{
		for(j=0;j<9;j++)
		{
			k = OrgMap[i][j];
			Pos[i][j][2] = k;
			if(k>='0'&&k<='9')
			{
				Pos[i][j][0] = k-'0';    // (3*i+j)/3 宫
				C[i] = C[i] | status[k-'0'];
				R[j] = R[j] | status[k-'0'];
				F[(3*i+j)/3] = F[(3*i+j)/3] | status[k-'0'];
			}
			else
			{
				Pos[i][j][0] = 0;
			}
		}
	}
	for(i=0;i<9;i++)
	{
		for(j=0;j<9;j++)
		{
			if(Pos[i][j][0]==0)
			{
				Pos[i][j][1] = (~C[i])&(~R[j])&(~F[(i*3+j)/3])&511;
				if(Pos[i][j][2]=='o')
					Pos[i][j][1] = Pos[i][j][1] & KEEPODD;     //过滤偶数
				if(Pos[i][j][2]=='e')
					Pos[i][j][1] = Pos[i][j][1] & KEEPEVEN;    //过滤奇数
				if(Pos[i][j][1]==0)
					return 0;
			}
		}
	}
	return 1;
}

int main()
{
	FILE*      fp;
	int        i,j;
	int        OrgMap[9][9];
	char       c;

	printf("%d %d %d %d %d\n",C[8],R[8],F[0],F[1],F[2]);
	if((fp=fopen("input.txt","r"))==NULL)
	{
		printf("error!\n");
		exit(0);
	}
	//读取棋盘数据
	for(i=0;i<9;i++)
	{
		for(j=0;j<9;j++)
		{
			if((c=fgetc(fp))!='\n')
			{
				OrgMap[i][j] = ('0'<=c&&c<='9')?(c-'0'):c;
			}
		}
	}
	fclose(fp);
	return 0;
}
