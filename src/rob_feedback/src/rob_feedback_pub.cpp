#include "ros/ros.h"
#include "std_msgs/String.h"
#include "sensor_msgs/JointState.h"
#include "geometry_msgs/WrenchStamped.h"
#include "robot_msgs/systemState.h"

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
#define MAX_IP 10

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
	js.name.resize(7);
	js.name = {"armjoint1","armjoint2","armjoint3","armjoint4","armjoint5","armjoint6","armjoint7"};
	js.position.resize(js.name.size());
	js.effort.resize(js.name.size());
	js.velocity.resize(js.name.size());

	geometry_msgs::WrenchStamped w_l;

	ros::Publisher js_pub = n.advertise<sensor_msgs::JointState>("joint_states", 1000);
	ros::Publisher wrc_pub = n.advertise<geometry_msgs::WrenchStamped>("wrench", 1000);
	ros::Publisher ss = n.advertise<robot_msgs::systemState>("systemstate", 1000);
	ros::Rate loop_rate(1000);

	int rec_len = 0;
	std::string s;
	std::vector<std::string> vStr;
	int pub_cnt = 0;
	int i = 0;
	double ft[6] = {0.0};
	robot_msgs::systemState ss_;

	while (ros::ok())
	{
		rec_len = recvfrom(sockSer, recvbuf, sizeof(recvbuf), MSG_DONTWAIT, (struct sockaddr*)&addrCli, &addrlen);
		if (rec_len>0)
		{
			// ROS_INFO("Cli:>%s\n", recvbuf);
			js.header.stamp = ros::Time::now();
			w_l.header.stamp = ros::Time::now();
			w_l.header.frame_id = "leftarm_link7";

			s = recvbuf;
			try{

				boost::split( vStr, s, boost::is_any_of( "," ), boost::token_compress_on );
				ROS_INFO_STREAM(vStr.size());
				for( i = 0; i < js.name.size(); i++)
				{
					js.position.at(i) = atof(vStr.at(i).c_str());
					if (isnan(js.position.at(i)))
					{
						js.position.at(i) = 0;
					}

					js.velocity.at(i) = atof(vStr.at(i + 7).c_str());
					if (isnan(js.velocity.at(i)))
					{
						js.velocity.at(i) = 0;
					}

					js.effort.at(i) = atof(vStr.at(i + 14).c_str());
					if (isnan(js.effort.at(i)))
					{
						js.effort.at(i) = 0;
					}

					// printf("%d,%f,%f,%f\n",i, js.position.at(i),js.effort.at(i), js.velocity.at(i));
				}

				for (i = 0; i< 6; i++)
				{
					ft[i] = atof(vStr.at(i + 21).c_str());
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

				ss_.header.stamp = ros::Time::now();
				ss_.voltage = atof(vStr.at(27).c_str());
				ss_.time_elapsed = atof(vStr.at(28).c_str());
				ss_.motor_state.clear();

				for (i = 0; i< 7; i++)
				{
					int data = atoi(vStr.at(29 + i).c_str());
					if (data > 1){
						data = 2;
					}
					ss_.motor_state.push_back(data);
				}

				ss_.arm_state = atoi(vStr.at(36).c_str());
				ss_.arm_fctrl_state = atoi(vStr.at(37).c_str());

				if (pub_cnt > int(1000.0/param_rate))
				{
					pub_cnt = 0;
					js_pub.publish(js);
					wrc_pub.publish(w_l);
					ss.publish(ss_);
				}
				pub_cnt ++;
			}
			catch(std::exception e1){
				ROS_WARN("failed");
			}	
		}
        
		ros::spinOnce();
		loop_rate.sleep();
	}
	close(sockSer);

	return 0;
}
