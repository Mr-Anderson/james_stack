#include <cwiid.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>

#include "ComTelnet.h"
#include "configure.h"

#include "DriveTank.h"

extern "C"
{
    #include <stdint.h>
}

#define SPEED       1.00     // m/s
#define TURNRATE    1.00     // rad/s (needs to be adjusted)
#define BOOST_1     1.5     // x1.5 forward speed boost
#define BOOST_2     1.75    // x1.75 forward speed boost
#define BOOST_3     2.00    // x2.00 forward speed boost
#define BOOST_4     2.50    // x2.50 forward speed boost

#define DEADSPACE_RGN 7
#define DEADSPACE(x)        ( (x <= DEADSPACE_RGN) && (x >= -DEADSPACE_RGN) ? 0 : x )

#define CALCSPEED(x)        (double)(DEADSPACE(x-127)/128.0)*SPEED
#define CALCTURNRATE(x)     (double)(DEADSPACE(x-127)/128.0)*TURNRATE*-1

void*   Wiimote_Control( void* );
void    Handle_Nunchuk( struct cwiid_state*, double&, double& );
void    Handle_Wiimote( struct cwiid_state*, double&, double& );
void    Handle_NightRider();
void    Set_LEDs();
void    handle_sigquit( int );
 
bool                running;
pthread_t           wiimote_pid;
DriveTank           Tank;

bdaddr_t            g_bdaddr    = *BDADDR_ANY;
cwiid_wiimote_t*    g_wiimote   = NULL;
struct cwiid_state  g_wii_state;
struct acc_cal      g_acc_cal;
bool                wiimote_connected;
bool                wiimote_suspended;
double              forward         = 0.0;
double              turnrate        = 0.0;

int main()
{
    signal( SIGQUIT, &handle_sigquit );
    signal( SIGINT,  &handle_sigquit );

    pthread_create( &wiimote_pid, NULL, *Wiimote_Control, NULL );
    pthread_join( wiimote_pid, NULL );
} /* main() */

void handle_sigquit( int arg )
{
    running = false;
} /* handle_sigquit() */

//----------------------------------------------------------------------
// Wiimote LED Scheme
// LED 1 == Connected to robot
// LED 2 == Have control over robot
// LED 3 == ???
// LED 4 == Motors are suspended
//----------------------------------------------------------------------

