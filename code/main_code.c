//없을까봐 수정한 라이브러리:벡터, fstream,string
//수정 내역: string -> char*형, .clear -> memset 함수 이용, 벡터의 .size -> while문으로 구함
//유의해야할 변수: stNum,stName(이거 두개가 벡터형이었음)
//궁금한 것: 코드는 순차진행인데, 그럼 점멸 방식은 어떻게 보여주지?
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "address_map_arm.h"


#define MAX 500    //지하철 역, 버스 갯수 최대값
#define INF 99999 //연결되지 않음을 나타냄
#define FLAG 0
#define STRING_MAX 256


typedef struct _Station
{                          //주로 입,출력 받거나 노선도에서 결과 출력할 때 사용
    int line;              //역의 호선
    char name[STRING_MAX]; //역 이름
    int x, y;              //지도에서 해당 역의 좌표값(추후 이미지 처리 위해 필요)
} Station;

///전체 노선에 대한 변수///
Station allStation[MAX]; //모든 지하철 역 + 버스 정보를 저장한 배열
int stNum[MAX];          //각 호선에 있는 역 중 가장 마지막 인덱스 저장
char stName[MAX][MAX][STRING_MAX];  //string이 담긴 2차원 배열
int stcnt;               //다른 함수에서 stName과 stNum의 size알기 위해 선언
double weight[MAX][MAX]; //간선 간의 가중치 저장하는 배열
double distance[MAX];    //최단 경로의 가중치를  저장하는 배열
int path[MAX];           //시작점부터 끝점 사이 지나는 노드
int s[MAX];              //구하고자 하는 최단 경로 저장하는 배열(원래 stack역할)
int found[MAX];          //해당 정점이 최단 거리가 발견되었는지 판단
int index1;
int stLine; //몇호선인지

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


///stName엔 값 잘 들어간다.
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
            //여기 stNum.push_back()이랑 stNAme.push_back어케 바꾸
            stNum[i] = index1; //원래 stLine
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
  

        printf("(호선)출발역을 입력하세요:\n");
        scanf("%d %s", &line1, src);
        printf("(호선)도착역을 입력하세요:\n");
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

void init() //사용하는 배열 들 초기화
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
우리는 버스가 0호선이므로 원래 값에서 다 +1 해줌
================================================================*/
int findIndex(int line, char st[]) ////string형 st
{
    int idx = 0;
    int i = 0;
    //stName[line-1].size()값 알아내기
  
    int flag = 0;
    for (i; i < MAX; i++) //원래 stName을 vector<vector<string>>으로 생각
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
    if (line > 0) //원래는 line >1 dlaus stNum[line-2]를 더했었음
        idx += stNum[line - 1];
    if (flag == 0) printf(" 매개변수로 주어진 %s 역(line %d) 과 일치하는 stName의 위치를 못찾았음.\n",st,line);
    return idx;
}


