#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <regex.h>

#define ENTRY_SIZE 32
#define FILE_SYSTEM_SIZE 1000


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char uchar;


/************************** STRUCT DEFINITION ****************************/
/*************************************************************************/

/**
 * File node
 * 文件节点，根据type决定是文件夹或文件
 */
typedef struct FileNode {
    char name[20];              // 文件名
    int type;            // 0:dir,1:file，-1:split node
    u32 size;
    int start_pos;       // start
    int sub_pos;         // start of sub dir, . and .. is 0
    u16 cluster;         // 簇号
    struct FileNode *child; // 子女节点
    struct FileNode *sibling;  // 兄弟节点
    struct FileNode *parent;   // parent node
    int fileNum; // 文件数
    int dirNum; // 目录数
}fileNode;

/**
 * FAT12文件系统结构
 */
#pragma pack(1) // align by 1 byte
typedef struct FAT12 {
    u16 BPB_BytesPerSector; // 每个扇区的字节数
    u8 BPB_SecPerClusters;  // 每个簇的扇区数
    u16 BPB_RsvSecCnt;      // Boot record占用的扇区数
    u8 BPB_NumFATs;         // FAT的数量,一般为2
    u16 BPB_RootEntCnt;     // 根目录文件数的最大值
    u16 BPB_TotSec16;       // 扇区数
    u8 BPB_Media;           // 媒体描述符
    u16 BPB_FATSz16;        // 一个FAT的扇区数
    u16 BPB_SecPerTrk;      // 每个磁道扇区数
    u16 BPB_NumHeads;       // 磁头数
    u32 BPB_HiddenSec;      // 隐藏扇区数
    u32 BPB_TotSec32;       // 如果BPB_TotSec16是0，则在这里记录
}BPB;
#pragma pack() // restore alignment

/**
 * Entry
 * 根目录入口
 */
struct RootEntry {
    char name[11];//8.3 file name
    uchar classification; //文件属性，0x10表示目录,0x20表示文件
    uchar reserves[10]; //保留位
    u16 wrtTime;
    u16 wrtDate;
    u16 fstCluster;
    u32 fileSize;
};



struct FileNode files[FILE_SYSTEM_SIZE]; // 最多1000个文件
BPB header;                     // FAT12
int RsvSec;                         // 引导扇区用的扇区数
int Bytes_Per_Sector;               // 每个扇区的字节数
int Root_Entry_Cnt;                 // 根目录文件数
int Root_Sec;                       // root sector
int NumFATs;                // number of FAT
int FATSz;                  // FAT size
int Pre_Sec;                    // pre sector


//nasm的方法
void my_printRed(char *str, int length);   //打印
void my_printWhite(char *str, int length); //打印

void printRed(char *str, int length) {
    my_printRed(str, length);
}
void printWhite(char *str, int length) {
    my_printWhite(str, length);
}


void getFAT12Info(FILE *fat12);

/**
 *
 * @param fat12
 * @param num
 * @return 数据区对应的簇号，还要乘512的那种
 */
int GetCluster(FILE *fat12, int num) ;

char* transNumToChar(int num);

void printNum(int num) {
    char * temp = transNumToChar(num);
    printWhite(temp,(int)strlen(temp));
    free(temp);
}

void readAndConstructTree(FILE *fat12, int address, int file_count, fileNode* current);

void initFileNode(fileNode* node){
    node->child = NULL;
    node->sibling = NULL;
    node->parent = NULL;
    node->type = -1;
    node->size = 0;
    node->start_pos = 0;
    node->sub_pos = 0;
    node->cluster = 0;
    node->fileNum = 0;
    node->dirNum = 0;
    memset(node->name,0,20);
}



void ls(FILE *fat12,fileNode *current,char* input);

//通过回溯current的父节点，来得到current的路径
char* getPath(fileNode*current);

// 当用户添加 -l 参数时，
//1. 在路径名后，冒号前，另输出此目录下直接子目录和直接子文件的数目，两
//个数字之间用空格连接。两个数字不添加特殊颜色
//2. 每个文件/目录占据一行，在输出文件/目录名后，空一格，之后：
//1. 若项为目录，输出此目录下直接子目录和直接子文件的数目，两个数字
//之间用空格连接。这两个数字不添加特殊颜色
//1. 不输出.和..目录的子目录和子文件数目
//2. 若项为文件，输出文件的大小（以字节为单位）
void doLsWithParam(fileNode* current,char* path,char* preOuput);

void doLsWithoutParam(fileNode *current,char* path,char* preOuput);

void cat(FILE *fat12,fileNode *current,char* input);