void* Wiimote_Control( void* arg )
{
    wiimote_connected           = false;
    wiimote_suspended           = false;
    
    bool        toggle_2        = false;

    running = true;
    while( running )
    {
        forward = 0.0;
        turnrate = 0.0;
        while( g_wiimote == NULL & running )
        {
            g_wiimote = cwiid_connect( &g_bdaddr, 
                CWIID_FLAG_CONTINUOUS|CWIID_FLAG_NONBLOCK);
            sleep(1);
        }
        
        if( g_wiimote != NULL )
        {
            wiimote_connected = true;
            cwiid_command( g_wiimote, CWIID_CMD_RPT_MODE, 
                CWIID_RPT_BTN       |
                CWIID_RPT_NUNCHUK   |
                CWIID_RPT_CLASSIC   |
                CWIID_RPT_ACC           );
            cwiid_get_acc_cal( g_wiimote, CWIID_EXT_NONE, &g_acc_cal );
        }
        
        while( running & wiimote_connected )
        {
            forward = 0.0;
            turnrate = 0.0;
            cwiid_get_state(g_wiimote, &g_wii_state);
            
            if((g_wii_state.buttons & CWIID_BTN_1) == CWIID_BTN_1 )
            {
                Tank.kickTheDog = true;
                Tank.SetVelocity( 0.0, 0.0 );
                Handle_NightRider();
            }
            
            if((g_wii_state.buttons & CWIID_BTN_HOME) == CWIID_BTN_HOME)
            {
                wiimote_connected = false;
                
                if( !wiimote_suspended )
                {
                    forward     = 0.0;
                    turnrate    = 0.0;

                    Tank.kickTheDog = true;
                    Tank.SetVelocity( forward, turnrate );
                }
                
                wiimote_suspended  = true;
            }
            
            if( wiimote_suspended )
            {
                if((g_wii_state.buttons & CWIID_BTN_2)==CWIID_BTN_2)
                {
                    if( toggle_2 == true ) // If haven't released 2
                    {
                        wiimote_suspended = true;
                    }
                    else
                    {
                        toggle_2 = true;
                        wiimote_suspended = false;
                    }
                }
                else
                {
                    toggle_2 = false;
                }
            }
            else /* if( !wiimote_suspended ) */
            {
                if((g_wii_state.buttons & CWIID_BTN_2)==CWIID_BTN_2)
                {
                    if( toggle_2 == true )
                    {
                        wiimote_suspended = false;
                    }
                    else
                    {
                        wiimote_suspended   = true;
                        forward     = 0.0;
                        turnrate    = 0.0;
                        toggle_2    = true;
                    }
                }
                else
                {
                    toggle_2 = false;
                }
                
                if(g_wii_state.ext_type == CWIID_EXT_NUNCHUK)
                {
                    Handle_Nunchuk( &g_wii_state, forward, turnrate );
                }
                else if(g_wii_state.ext_type == CWIID_EXT_CLASSIC)
                {
                    printf( "\tClassic Mode\n" );
                    printf( "Not currently supported\n" );
                }
                else if(g_wii_state.ext_type == CWIID_EXT_NONE)
                {
                    Handle_Wiimote( &g_wii_state, forward, turnrate );
                }
                else //Unknown, MotionPlus, Balance
                {
                    printf( "\nUnknown Mode\n" );
                }

                Tank.kickTheDog = true;
                Tank.SetVelocity( forward, turnrate );

                usleep(100000);
            } /* if( !wiimote_suspended ) */
            
            Set_LEDs();
            
        } /* while( wiimote_connected ) */
    
    wiimote_suspended = false;
    Set_LEDs();

    cwiid_disconnect( g_wiimote );
    g_wiimote = NULL;
    memset( &g_wii_state, 0, sizeof(g_wii_state) );

    sleep(5);
    
    } /* while( running ) */
} /* Wiimote_Control() */

void Handle_Nunchuk
     ( struct cwiid_state* state, double& forward, double& turnrate )
{
    uint8_t boost = 0;
    
    if((state->ext.nunchuk.buttons & CWIID_NUNCHUK_BTN_Z) == CWIID_NUNCHUK_BTN_Z)
    {
        boost++;
    }
    
    if((state->ext.nunchuk.buttons & CWIID_NUNCHUK_BTN_C) == CWIID_NUNCHUK_BTN_C)
    {
        boost++;
    }

    if((state->buttons & CWIID_BTN_A) == CWIID_BTN_A )
    {
        boost++;
    }
    
    if((state->buttons & CWIID_BTN_B) == CWIID_BTN_B )
    {
        boost++;
    }
    
    forward  = CALCSPEED(state->ext.nunchuk.stick[CWIID_Y]);
    turnrate = CALCTURNRATE(state->ext.nunchuk.stick[CWIID_X]);
    if( boost == 1 )
    {
        forward  *= BOOST_1;
        turnrate /= BOOST_1;
    }
    else if( boost == 2 )
    {
        forward  *= BOOST_2;
        turnrate /= BOOST_2;
    }
    else if( boost == 3 )
    {
        forward  *= BOOST_3;
        turnrate /= BOOST_3;
    }
    else if( boost == 4 )
    {
        forward  *= BOOST_4;
        turnrate /= BOOST_4;
    }

    printf( "\tNunchuk Mode\n" );
    printf( ( boost == 4 ? "M-M-M-MONSTER BOOST!\n" :
            ( boost == 3 ? "***ULTRA BOOST***\n" :
            ( boost == 2 ? "***TURBO BOOST***\n" :
            ( boost == 1 ? "   Boost\n" :
              "") ) ) ) );
    printf( "Raw Input\tx: %d\ty: %d\n", 
        state->ext.nunchuk.stick[CWIID_X], 
        state->ext.nunchuk.stick[CWIID_Y] );
    printf( "Output\tforward: %0.4f\tturnrate: %0.4f\n", 
        forward, turnrate );
} /* Handle_Nunchuk() */

