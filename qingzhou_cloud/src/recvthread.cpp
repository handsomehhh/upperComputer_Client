

#include<iostream>
#include"recvthread.h"
using namespace std;


recvthread::recvthread(){
}

recvthread::~recvthread()
{
}

int app::set_socket_nonblock(int sockfd)
{
     int block_flag = fcntl(sockfd, F_GETFL, 0);
     if(block_flag < 0){
       printf("qingzhou_cloud-->get socket fd flag error:%s\n", strerror(errno));
       return -1;
     }
     else{
       if(fcntl(sockfd, F_SETFL, block_flag | O_NONBLOCK) < 0){
           printf("qingzhou_cloud-->set socket fd non block error:%s\n", strerror(errno));
           return -1;
       }
     }
     return 0;
}

int app::net_tcp_init() //??????
{
    //这个线程一直在轮询 与服务器端的连接状态

        int ret = 0;
        clientIsConnect = 0;
        struct sockaddr_in server_addr, server_addr_camera;
        int err;int err_camera;
        int sockflag = 1;
        struct linger LINgerr;
        LINgerr.l_onoff = 0;
        while(1){
            ////cout<<"net_tcp_init "<<endl;
            clientfd = socket(AF_INET, SOCK_STREAM, 0); //创建套接字；TCP通信
clientfd_camera = socket(AF_INET, SOCK_STREAM, 0); // 创建 camera的套接字

            if (clientfd < 0) {
                    printf("qingzhou_cloud-->client : create socket error\n");
                    return -1;
            }
            if (clientfd_camera < 0) {
                    printf("qingzhou_cloud_camera-->client : create socket error\n");
                    return -1;
            }
            if(setsockopt(clientfd,SOL_SOCKET,SO_REUSEADDR,&sockflag,sizeof(int)) < 0){
                printf("qingzhou_cloud-->setsockopt error\n");
            }
            if(setsockopt(clientfd_camera,SOL_SOCKET,SO_REUSEADDR,&sockflag,sizeof(int)) < 0){
                printf("qingzhou_cloud-->setsockopt error\n");
            }

            // connect
            memset(&server_addr,0,sizeof(server_addr));
		memset(&server_addr_camera,0,sizeof(server_addr_camera));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(mfcCloudPortNum); //参数服务器中读取，端口号
            server_addr.sin_addr.s_addr = inet_addr(mfcCloudIP.c_str());//"127.0.0.1" ipTarget.c_str()  mfcCloudIP.c_str()

            server_addr_camera.sin_family = AF_INET;
            server_addr_camera.sin_port = htons(mfcCloudPortNum - 1111); //参数服务器中读取，端口号
            server_addr_camera.sin_addr.s_addr = inet_addr(mfcCloudIP.c_str());//"127.0.0.1" ipTarget.c_str()  mfcCloudIP.c_str()
cout<<"IP ADDRESS "<<mfcCloudIP.c_str()<<endl;
cout<<"PORT "<<mfcCloudPortNum<<endl;
            while(1){
                cout<<"net_tcp_init in while "<<endl;
                //轮询的方式 尝试与服务器端建立连接
                sleep(1);
                //cout<<"net_tcp_init in while 1"<<endl;
                //默认阻塞模式
                err = connect(clientfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)); 

		
		cout<<"net_tcp_init in while done1 "<<endl;
                //cout<<"net_tcp_init in while 2"<<endl;
                if(err == 0) {
                    //创建成功 创建视频流的socket

                    clientIsConnect = 1;
                    disconnectFlag = 2;
                    printf("qingzhou_cloud-->client : connect to server,clientfd = %d\n",clientfd);
                    recvflag = 0;
                    heartFlag = 0;

                    err_camera = connect(clientfd_camera,(struct sockaddr *)&server_addr_camera,sizeof(struct sockaddr));
                    if(err_camera != 0) {
                        cout<<"qingzhou_cloud_camera-->client : not connect to server";
                    } else {
                        printf("qingzhou_cloud_camera-->client : connect to server,clientfd = %d\n",clientfd_camera);
                    }
                    continue;
                    //return 0;
                }//cout<<"net_tcp_init in while done2 "<<endl;
                if  (disconnectFlag == 1){ //关闭socket连接
                    clientIsConnect = 0;
                    close(clientfd);
                    close(clientfd_camera);
                    printf("qingzhou_cloud-->disconnect both\n");
                    break;
                }//cout<<"net_tcp_init in while done3 "<<endl;
                if  (errno == EISCONN){ //参数sockfd的socket已是连线状态
                    clientIsConnect = 1;
                    sleep(2);
                    //第一次创建成功后，应该会一直进入这个if
                    
                    //确实是这样的
                    printf("qingzhou_cloud-->client : is not first connect (customize)\n");
                    continue;
                    //return 0;
                }cout<<"net_tcp_init in while done 4"<<endl;
                if(err < 0){
                    if (errno == EINPROGRESS){
                    }
                }

                cout<<"net_tcp_init in while done "<<endl;
            }
        }
}