void freeNode(fileNode *node){
    if (node->child!=NULL){
        freeNode(node->child);
    }
    if (node->sibling!=NULL){
        freeNode(node->sibling);
    }
    free(node);
}

int main(){
    FILE *fat12 = fopen("a.img", "rb");//打开文件
    getFAT12Info(fat12);
    fileNode *root = (fileNode *)malloc(sizeof(fileNode));
    initFileNode(root);
    root->type = 0;
    root->size = 0;
    root->parent = root;
    //读取名字时要将后面的空格去掉
    strcpy(root->name, ".");
    strcat(root->name, "\0");
    //先从根目录开始读取
    readAndConstructTree(fat12, Pre_Sec, Root_Entry_Cnt, root);

    while (1) {
        char input[100];
        printWhite("请输入指令（ls或cat或exit）:",strlen("请输入指令（ls或cat或exit）:") );
        //这里输入的指令是整串的，如ls -l /root/nju/software
        fgets(input, sizeof(input), stdin); // 读取一行输入
        // 删除换行符
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        //这里要解析指令，然后根据指令执行相应的操作
        if (strcmp(input, "exit") == 0) {
            freeNode(root);
            break;
        }
        char input_copy[sizeof(input)];
        strcpy(input_copy, input);
        char *order = strtok(input_copy, " ");
        //这里在比对第一个指令是ls还是cat，如果都不是就报错
        if (strcmp(order, "ls") == 0) {
            ls(fat12, root, input);
        } else if (strcmp(order, "cat") == 0) {
            cat(fat12, root, input);
        } else {
            printWhite("输入指令错误\n",strlen("输入指令错误\n"));
        }
        fflush(stdout);  // 清空输出缓冲区
    }
    return 0;
}

int GetCluster(FILE *fat12, int num) {
    //基地址
    int fat_base = RsvSec * Bytes_Per_Sector;
    //偏移地址
    int fat_pos = fat_base + num * 3 / 2;
    //奇数和偶数FAT项分开处理
    int type = num % 2;

    u16 bytes;
    u16 *bytes_ptr = &bytes;

    // 读两字节
    fseek(fat12, fat_pos, SEEK_SET);
    fread(bytes_ptr, 1, 2, fat12);

    // type: 0, 取低12位
    // type: 1, 取高12位
    if (type == 0) {
        return bytes & 0x0fff; //低12位
    } else {
        return bytes >> 4; // 高12位
    }
}

void getFAT12Info(FILE *fat12){
    struct FAT12 *head_ptr = &header;
    fseek(fat12, 11, SEEK_SET);//将文件指针定位到文件开头
    fread(head_ptr, sizeof(header), 1, fat12);//读取文件头
    Bytes_Per_Sector = header.BPB_BytesPerSector;//每个扇区的字节数
    RsvSec = header.BPB_RsvSecCnt;//Boot record占用的扇区数
    Root_Entry_Cnt = header.BPB_RootEntCnt;//根目录文件数
    NumFATs = header.BPB_NumFATs;//FAT的数量,一般为2
    FATSz = header.BPB_FATSz16;//一个FAT的扇区数
    Root_Sec = (Root_Entry_Cnt * ENTRY_SIZE + Bytes_Per_Sector - 1) / Bytes_Per_Sector;//根目录所占的扇区数
    Pre_Sec = RsvSec + NumFATs * FATSz;//根目录前的扇区数
}

