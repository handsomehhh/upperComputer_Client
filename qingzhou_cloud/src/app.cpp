#include "app.h"
#include <tf2/convert.h>
#include <tf2/utils.h>

#include <iostream>

#include <unistd.h>

using namespace cv;

app::app(ros::NodeHandle nh) {
     //variable init
     cloudsendrate = 5;
     recvflag = 0;
     disconnectFlag = 0;
     mfcCloudPortNum = 8050;
     heartFlag = 0;
     heartdisconnectCommand = 0;
     recvAckFlag = 0;
     clientIsConnect = 0;
     carstatusMsg.data = 0;

     nh.param("cloudsendrate",cloudsendrate,cloudsendrate);
     nh.param("mfcCloudIP",mfcCloudIP,mfcCloudIP);
     nh.param("mfcCloudPortNum",mfcCloudPortNum,mfcCloudPortNum);
     nh.param("bdebug",bdebug,bdebug);

    pub_app_command = nh.advertise<qingzhou_cloud::qingzhou_cloud>("clientCommand", 5);
    pub_start_stop_command = nh.advertise<qingzhou_cloud::startstopCommand>("startStopCommand", 5); //bring_up订阅
    pub_stop_point = nh.advertise<qingzhou_cloud::stoppoint>("stoppoint", 5); //qingzhou_nav订阅
    //pub_trafficLight = nh.advertise<dzcloud::trafficLight>("trafficLight", 5); //bring_up和qingzhou_nav都有订阅

    clientdetectConnectThread = boost::thread(boost::bind(&app::detectConnectThread, this));
    //111clientRecvThread = boost::thread(boost::bind(&app::RecvThreadFromMfc, this));
    //sendHeart = boost::thread(boost::bind(&app::sendHeartThread, this));
    
    sub_current_position = nh.subscribe("qingzhou_location",1,&app::callback_location,this);
    sub_current_battery = nh.subscribe("battery",1,&app::callback_battery,this);
    sub_car_status = nh.subscribe("carstatus",1,&app::callback_carstatus,this);
    sub_nav_status = nh.subscribe("navstatus",1,&app::callback_navstatus,this);     
    sub_car_speed = nh.subscribe("odom",1,&app::callback_speed,this);


    //传输图片
    sub_current_camera_to_upper = nh.subscribe("/csi_cam_0/image_raw/compressed",1,&app::callback_camera_to_upper,this);

    //****给move_base下发目标点******//

    is_new_goal = false;
    
    simple_nh = simple_nh("move_base_simple");
    pub_app_move_base_simple_goal = nh.advertise<geometry_msgs::PoseStamped>("goal",1);

    //****给move_base下发目标点******//


    std::cout<<"init app done\n";

 }

  double getYaw(geometry_msgs::PoseStamped pose)
  {
      return tf2::getYaw(pose.pose.orientation);
  }

 app::~app()
 {
 }
void app::callback_carstatus(const std_msgs::UInt8::ConstPtr &msg){
    carstatusMsg = *msg;
 	//printf("qingzhou_cloud-->callback_carstatus:%d\n",carstatusMsg.data);
}
void app::callback_navstatus(const std_msgs::UInt8::ConstPtr &msg){
    navstatusMsg = *msg;
// 	printf("qingzhou_cloud-->callback_navstatus\n");
}
void app::callback_location(const qingzhou_cloud::current_location::ConstPtr &msg){
    current_location = *msg;
 	//printf("qingzhou_cloud-->callback_location\n");
}
  
void app::callback_battery(const std_msgs::Float32::ConstPtr &msg){
    current_battery = *msg;
    //printf("qingzhou_cloud-->callback_battery = %.2f\n",current_battery.data);
}
  
void app::callback_speed(const nav_msgs::Odometry::ConstPtr &msg){
    sOdom = *msg;
    //printf("qingzhou_cloud-->callback_speed = %.2f, %.2f\n",sOdom.twist.twist.angular.z,sOdom.twist.twist.linear.x);
}
bool flag_camera = false;
void app::callback_camera_to_upper(const sensor_msgs::CompressedImage::ConstPtr &msg){
//ROS_INFO("receive carmera ");
    current_camera_to_upper = *msg;
flag_camera = true;
}

void app::run()
{
    ros::Rate rate(cloudsendrate);
    while(ros::ok()){
    ros::spinOnce();
	if(clientIsConnect == 1){
	    //12121dataProcKernelNet(0x01);

        //传输图片
        send_camera();
	}
//	dzcloud::trafficLight lightMsg;
//	pub_trafficLight.publish(lightMsg);

    rate.sleep();
//usleep(10000);
    //std::cout<<"once while done\n";
    }
 }


