//C++ includes
#include <iostream>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <sstream>
#include "ComTelnet.h"
#include "configure.h"
#include <stdio.h>
#include <usb.h>

//#include "'args.h"

using namespace std;
/*
using std::cout;
using std::cerr; 
using std::endl;
using std::log;
using std::memset;
*/
//C Includes

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>



#include <libcwiimote/wiimote.h>
#include <libcwiimote/wiimote_api.h>
}

//LocalIncludes
#include "DriveTank.h"

#define MAX_AXIS 16
#define MAX_BUTTONS MAX_AXIS

int nrstate = 0 , nrinc = 1;
int nrwait = 3;

bool sbtog = 1, standby = 0;

double SymLog( const double in )
	{
	if( in > 0 )
		{
		return log( in );
		}
	if( in < 0 )
		{
		return -log( -in );
		}
	return 0;
	}

double trim( const double in, const double amplitude )
	{
	if( in > fabs(amplitude) )
		{
		return fabs(amplitude);
		}

	if( in < -fabs(amplitude) )
		{
		return -fabs(amplitude);
		}
	return in;
	}


void StopRobot( ComTelnet & Module );

void fnExit(void)
{
//	ComTelnet ComModule( MAESTRO_IP, TELNET_PORT);
//	ComModule.Read();
//	StopRobot( ComModule );
	cout<<"LOL I QUIT!";
	//system("./wiimote_control");
	exit(0);
}

void ConnectWiimote(wiimote_t &wiimote)
{
	//Keep attempting to connect to wiimote until wiimote is connected	
	printf("Press buttons 1 and 2 on the wiimote now to connect.\n");
	int connected=1;
	while(connected!=0){
		connected = wiimote_connect(&wiimote, WIIMOTE_ADDR);
	}
	//After wiimote is connected, light the first LED to indicate successful connection
	wiimote.led.one  = 1;
}

void StopRobot( ComTelnet & Module )
{
/**	stringstream ssbuf;
	string sbuf="";
	for (char channel = 0; channel < NUMBER_OF_MOTORS; ++channel)
		{
		sbuf += "d";
		sbuf += ( '0' + channel + 1 );
		sbuf += ".mo=0;";
		}
	Module.Write( sbuf );
	cout<<"Shutdown Motors with " <<sbuf<<endl;
*/	//sbuf = Module.Read();
	//cout<<"Got Echo:"<<sbuf<<endl;
}

void NightRiderNextState(  wiimote_t &wiimote )
{
	const int nrone[] = {1,1,0,0,0,0,0},
	    nrtwo[]       = {0,1,1,1,0,0,0},
	    nrthree[]     = {0,0,0,1,1,1,0},
	    nrfour[]      = {0,0,0,0,0,1,1};
	
	if( nrstate < 6 & nrinc == 1)
	{
		nrstate++;
	}
	
	else if( nrstate > 0 & nrinc == 0)
	{
		nrstate--;
	}
	else if( nrstate == 6 )
	{
		nrstate--;
		nrinc = 0;
	}
	
	else if( nrstate == 0)
	{
		nrstate++;
		nrinc = 1;
	}
	
	std::cout << "nrstate = "<< nrstate << "    nrinc = " << nrinc 
		                << std::endl;
	wiimote.led.one = nrone[nrstate] ;
	wiimote.led.two = nrtwo[nrstate] ;
	wiimote.led.three = nrthree[nrstate] ;
	wiimote.led.four = nrfour[nrstate] ;
	    

}



