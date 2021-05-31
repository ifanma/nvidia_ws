# nvidia_ws
双臂机器人ROS工程

1、dealscript:监听/drScript服务，转为udp指令给工控机。

2、ip_camera:网络摄像头包，拉取rtsp流推出话题，用republish压缩。

3、arm_model:双臂机器人模型包。

4、rob_feedback:接受工控机udp信息，转为话题发布，/joint_states, /wrench_left, /wrench_right, /system_state。

5、robot_msgs:自定义服务与消息包。

6、serialdevice:串口设备，包括单个控的节点，电源板管理节点，头部云台节点，末端夹爪节点，sbus遥控器节点。

7、zed-ros-wrapper:zed-ros驱动包。