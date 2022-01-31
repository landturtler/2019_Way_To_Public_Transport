#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <intelfpgaup/video.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "address_map_arm.h"
#define PI 3.141592654
#define MAX 500   //지하철 역, 버스 갯수 최대값
#define INF 99999 //연결되지 않음을 나타냄
#define FLAG 0
#define STRING_MAX 256


typedef unsigned char byte;

//for 7-segment
char seg7[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
                 0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};

// The dimensions of the image
int width, height;
int screen_x, screen_y, char_x, char_y;

//subway
typedef struct _Station
{                          //주로 입,출력 받거나 노선도에서 결과 출력할 때 사용
    int line;              //역의 호선
    char name[STRING_MAX]; //역 이름
    int x, y;              //지도에서 해당 역의 좌표값(추후 이미지 처리 위해 필요)
} Station;
Station allStation[MAX];           //모든 지하철 역 + 버스 정보를 저장한 배열
int stNum[MAX];                    //각 호선에 있는 역 중 가장 마지막 인덱스+1 저장
char stName[MAX][MAX][STRING_MAX]; //string이 담긴 2차원 배열
int stcnt;                         //다른 함수에서 stName과 stNum의 size알기 위해 선언
double weight[MAX][MAX];           //간선 간의 가중치 저장하는 배열
double distance[MAX];              //최단 경로의 가중치를  저장하는 배열
int path[MAX];                     //시작점부터 끝점 사이 지나는 노드
int s[MAX];                        //구하고자 하는 최단 경로 저장하는 배열(원래 stack역할)
int found[MAX];                    //해당 정점이 최단 거리가 발견되었는지 판단
int index1;
int stLine; //몇호선인지
int notExist[25] = { 60, 175, 178, 205, 214, 242, 283, 298, 299, 313, 318, 325, 331,
333, 340, 342, 359, 364, 367, 385, 390, 401, 404, 426, 428 };//엘리베이터 없는 역의 index 저장
int OPTION;//장애인용 엘리베이터 고려 플래그

//Memory Functions
volatile int *LEDR_ptr; // virtual address pointer to red LEDs
volatile int *HEX0_ptr; // virtual address pointer to 7-segment
volatile int *HEX1_ptr; // virtual address pointer to 7-segment
volatile int *HEX2_ptr; // virtual address pointer to 7-segment
volatile int *HEX3_ptr; // virtual address pointer to 7-segment
volatile int *HEX4_ptr; // virtual address pointer to 7-segment
volatile int *HEX5_ptr; // virtual address pointer to 7-segment

int fd = -1;      // used to open /dev/mem for access to physical addresses
void *LW_virtual; // used to map physical addresses for the light-weight bridge

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

/*
 * Establish a virtual address mapping for the physical addresses starting at base, and
 * extending by span bytes.
 */
