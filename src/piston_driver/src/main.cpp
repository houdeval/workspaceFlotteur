#include <iostream>
#include <unistd.h>

#include <ros/ros.h>
#include <std_srvs/SetBool.h>
#include <std_msgs/UInt16.h>
#include <std_srvs/Empty.h>

#include "piston.h"
#include "piston_driver/PistonSpeed.h"
#include "piston_driver/PistonState.h"

using namespace std;

Piston p;
bool state_start = false;
uint16_t cmd_position_piston = 0;
uint16_t new_cmd_position_piston = 0;
bool state_emergency = false;

bool piston_enable(std_srvs::SetBool::Request  &req,
                   std_srvs::SetBool::Response &res){
    if(req.data == true)
        p.set_piston_enable(true);
    else
        p.set_piston_enable(false);

    res.success = true;
    return true;
}

bool piston_reset(std_srvs::Empty::Request  &req,
                  std_srvs::Empty::Response &res){
    p.set_piston_reset();
    return true;
}

bool piston_start(std_srvs::SetBool::Request  &req,
                  std_srvs::SetBool::Response &res){
    if(req.data == true){
        p.set_piston_start();
        state_start = true;
    }
    else{
        p.set_piston_stop();
        state_start = false;
    }

    res.success = true;
    return true;
}

bool piston_speed(piston_driver::PistonSpeed::Request  &req,
                  piston_driver::PistonSpeed::Response &res){
    p.set_piston_speed(req.speed);
    return true;
}

bool piston_emergency(std_srvs::SetBool::Request  &req,
                  std_srvs::SetBool::Response &res){
    if(req.data == true){
      new_cmd_position_piston = 0;
      p.set_piston_position(0);
      state_emergency = true;
    }
    else{
        state_emergency = false;
    }

    res.success = true;
    return true;
}

void position_callback(const std_msgs::UInt16::ConstPtr& msg){
    new_cmd_position_piston = msg->data;
}

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "piston");
    ros::NodeHandle n;

    // Parameters
    ros::NodeHandle n_private("~");
    double frequency = n_private.param<double>("frequency", 5.0);

    // Service (ON/OFF)
    ros::ServiceServer service_enable = n.advertiseService("enable", piston_enable);
    ros::ServiceServer service_start = n.advertiseService("start", piston_start);
    ros::ServiceServer service_speed = n.advertiseService("speed", piston_speed);
    ros::ServiceServer service_reset = n.advertiseService("reset", piston_reset);

    ros::ServiceServer service_emergency = n.advertiseService("emergency", piston_emergency);

    // Subscriber
    ros::Subscriber position_sub = n.subscribe("cmd_position_piston", 1, position_callback);

    // Publisher
    ros::Publisher state_pub = n.advertise<piston_driver::PistonState>("state", 1);
    piston_driver::PistonState state_msg;

    // Sensor initialization
    p.i2c_open();

    ros::Rate loop_rate(frequency);
    while (ros::ok()){
        ros::spinOnce();

        if(state_start && state_emergency==false){
            if(cmd_position_piston != new_cmd_position_piston){
                cmd_position_piston = new_cmd_position_piston;
                p.set_piston_position(cmd_position_piston);
            }
        }

        p.update_piston_all_data();
        state_msg.header.stamp = ros::Time::now();
        state_msg.position = p.m_position;
        state_msg.switch_out = p.m_switch_out;
        state_msg.switch_in = p.m_switch_in;
        state_msg.state = p.m_state;
        state_msg.system_on = p.m_system_on;
        state_msg.motor_on = p.m_motor_on;
        state_msg.enable_on = p.m_enable_on;
        state_msg.position_set_point = p.m_position_set_point;
        state_msg.motor_speed = p.m_motor_speed;
        state_pub.publish(state_msg);

        loop_rate.sleep();
    }

    return 0;
}