void readAndConstructTree(FILE *fat12, int address, int file_count, fileNode* current){
    //这里模拟的是读入一个目录的过程，在遇到有子目录的情况下再递归读入其他文件和目录
    //在递归读入的时候建立树结构
    struct RootEntry rootEntry;
    struct RootEntry *rootEntry_ptr = &rootEntry;
    for (int i = 0; i < file_count; ++i) {//读取根目录下的文件
        fseek(fat12,address * Bytes_Per_Sector + i*ENTRY_SIZE, SEEK_SET);
        fread(rootEntry_ptr, 1, ENTRY_SIZE, fat12);
        //处理rootEntry_ptr的数据
        if (rootEntry_ptr->classification!=0x10 && rootEntry_ptr->classification!=0x20) {
            continue;
        }
        int isFileOrDir = 1;
        for (int j = 0; j < 8; ++j) {
            if ( rootEntry_ptr->name[j] != ' ' && (rootEntry_ptr->name[j]<'0'||('9'<rootEntry_ptr->name[j]&& rootEntry_ptr->name[j] <'A')||('Z'<rootEntry_ptr->name[j] && rootEntry_ptr->name[j] <'a')||'z'<rootEntry_ptr->name[j])) {
                isFileOrDir = 0;
                break;
            }
        }
        if (!isFileOrDir)continue;
        if (rootEntry_ptr->classification == 0x10) {//是目录
            current->dirNum++;
            fileNode *dir = (fileNode *)malloc(sizeof(fileNode));
            initFileNode(dir);
            dir->type = 0;
            dir->size = rootEntry_ptr->fileSize;
            dir->start_pos = address;
            dir->cluster = rootEntry_ptr->fstCluster;
            dir->parent = current;
            for (int j = 0; j < 8; ++j) {//读取名字时要将后面的空格去掉
                if (rootEntry_ptr->name[j] == ' ') {
                    dir->name[j] = '\0';
                    break;
                }
                dir->name[j] = rootEntry_ptr->name[j];
            }
            if(current->child == NULL){
                current->child = dir;
            }else{
                fileNode *temp = current->child;
                while(temp->sibling != NULL){
                    temp = temp->sibling;
                }
                temp->sibling = dir;
            }
            readAndConstructTree(fat12, Pre_Sec + Root_Sec + rootEntry_ptr->fstCluster - 2,
                                 Bytes_Per_Sector / ENTRY_SIZE, dir);
        } else if (rootEntry_ptr->classification == 0x20) {//是文件
            current->fileNum++;
            fileNode *file = (fileNode *)malloc(sizeof(fileNode));
            initFileNode(file);
            file->type = 1;
            file->size = rootEntry_ptr->fileSize;
            file->start_pos = address;
            file->cluster = rootEntry_ptr->fstCluster;
            file->parent = current;
            int fileNameIdex = 0;//文件名的长度
            for (int j = 0; j < 8; ++j) {//读取名字时要将后面的空格去掉
                if (rootEntry_ptr->name[j] == ' ') {
                    file->name[fileNameIdex++] = '.';
                    break;
                }
                file->name[fileNameIdex++] = rootEntry_ptr->name[j];
            }
            for (int j = 8; j < 11; ++j) {//读取扩展名时要将后面的空格去掉
                if (rootEntry_ptr->name[j] == ' ') {
                    file->name[fileNameIdex++] = '\0';
                    break;
                }
                file->name[fileNameIdex++] = rootEntry_ptr->name[j];
            }
            if(current->child == NULL){
                current->child = file;
            }else{
                fileNode *temp = current->child;
                while(temp->sibling != NULL){
                    temp = temp->sibling;
                }
                temp->sibling = file;
            }
        }
    }
}

void ls(FILE *fat12,fileNode *current,char* input){
    //这里是ls的操作
    //这里要解析路径，然后根据路径找到对应的文件夹或文件
    //这里要注意的是路径是绝对路径，所以要从根目录开始找
    //这里要注意的是路径是绝对路径，所以要从根目录开始找
    //不添加-l参数时， 对每个目录路径，首先输出路径名，加一个冒号:，换行，再输出路径下的文件和目录列表。路径下的目录包括 . 和 .. （参考Linux系统中的 ls -a 命令）
    //使用红色(\033[31m)颜色输出目录名，不添加特殊颜色输出文件的文件名。
    //当用户不添加任何选项执行ls命令时，每个文件/目录项之间用两个空格隔开。
    //还要注意 对于 -l 参数用户可以在命令的任何位置、任意多次地设置-l参数，但只能设置一次路径名
    char *path = NULL;
    int hasPath = 0;
    int hasParam = 0;
    char input_copy[sizeof (input)];
    strcpy(input_copy,input);
    strtok(input_copy," ");//消耗掉ls
    char *temp = strtok(NULL," ");
    while(temp!=NULL){
        if (temp[0]=='-'){//说明是参数，参数能有多个
            int len = (int)strlen(temp);//参数长度，下面用于判断“-”后是否只有“l”
            for (int i = 1; i < len; ++i) {
                if (temp[i]!='l'){
                    printWhite("ls输入了不支持的命令参数\n",strlen("ls输入了不支持的命令参数\n"));
                    return;
                }
            }
            hasParam = 1;
        } else if (temp[0] == '/'){//说明是路径，路径只能有一个
            if (hasPath){
                printWhite("ls输入了多个路径\n",strlen("ls输入了多个路径\n"));
                return;
            }
            path = (char *)malloc(sizeof(temp));
            strcpy(path,temp);
            hasPath = 1;
        }else{
            printWhite("ls输入了不能解析的内容\n",strlen("ls输入了不能解析的内容\n"));
        }
        temp = strtok(NULL," ");
    }
        char *preOutput = (char *)malloc(100);


    //输出部分
    if (hasParam){
        doLsWithParam(current,path,preOutput);
    } else{
        doLsWithoutParam(current,path,preOutput);
    }

}

