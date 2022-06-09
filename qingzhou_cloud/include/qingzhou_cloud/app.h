#ifndef NODE_EXAMPLE_TALKER_H
#define NODE_EXAMPLE_TALKER_H

//c++ lib
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <boost/assert.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <boost/asio.hpp>
#include <boost/thread//thread.hpp>
#include <boost//assert.hpp>
#include <math.h>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>
// ROS lib
#include "ros/ros.h"
#include "ros/time.h"
//other lib
#include "pthread.h"
#include "recvthread.h"
#include "sendthread.h"
#include <dirent.h>
// Custom message includes. Auto-generated from msg/ directory.
#include <qingzhou_cloud/qingzhou_cloud.h>
#include <qingzhou_cloud/current_location.h>
#include <std_msgs/Float32.h>
#include <std_msgs/UInt8.h>
#include <nav_msgs/Odometry.h>

#include <sensor_msgs/CompressedImage.h>

#include <qingzhou_cloud/startstopCommand.h>
#include <qingzhou_cloud/stoppoint.h>
//#include <qingzhou_cloud/trafficlight.h>
//#include <qingzhou_cloud/roadLine.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

//****给move_base下发目标点******//

// #include <move_base_msgs/MoveBaseAction.h>
// #include <actionlib/client/simple_action_client.h>

// typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseGoal> ActionClient;

#include <geometry_msgs/PoseStamped.h>

//****给move_base下发目标点******//

using namespace std;

#define HDR1 0xff
#define HDR2 0xa5
#define HDR3 0x5a
#define MAX_ZMQ_ARRAYSIZE 10250

#define OFFLINETIME 3 //3s


class app
{
public:
  //! Constructor.
  app(ros::NodeHandle nh);
  ~app();
  void run();
private:

    volatile int runningstate;
    volatile int laststate;
    pthread_mutex_t mutex;

    volatile int pointIndex;
    int CarID;
    long sendTimes;
    volatile int recvPadFlag;
    volatile int tempSection;

    DIR *dir;
    struct dirent *ptr;
    int bdebug;
    int cloudsendrate;
    int cloudbaudrate;
    FILE* fp;
    FILE* recvSelectMapfp;
    FILE* saveMapFilefp;
    FILE* Appfp;

    string mfcCloudIP;
    int mfcCloudPortNum;


    void dataProcKernelNet(int carID);
    int readn(char* , int );
    int check_sum_func(unsigned char* car_status,int checkByte);

    volatile int recvflag;
    std_msgs::Float32 currentBattery;
    //send socket thread
public:
    ros::Publisher pub_app_command;
    ros::Publisher pub_start_stop_command;
    ros::Publisher pub_stop_point;
    //ros::Publisher pub_trafficLight;
    qingzhou_cloud::qingzhou_cloud pubCommand;
    ros::Subscriber sub_current_position;
    ros::Subscriber sub_current_battery;
    ros::Subscriber sub_car_status;
    ros::Subscriber sub_nav_status;
    ros::Subscriber sub_car_speed;
     
    int try_to_accept();
    int recv_and_processing_data(int serverfd,char* data);
    int init_socket();
    int set_socket_to_listen(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
private:
    int serverfd;
    int socketfd;
    int port;
    string clientIP;
    volatile int connected;
    sockaddr_in addr_client;
    boost::thread sendHeart;
    int sendHeartThread();

    boost::thread saveMapFile;
    int saveMapFileThread();

	


    //传输图片
    ros::Subscriber sub_current_camera_to_upper;
    sensor_msgs::CompressedImage current_camera_to_upper;

    //recv socket thread
public:

//camera
int socketfd_camera;
int serverfd_camera;
int port_camera;
int clientfd_camera;

    int clientfd;
    volatile int disconnectFlag;
    volatile int clientIsConnect;
    volatile int heartFlag;
    volatile int heartdisconnectCommand;
    volatile int recvAckFlag;
    int net_tcp_init();
    int set_socket_nonblock(int sockfd);
    void RecvThreadFromMfc();
    boost::thread clientRecvThread;
    void detectConnectThread();
    boost::thread clientdetectConnectThread;
    
    qingzhou_cloud::current_location current_location;
    std_msgs::Float32 current_battery;
    std_msgs::UInt8 carstatusMsg;
    std_msgs::UInt8 navstatusMsg;    
    nav_msgs::Odometry sOdom;
    void callback_location(const qingzhou_cloud::current_location::ConstPtr &msg);
    void callback_battery(const std_msgs::Float32::ConstPtr &msg);
    void callback_carstatus(const std_msgs::UInt8::ConstPtr &msg);
    void callback_navstatus(const std_msgs::UInt8::ConstPtr &msg);
    void callback_speed(const nav_msgs::Odometry::ConstPtr &msg);

    //订阅相机图片消息
    void callback_camera_to_upper(const sensor_msgs::CompressedImage::ConstPtr &msg);

    int recvn(void *buf,const int recvLen);

    //向上位机发送消息
    void send_camera();

    //****给move_base下发目标点******//
private:
    bool is_new_goal = false;                               //标识是否有新目标点从上位机下发 （这个好像没用了）
    geometry_msgs::PoseStamped current_goal;                //记录最新发布的目标点信息

    ros::NodeHandle simple_nh;
    ros::Publisher pub_app_move_base_simple_goal;           //通过话题发布给move_base

public:
    //在类构造函数中增加actionClient的初始化

    void sendGoal(float x, float y);                        //接收到目标点后，发送目标点

    

};

#endif // NODE_EXAMPLE_TALKER_H
