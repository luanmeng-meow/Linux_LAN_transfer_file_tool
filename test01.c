#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

#define RUN_PROT 8082
#define SEND_WAIT 2
#define RECV_WAIT 2

static pthread_mutex_t prot_ipbuf;
static int * ipbuf=NULL;
static int ipbuf_count=0;

static char choice_ip_buf[16]={0};
static int choice_ip_buf_int;

static char localip[16]={0};
static int localip_int;

static char filename[1024]={0};

void pe(const char *title)
{   perror(title);
    abort();
}

void get_localip()
{   
    int fifofd=open("get_localip.temp",O_CREAT|O_RDONLY,0666);
    if(fifofd==-1)
        pe("openfifo");
    
    system("ifconfig |grep 192 > get_localip.temp");

    short ret;
    char buff[1024];
    ret=read(fifofd,buff,1023);
    if(ret==-1)
        pe("read");
    else
        buff[ret]='\0';


    strtok(buff," ");
    strcpy(localip,strtok(NULL," "));

    close(fifofd);
    unlink("get_localip.temp");
}

void ctrlip01(char *data,int num)
{   
    for(int i=0;i<3;i++)
    {   while(*data!='.' && (data++,1));
        data++;
    }
    char *bank=data;
    while(*data &&(*data=0,data++,1));
    sprintf(bank,"%d",num);
}

char gettype(unsigned char type)
{  
    switch (type)
    {   case DT_REG:
            return 'f';

        case DT_LNK:
            return 'l';

        case DT_DIR:
            return 'd';
        
        case DT_BLK:
            return 'b';
        
        case DT_CHR:
            return 'c';
        
        case DT_FIFO:
            return 'p';
        
        case DT_SOCK:
            return 's';

        default:
            return '?';
    }

}

void showfile()
{   DIR* dp=opendir(".");
    if(dp==NULL)
        pe("opendir");

    struct stat sa;
    struct dirent *cur;
    readdir(dp);
    readdir(dp);
    while(cur=readdir(dp))
    {   
        stat(cur->d_name,&sa);
        printf("[%c/%d]%s\n",gettype(cur->d_type),sa.st_size,cur->d_name);
    }
    closedir(dp);
}

void* scanf_pthread(void *data)
{   int scanf_sock=socket(AF_INET,SOCK_STREAM,0);
    if(scanf_sock==-1)
        pe("scanf_socket");

    int error;
    int ret=connect(scanf_sock,(struct sockaddr *)data,sizeof(struct sockaddr));
    error=errno;

    if( ret==0 || (ret==-1 && error==ECONNREFUSED ) )
    {   
        if(((struct sockaddr_in *)data)->sin_addr.s_addr == localip_int)
            goto up1;
        // strcmp();
        pthread_mutex_lock(&prot_ipbuf);

        ipbuf_count++;
        ipbuf=realloc(ipbuf,ipbuf_count*sizeof(int));
        if(ipbuf==NULL)
            pe("realloc");

        ipbuf[ipbuf_count-1]=((struct sockaddr_in *)data)->sin_addr.s_addr;

        pthread_mutex_unlock(&prot_ipbuf);
        sleep(1);
    }
    else if(ret==-1 && ( error==EHOSTUNREACH || error==ETIMEDOUT ||errno==ENETUNREACH ))
        goto up1;
    else
    {   printf("[%#x]",pthread_self());
        fflush(stdout);
        pe("connect");
    }

    up1:
    close(scanf_sock);

    pthread_exit((void *)0);
}

void work01_scanf()
{   
    pthread_mutex_init(&prot_ipbuf,NULL);
    
    // void *thid_attr=NULL;l
    // pthread_attr_t thid_attr;
    // pthread_attr_init(&thid_attr);
    // pthread_attr_setstacksize(&thid_attr,1024*1024);
    // pthread_attr_setdetachstate(&thid_attr,1);

    get_localip();

    int ret;
    char ipbuf_temp[16]={0};
    strcpy(ipbuf_temp,localip);

    pthread_t thid[255];
    // struct sockaddr_in data[255];
    struct sockaddr_in *data = malloc(255 * sizeof(struct sockaddr_in));

    for(register int j=1,temp; j<4; j++)
    {   register int temp=j*85;
        
        for(register int i=(j-1)*85; i<temp; i++)
        {   
            bzero(&data[i],sizeof(data));
            data[i].sin_family=AF_INET;
            data[i].sin_port=htons(80);
            ctrlip01(ipbuf_temp,i);
            data[i].sin_addr.s_addr=inet_addr(ipbuf_temp);

            ret=pthread_create(&thid[i],NULL,scanf_pthread,(void *)&data[i]);
            if(ret!=0)
                pe("pthread_create");

            // printf("<%d>",i);
            fflush(stdout);
        }
        for(register int i=(j-1)*85; i<temp; i++)
            pthread_join(thid[i],NULL);
    }
    // for(int i=0;i<10;i++)
        // pthread_join( thid[i],NULL);
    free(data);
}