void *map_physical(int fd, unsigned int base, unsigned int span)
{
    void *virtual_base;

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

//Close the previously-opened virtual address mapping
int unmap_physical(void *virtual_base, unsigned int span)
{
    if (munmap(virtual_base, span) != 0)
    {
        printf("ERROR: munmap() failed...\n");
        return (-1);
    }
    return 0;
}

// Image Functions
struct pixel
{
    byte b;
    byte g;
    byte r;
};

inline void border(int *x, int *y, int w, int h)
{
    int x_tmp = *x;
    int y_tmp = *y;
    if (x_tmp < 0)
        x_tmp = 0;
    if (x_tmp >= w)
        x_tmp = w - 1;
    if (y_tmp < 0)
        y_tmp = 0;
    if (y_tmp >= h)
        y_tmp = h - 1;
    *x = x_tmp;
    *y = y_tmp;
}

inline int clip(int x, int min, int max)
{
    int x_tmp = x;
    if (x < min)
        x_tmp = min;
    if (x > max)
        x_tmp = max;
    return x_tmp;
}

int read_bmp(char *filename, byte **header, struct pixel **data)
{
    struct pixel *data_tmp;
    byte *header_tmp;
    FILE *file = fopen(filename, "rb");

    if (!file)
        return -1;

    // read the 54-byte header
    header_tmp = malloc(48 * sizeof(byte));
    fread(header_tmp, sizeof(byte), 48, file);

    // get height and width of image from the header
    width = *(int *)(header_tmp + 18);  // width is a 32-bit int at offset 18
    height = *(int *)(header_tmp + 22); // height is a 32-bit int at offset 22

    // Read in the image
    int size = width * height;
    data_tmp = malloc(size * sizeof(struct pixel));
    fread(data_tmp, sizeof(struct pixel), size, file); // read the data
    fclose(file);

    *header = header_tmp;
    *data = data_tmp;

    printf("BMP file name: %s\n", filename);
    printf(" - Image size: %d x %d\n", width, height);
    printf(" - Header: %s\n", (char *)header_tmp);

    return 0;
}

void convert_to_red(struct pixel *data, int i, int j)
{
    int x, y;
    j = height - j;
    struct pixel(*image)[width] = (struct pixel(*)[width])data;
    for (y = j - 8; y <= j + 8; y++)
    {
        for (x = i - 8; x <= i + 8; x++)
        {
            image[y][x].r = 255;
            image[y][x].g = 0;
            image[y][x].b = 0;
        }
    }
}

void draw_red_line(struct pixel *data, int *a, int *b)
{
    int flag1 = 0, flag2 = 0;
    flag1 = b[0] > a[0] ? 1 : -1; //a.x<b.x이면 1,a>=b이면 -1
    flag2 = b[1] > a[0] ? 1 : -1; //a.y < b.y이면 1, 아니면 -1

    int X = 0, Y = 0, remainToAdd = 1, remainCnt = 0; //시작 좌표는 x좌표가 왼쪽인 점
    float slop = 0, remain = 0;

    if (flag1 == 1)
        X = a[0], Y = a[1];
    else
        X = b[0], Y = b[1];
    slop = (a[1] - b[1]) / (a[0] - b[0]);
    remain = slop - (int)slop; //정수로 바꾸느라 없어지는 부분 체크. 나머지 부분 모아서 합이 1이 넘을 경우엔 y좌표 +1 시켜줌
    while (remainToAdd * remain < 1)
        remainToAdd++;
    int check = a[0];
    ////startX,startY좌표를 빨간색 처리함
    convert_to_red(data, X, Y);
    while (check!=b[0])
    {
        X++;
        Y = Y + (int)slop;
        remainCnt++;
        if (remainCnt == remainToAdd)
        {
            Y++;
            remainCnt = 0;
        }

        //X,Y좌표부분 빨간색 처리
        convert_to_red(data, X, Y);
        check++;
    }
}

void write_bmp(char *filename, byte *header, struct pixel *data)
{
    FILE *file = fopen(filename, "wb");
    struct pixel(*image)[width] = (struct pixel(*)[width])data;
    fwrite(header, sizeof(byte), 54, file);
    int y, x;

    // the r field of the pixel has the grayscale value; copy to g and b.
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            image[y][x].b = image[y][x].r;
            image[y][x].g = image[y][x].r;
        }
    }
    int size = width * height;
    fwrite(image, sizeof(struct pixel), size, file); // write the data
    fclose(file);
}

void write_signed_bmp(char *filename, byte *header, signed int *data)
{
    FILE *file = fopen(filename, "wb");
    struct pixel bytes;
    int val;

    signed int(*image)[width] = (signed int(*)[width])data; // allow image[][]
    // write the 54-byte header
    fwrite(header, sizeof(byte), 54, file);
    int y, x;

    // convert the derivatives' values to pixels by copying each to an r, g, and b
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            val = abs(image[y][x]);
            val = (val > 255) ? 255 : val;
            bytes.r = val;
            bytes.g = bytes.r;
            bytes.b = bytes.r;
            fwrite(&bytes, sizeof(struct pixel), 1, file); // write the data
        }
    }
    fclose(file);
}

