//������� ������ ���̺귯��:����, fstream,string
//���� ����: string -> char*��, .clear -> memset �Լ� �̿�, ������ .size -> while������ ����
//�����ؾ��� ����: stNum,stName(�̰� �ΰ��� �������̾���)
//�ñ��� ��: �ڵ�� ���������ε�, �׷� ���� ����� ��� ��������?
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "address_map_arm.h"


#define MAX 500    //����ö ��, ���� ���� �ִ밪
#define INF 99999 //������� ������ ��Ÿ��
#define FLAG 0
#define STRING_MAX 256


typedef struct _Station
{                          //�ַ� ��,��� �ްų� �뼱������ ��� ����� �� ���
    int line;              //���� ȣ��
    char name[STRING_MAX]; //�� �̸�
    int x, y;              //�������� �ش� ���� ��ǥ��(���� �̹��� ó�� ���� �ʿ�)
} Station;

///��ü �뼱�� ���� ����///
Station allStation[MAX]; //��� ����ö �� + ���� ������ ������ �迭
int stNum[MAX];          //�� ȣ���� �ִ� �� �� ���� ������ �ε��� ����
char stName[MAX][MAX][STRING_MAX];  //string�� ��� 2���� �迭
int stcnt;               //�ٸ� �Լ����� stName�� stNum�� size�˱� ���� ����
double weight[MAX][MAX]; //���� ���� ����ġ �����ϴ� �迭
double distance[MAX];    //�ִ� ����� ����ġ��  �����ϴ� �迭
int path[MAX];           //���������� ���� ���� ������ ���
int s[MAX];              //���ϰ��� �ϴ� �ִ� ��� �����ϴ� �迭(���� stack����)
int found[MAX];          //�ش� ������ �ִ� �Ÿ��� �߰ߵǾ����� �Ǵ�
int index1;
int stLine; //��ȣ������

void init();
int findIndex(int, char[]);
char* findString(int idx);
int findLine(int idx);
void AddEdge(int, char[], int, char[], double);
int Choose(int);
void ShortestPath(const int, const int);
void result(int, char[], int, char[]);
//void pathToImage();
void calculateTime(int[], int,int,int);


int open_physical(int);
void* map_physical(int, unsigned int, unsigned int);
void close_physical(int);
int unmap_physical(void*, unsigned int);


///stName�� �� �� ����.
int main()
{
    init();
    char src[STRING_MAX] = "\0", dst[STRING_MAX] = "\0", s3[STRING_MAX] = "\0", s4[STRING_MAX] = "\0";
    int line1 = 0, line2 = 0;

    FILE* fp = NULL;
    fp = fopen("stations_with_bus.txt", "r");
    int cnt = 0;
    if (fp != NULL)
    {
        int numLine = 0, numStation = 0;
        if (fscanf(fp, "%d\n", &numLine) < 0)
            printf("fscanf failed 1\n");
        int i = 0, j = 0;
        for (i = 0; i < numLine; i++)
        {
            if (fscanf(fp, "%d\n", &numStation) < 0)
                printf("fscanf failed \n");
            for (j = 0; j < numStation; j++)
            {
                Station* station;
                station = (Station*)malloc(sizeof(Station));
                char temp[STRING_MAX] = "\0";
                if (fscanf(fp, "%s %d %d\n", temp, &station->x, &station->y) < 0)
                    printf("fscanf failed\n");
                station->line = i;
                strcpy(station->name, temp);
                allStation[index1++] = *station;
                strcpy(stName[i][j], temp);
                free(station);
            }
            //���� stNum.push_back()�̶� stNAme.push_back���� �ٲ�
            stNum[i] = index1; //���� stLine
        }
        if (fscanf(fp, "%d\n", &numStation) < 0)
            printf("fscanf failed \n");
      
        for (j = 0; j < numStation; j++)
        {
            char s3[STRING_MAX] = "\0", s4[STRING_MAX] = "\0";
            int line1 = 0, line2 = 0;
            if (fscanf(fp, "%d %s %d %s\n", &line1, s3, &line2, s4) < 0)
                printf("fscanf failed\n");
            if (line1 == line2)
                AddEdge(line1, s3, line2, s4, 1);
            else
                AddEdge(line1, s3, line2, s4, 0.5);
        }
        fclose(fp);
  

        printf("(ȣ��)��߿��� �Է��ϼ���:\n");
        scanf("%d %s", &line1, src);
        printf("(ȣ��)�������� �Է��ϼ���:\n");
        scanf(" %d %s", &line2, dst);
        printf("============================================ \n");
        result(line1, src, line2, dst);
    }
    else
    {
        printf("input.txtis not exit\n");
    }
    system("pause");
    return 0;
}