int menu01()
{   
    while(1)
    {   system("clear");
        puts("### Welcome to LAN file transfer samll tool ###");
        puts("Please inputr your action:");
        puts("[1] send file");
        puts("[2] receive file");
        puts("[3] exit");
        printf(">> ");
        fflush(stdout);

        int action=getchar();
        while(getchar()!='\n');

        action=atoi((char *)&action);
        if( action==1 || action==2 )
            return action;
        else if(action==3)
            exit(0);
        else
        {   puts("illegal input !");
            puts("Please input agagin!");
            sleep(1);
            continue; 
        }
    }
}

int menu02()
{   
    up2:
    puts("Please wait 5~10 sec ...");
    puts("Scanning the local network...");
    
    get_localip();
    localip_int=inet_addr(localip);

    work01_scanf();
    
    int i=0,ac;
    char buff[16]={0};
    if(ipbuf_count==0)
    {   puts("network not have other computer...");
        sleep(3);
        return 1;
    }

        while(1)
        {   i=0;
            system("clear");
            printf("My localhost:[%s]\n",localip);
            printf("Please input the connect computer:\n");
            for(; i<ipbuf_count; i++)
            {   inet_ntop(AF_INET,&ipbuf[i],buff,16);
                printf("[%d] %s \n",i+1,buff);
            }
            printf("[%d] flush network\n",++i);
            printf("[%d] back menu\n",++i);
            printf(">>");
            fflush(stdout);
         
            ac=getchar();
            while(getchar()!='\n');
            ac=atoi((char *)&ac);
            
            if( 1<=ac && ac<=ipbuf_count)
                return ac-1;

            else if(ac==(i-1))
            {   free(ipbuf);
                ipbuf_count=0;
                goto up2; 
            }
                
            else if(ac==i)
            {   free(ipbuf);
                ipbuf_count=0;
                return 1; 
            }

            else if(ac==0)
            {   puts("illegal input !");
                puts("Please input agagin!");
                sleep(1);
                continue;
            }
        }
        
}

int menu03()
{   
    int ret;
    char *position;
    while(1)
    {   position=getcwd(NULL,0);
        system("clear");
        
        puts("+--------------------------------------------+");
        printf("| [<filename>]:  send   <file> \n",choice_ip_buf);
        printf("| [cd <dir>]:    change <dire>\n");
        printf("| [quit]:        back to main menu\n");
        printf("| [exit]:        end the program\n");
        puts("+---------------------------------------------+");
        
        printf("| %s\n",position);
        puts("+---------------------------------------------+");
        showfile();    
        puts("+---------------------------------------------+");

        puts("Please inputer the send file:");
        printf("str>>");
        fflush(stdout);


        fgets(filename,1024,stdin);
        filename[strlen(filename)-1]='\0';

        if(strncmp(filename,"cd",2)==0)
        {   
            position=filename+3;
            ret=chdir(position);
            if(ret==-1)
            {   puts("path is illegal !");
                sleep(1);
            }
            continue;
        }
        else if(strncmp(filename,"quit",4)==0)
        {   return 1;

        }
        else if(strncmp(filename,"exit",4)==0)
        {   exit(0);

        }
        else
        {   
            if(access(filename,F_OK)==-1)
            {   puts("file name is illegal !");
                sleep(1);
                continue;
            }
            else
            {   char buff_temp[1024]={0};
                strcpy(buff_temp,getcwd(NULL,0));
                strcat(buff_temp,"/");
                strcat(buff_temp,filename);
                strcpy(filename,buff_temp);
                return 0;
            }
        }
    }
}

void sigalarm(int sig)
{   printf("# The connection timed out...\n");
    sleep(SEND_WAIT);
    
    // execvp("main",NULL);
    
    // int pid=fork();
    // if(pid==0)
    //     execvp("main",NULL);
    // else
    //     exit(0);
    exit(0);
}