void app::send_camera()
{
if(flag_camera == false) return ;
flag_camera = false;
    try
    {
        //sensor_msgs::ImageConstPtr& source;
        cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(current_camera_to_upper,  sensor_msgs::image_encodings::TYPE_8UC3);
        std::cout<<"转换成功！！！！！！！！！！！！！！！！！！！！！！！"<<std::endl;
        cv::Mat img__ = cv_ptr->image;
resize(img__,img__,cv::Size((int)img__.cols * 0.4,(int)img__.cols*0.4));

        //cv::cvtColor(img__, img__, CV_BGR2RGB);//?????

		// -----------------------------------------------------------------------------------------
		std::vector<uchar> data_encode;
		std::vector<int> quality;
		quality.push_back(IMWRITE_JPEG_QUALITY);
		quality.push_back(10);//进行50%的压
		imencode(".jpg", img__, data_encode,quality);//将图像编码
		char encodeImg[65535];

		int nSize = data_encode.size();
std::cout<<"data_encode size :"<<nSize<<endl;
		for (int i = 0; i < nSize; i++)
		{
			encodeImg[i] = data_encode[i];
		}

//Mat new_image = imdecode(data_encode, cv::IMREAD_COLOR);

//imshow("image", new_image);
//		waitKey(30);



//start
char st[4] = {0x10,0x20,0x30,0x40};

char en[4] = {0x50,0x60,0x70,0x40};
send(clientfd_camera, st, 4, 0);
int start_pos = 0;
while(start_pos + 2000 <= nSize) {
	send(clientfd_camera, encodeImg + start_pos, 2000, 0);
	start_pos += 2000;
}
//last
if(start_pos != nSize) {
	send(clientfd_camera, encodeImg + start_pos, nSize - start_pos, 0);
	
}
send(clientfd_camera, en, 4, 0);


        //发送encodeImg
        //int ret = send(clientfd_camera, (char *)&encodeImg, 20000, 0);

        //if(ret < 0)
        //    printf("qingzhou_cloud-->send to MFC server error %d\n",errno);

    }
    catch (cv_bridge::Exception& e)
    {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }
}


void app::dataProcKernelNet(int carID){
    float x = current_location.x;
    float y = current_location.y;
    float theta = current_location.heading;
    float currentAngleSpeed = sOdom.twist.twist.angular.z;
    float currentLinerSpeed = sOdom.twist.twist.linear.x;
    char navstatus = 0;  
    char carstatus = carstatusMsg.data;   
 	//printf("qingzhou_cloud--> x:%.2f,y:%.2f,AngleSpeed:%.2f,LinerSpeed:%.2f,navstatus:%d,status:%d\n",x,y,currentAngleSpeed,currentLinerSpeed,navstatus,status);

    char send_buf[126] = {0,};
    send_frame_t* send_frame = (send_frame_t*)send_buf; //可以这样
    send_frame->head[0] = 0x02;
    send_frame->head[1] = 0x20;
    send_frame->head[2] = 0x02;
    send_frame->head[3] = 0x20;
    send_frame->len = 31;
    send_frame->command = 0xaa;
    memcpy(send_buf+9,&carID,4);
    memcpy(send_buf+13,&x,4);
    memcpy(send_buf+17,&y,4);
    memcpy(send_buf+21,&theta,4);
    //memcpy(send_buf+25,&battery,4);
    struct info{
	int carID1;float x1;float y1;float theta1;float angleSpeed;float linerSpeed;char navstatus1;char carstatus1;
	};
	
    struct info info1{
        carID,
        x,
        y,
        theta,
        currentAngleSpeed,
	    currentLinerSpeed,
	    navstatus,
	    carstatus,
    };

    //send_buf[29] = 0x00;
    //send_buf[30] = 0x00;
    //send_buf[31] = 0x00;
    //send_buf[32] = 0x01;
    //send_buf[33] = 0x00;
    //send_buf[34] = 0x02;
    
    //send_buf[35] = 0x03;
    //send_buf[36] = 0x30;
    //send_buf[37] = 0x03;
    //send_buf[38] = 0x30;
    //int ret = send(clientfd, send_buf, send_frame->len + 8, 0);
    int ret = send(clientfd, (char *)&info1, sizeof(info1), 0);
    if(ret < 0)
	printf("qingzhou_cloud-->send to MFC server error %d\n",errno);




    //传输图片
}