void init() //����ϴ� �迭 �� �ʱ�ȭ
{
    int i, j;
    for (i = 0; i < MAX; i++)
        for (j = 0; j < MAX; j++)
            if (i == j)
                weight[i][j] = 0;
            else
                weight[i][j] = INF;
    index1 = 0;
}

/*==============================================================
�츮�� ������ 0ȣ���̹Ƿ� ���� ������ �� +1 ����
================================================================*/
int findIndex(int line, char st[]) ////string�� st
{
    int idx = 0;
    int i = 0;
    //stName[line-1].size()�� �˾Ƴ���
  
    int flag = 0;
    for (i; i < MAX; i++) //���� stName�� vector<vector<string>>���� ����
    {
       
        char ithName[STRING_MAX] = "\0";
        strcpy(ithName, stName[line][i]);
        if (strcmp(ithName, st) == 0)
        {
            flag = 1;
            idx = i;
            break;
        }

    }
    if (line > 0) //������ line >1 dlaus stNum[line-2]�� ���߾���
        idx += stNum[line - 1];
    if (flag == 0) printf(" �Ű������� �־��� %s ��(line %d) �� ��ġ�ϴ� stName�� ��ġ�� ��ã����.\n",st,line);
    return idx;
}


///index�� ������ ���� ȣ������ ���
int findLine(int idx)
{
    int i = 0;
    for (i; i < 10; i++)
    {
        if (idx < stNum[i])
            return i;
    }
   // return stNum[9]; //////////////�̷��� ���°� �´���
}

char* findString(int Inde) 
{
    int i = 0;
    while (Inde > stNum[i]) i++;
    
    int offset = Inde - stNum[i];
    char ret[STRING_MAX] = "\0";
    strcpy(ret, stName[i][offset]);
    return ret;
}


void AddEdge(int line1, char st1[], int line2, char st2[], double we) //����ġ ���� �߰�
{
    int i, j;
    i = findIndex(line1, st1);
    j = findIndex(line2, st2);
 
    weight[i][j] = we;
    weight[j][i] = we;
}