int running01()
{   
    char action;
    while(1)
    {   system("clear");
        printf("Do you want to do send file?\n");
        printf("+--------------------------------------------------------------\n");
        printf("| localhost[%s] ----> target[%s]\n",localip,choice_ip_buf);
        printf("+--------------------------------------------------------------\n");
        printf("| file name:[%s]\n",filename);
        printf("+--------------------------------------------------------------\n");
        printf("[y] send \n");
        printf("[n] back \n");
        
        action=getchar();
        while(getchar()!='\n');

        if(action=='y')
            break;
        else if(action=='n')
            return 1;
        else
        {   puts("Choice is illegal !");
            sleep(1);
            continue;
        }
    }

    int ret;
    int sock=socket(AF_INET,SOCK_DGRAM,0);
    if(sock==-1)
        pe("running01 socket");

    int fd=open(filename,O_RDWR);
    if(fd==-1)
        pe("running01 open");

    struct sockaddr_in sock_attr;
    socklen_t sock_attr_len=sizeof(sock_attr);

    system("clear");
    sock_attr.sin_family = AF_INET;
    sock_attr.sin_port = htons(8082);  
    sock_attr.sin_addr.s_addr = INADDR_ANY;  
    bind(sock,(struct sockaddr *)&sock_attr,sock_attr_len);
    printf("# Waiting for connection... \n");
    sleep(SEND_WAIT);

    char receive_buff;      //wait bebe..
    while(1)
    {   ret=recvfrom(sock,(void *)&receive_buff,1,0,(struct sockaddr *)&sock_attr,&sock_attr_len);
        if(ret==-1)
            pe("running recvfrom");
        
        
        if(sock_attr.sin_addr.s_addr==choice_ip_buf_int)
            break;
        else
        {   printf("get noise...\n");
            continue;
        }
    }
    
    struct stat sa;
    ret=fstat(fd,&sa);
    if(ret==-1)
        pe("running01 stat");

    int file_size,file_part_count,file_part_rest;
    file_size=sa.st_size;
    file_part_count=file_size/1024;
    file_part_rest=file_size%1024;
    
    double decorate_part_size=100/file_part_count;
    double decorate_cur=0;

    printf("# Trying to connect...");
    sleep(SEND_WAIT);

    sendto(sock,(void *)&file_part_count,4,0,(struct sockaddr *)&sock_attr,sock_attr_len);   
    
    signal(SIGALRM,sigalarm);
    alarm(0);
    alarm(5);
    ret=recvfrom(sock,(void *)&receive_buff,1,0,(struct sockaddr *)&sock_attr,&sock_attr_len);
    if(ret==-1)
        pe("running recvfrom");

    if( sock_attr.sin_addr.s_addr==choice_ip_buf_int && receive_buff=='y')
        alarm(0);
    else
        sigalarm(13);

    printf("# Connection successful...\n");
    sleep(SEND_WAIT);

    bzero(&sock_attr,sock_attr_len);
    sock_attr.sin_family=AF_INET;
    sock_attr.sin_addr.s_addr=choice_ip_buf_int;
    sock_attr.sin_port=htons(RUN_PROT);    


    printf("# The data packet is ready...\n");
    sleep(SEND_WAIT);

    printf("# File being sending... \n");
    printf("totla: [%d] bit \n",sa.st_size);
    printf("Progress: [0%%]");
    fflush(stdout);
    sleep(SEND_WAIT);

    char send_buf[1024];
    if(file_part_count==0)
    {   ret=read(fd,send_buf,file_part_rest);
        if(ret==-1)
            pe("running read1");

        ret=sendto(sock,send_buf,file_part_rest,0,(struct sockaddr *)&sock_attr,sock_attr_len);
        if(ret==-1)
            pe("running sendto1");
    }
    else
    {   for(int i=0; i<file_part_count; i++)
        {   ret=read(fd,send_buf,1024);
            if(ret==-1)
                pe("running read2"); 
            
            ret=sendto(sock,send_buf,1024,0,(struct sockaddr *)&sock_attr,sock_attr_len);
            if(ret==-1)
                pe("runing sendto2");

            printf("\rProgress: [%.0lf%%]",decorate_cur+=decorate_part_size);
            fflush(stdout);
        }
        
        ret=read(fd,send_buf,file_part_rest);
        if(ret==-1)
            pe("running read3");
        
        ret=sendto(sock,send_buf,file_part_rest,0,(struct sockaddr *)&sock_attr,sock_attr_len);
        if(ret==-1)
            pe("running sendto3");
        
    }
    printf("\rProgress: [100%%]\n");
    fflush(stdout);

//   printf("### Running... ###\n");
//     printf("Host send file:\n");
//     printf("[%s]-->[%s]\n\n",localip,choice_ip_buf);
//     printf("lohost  :[%s][%#x]\n",localip,localip_int);
//     printf("client  :[%s][%#x]\n",choice_ip_buf,choice_ip_buf_int);
//     printf("file:<%s>\n",filename);
    close(sock);
    
    printf("[Press any key to continue...]\n");
    getchar();    
    while(getchar()!='\n');
}