void cat(FILE *fat12,fileNode *current,char* input) {
    // 用户输入 cat 文件名 ，输出文件名对应文件的内容，若路径不存在或不是一个普通
    //文件则报错，给出提示，提示内容不做严格限定，但必须体现出错误所在。
    strtok(input, " ");
    char *path = strtok(NULL, " ");
    char * FileOrDir = strtok(path, "/");
    while(strchr(FileOrDir,'.')==NULL&& FileOrDir[0]!='.'){//此时说明是目录
        if (strcmp(FileOrDir,"..")==0){
            current = current->parent;
        }else if (strcmp(FileOrDir,".")==0){
            current = current;
        } else{//这里要匹配对应的目录
            //如果匹配到了就进入下一级目录,如果没有匹配到就报错
            fileNode * temp;
            temp = current->child;
            while(temp!=NULL){
                if(strcmp(temp->name,FileOrDir)==0){
                    current = temp;
                    break;
                }
                temp = temp->sibling;
            }
            if (temp ==NULL){
                printWhite("cat输入路径不存在\n",strlen("cat输入路径不存在\n"));
                return;
            }
            if (temp->type!=0){
                printWhite("cat输入路径不是一个目录\n",strlen("cat输入路径不是一个目录\n"));
                return;
            }
            current = temp;
        }
        FileOrDir=strtok(NULL,"/");
    }
    //此时说明是文件
    fileNode *temp = current->child;
    while(temp!=NULL){
        if(strcmp(temp->name,FileOrDir)==0){
            current = temp;
            break;
        }
        temp = temp->sibling;
    }
    if (temp ==NULL){
        printWhite("cat的文件不存在\n",strlen("cat的文件不存在\n"));
        return;
    }
    printWhite("\n",strlen("\n"));
    //输出文件内容
    u16 cluster = current->cluster;
    u16 nextCluster = GetCluster(fat12,cluster);
    u32 size = current->size;
    while( size>0){
        fseek(fat12, (RsvSec + NumFATs * FATSz + Root_Sec + (cluster - 2) ) * Bytes_Per_Sector, SEEK_SET);
        char *buf = (char *)malloc(Bytes_Per_Sector);
        fread(buf, 1, Bytes_Per_Sector, fat12);
        if(size<Bytes_Per_Sector){
            printWhite(buf,(int )size);
            size = 0;
            break;
        }
        printWhite(buf,Bytes_Per_Sector);
        size -= Bytes_Per_Sector;
        cluster = nextCluster;
        nextCluster = GetCluster(fat12,cluster);
    }
}

void doLsWithParam(fileNode* current,char* path,char* preOuput){//这里是目录，会递归调用
    if (path != NULL){
        //这里进行path地址寻址
        char * FileOrDir = strtok(path, "/");
        while(FileOrDir!=NULL){//此时说明是目录
            if (strcmp(FileOrDir,"..")==0){
                current = current->parent;
            }else if (strcmp(FileOrDir,".")==0){
                current = current;
            } else{//这里要匹配对应的目录
                //如果匹配到了就进入下一级目录,如果没有匹配到就报错
                fileNode * temp;
                temp = current->child;
                while(temp!=NULL){
                    if(strcmp(temp->name,FileOrDir)==0){
                        current = temp;
                        break;
                    }
                    temp = temp->sibling;
                }
                if (temp ==NULL){
                    printWhite("ls输入路径不存在\n",strlen("ls输入路径不存在\n"));
                    return;
                }
                if (temp->type!=0){
                    printWhite("ls输入路径不是一个目录\n",strlen("ls输入路径不是一个目录\n"));
                    return;
                }
                current = temp;
            }
            FileOrDir=strtok(NULL,"/");
        }

        //递归获得路径名
        strcpy(preOuput,getPath(current));
    }

    //如果是根目录
    if (strcmp(current->name,".")==0){
        strcat(preOuput,"/");
        printWhite(preOuput,(int )strlen(preOuput));
        printWhite(" ",1);
        printNum(current->dirNum);
        printWhite(" ",1);
        printNum(current->fileNum);
        printWhite(":\n",strlen(":\n"));
    }else{
        strcat(preOuput,"/");
        printWhite(preOuput,(int )strlen(preOuput));
        printWhite(":\n",strlen(":\n"));
    }

    if (strcmp(current->name,".")!=0){//如果不是根目录，就要输出“.”和“..”，注意这里要输出红色的
        printRed(".",(int )strlen("."));
        printWhite("\n", strlen("\n"));
        printRed("..",(int )strlen(".."));
        printWhite("\n", strlen("\n"));
    }

    //第一次遍历
    fileNode *temp = current->child;
    while(temp!=NULL){
        if (temp->type == 0){
            printRed(temp->name,(int )strlen(temp->name));//注意这里要输出红色的
            printWhite(" ",1);
            printNum(temp->dirNum);
            printWhite(" ",1);
            printNum(temp->fileNum);
            printWhite("\n",strlen("\n"));
        }else{
            printWhite(temp->name,(int )strlen(temp->name));
            printWhite(" ",1);
            printNum((int)temp->size);
            printWhite("\n",strlen("\n"));
        }
        temp = temp->sibling;
    }
    printWhite("\n",strlen("\n"));


    //第二次遍历
    temp = current->child;
    while (temp!=NULL){
        if (temp->type == 0){
            char* preOuputCopy = (char *)malloc(sizeof(preOuput));
            strcpy(preOuputCopy,preOuput);
            doLsWithParam(temp,NULL,strcat(preOuputCopy,temp->name));
        }
        temp = temp->sibling;
    }
    fflush(stdout);  // 清空输出缓冲区
    free(preOuput);


}