int Choose(int n) //found[w]�� 0�̰� distance[u]�� distance[w] �� �ּ��� �� ��ȯ
{
    double min = INF;
    int w = 0;
    int u = -1; //������ ������ ���� �����Ƿ� �ʱⰪ�� -1
    for (w; w < n; w++)
        if (!found[w] && distance[w] < min)
        {
            min = distance[w];
            u = w;
        }
    return u;
}
void ShortestPath(const int n, const int v)
{
    //distance[j], 0<=j<n�� n���� ������ ���� ���� �׷��� G����
    //���� v���� ���� j������ �ִ� ��� ���̷� ������
    //������ ���̴� weight[i][j]�� �־�
    int i = 0;
    int j = 0;
    for (j; j < n; j++)
    {
        found[j] = 0;
        path[j] = -1;
        distance[j] = weight[v][j];
    }
    found[v] = 1;
    distance[v] = 0;

    for (i; i < n - 2; i++) //���� v�κ��� n-1�� ��θ� ����
    {
        int u = Choose(n); //found[w]�� 0�̰� distance[u]�� distance[w] �� �ּ��� �� ��ȯ
        found[u] = 1;
        int w = 0;
        for (w; w < n; w++)
            if (!found[w] && distance[u] + weight[u][w] < distance[w])
            {
                path[w] = u; //�ٲ�� ���� ����
                distance[w] = distance[u] + weight[u][w];
            }
    }
}
void result(int line1, char src[], int line2, char dst[])
{
    int i =0, j=0, idx=0, stcnt = 0;
    i = findIndex(line1, src);
    j = findIndex(line2, dst);
    ShortestPath(MAX, i);
    idx = j; //�������� �ٲ�� �ȵǹǷ�
    int z = 0;
   /*
    for (z; z < MAX; z++) {
        printf("path[%d] = %d(�ε�����)\n", z, path[z]);
    }
    */
    printf(" %s (line %d ) -> ", allStation[i].name, allStation[i].line);
    while (1) //������ ���������� ��߿� ���������� stack�� �ְ�
    {
        if (path[idx] == -1)
            break;
        s[stcnt] = path[idx];
        stcnt++;
        idx = path[idx];
    }

    stcnt--;
    //pathToImage(s);
    //calculateTime(s,stcnt,i,j);////////////////����� �ε����� ������ �ε��� ���Ե�
    while (stcnt >= 0) //������ �������� ����ϸ� ������ �´�
    {
        int v = s[stcnt--];
        printf(" %s (line %d ) -> ", allStation[v].name, allStation[v].line);
    }
    printf("%s (line %d ) \n", allStation[j].name, allStation[j].line);
    double time = distance[j];
    double second = time - (int)time;
    printf("�ҿ�ð� : %d ��", (int)time);
    if (second)
        printf("30��");
    printf("\n");
    memset(s, 0, sizeof(s)); //s �ٽ� �ʱ�
}


