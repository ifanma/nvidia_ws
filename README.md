# nvidia_ws
双臂机器人ROS工程

1、hand_ctrl:监听/joy话题，发布/left_hand和/right_hand话题，0、1方式控制因时手。
2、dealscript:监听/drScript服务，转为udp指令给工控机。
3、inspire_hand:因时手驱动包，使用其中接收话题运动的节点。
4、ip_camera:网络摄像头包，拉取rtsp流推出话题，用republish压缩。
5、manipulator:双遥控器包，臂车分离。
6、mobile_dual_arm_robot:双臂机器人模型包。
7、rob_feedback:接受工控机udp信息，转为话题发布，/joint_states, /wrench_left, /wrench_right, /system_state。
8、robot_msgs:自定义服务与消息包。
9、serialdevice:串口设备，包括单个控的节点，电源板管理节点，头部云台节点，末端夹爪节点，sbus遥控器节点。
10、toolbase:快换接口的驱动包。
11、veltrans:/cmd_vel话题转为udp指令节点。
12、zed-ros-wrapper:zed-ros驱动包。
