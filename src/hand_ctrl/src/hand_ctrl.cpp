#include "ros/ros.h"
#include "std_msgs/String.h"
#include "sensor_msgs/Joy.h"
#include "std_msgs/Int32MultiArray.h"

bool b_leftup = 0;
bool b_leftdw = 0;
bool b_rightup = 0;
bool b_rightdw = 0;

std::string topic_l;
std::string topic_r;

static float left_angle = 1000;
static float right_angle = 1000;

std_msgs::Int32MultiArray l;
std_msgs::Int32MultiArray r;

float step = 0.01;
int  rate = 20;

int ready = 0;

ros::Publisher publ;
ros::Publisher pubr;
//回调函数
void chatterCallback(const sensor_msgs::Joy::ConstPtr& joy)
{
    b_leftup = joy->buttons[2];
    b_rightup = joy->buttons[3];
    b_leftdw = joy->buttons[9];
    b_rightdw = joy->buttons[5];

    ready = 1;
}

int main(int argc, char **argv)
{
 	ros::init(argc,argv,"hand_ctrl");
 	ros::NodeHandle n("~");

    n.getParam("left_topic", topic_l);
    n.getParam("right_topic", topic_r);
    n.getParam("move_step", step);
    n.getParam("loop_rate", rate);
 	ros::Subscriber sub = n.subscribe("/joy", 10, chatterCallback);

    publ = n.advertise<std_msgs::Int32MultiArray>(topic_l.c_str(), 10);
    pubr = n.advertise<std_msgs::Int32MultiArray>(topic_r.c_str(), 10);

    ros::Rate loop_rate(rate);
    
    ROS_INFO("control node init ready!");

    while(!ros::isShuttingDown())
    {
        if (b_leftup)
        {
            left_angle += step;
        }
        else if (b_leftdw){
            left_angle -= step;
        }

        if (b_rightup)
        {
            right_angle += step;
        }
        else if (b_rightdw){
            right_angle -= step;
        }

        if (right_angle < 0) right_angle = 0;
        if (right_angle > 1000) right_angle = 1000;
        if (left_angle < 0) left_angle = 0;
        if (left_angle > 1000) left_angle = 1000;

        ROS_INFO("%d, %d, %d, %d, %f, %f\n", b_leftup, b_leftdw, b_rightup, b_rightdw, left_angle, right_angle);

        l.data.clear();
        l.data.push_back(left_angle);
        l.data.push_back(left_angle);
        l.data.push_back(left_angle);
        l.data.push_back(left_angle);
        l.data.push_back(left_angle);
        l.data.push_back(0);

        r.data.clear();
        r.data.push_back(right_angle);
        r.data.push_back(right_angle);
        r.data.push_back(right_angle);
        r.data.push_back(right_angle);
        r.data.push_back(right_angle);
        r.data.push_back(0);

        if (ready)
        {
            ready = 0;
            publ.publish(l);
            pubr.publish(r);
        }


        ros::spinOnce();
        loop_rate.sleep();
    }
	return  0;
}