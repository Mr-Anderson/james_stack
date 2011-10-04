/*******************************************************************************
 * @file Edge_Detection.cpp
 * @author James Anderson <jra798>
 * @version 1.0
 * @brief finds edges in image using sobel and then outputs a greyscale image of edges
 ******************************************************************************/


/***********************************************************
* ROS specific includes
***********************************************************/
#include "ros/ros.h"

/***********************************************************
* Message includes
***********************************************************/
#include <usb_rocket/rocket.h>

/***********************************************************
* Other includes
***********************************************************/
#include <iostream>
#include <fstream>
//#include <dynamic_reconfigure/server.h>
//#include <MST_Edge_Detection/Edge_Detection_ParamsConfig.h>
#include <stdio.h>


/***********************************************************
* Global variables
***********************************************************/

//I dont konw how to pass vars into a callback
usb_dev_handle* launcher;

//MST_Edge_Detection::Edge_Detection_ParamsConfig params;

/***********************************************************
* Function prototypes
***********************************************************/

/***********************************************************
* Namespace Changes
***********************************************************/
using std::string;

/***********************************************************
* Private Functions
***********************************************************/

/***********************************************************
* @fn movement_handler(char control)
* @brief sends the specifided message to the usb device
* @param takes the device handler message and inex to be sent
* @return gives back the delay of the message
***********************************************************///wrapper for control_msg
int sendMessage(char* msg, int index)
{
  int j = usb_control_msg(launcher, USB_DT_HID, USB_REQ_SET_CONFIGURATION, USB_RECIP_ENDPOINT, index, msg, 8, 1000);

  //be sure that msg is all zeroes again
  msg[0] = 0x0;
  msg[1] = 0x0;
  msg[2] = 0x0;
  msg[3] = 0x0;
  msg[4] = 0x0;
  msg[5] = 0x0;
  msg[6] = 0x0;
  msg[7] = 0x0;

  return j;
}

/***********************************************************
* @fn movementHandler(char control)
* @brief sends the specifided message to the usb device
* @param takes usb device handler and hex command
***********************************************************/
void movementHandler( char control)
{
  char msg[8];
  
  //reset
  msg[0] = 0x0;
  msg[1] = 0x0;
  msg[2] = 0x0;
  msg[3] = 0x0;
  msg[4] = 0x0;
  msg[5] = 0x0;
  msg[6] = 0x0;
  msg[7] = 0x0;

  //send 0s
  int delay = sendMessage( msg, 1);

  //send control
  msg[0] = control;
  delay = sendMessage( msg, 0);

  //and more zeroes
  delay = sendMessage( msg, 1);
}


/***********************************************************
* Message Callbacks
***********************************************************/

/***********************************************************
* @fn imageCallback(const sensor_msgs::ImageConstPtr& msg)
* @brief preforms sobel edge detection on image
* @pre takes in a ros message of a raw or cv image
* @post publishes a CV_32FC1 image using cv_bridge
* @param takes in a ros message of a raw or cv image 
***********************************************************/
void rocketCallback(const usb_rocket::rocket& msg)
{
  if (msg.up)
  {
    movementHandler(1);
  }

  else if (msg.down)
  {
    movementHandler(2);
  }   

  else if (msg.left)
  {
    movementHandler(4);
  }
  else if (msg.right)
  {
    movementHandler(8);
  } 
  else if (msg.fire)
  {
    movementHandler(10);
  }
  else 
  {
    //could also be 10 or 16
    movementHandler(0);
  }
  
}



/***********************************************************
* @fn setparamsCallback(const sensor_msgs::ImageConstPtr& msg)
* @brief callback for the reconfigure gui
* @pre has to have the setup for the reconfigure gui
* @post changes the parameters
***********************************************************/
/*
void setparamsCallback(usb_rocket::usb_rocket_ParamsConfig &config, uint32_t level)
{
  
  
  if(config.third_order && !config.second_order)
  {
  	config.third_order = false; 
  }
  
  // set params
  params = config;
  
}
*/

/***********************************************************
* @fn main(int argc, char **argv)
* @brief starts the usb_rocket node and runs usb inialization and shutdown
***********************************************************/
int main(int argc, char **argv)
{
  //setup node
  ros::init(argc, argv, "usb_rocket");
  ros::NodeHandle n;
  
  //setup dynamic reconfigure gui
  //dynamic_reconfigure::Server<MST_Edge_Detection::Edge_Detection_ParamsConfig> srv;
  //dynamic_reconfigure::Server<MST_Edge_Detection::Edge_Detection_ParamsConfig>::CallbackType f;
  //f = boost::bind(&setparamsCallback, _1, _2);
  //srv.setCallback(f);
  
  
  //get topic name
  string topic;
  ros::Subscriber rocket_sub;
  topic = n.resolveName("rocket");
  rocket_sub = n.subscribe( topic , 1, rocketCallback  );

  //usb setup
  struct usb_bus *busses, *bus;
  struct usb_device *dev = NULL;
  
  //setup variables for finding and tracking launcher
  int claimed;
  bool found = false ;
  
  //run usb initialization code
  usb_init();
  usb_find_busses();
  usb_find_devices();
  
  //search all usb busses for rocket launcher
  busses = usb_get_busses();
  for (bus = busses; bus && !dev; bus = bus->next) 
  {
    for (dev = bus->devices; dev; dev = dev->next) 
    {
      //0x0a81 is green launcher 0x0701 is blue launcher
      if (dev->descriptor.idVendor == 0x0a81 && dev->descriptor.idProduct == 0x0701) 
      {
        //set launcher pointer to the device
        launcher = usb_open(dev);
        found = true;
      }
    }
  }
  
  if(found)
  {
    ROS_INFO("Found rocket launcher");
    
    //detach device from another program
    usb_detach_kernel_driver_np(launcher, 0);
    usb_detach_kernel_driver_np(launcher, 1);
    
    //try to claim device
    claimed = usb_set_configuration(launcher, 1);
    claimed = usb_claim_interface(launcher, 0);
    
    if(claimed == 0)
    {
      //run callbacks
      ros::spin();
    }
    else
    {
      ROS_INFO("Unable to claim rocket launcher");
    }
  }
  else
  {
    ROS_INFO("Unable to find rocket launcher");
  }
  
  //release rocket launcher and close
  usb_release_interface(launcher, 0);
  usb_close(launcher);
  
  return 0;
}