int runing02()
{   int sock=socket(AF_INET,SOCK_DGRAM,0);
    if(sock==-1)
        pe("runing02 socket");
    
    int ret;
    int broadcast_enable = 1;
    ret=setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    if(ret==-1)
        pe("setsockopt");
        
    struct sockaddr_in sock_attr;
    socklen_t sock_attr_len=sizeof(sock_attr);


    system("clear");
    printf("# Configuring data... \n");
    sleep(RECV_WAIT);

    bzero(&sock_attr,sock_attr_len);
    sock_attr.sin_family=AF_INET;
    sock_attr.sin_addr.s_addr=INADDR_ANY;
    sock_attr.sin_port=htons(8082);
    ret=bind(sock,(struct sockaddr *)&sock_attr,sock_attr_len);
    if(ret==-1)
        pe("running02 bind");

    bzero(&sock_attr,sock_attr_len);
    sock_attr.sin_family=AF_INET;
    sock_attr.sin_addr.s_addr=inet_addr("255.255.255.255");
    sock_attr.sin_port=htons(8082);
    
    sendto(sock,"1",1,0,(struct sockaddr *)&sock_attr,sock_attr_len);
    if(ret==-1)
        pe("running02 sendto");

    signal(SIGALRM,sigalarm);
    alarm(5);
    printf("# The broadcast has been send...\n");
    sleep(RECV_WAIT);

    int part;
    ret=recvfrom(sock,&part,4,0,(struct sockaddr *)&sock_attr,&sock_attr_len);
    if(ret==-1)
        pe("running02 recvfrom");
    
    part++;
    alarm(0);

    printf("# Received the echo ...\n");
    printf("# Trying to connect...\n");
    sleep(RECV_WAIT);

    ret=sendto(sock,"y",1,0,(struct sockaddr *)&sock_attr,sock_attr_len);
    if(ret==-1)
        pe("running02 sendto");

    printf("# The connection successful...\n"); 
    printf("# receiving file data...\n");
    sleep(RECV_WAIT);

    char recv_filename_buff[1024]={0};
    time_t ti=time(NULL);
    struct tm *t;
    t=localtime(&ti);

    sprintf(recv_filename_buff,"Receve_file_%d/%d/%d_",t->tm_hour,t->tm_min,t->tm_sec);
    int fd=open(recv_filename_buff,O_WRONLY|O_CREAT|O_EXCL,0666);
    if(fd==-1)
        pe("running open");

    int recv_size=0;
    char recv_buff[1024];
    for(int i=0;i<part;i++)
    {   ret=recvfrom(sock,recv_buff,1024,0,NULL,NULL);
        if(ret==-1)
            pe("running02 recvfrom");
        
        recv_size+=ret;
        write(fd,recv_buff,ret);
    }
    printf("# receive successful...\n");
    printf("# total [%d]bit \n",recv_size);
    
    close(sock);

    printf("[Press any key to continue...]\n");
    getchar();    
    while(getchar()!='\n');
}

int main()
{   
    int action,choice_ip,choice_fd,choice_send;
    while(1)
    {   action=menu01();
        switch (action)
        {   case 1:
                choice_ip=menu02();
                if(choice_ip==1)
                    continue;

                choice_ip_buf_int=ipbuf[choice_ip];
                free(ipbuf);
                ipbuf_count=0;
                inet_ntop(AF_INET,(void *)&choice_ip_buf_int,choice_ip_buf,16);

                choice_fd=menu03();
                if(choice_fd==1)
                    continue;
                
                choice_send=running01();
                if(choice_send==1)
                    continue;

            case 2:
                action=runing02();
                if(action==1)
                    continue;
                break;
        }    
    }


}