/*
==============================================================
calculateTime(): ��� �� ��ġ�� ���� 3�� ȣ���� �ҿ� �ð��� 7-segment�� ���
                  ��� �� ��ġ�� ȣ���� �����ϴ� LED ����. �� ��, ���� ȣ���� ��� ����, �ٷ� ���� ȣ���� �����Ÿ����� ����
        -path�� ����� index�� cnt++�� ����, ���� ���� ������ ȯ�� �����̸� ���ݱ��� ���ߴ� cnt�� 7-segment�� �����(cnt�� �ٽ� 0)
-����:http://contents.kocw.or.kr/document/region/2010/16\02/16_02_09_micro.pdf
 https://www.kanda.com/7segment.php

===============================================================
*/
void calculateTime(int s[], int stcnt,int i, int j) { //���⿡ �׳� LED �Լ����� �߰�����)

    int i,id1,id2; //������
    int getTime[20] = {0}; //��� �� ȣ�� �� �ҿ� �ð� ���
    int getLine[20] = { -1 }; //��ü ��� �� �����ϴ� ȣ�� ��ȣ ���
    char getName[MAX][STRING_MAX]; //��ü ����� �� �̸� �� flag����(���� �̿� ����) ���
    int IsBus[MAX]; //�ش� ��ΰ� ���� �뼱�� ����ϴ��� ���� ���(���� getName�� ���ÿ� ���� ��)
    int ind = 0; //getLine,getTime�� �迭 �ε���
    int cnt = 0; //��ü ����� �� ����
    //s[stcnt]���� ������� ������ ����Ǿ����� ����. �� ���� ������ ��!
    //s[stcnt]���� ���� ��� ���� ���۰����� �ִ´�.
    
    id1 = i;
    getLine[ind] = findLine(i);
    strcpy(getName[cnt], findString(i));
    if (findLine(i) == 0)IsBus[cnt] = 1; else IsBus[cnt] = 0;
    cnt++;
    while(stcnt >=0) //s�� �ִ� ��θ� ����ġ�� path���� ����
    {
        id2 = s[stcnt--];
        strcpy(getName[cnt],findString(id2));
        if(abs(id2 - id1) > 1) //ȣ���� �ٲ���ٸ�
        {
            getLine[++ind] = findLine(id2); //�ٲ� ȣ�� ������ getLine�� ����
            if (findLine(id2) == 0)IsBus[cnt] = 1;
            else IsBus[cnt] = 0;//�ٲ� ȣ���� ������ flag üũ 
        }
        else 
        {
            getTime[ind] += weight[id1][id2];
            IsBus[cnt] = 0;
        }
        cnt++;
        id1 = id2;
    }
    //������ �� Ȯ��
    strcpy(getName[cnt],findString(j));
    if (abs(id1 - j) > 1) //ȯ���� �̷�� ���ٸ�
    {
        if (findLine(j) == 0) IsBus[cnt] = 1;
        else IsBus[cnt] = 0;
        getLine[++ind] = findLine(j);
    }
    else
    {
        getTime[ind] += weight[id1][j];
        IsBus[cnt] = 0;
    }
    cnt++;//���߿� cnt�� for�� ���� �� ���� i <cnt��� �ϱ� ������ cnt++��.




    //�� ȣ�� �� �ɸ��� �ð� ���(lab7 ������)
char* seg7[10] =   {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
                   0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};

void *LW_virtual;             // used to map physical addresses for the light-weight bridge
volatile int * HEX_ptr; // virtual pointer to HEX displays
volatile int * LEDR_ptr;
int fd = -1;               // used to open /dev/mem for access to physical addresses
 int l;
   // Create virtual memory access to the FPGA light-weight bridge
   if ((fd = open_physical (fd)) == -1)
      return (-1);
   if ((LW_virtual = map_physical (fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)) == NULL)
      return (-1);

   // Set virtual address pointer to I/O port
   HEX_ptr = (unsigned int *) (LW_virtual + HEX3_HEX0_BASE);
   LEDR_ptr = (unsigned int *) (LW_virtual + LEDR_BASE);

    for(l = 2; l>=0; l--) {
        *HEX_ptr = seg7[(getTime[l]%10) & 0xF]; //���� �ڸ� �� ���
        *(HEX_ptr + 3) = seg7[(getTime[l]/10) & 0xF]; //10�� �ڸ� �� ���
        HEX_ptr += 6;
        }

    //LED ����: ���� Ÿ�� �ִ� LED�� ��� �����ְ�, ���� �뼱�� �����Ÿ�. �����ʺ��� ������� 0~9(0���� ���� �뼱)
    while (getLine[l] != -1) {
        int Line1 = getLine[l], wait = getTime[l], Line2 = getLine[++l];
        if (Line2 == -1) { //ó������ -1�� ���Դٴ� ���� Line1�� ������ȣ���̶�� ��. �� ���� ������ ���� Line1�� ���
            LEDR_ptr = oxff;//���� ������ LED�� ���
            sleep(wait);
        }
        else
        {
            while (wait > 0) {
                LEDR_ptr = ;//���� ȣ��, ���� ȣ�� �� �� �� ����
                sleep(1);
                if (--wait == 0) break; //�߰��� wait ��ȣ �� �Ǹ� ����
                LEDR_ptr = ;//���� ȣ���� ��
                sleep(1);
                wait--;
            }
        }
    //Line2���Ҷ� �̹� l++�����Ƿ� ���� �÷��� �����൵ ��
    }

   unmap_physical (LW_virtual, LW_BRIDGE_SPAN);   // release the physical-memory mapping
   close_physical (fd);   // close /dev/mem
}


// Open /dev/mem, if not already done, to give access to physical addresses
int open_physical(int fd)
{
    if (fd == -1)
        if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1)
        {
            printf("ERROR: could not open \"/dev/mem\"...\n");
            return (-1);
        }
    return fd;
}

// Close /dev/mem to give access to physical addresses
void close_physical(int fd)
{
    close(fd);
}

void* map_physical(int fd, unsigned int base, unsigned int span)
{
    void* virtual_base;

    // Get a mapping from physical addresses to virtual addresses
    virtual_base = mmap(NULL, span, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, base);
    if (virtual_base == MAP_FAILED)
    {
        printf("ERROR: mmap() failed...\n");
        close(fd);
        return (NULL);
    }
    return virtual_base;
}


int unmap_physical(void* virtual_base, unsigned int span)
{
    if (munmap(virtual_base, span) != 0)
    {
        printf("ERROR: munmap() failed...\n");
        return (-1);
    }
    return 0;
}