int app::recvn(void *buf,const int recvLen){
    int recvlength = recvLen;
    int readlen = 0;
    char *ptr = (char*)buf;
    while(recvlength > 0){
        int ret = recv(clientfd, ptr, recvlength, 0); //读取 recvlength：缓冲区长度
        if(ret == 0){
            clientIsConnect = 0;//I think socket is disconnect
            disconnectFlag = 1;
            close(clientfd);
            readlen = -1;
            break;
        }
        if(ret < 0){
            if(errno == EAGAIN){
                continue;
            }
        }
        recvlength -= ret;
        readlen += ret;
        ptr += ret;
    }
    return readlen;
}

void app::RecvThreadFromMfc(){

    char recvbuf[9] = {0,}; //?
    char recvInfobuf[10256] = {0,};
    recv_frame_t* pheader = (recv_frame_t*)recvbuf;
    unsigned int frame_len = 0;
    while(1){
        usleep(50000);
        if(clientIsConnect == 1){ //连接已经创建
        //cout<<"RecvThreadFromMfc: enter while\n";
            volatile int size = 0;
            volatile int intoret = 0;
            volatile int ret = 0;
            struct timeval tv;
            //https://blog.csdn.net/Fuel_Ming/article/details/122931926
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(clientfd, &readfds);
            //cout<<"RecvThreadFromMfc: clientfd is "<<clientfd<<endl;
            tv.tv_sec = 0;
            tv.tv_usec = 50000;
            ret = select(clientfd+1, &readfds, NULL, NULL, &tv); //返回准备就绪的描述符的数量（这里是1）
            //cout<<"RecvThreadFromMfc: ret is "<<ret<<endl;
            if(ret == 0){
                continue;
            }
            else if(ret < 0 && errno != EINTR) {
                cout<<"RecvThreadFromMfc: think socket is disconnect\n";
                clientIsConnect = 0;//I think socket is disconnect
                close(clientfd);
                continue;
            }else if((ret > 0)&&(errno != EINTR)){
                

                if(FD_ISSET(clientfd,&readfds)){
                    //正常读取
                    cout<<"RecvThreadFromMfc: read data"<<endl;
                    size = recvn(recvbuf, 9);
                    cout<<"RecvThreadFromMfc: data size "<<size<<endl;
                    if(size < 0){
                        printf("qingzhou_cloud recv failed....\n");
                        continue;
                    }
                    if(size > 0){
                        heartdisconnectCommand = 0;
                        if(bdebug == 1){
                            printf("recv size is %d\n",size);
                            for(int i = 0;i < size;i++){
                                printf(" %02x",recvbuf[i]);
                            }
                            printf("\n");
                        }
                        // 0-3byte固定
                        if((recvbuf[0] != 0x02)||(recvbuf[1] != 0x20)||(recvbuf[2] != 0x02)||(recvbuf[3] != 0x20)){
                            printf("qingzhou_cloud recv header error....\n");
                            continue;
                        }
                        //4-7byte：长度
                        frame_len = (recvbuf[4] & 0xff) + ((recvbuf[5] & 0xff)<<8) + ((recvbuf[6] & 0xff)<<16) + ((recvbuf[7] & 0xff)<<24);

                        intoret = recvn(recvInfobuf, frame_len-1);
                        if(bdebug == 1){
                            printf("recv len is %d,command is %02x,para[0] is %d\n",pheader->len,pheader->command,recvInfobuf[0]);
                        }
                        if(intoret  < 0){
                            printf("qingzhou_cloud recv Info failed....\n");
                            continue;
                        }else {
                            heartFlag = 0;
                        //    recvflag = 0;
                            if(bdebug == 1){
                                printf("recv info size is %d\n",intoret);
                                for(int i = 0;i < intoret;i++){
                                    printf(" %02x",recvInfobuf[i]);
                                }
                                printf("\n");
                            }
                        }
                        switch(pheader->command){
                        case 0x10:{//start navigation
                            recvflag = 1;
                            qingzhou_cloud::startstopCommand ssCommand;
                            ssCommand.startstopcommand = 0x01;
                            pub_start_stop_command.publish(ssCommand);
                            heartFlag = 0;
                            //printf("recv start navigation command %d\n",recvInfobuf[0]);
                            printf("recv start navigation command %d\n",ssCommand.startstopcommand);                            
                            break;
                        }
                        case 0x20:{//stop
                            recvflag = 2;
                            qingzhou_cloud::startstopCommand ssCommand;
                            ssCommand.startstopcommand = 0x02;
                            pub_start_stop_command.publish(ssCommand);
                            heartFlag = 0;
                            //printf("Recv stop navigation command %d\n",recvInfobuf[0]);
                            printf("Recv stop navigation command %d\n",ssCommand.startstopcommand);
                            break;
                        }
                        case 0x30:{//first stop point 
                            float x = 0,y = 0;
                            memcpy(&x,recvInfobuf+3,4);
                            memcpy(&y,recvInfobuf+4+3,4);
                            printf("Recv first stop point x = %.2f,y =  %.2f\n",x,y);
                            recvflag = 7;
                            heartFlag = 0;
                            qingzhou_cloud::stoppoint spoint;
                            spoint.X = x;
                            spoint.Y = y;
                            pub_stop_point.publish(spoint);
                            break;
                        }
                        case 0x40:{//second stop point 
                            //   printf("Recv second stop point\n");
                            float x = 0,y = 0;
                            memcpy(&x,recvInfobuf+3,4);
                            memcpy(&y,recvInfobuf+4+3,4);
                            printf("Recv second stop point x = %.2f,y =  %.2f\n",x,y);
                            recvflag = 3;
                            heartFlag = 0;
                            qingzhou_cloud::stoppoint spoint;
                            spoint.X = x;
                            spoint.Y = y;
                            pub_stop_point.publish(spoint);
                            if(recvInfobuf[0] == 1){
                                //do some work
                            }
                            break;
                        }
                        case 0x50:{//third stop command
                            recvflag = 6;
                            heartFlag = 0;
                            float x = 0,y = 0;
                            memcpy(&x,recvInfobuf+3,4);
                            memcpy(&y,recvInfobuf+4+3,4);
                            printf("Recv third stop point x = %.2f,y =  %.2f\n",x,y);
                            
                            qingzhou_cloud::stoppoint spoint;
                            spoint.X = x;
                            spoint.Y = y;
                            pub_stop_point.publish(spoint);
                            break;
                        }
                        case 0x77:{//heart recv
                            heartFlag = 0;
                            break;
                        }
                        default:break;
                        }
                    }
                }
            }
        }
    }
}

void app::detectConnectThread(){
    int ret = net_tcp_init();
    if(ret < 0){
        printf("qingzhou_cloud detectConnectThread-->client : connect error\n");
    }else if(ret == 0){
        //这个我怎么感觉退不出来
        //确实退不出来
        printf("qingzhou_cloud detectConnectThread-->client : connect success\n");
    }
}



