#include <ros/ros.h> 
#include <serial/serial.h>  //ROS已经内置了的串口包 
#include <std_msgs/String.h> 
#include <std_msgs/Empty.h> 

#include <geometry_msgs/Twist.h>
#include <sensor_msgs/JointState.h>
#include <sensor_msgs/Joy.h>
#include "map"
# include "math.h"

// ===========================================
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <ifaddrs.h>
#define MAX_LENGTH 254

int port = 0;
std::string hostIP;

struct sockaddr_in addrCli;
struct sockaddr_in addrSer;
int sockCli;
socklen_t addrlen;

char sendbuf[MAX_LENGTH];

// ===========================================
#define DEADZONE 20
# define rDEADZONE 20

# define max_step_angle 10.0

# define DEG2RAD 3.1415926/180.0
# define RAD2DEG 180.0/3.1415926

# define GUANJIE 2
# define STOP 0
# define HUILING 1

serial::Serial ser; //声明串口对象

/* serial */
std::string param_port_path_;
int param_baudrate_;
int param_loop_rate_;
serial::parity_t param_patity_;

// ===========================================
#define CHANNEL 9
int i = 0;
int j = 0;
uint8_t sum = 0;
int rec_right[CHANNEL] = {0};
int lastrec[CHANNEL] = {0};

double js_angle[3] ={0.0};
std::map<std::string, double> jsmap;
std::map<std::string, double>::iterator iter;

uint32_t watchdog = 0;
uint32_t js_wd = 0;
uint8_t js_ready = 0;
double beta_cmd;
double rightarmbeta;
double leftarmbeta;

double movespeed = 1;
double moveSpeedDownLim = 0.1;
double moveSpeedUpLim = 2.0;
double joy[6] = {0.0};
int joy_wd = 0;
uint8_t joy_ready = 0;

std::vector<double> recf[CHANNEL];

double fillter(double rec, int index, int times)
{
    double sum = 0.0;
    int i;
    while (recf[index].size() <= times)
    {
        recf[index].push_back(rec);
    }

    while (recf[index].size() > times)
    {
        recf[index].erase(recf[index].begin());

        for (i = 0, sum = 0; i< recf[index].size(); i++)
        {
            sum += recf[index].at(i);
        }

        sum /= (double)(recf[index].size());
    }

    return sum;
}

void js_callback(const sensor_msgs::JointState &js)
{
    if (abs(js.header.stamp.now().toSec() - ros::Time::now().toSec()) > 0.5)
    {
        ROS_WARN("Too Old Data");
        return;
    }
    for (i = 0; i< js.name.size(); i++)
    {
        jsmap.insert(std::pair<std::string, double>(js.name.at(i).c_str(), js.position.at(i)));
    }

    iter = jsmap.find("leftleg_joint2");  
    if(iter != jsmap.end())  {
        js_angle[0] = iter->second;
        // std::cout<<"Find, the value is "<<iter->second<<std::endl; 
    } 
    else{
        std::cout<<"Do not Find"<<std::endl;  
    }

    iter = jsmap.find("leftleg_joint1");  
    if(iter != jsmap.end()){
        js_angle[1] = iter->second;
        // std::cout<<"Find, the value is "<<iter->second<<std::endl;  
    }
    else  {
        std::cout<<"Do not Find"<<std::endl;  
    }

    iter = jsmap.find("yao_joint");  
    if(iter != jsmap.end()){
        js_angle[2] = iter->second;
        // std::cout<<"Find, the value is "<<iter->second<<std::endl;  
    }
    else
    {
        std::cout<<"Do not Find"<<std::endl;  
    }
    jsmap.clear();
    js_wd = 0;
    js_ready = 1;

    // printf("%f,%f,%f\n",js_angle[0], js_angle[1], js_angle[2]);
}