void doLsWithoutParam(fileNode *current,char* path,char* preOuput){//这里是目录，会递归调用
    if (path != NULL){
        //这里进行path地址寻址
        char * FileOrDir = strtok(path, "/");
        while(FileOrDir!=NULL){//此时说明是目录
            if (strcmp(FileOrDir,"..")==0){
                current = current->parent;
            }else if (strcmp(FileOrDir,".")==0){
                current = current;
            }else{//这里要匹配对应的目录
                //如果匹配到了就进入下一级目录,如果没有匹配到就报错
                fileNode * temp;
                temp = current->child;
                while(temp!=NULL){
                    if(strcmp(temp->name,FileOrDir)==0){
                        current = temp;
                        break;
                    }
                    temp = temp->sibling;
                }
                if (temp ==NULL){
                    printWhite("ls输入路径不存在\n",strlen("ls输入路径不存在\n"));
                    return;
                }
                if (temp->type!=0){
                    printWhite("ls输入路径不是一个目录\n",strlen("ls输入路径不是一个目录\n"));
                    return;
                }
                current = temp;
            }
            FileOrDir=strtok(NULL,"/");
        }
        //递归获得路径名
        strcpy(preOuput,getPath(current));
    }

    //如果是根目录
    if (strcmp(current->name,".")==0){
        strcat(preOuput,"/");
        printWhite(preOuput,(int )strlen(preOuput));
        printWhite(":\n",strlen(":\n"));
    }else{
        strcat(preOuput,"/");
        printWhite(preOuput,(int )strlen(preOuput));
        printWhite(":\n",strlen(":\n"));
    }

    if (strcmp(current->name,".")!=0){//如果不是根目录，就要输出“.”和“..”，注意这里要输出红色的
        printRed(".  ",(int )strlen(".  "));
        printRed("..  ",(int )strlen("..  "));
    }


    //第一次遍历
    fileNode *temp = current->child;
    while(temp!=NULL){
        if (temp->type == 0){//注意这里要输出红色的
            printRed(temp->name,(int )strlen(temp->name));
            printWhite("  ",2);
        }else{
            printWhite(temp->name,(int )strlen(temp->name));
            printWhite("  ",2);
        }
        temp = temp->sibling;
    }
    printWhite("\n",strlen("\n"));


    //第二次遍历
    temp = current->child;
    while (temp!=NULL){
        if (temp->type == 0){
            char* preOuputCopy = (char *)malloc(sizeof(preOuput));
            strcpy(preOuputCopy,preOuput);
            doLsWithoutParam(temp,NULL,strcat(preOuputCopy,temp->name));
        }
        temp = temp->sibling;
    }
    fflush(stdout);  // 清空输出缓冲区
    free(preOuput);

}

char * getPath(fileNode*current){
    char *path = (char *)malloc(100);
    fileNode *temp = current;
    if (temp->parent == temp){//说明是根目录，直接返回
        return path;
    }
    strcat(path,"/");
    strcat(path,temp->name);
    return strcat(getPath(temp->parent),path);
}

char* transNumToChar(int num){
    char *str = (char *)malloc(100);
    char *temp = (char *)malloc(100);
    int i = 0;
    while(num!=0){
        str[i++] = num%10 + '0';
        num = num/10;
    }
    for (int j = i-1; j >= 0; --j) {
        temp[j] = str[i-j-1];
    }
    temp[i] = '\0';
    free(str);
    return temp;
}