//allStation[i].name을 비교해서 index찾고 해당 index의 x,y좌표 출력
int *findPos(char na[STRING_MAX])
{
    int i = 0, j = 0, flag = 0;
    for (; i < MAX; i++)
    {
        char temp[STRING_MAX];
        strcpy(temp, allStation[i].name);
        if (strcmp(na, temp) == 0)
        {
            flag = 1;
            break;
        }
    }
    if (flag == 0)
        printf("매개변수인 %s의 위치를 allStation에서 못찾음. 역 이름을 확인해주세요.\n", na);
    int getx = 0, gety = 0;
    getx = allStation[i].x;
    gety = allStation[i].y;
    int ret[2] = {getx, gety};
    return ret;
}
// Render an image on the VGA display
void draw_image(struct pixel *data)
{
    int x, y, stride_x, stride_y, i, j, vga_x, vga_y;

    int r, g, b, color;
    struct pixel(*image)[width] = (struct pixel(*)[width])data; // allow image[][]

    video_clear();
    // scale the image to fit the screen
    stride_x = (width > screen_x) ? (width / screen_x) : 1;
    stride_y = (height > screen_y) ? (height / screen_y) : 1;
    // scale proportionally (don't stretch the image)
    stride_y = (stride_x > stride_y) ? stride_x : stride_y;
    stride_x = (stride_y > stride_x) ? stride_y : stride_x;
    for (y = 0; y < height; y += stride_y)
    {
        for (x = 0; x < width; x += stride_x)
        {
            // find the average of the pixels being scaled down to the VGA resolution
            r = 0;
            g = 0;
            b = 0;
            for (i = 0; i < stride_y; i++)
            {
                for (j = 0; j < stride_x; ++j)
                {
                    r += image[y + i][x + j].r;
                    g += image[y + i][x + j].g;
                    b += image[y + i][x + j].b;
                }
            }
            r = r / (stride_x * stride_y);
            g = g / (stride_x * stride_y);
            b = b / (stride_x * stride_y);

            // now write the pixel color to the VGA display
            r = r >> 3; // VGA has 5 bits of red
            g = g >> 2; // VGA has 6 bits of green
            b = b >> 3; // VGA has 5 bits of blue
            color = r << 11 | g << 5 | b;
            vga_x = x / stride_x;
            vga_y = y / stride_y;
            if (screen_x > width / stride_x) // center if needed
                video_pixel(vga_x + (screen_x - (width / stride_x)) / 2, (screen_y - 1) - vga_y, color);
            else if ((vga_x < screen_x) && (vga_y < screen_y))
                video_pixel(vga_x, (screen_y - 1) - vga_y, color);
        }
    }
    video_show();
}

//subway
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

//버스 : 0호선
int findIndex(int line, char st[]) ////string형 st
{
    int idx = 0;
    int i = 0;
    //stName[line-1].size()값 알아내기

    int flag = 0;
    for (; i < MAX; i++) //원래 stName을 vector<vector<string>>으로 생각
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
    if (flag == 0)
         printf(" 매개변수로 주어진 %s 역(line %d) 과 일치하는 stName의 위치를 찾지못함. 호선 혹은 역 이름을 다시한번 확인해주세요.\n", st, line);
    return idx;
}

///index를 받으면 무슨 호선인지 출력
int findLine(int idx)
{
    int i = 0;
    for (; i < 10; i++)
    {
        if (idx < stNum[i])
            return i;
    }
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
    for (; w < n; w++)
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
    for (; j < n; j++)
    {
        found[j] = 0;
        path[j] = -1;
        distance[j] = weight[v][j];
    }
    found[v] = 1;
    distance[v] = 0;

    for (; i < n - 2; i++) //정점 v로부터 n-1개 경로를 결정
    {
        int u = Choose(n); //found[w]가 0이고 distance[u]가 distance[w] 중 최소일 때 반환
        found[u] = 1;
        int w = 0;
        for (; w < n; w++)
            if (!found[w] && distance[u] + weight[u][w] < distance[w])
            {
                path[w] = u; //바뀌기 전에 저장
                distance[w] = distance[u] + weight[u][w];
            }
    }
}

//for calculateTime
int id1, id2;                  //비교인자
int getTime[20] = { 0 };         //경로 중 호선 별 소요 시간 출력
int getLine[20] = { -1 };        //전체 경로 중 경유하는 호선 번호 출력
char getName[MAX][STRING_MAX]; //전체 경로의 역 이름 및 flag유무(버스 이용 유무) 출력
int IsBus[MAX];                //해당 경로가 버스 노선을 사용하는지 유무 출력(위의 getName과 동시에 봐야 함)
int ind = 0; //getLine,getTime용 배열 인덱스
int cnt = 0; //전체 경로의 역 개수
//s[stcnt]에는 출발점과 끝점이 저장되어있지 않음. 이 점을 유의할 것!
//s[stcnt]에는 없는 출발 점을 시작값으로 넣는다.