///index를 받으면 무슨 호선인지 출력
int findLine(int idx)
{
    int i = 0;
    for (i; i < 10; i++)
    {
        if (idx < stNum[i])
            return i;
    }
   // return stNum[9]; //////////////이렇게 쓰는거 맞는지
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


void AddEdge(int line1, char st1[], int line2, char st2[], double we) //가중치 간선 추가
{
    int i, j;
    i = findIndex(line1, st1);
    j = findIndex(line2, st2);
 
    weight[i][j] = we;
    weight[j][i] = we;
}

int Choose(int n) //found[w]가 0이고 distance[u]가 distance[w] 중 최소일 때 반환
{
    double min = INF;
    int w = 0;
    int u = -1; //정점이 음수일 수는 없으므로 초기값을 -1
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
    //distance[j], 0<=j<n은 n개의 정점을 가진 방향 그래프 G에서
    //정점 v에서 정점 j까지의 최단 경로 길이로 설정됨
    //간선의 길이는 weight[i][j]로 주어
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

    for (i; i < n - 2; i++) //정점 v로부터 n-1개 경로를 결정
    {
        int u = Choose(n); //found[w]가 0이고 distance[u]가 distance[w] 중 최소일 때 반환
        found[u] = 1;
        int w = 0;
        for (w; w < n; w++)
            if (!found[w] && distance[u] + weight[u][w] < distance[w])
            {
                path[w] = u; //바뀌기 전에 저장
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
    idx = j; //도착역이 바뀌면 안되므로
    int z = 0;
   /*
    for (z; z < MAX; z++) {
        printf("path[%d] = %d(인덱스값)\n", z, path[z]);
    }
    */
    printf(" %s (line %d ) -> ", allStation[i].name, allStation[i].line);
    while (1) //도착역 직전역부터 출발역 다음역까지 stack에 넣고
    {
        if (path[idx] == -1)
            break;
        s[stcnt] = path[idx];
        stcnt++;
        idx = path[idx];
    }

    stcnt--;
    //pathToImage(s);
    //calculateTime(s,stcnt,i,j);////////////////출발지 인덱스와 목적지 인덱스 포함됨
    while (stcnt >= 0) //스택이 빌때까지 출력하면 순서가 맞다
    {
        int v = s[stcnt--];
        printf(" %s (line %d ) -> ", allStation[v].name, allStation[v].line);
    }
    printf("%s (line %d ) \n", allStation[j].name, allStation[j].line);
    double time = distance[j];
    double second = time - (int)time;
    printf("소요시간 : %d 분", (int)time);
    if (second)
        printf("30초");
    printf("\n");
    memset(s, 0, sizeof(s)); //s 다시 초기
}


/*
==============================================================
calculateTime(): 경로 중 거치는 최초 3개 호선의 소요 시간을 7-segment로 출력
                  경로 중 거치는 호선과 대응하는 LED 점등. 이 때, 현재 호선은 계속 점등, 바로 직후 호선은 깜빡거림으로 점등
        -path에 저장된 index로 cnt++한 다음, 만약 현재 정점이 환승 정점이면 지금까지 더했던 cnt를 7-segment로 출력함(cnt는 다시 0)
-참고:http://contents.kocw.or.kr/document/region/2010/16\02/16_02_09_micro.pdf
 https://www.kanda.com/7segment.php

===============================================================
*/
void calculateTime(int s[], int stcnt,int i, int j) { //여기에 그냥 LED 함수까지 추가하자)

    int i,id1,id2; //비교인자
    int getTime[20] = {0}; //경로 중 호선 별 소요 시간 출력
    int getLine[20] = { -1 }; //전체 경로 중 경유하는 호선 번호 출력
    char getName[MAX][STRING_MAX]; //전체 경로의 역 이름 및 flag유무(버스 이용 유무) 출력
    int IsBus[MAX]; //해당 경로가 버스 노선을 사용하는지 유무 출력(위의 getName과 동시에 봐야 함)
    int ind = 0; //getLine,getTime용 배열 인덱스
    int cnt = 0; //전체 경로의 역 개수
    //s[stcnt]에는 출발점과 끝점이 저장되어있지 않음. 이 점을 유의할 것!
    //s[stcnt]에는 없는 출발 점을 시작값으로 넣는다.
    
    id1 = i;
    getLine[ind] = findLine(i);
    strcpy(getName[cnt], findString(i));
    if (findLine(i) == 0)IsBus[cnt] = 1; else IsBus[cnt] = 0;
    cnt++;
    while(stcnt >=0) //s는 최단 경로를 지나치는 path들의 모임
    {
        id2 = s[stcnt--];
        strcpy(getName[cnt],findString(id2));
        if(abs(id2 - id1) > 1) //호선이 바뀌었다면
        {
            getLine[++ind] = findLine(id2); //바뀐 호선 정보를 getLine에 저장
            if (findLine(id2) == 0)IsBus[cnt] = 1;
            else IsBus[cnt] = 0;//바뀐 호선이 버스면 flag 체크 
        }
        else 
        {
            getTime[ind] += weight[id1][id2];
            IsBus[cnt] = 0;
        }
        cnt++;
        id1 = id2;
    }
    //도착지 역 확인
    strcpy(getName[cnt],findString(j));
    if (abs(id1 - j) > 1) //환승이 이루어 졌다면
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
    cnt++;//나중에 cnt로 for문 돌릴 때 보통 i <cnt라고 하기 때문에 cnt++함.




    //각 호선 별 걸리는 시간 출력(lab7 참고함)
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
        *HEX_ptr = seg7[(getTime[l]%10) & 0xF]; //일의 자리 수 출력
        *(HEX_ptr + 3) = seg7[(getTime[l]/10) & 0xF]; //10의 자리 수 출력
        HEX_ptr += 6;
        }

    //LED 점등: 현재 타고 있는 LED는 계속 켜져있고, 다음 노선은 깜빡거림. 오른쪽부터 순서대로 0~9(0번은 버스 노선)
    while (getLine[l] != -1) {
        int Line1 = getLine[l], wait = getTime[l], Line2 = getLine[++l];
        if (Line2 == -1) { //처음으로 -1이 나왔다는 것은 Line1이 마지막호선이라는 뜻. 이 때는 깜빡임 없이 Line1만 출력
            LEDR_ptr = oxff;//현재 라인을 LED로 출력
            sleep(wait);
        }
        else
        {
            while (wait > 0) {
                LEDR_ptr = ;//현재 호선, 다음 호선 둘 다 불 들어옴
                sleep(1);
                if (--wait == 0) break; //중간에 wait 신호 다 되면 끝냄
                LEDR_ptr = ;//현재 호선만 들어감
                sleep(1);
                wait--;
            }
        }
    //Line2구할때 이미 l++했으므로 따로 플러스 안해줘도 됨
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

