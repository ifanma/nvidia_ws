#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Float32.h"
#include "sensor_msgs/JointState.h"
#include "geometry_msgs/WrenchStamped.h"
#include "serial_dev_msgs/systemState.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <sstream>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
 

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <ifaddrs.h>

#define MAX_LENGTH 254
# define MAX_IP 10

int port = 0;
std::string hostIP;
char recvbuf[1024];
int param_rate;

/**
 * This tutorial demonstrates simple sending of messages over the ROS system.
 */
int main(int argc, char *argv[])
{
	ros::init(argc, argv, "rob_feedback");
	ros::NodeHandle n;
	struct sockaddr_in addrCli;
	struct sockaddr_in addrSer;
 
	n.param<int>("rob_feedback/port",port, 0);
	n.param<std::string>("rob_feedback/hostIP",hostIP, "127.0.0.1");
	n.param<int>("rob_feedback/publish_rate",param_rate, 100);
	std::cout<<hostIP<<std::endl;

	int sockSer = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockSer == -1)
    {
        perror("socket");
        exit(1);
    }

    addrSer.sin_family = AF_INET;
    addrSer.sin_port = htons(port);//????
    addrSer.sin_addr.s_addr = inet_addr(hostIP.c_str());//????????? 0?127.0.0.1?? 1?????ip
 
    socklen_t addrlen = sizeof(struct sockaddr);
    int ret = bind(sockSer, (struct sockaddr*)&addrSer, addrlen);
    if(ret == -1)
    {
        perror("bind failed");
        exit(1);
    }
 
	// Jointstates things
	sensor_msgs::JointState js;
	js.name.resize(22);
	js.name = {"leftarm_joint1","leftarm_joint2","leftarm_joint3","leftarm_joint4","leftarm_joint5","leftarm_joint6","leftarm_joint7",
				"rightarm_joint1","rightarm_joint2","rightarm_joint3","rightarm_joint4","rightarm_joint5","rightarm_joint6","rightarm_joint7",
				"dt_joint","yt_joint","pt_joint","yao_joint", 
				"leftleg_joint1","leftleg_joint2","rightleg_joint1","rightleg_joint2"};
	js.position.resize(js.name.size());
	js.effort.resize(js.name.size());
	js.velocity.resize(js.name.size());

	geometry_msgs::WrenchStamped w_r;
	geometry_msgs::WrenchStamped w_l;

	ros::Publisher js_pub = n.advertise<sensor_msgs::JointState>("joint_states", 1000);
	ros::Publisher wrc_l_pub = n.advertise<geometry_msgs::WrenchStamped>("wrench_left", 1000);
	ros::Publisher wrc_r_pub = n.advertise<geometry_msgs::WrenchStamped>("wrench_right", 1000);
	ros::Publisher ss_pub = n.advertise<serial_dev_msgs::systemState>("system_state", 1000);	ros::Rate loop_rate(1000);

	int rec_len = 0;
	std::string s;
	std::vector<std::string> vStr;
	double joint[14];
	int pub_cnt = 0;
	int i = 0, j = 0;
	double ft[12] = {0.0};
	int lastsize = 0;

	serial_dev_msgs::systemState ss;

	while (ros::ok())
	{
	
	// 22 关节角， 22 电流 + 4履带电机电流， 12 力传感器数据
		rec_len = recvfrom(sockSer, recvbuf, sizeof(recvbuf), MSG_DONTWAIT, (struct sockaddr*)&addrCli, &addrlen);
		if (rec_len>0)
		{
			// ROS_INFO("Cli:>%s\n", recvbuf);
			js.header.stamp = ros::Time::now();
			w_r.header.stamp = ros::Time::now();
			w_r.header.frame_id = "toollink_r";
			w_l.header.frame_id = "toollink_l";
			w_l.header.stamp = ros::Time::now();
			ss.header.stamp = ros::Time::now();

			s = recvbuf;
			try{

				boost::split( vStr, s, boost::is_any_of( "," ), boost::token_compress_on );
				if (vStr.size() != lastsize){
					ROS_INFO_STREAM(vStr.size());
				}
				lastsize = vStr.size();
				for( i = 0; i < js.name.size(); i++)
				{
					js.position.at(i) = atof(vStr.at(i).c_str());
					if (isnan(js.position.at(i)))
					{
						js.position.at(i) = 0;
					}

					if (i < 14)
					{
						js.velocity.at(i) = atof(vStr.at(i + 61).c_str());
						if (isnan(js.velocity.at(i)))
						{
							js.velocity.at(i) = 0;
						}
					}

					js.effort.at(i) = atof(vStr.at(i + 22).c_str());
					if (isnan(js.effort.at(i)))
					{
						js.effort.at(i) = 0;
					}

					// printf("%d,%f,%f,%f\n",i, js.position.at(i),js.effort.at(i), js.velocity.at(i));
				}

				for (i = 0; i< 4; i++)
				{
					js.velocity.at(i + 17) = atof(vStr.at(22 + 22 + i).c_str());
					if (isnan(js.velocity.at(i + 18)))
					{
						js.velocity.at(i + 18) = 0;
					}
				}

				for (i = 0; i< 12; i++)
				{
					ft[i] = atof(vStr.at(48 + i).c_str());
					if (isinf(ft[i]))
					{
						ft[i] = 0;
					}
				}
				w_l.wrench.force.x = ft[0];
				w_l.wrench.force.y = ft[1];
				w_l.wrench.force.z = ft[2];
				w_l.wrench.torque.x = ft[3];
				w_l.wrench.torque.y = ft[4];
				w_l.wrench.torque.z = ft[5];

				w_r.wrench.force.x = ft[6];
				w_r.wrench.force.y = ft[7];
				w_r.wrench.force.z = ft[8];
				w_r.wrench.torque.x = ft[9];
				w_r.wrench.torque.y = ft[10];
				w_r.wrench.torque.z = ft[11];

				ss.voltage = atof(vStr.at(60).c_str());

				i = 75;
				ss.left_motor_state.clear();
				for (j = 0; j< 7; j++)
				{
					ss.left_motor_state.push_back(atof(vStr.at(i ++).c_str()));
				}
				ss.leftarm_state = atof(vStr.at(i ++).c_str());
				ss.leftarm_fctrl_state = atof(vStr.at(i ++).c_str());

				ss.right_motor_state.clear();
				for (j = 0; j< 7; j++)
				{
					ss.right_motor_state.push_back(atof(vStr.at(i ++).c_str()));
				}
				ss.rightarm_state = atof(vStr.at(i ++).c_str());
				ss.rightarm_fctrl_state = atof(vStr.at(i ++).c_str());

				ss.head_motor_state.clear();
				for (j = 0; j< 3; j++)
				{
					ss.head_motor_state.push_back(atof(vStr.at(i ++).c_str()));
				}
				ss.head_state = atof(vStr.at(i ++).c_str());

				ss.leg_motor_state.clear();
				for (j = 0; j< 5; j++)
				{
					ss.leg_motor_state.push_back(atof(vStr.at(i ++).c_str()));
				}
				ss.leg_state = atof(vStr.at(i ++).c_str());

				ss.trc_motor_state.clear();
				for (j = 0; j< 4; j++)
				{
					ss.trc_motor_state.push_back(atof(vStr.at(i ++).c_str()));
				}
				ss.trc_state = atof(vStr.at(i ++).c_str());

				ss.time_elapsed = ((float)atol(vStr.at(i).c_str())) / 1000000.0;

				if (i - 75 != 33){
					ROS_INFO("wrong index: %d", i - 75);
				}

				if (pub_cnt > int(1000.0/param_rate))
				{
					pub_cnt = 0;
					js_pub.publish(js);
					wrc_l_pub.publish(w_l);
					wrc_r_pub.publish(w_r);
					ss_pub.publish(ss);
				}
				pub_cnt ++;
			}
			catch(std::exception e1){
				ROS_WARN("deal failed\n");
				// ROS_INFO_STREAM(e1.what());
			}	
		}
        
		ros::spinOnce();
		loop_rate.sleep();
	}
	close(sockSer);

	return 0;
}
