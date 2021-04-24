#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Int32.h"
#include "serial/serial.h"

#define MAX_TIMES 10

uint8_t cmd_servoOn[8] = {0x88, 0, 0, 0, 0, 0, 0, 0};
uint8_t cmd_close[8] = {0xa2, 0, 0, 0, 0x11, 0x11, 0x02, 0};
uint8_t cmd_open[8] = {0xa2, 0, 0, 0, 0xee, 0xee, 0xfd, 0xff};
uint8_t cmd_stop[8] = {0xa2, 0, 0, 0, 0, 0, 0, 0};

serial::Serial ser; //声明串口对象
std::string param_port_path_ = "/dev/ttyUSB0";
int param_baudrate_ = 9600;
int64_t start_pos = 0;
int64_t start_pos_some[MAX_TIMES] = {0};

int close_pos = 0;
int open_pos = 0;

int fresh_flag = 0;
int clear_loop = 50;

//回调函数
void callback(const std_msgs::Int32::ConstPtr& cmd)
{
    if (cmd->data == 1)
    {
        ser.write(cmd_close, 8);
        ROS_INFO("close");
    }
    if (cmd->data == 0)
    {
        ser.write(cmd_open, 8);
        ROS_INFO("open");
    } 
    fresh_flag = 0;
    
}

int main(int argc, char **argv)
{
 	ros::init(argc,argv,"toolbase");
 	ros::NodeHandle n("~");
    int rate = 0;

    n.getParam("loop_rate", rate);
    n.getParam("clear_loop", clear_loop);
    n.getParam("portname", param_port_path_);
    n.getParam("baudrate", param_baudrate_);

    try 
    { 
    //设置串口属性，并打开串口 
        ser.setPort(param_port_path_); 
        ser.setBaudrate(param_baudrate_); 
        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        ser.setTimeout(to); 
        ser.open(); 
    } 
    catch (serial::IOException& e) 
    { 
        ROS_ERROR_STREAM("Unable to open port "); 
        return -1; 
    } 

    //检测串口是否已经打开，并给出提示信息 
    if(ser.isOpen()) 
    { 
        ROS_INFO_STREAM("Serial Port:"<<param_port_path_<<" initialized"); 
    } 
    else 
    { 
        return -1; 
    } 

    for (int i = 0; i< 2; i++)
    {
        ser.write(cmd_servoOn, 8);
        ros::Duration(0.1).sleep();
    }

    ros::Subscriber sub = n.subscribe("/toolchanger", 10, callback);
    ros::Rate loop_rate(rate);
    ROS_INFO("control node init ready!");

    while(ros::ok()) 
    { 
        if (fresh_flag > clear_loop )
        {
            fresh_flag = -1;
            ser.write(cmd_stop, 8);
        }
        else if (fresh_flag >= 0){
            fresh_flag ++;
        }
        
        ROS_INFO("freshflag: %d\n", fresh_flag);
        //处理ROS的信息，比如订阅消息,并调用回调函数 
        ros::spinOnce(); 
        loop_rate.sleep(); 

    }
    ser.close();

	return  0;
}