void calculateTime(int stc, int i, int j) //i에서 j까지 걸리는 시간 및 호선 계산
{
    id1 = i;
    getLine[ind] = findLine(i);
    strcpy(getName[cnt], allStation[i].name);
    if (findLine(i) == 0)
        IsBus[cnt] = 1;
    else
        IsBus[cnt] = 0;
    cnt++;
    while (stc >= 0) //s는 최단 경로를 지나치는 path들의 모임
    {
        id2 = s[stc--];
        strcpy(getName[cnt], allStation[id2].name);
        if (weight[id1][id2] == 1) //호선이 바뀌었다면
        {
            getLine[++ind] = findLine(id2); //바뀐 호선 정보를 getLine에 저장
            if (findLine(id2) == 0)
                IsBus[cnt] = 1;
            else
                IsBus[cnt] = 0; //바뀐 호선이 버스면 flag 체크
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
    strcpy(getName[cnt], allStation[j].name);
    if (weight[j][id2] == 1) //환승이 이루어 졌다면
    {
        if (findLine(j) == 0)
            IsBus[cnt] = 1;
        else
            IsBus[cnt] = 0;
        getLine[++ind] = findLine(j);
    }
    else
    {
        getTime[ind] += weight[id1][j];
        IsBus[cnt] = 0;
    }
    cnt++; //나중에 cnt로 for문 돌릴 때 보통 i <cnt라고 하기 때문에 cnt++함.
}



///////수정함
void result(int line1, char src[], int line2, char dst[])
{
    int i = 0, j = 0, idx = 0, stcnt = 0, check = 0;

        i = findIndex(line1, src);
        j = findIndex(line2, dst);
        if (OPTION == 1) {
            for (check; check < 25; check++) {
                if (i == notExist[check]) {
                    printf("출발지: %d호선 %s역은 엘리베이터 1역 1동선 확보역사가 아닙니다.\n", allStation[i].line, allStation[i].name);
                    printf("출발지:%s ->%s 로 변경합니다.\n", allStation[i].name, allStation[i - 1].name);
                    i--;
                    break;
                }
                else if (j == notExist[check]) {
                    
                    printf("도착지: %d호선 %s역은 엘리베이터 1역 1동선 확보역사가 아닙니다.\n", allStation[j].line, allStation[j].name);
                    printf("도착지:%s ->%s 로 변경합니다.\n", allStation[j].name, allStation[j - 1].name);
                    j--;
                    break;
                }
            }
        }
    ShortestPath(MAX, i);
    idx = j; //도착역이 바뀌면 안1되므로
    printf(" %s (line %d ) ->\n", allStation[i].name, allStation[i].line);
    while (1) //도착역 직전역부터 출발역 다음역까지 stack에 넣고
    {
        if (path[idx] == -1)
            break;
        s[stcnt] = path[idx];
        stcnt++;
        idx = path[idx];
    }

    stcnt--;
    int st = stcnt;
    calculateTime(st, i, j); ////////////////출발지 인덱스와 목적지 인덱스 포함됨
    while (stcnt >= 0)          //스택이 빌때까지 출력하면 순서가 맞다
    {
        int v = s[stcnt--];
        printf("%s (line %d ) ->\n ", allStation[v].name, allStation[v].line);
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

int expo(int a)
{
    int s = 1;
    s = s << (a - 1);
    return s;
}

int main(int argc, char *argv[])
{
    int mm = 0;
    printf("기본 빠른 길은 0, 교통 약자를 위한 빠른 길은 1을 선택하십시오:\n");
    scanf("%d", &mm);
    OPTION = mm;
    //find path
    init();
    char src[STRING_MAX] = "\0", dst[STRING_MAX] = "\0"; // , s3[STRING_MAX] = "\0", s4[STRING_MAX] = "\0";
    int line1 = 0, line2 = 0;

    FILE *fp = NULL;
    fp = fopen("input.txt", "r");
    //int cnt = 0;
    if (fp != NULL)
    {
        int numLine = 0, numStation = 0;
        if (fscanf(fp, "%d\n", &numLine) < 0)
            printf("fscanf failed 1\n");
        int i, j;
        for (i = 0; i < numLine; i++)
        {
            if (fscanf(fp, "%d\n", &numStation) < 0)
                printf("fscanf failed \n");
            for (j = 0; j < numStation; j++)
            {
                Station *station;
                station = (Station *)malloc(sizeof(Station));
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
                AddEdge(line1, s3, line2, s4, 2);
            else
                AddEdge(line1, s3, line2, s4, 1);
        }
        fclose(fp);

        //장애인 전용 빠른길일 경우, 엘리베이터 없는 역의 환승 가중치를 INF로 바꿈)
        if (OPTION == 1) {
            weight[62][178] = INF; //신설동 1->2 환승 삭제
            weight[178][62] = INF; //신설동 2->1 환승 삭제, 청량리(1,2), 신설동(1,2), 충무로(3,4), 교대(2,3), 까치산(2,5), 종로3가(1,3,5), 고속터미널(3,7,9)
            weight[205][241] = INF; //충무로 3->4 환승 삭제
            weight[241][205] = INF; //충무로 4->3 환승 삭제
            weight[154][214] = INF; //교대 2->3 환승 삭제
            weight[214][154] = INF; //교대 3->2 환승 삭제
            weight[182][283] = INF; //까치산 2->5 환승 삭제
            weight[283][182] = INF; //까치산 5->2 환승 삭제
            weight[55][299] = INF; // 종로3가 1->5 환승 삭제
            weight[299][55] = INF; //종로3가 5->1 환승 삭제
            weight[203][299] = INF; //종로3가 3->5 환승 삭제
            weight[299][203] = INF; //종로3가 5->3 환승 삭제
            weight[213][390] = INF; //고터 3->7 환승 삭제
            weight[390][213] = INF; //고터 7->3 환승 삭제
            weight[390][455] = INF; //고터 7->9 환승 삭제
            weight[455][390] = INF; //고터 9->7 환승 삭제
        }

        if (OPTION == 1)
         printf("======교통 약자를 위한 편한 길 찾기======\n");
        else 
         printf("=============빠른 길 찾기================ \n");


        printf("(호선)출발역을 입력하세요:\n");
        scanf("%d %s", &line1, src);
        printf("(호선)도착역을 입력하세요:\n");
        scanf(" %d %s", &line2, dst);
        printf("============================================ \n");
        result(line1, src, line2, dst);

        //image processing
        struct pixel *image;
        byte *header;
        // Open input image file (24-bit bitmap image)
        if (read_bmp("subway.bmp", &header, &image) < 0)
        {
            printf("Failed to read BMP\n");
            return 0;
        }
        if (!video_open())
        {
            printf("Error\n");
            return -1;
        }
        video_read(&screen_x, &screen_y, &char_x, &char_y);
        draw_image(image);

        for (i = 0; i < cnt; i++)
        {
            int *a, *b;
            char ss[STRING_MAX] = "\0";
            strcpy(ss, getName[i]);
            a = findPos(ss);
            if (IsBus[i])
            {
                strcpy(ss, getName[i + 1]);
                b = findPos(ss);
                draw_red_line(image, a, b);
            }
            else
            {
                convert_to_red(image, a[0], a[1]);
            }
        }
        //write_signed_bmp("stage_red.bmp", header, image);
        //write_bmp("stage_red.bmp", header, image);

        draw_image(image);
        //        write_bmp("edges.bmp", header, image);

        //Memory
        // Create virtual memory access to the FPGA light-weight bridge
        if ((fd = open_physical(fd)) == -1)
            return (-1);
        if ((LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)) == NULL)
            return (-1);

        LEDR_ptr = (unsigned int *)(LW_virtual + LEDR_BASE);
        HEX0_ptr = (unsigned int *)(LW_virtual + HEX3_HEX0_BASE);
        HEX4_ptr = (unsigned int *)(LW_virtual + HEX5_HEX4_BASE);

        for (i = 0; i < 17; i++)
        {
            int d1 = getTime[i];
            if (d1 == 0)
                break;
            int d2 = getTime[i + 1];
            int d3 = getTime[i + 2];
            int blink = 0;
            int b1 = getLine[i];
            int b2 = getLine[i + 1];
            int b3 = getLine[i + 2];
            if (b1 == b3)
                b3 = 0;
            while (d1 != 0)
            {
                *HEX4_ptr = (seg7[d1 / 10] << 8) | seg7[d1 % 10];
                *HEX0_ptr = (seg7[d2 / 10] << 24) | (seg7[d2 % 10] << 16) | (seg7[d3 / 10] << 8) | seg7[d3 % 10];
                *LEDR_ptr = *LEDR_ptr + expo(b1) + 1;
                if (blink % 2 == 0 && blink % 3 == 0)
                    *LEDR_ptr = expo(b1) + expo(b2) + expo(b3);
                else if (blink % 2 == 0)
                    *LEDR_ptr = expo(b1) + expo(b2);
                else if (blink % 3 == 0)
                    *LEDR_ptr = expo(b1) + expo(b3);
                else
                    *LEDR_ptr = expo(b1);
                d1--;
                blink++;
                sleep(1);
            }
        }

        //end memory
        unmap_physical(LW_virtual, LW_BRIDGE_SPAN); // release the physical-memory mapping
        close_physical(fd);                         // close /dev/mem
    }
    else
    {
        printf("input.txt is not exit\n");
    }

    return 0;
}