void Handle_Wiimote
    ( struct cwiid_state* state, double& forward, double& turnrate )
{
    uint8_t boost = 0;
    double a_x, a_y, a_z;
    double pitch, roll;
    
    if((state->buttons & CWIID_BTN_A) == CWIID_BTN_A )
    {
        a_x = (((double)state->acc[CWIID_X] - g_acc_cal.zero[CWIID_X]) /
              (g_acc_cal.one[CWIID_X] - g_acc_cal.zero[CWIID_X]));

        a_y = (((double)state->acc[CWIID_Y] - g_acc_cal.zero[CWIID_Y]) /
              (g_acc_cal.one[CWIID_Y] - g_acc_cal.zero[CWIID_Y]));

        a_z = (((double)state->acc[CWIID_Z] - g_acc_cal.zero[CWIID_Z]) /
              (g_acc_cal.one[CWIID_Z] - g_acc_cal.zero[CWIID_Z]));
              
        roll = atan(a_x/a_z);
        if (a_z <= 0.0) 
        {
            roll += M_PI * ((a_x > 0.0) ? 1 : -1);
        }
        	
        pitch = atan(a_y/a_z*cos(roll));
        
        forward  = pitch;
        turnrate = -1*roll;

        printf( "\tAccelerometer Mode\n" );
        printf( "Output\tforward: %0.4f\tturnrate: %0.4f\n", 
            forward, turnrate );
    }
    else
    {
        if((state->buttons & CWIID_BTN_UP) == CWIID_BTN_UP)
        {
            forward = 0.25*SPEED;
        }
        else if((state->buttons & CWIID_BTN_DOWN) == CWIID_BTN_DOWN)
        {
            forward = -0.25*SPEED;
        }

        if((state->buttons & CWIID_BTN_RIGHT) == CWIID_BTN_RIGHT)
        {
            turnrate = -0.25*TURNRATE;
        }
        else if((state->buttons & CWIID_BTN_LEFT) == CWIID_BTN_LEFT)
        {
            turnrate = 0.25*TURNRATE;
        }

        if((state->buttons & CWIID_BTN_B) == CWIID_BTN_B)
        {
            boost++;
        }

        if( boost == 1 )
        {
            forward  *= BOOST_1;
            turnrate /= BOOST_1;
        }

        printf( "\tWiimote Mode\n" );
        printf( ( boost == 1 ? "   Boost\n"          :
                  "" ) );
        printf( "Output\tforward: %0.4f\tturnrate: %0.4f\n", 
            forward, turnrate );
    }
} /* Handle_Wiimote() */

void Handle_NightRider()
{
    struct cwiid_state state;
    const unsigned int delay = 50000; //microseconds
    cwiid_get_state( g_wiimote, &state );
    
    unsigned int led_state[] =
        {
            CWIID_LED1_ON,
            CWIID_LED1_ON|CWIID_LED2_ON,
            CWIID_LED2_ON,
            CWIID_LED2_ON|CWIID_LED3_ON,
            CWIID_LED3_ON,
            CWIID_LED3_ON|CWIID_LED4_ON,
            CWIID_LED4_ON
        };
    
    while( (state.buttons & CWIID_BTN_1)==CWIID_BTN_1)
    {
        for( int i = 0; i < 7; i++ )
        {
            cwiid_command( g_wiimote, CWIID_CMD_LED, led_state[i] );
            usleep(delay);
        }
        for( int i = 7-1; i >= 0; i-- )
        {
            cwiid_command( g_wiimote, CWIID_CMD_LED, led_state[i] );
            usleep(delay);
        }
        cwiid_get_state( g_wiimote, &state );
    }
}

void Set_LEDs()
{
    unsigned int chosen_leds = 0;
    if( wiimote_connected )
    {
        chosen_leds |= CWIID_LED1_ON;
    }
    if( wiimote_suspended )
    {
        chosen_leds |= CWIID_LED4_ON;
    }
    cwiid_command( g_wiimote, CWIID_CMD_LED, chosen_leds );
}