void joy_callback(const sensor_msgs::JoyPtr & j)
{
    if (j->buttons[0] == 1)
        movespeed *= 0.97;

    if (j->buttons[1] == 1)
        movespeed *= 1.03;

    if (movespeed > moveSpeedUpLim)
        movespeed = moveSpeedUpLim;
    if (movespeed < moveSpeedDownLim)
        movespeed = moveSpeedDownLim;

    for (int i = 0; i< 6; i++)
    {
        joy[i] = j->axes[i] * movespeed;
    }

    joy_wd = 0;
    joy_ready = 1;
}

void UDP_send(char *ch)
{
    sendto(sockCli, ch, strlen(ch)+1, 0, (struct sockaddr*)&addrSer, addrlen);
    ch[strlen(ch)] == '\0';
    ROS_INFO("%s",ch);
}

int main (int argc, char** argv) 
{ 
    //初始化节点 
    ros::init(argc, argv, "ardu_remoter_pub"); 
    //声明节点句柄 
    ros::NodeHandle nh; 
	geometry_msgs::Twist cmd;

    uint8_t rec[2000] = {'\0'}; 
    int intool = 0;
    double armx;
    double army;
    double armz;
    double armRx;
    double armRy;
    double armRz;
    double carx;
    double carz;

    //发布主题 
    ros::Publisher remo_pub = nh.advertise<geometry_msgs::Twist>("cmd_vel", 1000); 
    ros::Subscriber js_sub = nh.subscribe("joint_states", 10, js_callback);
    ros::Subscriber joy_sub = nh.subscribe("/spacenav/joy", 10, joy_callback);
    
	nh.param<std::string>("ardu_remoter_pub/port", param_port_path_, "/dev/remote_USB");
	nh.param<int>("ardu_remoter_pub/baudrate", param_baudrate_, 9600);
	nh.param<int>("ardu_remoter_pub/loop_rate", param_loop_rate_, 20);
	nh.param<double>("ardu_remoter_pub/armx", armx, 2);
	nh.param<double>("ardu_remoter_pub/army", army, 2);
	nh.param<double>("ardu_remoter_pub/armz", armz, 2);
	nh.param<double>("ardu_remoter_pub/armRx", armRx, 3);
	nh.param<double>("ardu_remoter_pub/armRy", armRy, 3);
	nh.param<double>("ardu_remoter_pub/armRz", armRz, 3);
	nh.param<double>("ardu_remoter_pub/carx", carx, 1);
	nh.param<double>("ardu_remoter_pub/carz", carz, 1);
    nh.param<double>("ardu_remoter_pub/rightarmbeta", rightarmbeta, 1.0);
    nh.param<double>("ardu_remoter_pub/leftarmbeta", leftarmbeta, 1.0);

    nh.param<double>("ardu_remoter_pub/moveSpeedInit", movespeed, 1);
    nh.param<double>("ardu_remoter_pub/moveSpeedUplim", moveSpeedUpLim, 2);
    nh.param<double>("ardu_remoter_pub/moveSpeedDownlim", moveSpeedDownLim, 0.1);

    nh.param<int>("robot_port",port, 0);
	nh.param<std::string>("robot_ip",hostIP, "127.0.0.1");

    sockCli = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockCli == -1)
    {
        perror("socket");
        exit(1);
    }

    addrSer.sin_family = AF_INET;
    addrSer.sin_port = htons(port);//????
    addrSer.sin_addr.s_addr = inet_addr(hostIP.c_str());//????????? 0?127.0.0.1?? 1?????ip
 
    addrlen = sizeof(struct sockaddr);

    ROS_INFO("udp init ok,IP:%s,port %d", hostIP.c_str(), port);
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
        ROS_INFO_STREAM("Serial Port"<<param_port_path_<<" initialized"); 
    } 
    else 
    { 
        return -1; 
    } 

    //指定循环的频率 
    ros::Rate loop_rate(param_loop_rate_); 
    int middle_cali = 0;
    i=0;
    uint8_t rec1=0;
    uint8_t index = 0;
    double angle1 = 0.0;
    double angle2 = 0.0;
    double angle3 = 0.0;

    double last_angle1 = 0.0;
    double last_angle2 = 0.0;
    double last_angle3 = 0.0;

    int slow;
    double ref_rot[3] = {0.0};
    double ref_ang[3] = {0.0};
    int ref_flag = 0;

    int ldmode;
    int last_ldmode;
    double speedRate = 0.0;

    double speedx = 0;
    double speedy = 0;
    double speedz = 0;
    double speedRx = 0;
    double speedRy = 0;
    double speedRz = 0;

    int left_once = 0;
    int right_once = 0;
    int first_lock = 1;
    int enonce = 0;
    int rightforceEnonce = 0;
    int rightenonce = 0;
    int leftforceEnonce = 0;
    int leftenonce = 0;
    
    while(ros::ok()) 
    { 
        // while(ser.available() < 10);
        if(ser.available()){
            if (first_lock == 0)
            {
                first_lock = 1;
                printf("Armed\n");
            }
            watchdog = 0;
            ser.read(rec,1);
            if (rec[0] == 0xaa)
            {
                // jointstate接收看门狗
                js_wd ++;
                if (js_wd > 10)     // 反馈数据50Hz，遥控器大致25Hz，几乎每两个周期就会有一次清零机会
                {
                    js_wd = 0;
                    js_ready = 0;
                }

                joy_wd ++;
                if (joy_wd > 10)     // 反馈数据30Hz，遥控器大致25Hz，几乎每个周期就会有一次清零机会
                {
                    joy_wd = 0;
                    joy_ready = 0;
                }

                ser.read(rec+1,CHANNEL * 2 +1);
                for(j = 0; j < CHANNEL ; ++j){
                    rec_right[j] = ( rec[2*j+1] <<8 ) + rec[2*j+2];
                    sum += rec[2*j+1];
                    sum += rec[2*j+2];
                }
                sum += 0xaa;
                // ROS_INFO("sum=%x;rec[9]=%x\n",sum,rec[9]);
                if (rec[CHANNEL * 2 +1] == sum ){
                    // ROS_INFO("ok\n");
                    for (i = 0; i< CHANNEL; i++)
                    {
                        if (i == 2){
                            rec_right[2] -= 1000;
                        }
                        else{
                            rec_right[i] -= 1500;
                        }
                    }
                    
                    for (i= 0; i <CHANNEL ;i++){
                        if (i == 2){
                            if(rec_right[i] > 1000) rec_right[i] = 1000;
                            if(rec_right[i] < 0) rec_right[i] = 0;
                            if (rec_right[i]<100) rec_right[i] = 0;
                            else rec_right[i] -=100;
                        }
                        else if(i >= 4 && i <= 6){
                            if(rec_right[i] > 500) rec_right[i] = 500;
                            if(rec_right[i] < -500) rec_right[i] = -500;
                            
                            rec_right[i] = (int)(fillter((double)rec_right[i], i, 20));
                            if (abs(rec_right[i] - lastrec[i]) < 2 )
                            {
                                rec_right[i] = lastrec[i];
                            }
                            lastrec[i] = rec_right[i];

                            if (abs(rec_right[i])<rDEADZONE) rec_right[i] = 0;
                            else{
                                if (rec_right[i] > 0) rec_right[i] -= rDEADZONE;
                                if (rec_right[i] < 0) rec_right[i] += rDEADZONE;
                            }
                        }
                        else{
                            if(rec_right[i] > 500) rec_right[i] = 500;
                            if(rec_right[i] < -500) rec_right[i] = -500;
                            if (abs(rec_right[i])<DEADZONE) rec_right[i] = 0;
                            else{
                                if (rec_right[i] > 0) rec_right[i] -= DEADZONE;
                                if (rec_right[i] < 0) rec_right[i] += DEADZONE;
                            }
                        }
                    }

                    // ROS_INFO("1:%d 2:%d 3:%d 4:%d 5:%d 6:%d 7:%d 8:%d 9:%d  ",rec_right[0],rec_right[1],rec_right[2],rec_right[3],rec_right[4],rec_right[5],rec_right[6],rec_right[7],rec_right[8]);

                    if (abs(rec_right[7]) < DEADZONE)       // 右手拨杆中位
                    {
                        // cmd.linear.x  = float( rec_right[2] * rec_right[1] ) / 450000.0 * carx;
                        // cmd.linear.y  = 0.0; //float( rec_right[2] ) * float( rec_right[0] ) / 450000.0 * MAX_y ;
                        // cmd.linear.z  = 0;
                        // cmd.angular.x = 0;
                        // cmd.angular.y = 0;
                        // cmd.angular.z = float( rec_right[2] * rec_right[3] ) / 450000.0 * carz ;

                        // remo_pub.publish(cmd);

                        // if (abs(rec_right[0] ) <DEADZONE)
                        // {
                        //     ldmode = STOP;
                        //     if (last_ldmode == HUILING)
                        //     {   
                        //         sprintf(sendbuf,"stopMove(3)\n");
                        //         UDP_send(sendbuf);
                        //     }

                        //     if(rec_right[8] > 200){
                        //         if (rec_right[1] > 400){
                        //             if (enonce == 0){
                        //                 enonce = 1;
                        //                 sprintf(sendbuf,"EnMotor(2,-1)\n");
                        //                 UDP_send(sendbuf);
                        //                 sprintf(sendbuf,"EnMotor(3,-1)\n");
                        //                 UDP_send(sendbuf);
                        //                 sprintf(sendbuf,"EnMotor(4,-1)\n");
                        //                 UDP_send(sendbuf);
                        //             }
                        //         }
                        //         else if (rec_right[1] < -400)
                        //         {
                        //             if (enonce == 0){
                        //                 enonce = 1;
                        //                 sprintf(sendbuf,"DisMotor(2,-1)\n");
                        //                 UDP_send(sendbuf);
                        //                 sprintf(sendbuf,"DisMotor(3,-1)\n");
                        //                 UDP_send(sendbuf);
                        //                 sprintf(sendbuf,"DisMotor(4,-1)\n");
                        //                 UDP_send(sendbuf);
                        //             }
                        //         }
                        //         else if (abs(rec_right[1]) < DEADZONE)
                        //         {
                        //             enonce = 0;
                        //         }
                                
                        //     }
                        // }
                        // else if(rec_right[0] > 400 && last_ldmode == STOP)        // 右手摇杆打左
                        // {   
                        //     ldmode = GUANJIE;
                        //     ref_rot[0] = rec_right[4];
                        //     ref_rot[1] = rec_right[5];
                        //     ref_rot[2] = rec_right[6];

                        //     ref_ang[0] = js_angle[0];
                        //     ref_ang[1] = js_angle[1];
                        //     ref_ang[2] = js_angle[2];

                        //     angle1 = (double)(rec_right[4] - ref_rot[0]) /500.0 * 180.0 + ref_ang[0] * RAD2DEG;
                        //     angle2 = (double)(rec_right[5] - ref_rot[1]) /500.0 * 60.0 + ref_ang[1] * RAD2DEG;
                        //     angle3 = (double)(rec_right[6] - ref_rot[2]) /500.0 * 180.0 + ref_ang[2] * RAD2DEG;

                        //     last_angle1 = angle1;
                        //     last_angle2 = angle2;
                        //     last_angle3 = angle3;

                        // }
                        // else if (rec_right[0] < -400 && last_ldmode == STOP)
                        // {
                        //     ldmode = HUILING;

                        //     ref_ang[0] = js_angle[0];
                        //     ref_ang[1] = js_angle[1];
                        //     ref_ang[2] = js_angle[2];
                        // }

                        // if (ldmode == GUANJIE)
                        // {
                        //     angle1 = (double)(rec_right[4] - ref_rot[0]) /500.0 * 180.0 + ref_ang[0] * RAD2DEG;
                        //     angle2 = (double)(rec_right[5] - ref_rot[1]) /500.0 * 60.0 + ref_ang[1] * RAD2DEG;
                        //     angle3 = (double)(rec_right[6] - ref_rot[2]) /500.0 * 180.0 + ref_ang[2] * RAD2DEG;

                        //     if (angle1 - last_angle1 > max_step_angle)
                        //     {
                        //         angle1 = last_angle1 + max_step_angle;
                        //     }
                        //     else if (angle1 - last_angle1 < -max_step_angle)
                        //     {
                        //         angle1 = last_angle1 - max_step_angle;
                        //     }
                        //     last_angle1 = angle1;

                        //     if (angle2 - last_angle2 > max_step_angle)
                        //     {
                        //         angle2 = last_angle2 + max_step_angle;
                        //     }
                        //     else if (angle2 - last_angle2 < -max_step_angle)
                        //     {
                        //         angle2 = last_angle2 - max_step_angle;
                        //     }
                        //     last_angle2 = angle2;

                        //     if (angle3 - last_angle3 > max_step_angle)
                        //     {
                        //         angle3 = last_angle3 + max_step_angle;
                        //     }
                        //     else if (angle3 - last_angle3 < -max_step_angle)
                        //     {
                        //         angle3 = last_angle3 - max_step_angle;
                        //     }
                        //     last_angle3 = angle3;
                            
                        //     if (js_ready == 1){
                        //         sprintf(sendbuf,"moveFollow(3,%.3f,%.3f,%.3f,%.3f,%.3f)\n", angle3 * DEG2RAD, angle1 * DEG2RAD, angle2 * DEG2RAD, -angle2 * DEG2RAD, angle1 * DEG2RAD);
                        //         UDP_send(sendbuf);
                        //     }
                        // }

                        // if (ldmode == HUILING)
                        // {
                        //     // TODO：如果旋扭回到中值附近则发送对应零给关节
                        //     if (abs(rec_right[4]) <rDEADZONE){
                        //         angle1 = 0.0;
                        //     }
                        //     else {
                        //         angle1 = ref_ang[0];
                        //     }
                    
                        //     if (abs(rec_right[5]) <rDEADZONE){
                        //         angle2 = 0.0;
                        //     }
                        //     else {
                        //         angle2 = ref_ang[1];
                        //     }

                        //     if (abs(rec_right[6]) <rDEADZONE){
                        //         angle3 = 0.0;
                        //     }
                        //     else {
                        //         angle3 = ref_ang[2];
                        //     }

                        //     if (last_ldmode == STOP)
                        //     {
                        //         sprintf(sendbuf,"moveJ(3,%.3f,%.3f,%.3f,%.3f,%.3f,0.3)\n", angle3, angle1, angle2, -angle2, angle1);
                        //         UDP_send(sendbuf);
                        //     } 
                        // }

                        // last_ldmode = ldmode;
                    }
                    else if (rec_right[7] > 400)        // 右手拨杆向前
                    {
                        beta_cmd = float( rec_right[4] ) / 500.0 * leftarmbeta;
                        if (leftarmbeta > 0){
                            if (beta_cmd < 0){
                                beta_cmd = 0;
                            }
                        }
                        else if (leftarmbeta < 0){
                            if (beta_cmd > 0){
                                beta_cmd = 0;
                            }
                        }

                        if (rec_right[5] > 0){
                            intool = 1;
                        }
                        else {
                            intool = 0;
                        }

                        if (rec_right[6] > 0)       // 使用遥控器触发speedl
                        {
                            if (rec_right[2] > 550 && rec_right[2] < 800){
                                speedx = 0;
                                speedy = 0;
                                speedz = 0;
                                speedRx = float( rec_right[1] ) / 500.0 * armRx;
                                speedRy = float( rec_right[3] ) / 500.0 * armRy;
                                speedRz = float( rec_right[0] ) / 500.0 * armRz;
                                if(rec_right[8] > 200)
                                {
                                    left_once = 0;
                                    sprintf(sendbuf,"speedL(0,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d)\n", speedx, speedy, speedz, speedRx, speedRy, speedRz, beta_cmd, intool);
                                    UDP_send(sendbuf);
                                }
                            }
                            else if (rec_right[2] < 450){
                                speedRx = 0;
                                speedRy = 0;
                                speedRz = 0;
                                speedx = float( rec_right[1] ) / 500.0 * armx;
                                speedy = float( rec_right[3] ) / 500.0 * army;
                                speedz = float( rec_right[0] ) / 500.0 * armz;
                                if(rec_right[8] > 200)
                                {
                                    left_once = 0;
                                    sprintf(sendbuf,"speedL(0,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d)\n", speedx, speedy, speedz, speedRx, speedRy, speedRz, beta_cmd,intool);
                                    UDP_send(sendbuf);
                                }
                            }
                            else if (rec_right[2] > 880){
                                if(rec_right[8] > 200)
                                {
                                    if (left_once == 1)
                                    {
                                        left_once = 0;
                                        sprintf(sendbuf,"moveJ(0,0,0,600,0,0,0,0.3)\n");
                                        UDP_send(sendbuf);
                                    }
                                }
                            }
                        }
                        else        // 使用六维触发speedl
                        {
                            if (joy_ready)
                            {
                                sprintf(sendbuf,"speedL(0,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d)\n", joy[0], joy[1], joy[2], joy[3], joy[4], joy[5], beta_cmd, intool);
                                UDP_send(sendbuf);
                            }
                            else
                            {
                                ROS_WARN("please check Joy topic");
                            }
                        }

                        if (rec_right[8] < 150) {
                            if (left_once == 0)
                            {
                                left_once = 1;
                                sprintf(sendbuf,"stopMove(0)\n");
                                UDP_send(sendbuf);
                            }

                            if (rec_right[0] > 400)
                            {
                                if (rec_right[1] > 400){
                                    if (leftenonce == 0){
                                        leftenonce = 1;
                                        sprintf(sendbuf,"EnMotor(0,-1)\n");
                                        UDP_send(sendbuf);
                                    }
                                }
                                else if (rec_right[1] < -400)
                                {
                                    if (leftenonce == 0){
                                        leftenonce = 1;
                                        sprintf(sendbuf,"DisMotor(0,-1)\n");
                                        UDP_send(sendbuf);
                                    }
                                }
                                else if (abs(rec_right[1]) < DEADZONE)
                                {
                                    leftenonce = 0;
                                }
                            }
                            else if (rec_right[0] < -400)
                            {
                                if (rec_right[1] > 400){
                                    if (leftforceEnonce == 0){
                                        leftforceEnonce = 1;
                                        sprintf(sendbuf,"forceEn(0)\n");
                                        UDP_send(sendbuf);
                                    }
                                }
                                else if (rec_right[1] < -400)
                                {
                                    if (leftforceEnonce == 0){
                                        leftforceEnonce = 1;
                                        sprintf(sendbuf,"forceDis(0)\n");
                                        UDP_send(sendbuf);
                                    }
                                }
                                else if (abs(rec_right[1]) < DEADZONE)
                                {
                                    leftforceEnonce = 0;
                                }
                            }
                        }
                    }
                    else if (rec_right[7] < -400)       // 右手拨杆向后
                    {
                        beta_cmd = float( rec_right[4] ) / 500.0 * rightarmbeta;
                        if (rightarmbeta > 0){
                            if (beta_cmd < 0){
                                beta_cmd = 0;
                            }
                        }
                        else if (rightarmbeta < 0){
                            if (beta_cmd > 0){
                                beta_cmd = 0;
                            }
                        }

                        if (rec_right[5] > 0){
                            intool = 1;
                        }
                        else {
                            intool = 0;
                        }

                        if (rec_right[6] > 0)       // 使用遥控器触发speedl
                        {
                            if (rec_right[2] > 550 && rec_right[2] < 800){
                                speedx = 0;
                                speedy = 0;
                                speedz = 0;
                                speedRx = float( rec_right[1] ) / 500.0 * armRx;
                                speedRy = float( rec_right[3] ) / 500.0 * armRy;
                                speedRz = float( rec_right[0] ) / 500.0 * armRz;
                                if(rec_right[8] > 200)
                                {
                                    right_once = 0;
                                    sprintf(sendbuf,"speedL(1,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d)\n", speedx, speedy, speedz, speedRx, speedRy, speedRz, beta_cmd, intool);
                                    UDP_send(sendbuf);
                                }
                            }
                            else if (rec_right[2] < 450){
                                speedRx = 0;
                                speedRy = 0;
                                speedRz = 0;
                                speedx = float( rec_right[1] ) / 500.0 * armx;
                                speedy = float( rec_right[3] ) / 500.0 * army;
                                speedz = float( rec_right[0] ) / 500.0 * armz;
                                if(rec_right[8] > 200)
                                {
                                    right_once = 0;
                                    sprintf(sendbuf,"speedL(1,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d)\n", speedx, speedy, speedz, speedRx, speedRy, speedRz, beta_cmd, intool);
                                    UDP_send(sendbuf);
                                }
                            }
                            else if (rec_right[2] > 880){
                                if(rec_right[8] > 200)
                                {
                                    if (right_once == 1)
                                    {
                                        right_once = 0;
                                        sprintf(sendbuf,"moveJ(1,0,0,600,0,0,0,0.3)\n");
                                        UDP_send(sendbuf);
                                    }
                                }
                            }
                        }
                        else        // 使用六维触发speedl
                        {
                            if (joy_ready)
                            {
                                sprintf(sendbuf,"speedL(1,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d)\n", -joy[0], joy[1], -joy[2], -joy[3], joy[4], -joy[5], beta_cmd, intool);
                                UDP_send(sendbuf);
                            }
                            else
                            {
                                ROS_WARN("please check Joy topic");
                            }
                        }
                        

                        if (rec_right[8] < 150) {
                            if (right_once == 0)
                            {
                                right_once = 1;
                                sprintf(sendbuf,"stopMove(1)\n");
                                UDP_send(sendbuf);
                            }

                            if (rec_right[0] > 400)
                            {
                                if (rec_right[1] > 400){
                                    if (rightenonce == 0){
                                        rightenonce = 1;
                                        sprintf(sendbuf,"EnMotor(1,-1)\n");
                                        UDP_send(sendbuf);
                                    }
                                }
                                else if (rec_right[1] < -400)
                                {
                                    if (rightenonce == 0){
                                        rightenonce = 1;
                                        sprintf(sendbuf,"DisMotor(1,-1)\n");
                                        UDP_send(sendbuf);
                                    }
                                }
                                else if (abs(rec_right[1]) < DEADZONE)
                                {
                                    rightenonce = 0;
                                }
                            }
                            else if (rec_right[0] < -400)
                            {
                                if (rec_right[1] > 400){
                                    if (rightforceEnonce == 0){
                                        rightforceEnonce = 1;
                                        sprintf(sendbuf,"forceEn(1)\n");
                                        UDP_send(sendbuf);
                                    }
                                }
                                else if (rec_right[1] < -400)
                                {
                                    if (rightforceEnonce == 0){
                                        rightforceEnonce = 1;
                                        sprintf(sendbuf,"forceDis(1)\n");
                                        UDP_send(sendbuf);
                                    }
                                }
                                else if (abs(rec_right[1]) < DEADZONE)
                                {
                                    rightforceEnonce = 0;
                                }
                            }
                        }
                    }
                }
                sum = 0;
                ser.flushInput(); 
            }
        } 
        else{
            watchdog++;
            if (watchdog > param_loop_rate_ * 3){  // 3秒检测不到则认为已经加锁
                watchdog = 0;
                if (first_lock == 1)
                {
                    first_lock = 0;
                    printf("DisArmed\n");
                }
            }
        }    
        //处理ROS的信息，比如订阅消息,并调用回调函数 
        ros::spinOnce(); 
        loop_rate.sleep(); 
    } //
    ser.close();
    close(sockCli);
}
