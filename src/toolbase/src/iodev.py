#!/usr/bin/env python

import Jetson.GPIO as GPIO
import time
import rospy
from std_msgs.msg import Int8

LED = 12

def subcb(cmd):
    if cmd.data == 1:
        GPIO.output(LED, GPIO.LOW)
        print("in 1")
    if cmd.data == 0:
        GPIO.output(LED, GPIO.HIGH)
        print("in 0")


def main():

    rospy.init_node("iodev")
    sub = rospy.Subscriber("/toolcmd", Int8, subcb)
    
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(LED, GPIO.OUT, initial=GPIO.HIGH)

    rospy.spin()
    GPIO.cleanup()
        
if __name__=='__main__':
    main()