int main (int argc, char **argv)
{
	int numOfDrivetanks = 1;
	
	clock_t nrclock;
	nrclock = clock () + nrwait * CLOCKS_PER_SEC;
	
	DriveTank Tank;
	while(1)
	{
		
		cout<<"Opening Wiimote!"<<endl;
		wiimote_t wiimote = WIIMOTE_INIT;
		//wiimote_report_t report = WIIMOTE_REPORT_INIT;
		ConnectWiimote(wiimote);
		atexit (fnExit);
		
		struct usb_bus *busses, *bus;
		struct usb_device *dev = NULL;
		
		usb_dev_handle *launcher;
        char data[8];
        char empty_data[8];
        int ret;
        bool is_open = false ;


		
		while (wiimote_is_open(&wiimote))
		{

			double newspeed = 0;
			double newturnrate = 0;
			double xaxis = 0.0 ;
			double yaxis = 0.0 ;
			int turbo ;
			int wiimotedisconnect = 0;
			
				if (is_open == false) 
				{
					cout<< "Unable to open device" << endl;
				}
				else
				{
					cout<< "Able to open device" << endl;
				}
			
			
			if(  clock() > nrclock  )
			{
				//NightRiderNextState(wiimote);

				nrclock = clock () + nrwait * CLOCKS_PER_SEC;
			}
			
			
			if (wiimote_update(&wiimote) < 0) {
				wiimotedisconnect=1;
			}
	      
			if (wiimote.keys.home) {
				wiimotedisconnect=1;
			}
		
			if (wiimote.keys.bits & WIIMOTE_KEY_1) {
			
					wiimote.rumble = 1;
			}
			else {
					wiimote.rumble = 0;
			}
			if (wiimote.keys.b) {
			
					turbo = 1;
					//wiimote.rumble = 1;
			}
			else {
					turbo = 0;
					//wiimote.rumble = 0;
			}
	
			wiimote.mode.acc = 1;
			
			

			std::cout << "Mode : " <<  wiimote.mode.bits
		                << std::endl;
			if ( wiimote.mode.bits == 0x35)
			{
				std::cout << "in nunchuk mode " 
		                << std::endl;
				yaxis = (wiimote.ext.nunchuk.joyy - 125) / 125.0;
				xaxis = (-(wiimote.ext.nunchuk.joyx - 125) / 125.0);
				
				if (wiimote.ext.nunchuk.keys.c && wiimote.ext.nunchuk.keys.z)
				{
					std::cout <<"Turbo!!!!!"<<std::endl;
					turbo = 2;
					
				}
				
				if(wiimote.ext.nunchuk.keys.z)
				{
					std::cout<<"Turbo"<<std::endl;
					turbo = 1;
				}
				else
				{
					std::cout<<"Normal"<<std::endl;
					turbo = 0;
				}
				
				usb_init();
				usb_find_busses();
				usb_find_devices();

				busses = usb_get_busses();

				for (bus = busses; bus && !dev; bus = bus->next) 
				{
					for (dev = bus->devices; dev; dev = dev->next) 
					{
						if (dev->descriptor.idVendor == 0x0a81 &&
							dev->descriptor.idProduct == 0x0701) 
						{
							launcher = usb_open(dev);
							is_open = true ;
						}
					}
				}
				
				usb_detach_kernel_driver_np(launcher, 0);
				usb_detach_kernel_driver_np(launcher, 1);

				ret = usb_set_configuration(launcher, 1);
				ret = usb_claim_interface(launcher, 0);
		
				memset(data, 0, 8);
				memset(empty_data, 0, 8);
				
				if (wiimote.keys.up)
				{
					data[0] = 0x01; 
				}
				
				else if (wiimote.keys.down)
				{
					data[0] = 0x02; 
				}   
				
				else if (wiimote.keys.left)
				{
					data[0] = 0x04; 
				}
				
				else if (wiimote.keys.right)
				{
					data[0] = 0x08; 
				} 
				else if (wiimote.keys.b)
				{
					data[0] = 0x10;
				}
				else 
				{
					data[0] = 0x20;
			    }

				
	
			}
	
				usb_init();
				usb_find_busses();
				usb_find_devices();

				busses = usb_get_busses();

				for (bus = busses; bus && !dev; bus = bus->next) 
				{
					for (dev = bus->devices; dev; dev = dev->next) 
					{
						if (dev->descriptor.idVendor == 0x0a81 &&
							dev->descriptor.idProduct == 0x0701) 
						{
							launcher = usb_open(dev);
							is_open = true ;
						}
					}
				}
				
				usb_detach_kernel_driver_np(launcher, 0);
				usb_detach_kernel_driver_np(launcher, 1);

				ret = usb_set_configuration(launcher, 1);
				ret = usb_claim_interface(launcher, 0);
		
				memset(data, 0, 8);
				memset(empty_data, 0, 8);
				
				if (wiimote.keys.up)
				{
					data[0] = 0x01; 
				}
				
				else if (wiimote.keys.down)
				{
					data[0] = 0x02; 
				}   
				
				else if (wiimote.keys.left)
				{
					data[0] = 0x04; 
				}
				
				else if (wiimote.keys.right)
				{
					data[0] = 0x08; 
				} 
				else if (wiimote.keys.b)
				{
					data[0] = 0x10;
				}
				else 
				{
					data[0] = 0x20;
			    }

				if (wiimote.keys.a) // tilt mode
				{
					yaxis = (wiimote.axis.y - 134) / 12.5 ;
					xaxis = (-(wiimote.axis.x - 134) / 12.5)/2 ;
				}   
		
			}
			else
			{
				yaxis = 0 ;
				xaxis = 0 ;	
			}
		
			std::cout << "x: " << xaxis
		                << "y: " << yaxis
		                << std::endl;
		                 
			if (turbo == 2)
			{
				yaxis *= 2.0;
				xaxis *= 0.75;
			}
			else if(turbo == 1)
			{
				xaxis *= 0.75;
				yaxis *= 1.5;
			}
			else if(turbo == 0)
			{
				yaxis *= 0.5;
				xaxis *= 1.0;
			}
			
			if(yaxis < 0.1)
			{
				xaxis *= 1.25;
			}
			
			
				//xaxis = trim( xaxis , 1 );
				//yaxis = trim( yaxis , 1 );
				cout << "speed: " << yaxis
		                << "turn: " << xaxis
		                << std::endl;
		                
			if (wiimote.keys.bits & WIIMOTE_KEY_2 ) {
			
					if(standby == 0 & sbtog == 1)
					{
						wiimote.led.four = 1;
						standby = 1;
						Tank.SetVelocity( 0, 0);
					}
					else if(standby == 1 & sbtog == 1)
					{
						//ComTelnet ComModule( MAESTRO_IP, TELNET_PORT);
						//ComModule.Read();
						//StopRobot( ComModule );
				          atexit (fnExit);
						wiimote.led.four = 0;
						standby = 0;
					}
					sbtog = 0;
					
					
			}
			else 
			{
					sbtog = 1;
			}
			
			
			if(standby == 0)
			{
				//if(yaxis  > 0.008 || yaxis < -0.008) {
					Tank.kickTheDog = true;
					cout << "Kicked the dog" << endl;
//				}
				Tank.SetVelocity(yaxis, xaxis);
				cout<<endl<<endl;	
			}
			
			if(is_open == true)
			{
				ret = usb_control_msg(launcher,
				USB_DT_HID,
				USB_REQ_SET_CONFIGURATION,
				USB_RECIP_ENDPOINT,
				1,
				empty_data,
				8,		// Length of data.
				5000);		// Timeout
			
				ret = usb_control_msg(launcher,
				USB_DT_HID,
				USB_REQ_SET_CONFIGURATION,
				USB_RECIP_ENDPOINT,
				0,
				data,
				8,		// Length of data.
				5000);		// Timeout
			
				ret = usb_control_msg(launcher,
				USB_DT_HID,
				USB_REQ_SET_CONFIGURATION,
				USB_RECIP_ENDPOINT,
				1,
				empty_data,
				8,		// Length of data.
				5000);		// Timeout
			}
			
			if (ret != 8) 
			{
				cout << " cant send " << endl;
			}

			usb_release_interface(launcher, 0);
			usb_close(launcher);
			
			if(wiimotedisconnect == 1)
			{
				wiimote_disconnect(&wiimote);
				Tank.SetVelocity( 0, 0);
				numOfDrivetanks++;
				usb_release_interface(launcher, 0);
				usb_close(launcher);
				//if(Tank)
				//	delete Tank;
				//StopRobot( ComModule );
			}

		}
//	ComTelnet ComModule( MAESTRO_IP, TELNET_PORT);
//	ComModule.Read();
//	StopRobot( ComModule );
	}
		//Tank.~DriveTank();
//	}
	cout<<"THIS SHOULD NEVER HAPPEN!"<<endl;
	return 0;
} 
