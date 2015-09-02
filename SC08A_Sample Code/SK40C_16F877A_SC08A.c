//==========================================================================
//	Author					: CYTRON	
//	Project					: SK40C UART - SC08A
//	Project description		: Sample code for controlling SC08A
//							  This sample source code is valid for 
//							  20MHz crystal.
//
// Getting started with this sample code
//==========================================================================
// After connect SK40C with SC08A and Servo Motor using UART, supply the power to both SK40C and SC08A. 
// Please bear in mind that, Servo Power for RC servo motor is normally 5-6V depending on the RC Servo Manufacturer
// Switch ON the power supplies, it is normal that the servo motor will vibrate for few second upon power up
// Green and Orange LEDS will be light ON
// Press SW1 on SK40C to activate all 8 channel, servo motor will move to middle position 1.5ms pulses and stay at that position .
// You must press the SW1 first to activate the servo channel.
// After that, press SW2 on SK40C. All the servo channel will move back and forth to each end of position infinity.
//
//	include
//==========================================================================
#include <pic.h>

//	configuration
//==========================================================================
__CONFIG ( 0x3F32 );				//configuration for the  microcontroller

//	define
//==========================================================================
#define	SW1			RB0			
#define	SW2			RB1			


#define	LED1		RB6			
#define	LED2		RB7				



//	function prototype				(every function must have a function prototype)
//==========================================================================

void delay(unsigned long position);			

unsigned char uart_rec(void);			//receive uart value
void uart_send(unsigned char position);
void uart_str(const char *s);

void position_speed_cmd(unsigned char channel, unsigned int position, unsigned char speed);
void on_off_cmd(unsigned char channel, unsigned char on_off);
unsigned int request_position_cmd(unsigned char channel);
unsigned char compare_position(unsigned char channel, unsigned int position);
void init_servo_cmd(unsigned char channel, unsigned int position);

void store_eeprom(unsigned char position,unsigned char location);
unsigned char get_eeprom(unsigned char location);
//unsigned int read_position(unsigned char channel, unsigned int position);

//	global variable
//==========================================================================
	unsigned int position;

//	main function					(main fucntion of the program)
//==========================================================================
void main()
{
	unsigned int i,j,k,a=0, temp=0;
	unsigned char sp=0;

	//set I/O input output
	TRISB = 0b00000011;					//configure PORTB I/O direction

	//Configure UART
	SPBRG=129;			//set baud rate as 9600 baud for 20Mhz
	BRGH=1;				//baud rate high speed option
	TXEN=1;				//enable transmission
	TX9 =0;				//8-bit transmission
	RX9 =0;				//8-bit reception	
	CREN=1;				//enable reception
	SPEN=1;				//enable serial port
	SYNC=0;				//Asynchronous mode

	LED1=0;				// OFF LED1
	LED2=1;				// OFF LED2
	
	sp=50;				//speed value for the servo

	while(1)
	{
		if(SW1==0)
		{
			while(SW1==0);						//SW1 press and waiting for release
			on_off_cmd(0,1);					//ON all the channel
			LED2^=1;
				
		}
		else if(SW2==0)							//if sw is pressed
		{

			while(SW2==0);						//SW2 press and waiting for release

			while(1)							//infinity loop
			{
				for(k=1; k<9; k++)				//servo channel 1-8 loop
				{
					position_speed_cmd(k,7000,sp);	//send command Servo posisitoning with speed
				}
				
			
				while(request_position_cmd(1)<6900)	//read the position from channel 1 after sending request position command
				{								//if the position is smaller than 6900, loop here untill the position is bigger then 6900
				}

				for(k=1; k<9; k++)				//after position bigger than 6900
				{								//servo channel 1-8 loop
					position_speed_cmd(k,1000,sp);	//send command Servo posisitoning with speed
				}
				while(request_position_cmd(1)>1100)	//read the position from channel 1 after sending request position command
				{								//if the position is bigger than 1100, loop here untill the position is smaller then 1100
				}

			}
		}	

	}
}


//subroutine for SC08A command
//======================================================================================================
//command Servo positioning with speed
void position_speed_cmd(unsigned char channel, unsigned int position, unsigned char speed)		//send 4 bytes of command to control servo's position and speed
{
	unsigned char first_byte=0, higher_byte=0, lower_byte=0;

		// First byte combined the mode and servo channel
		// ob xxx   xxxxx    (in binary)
		//	  mode channel
		// for servo positioning with speed command, the mode value is 0b111xxxxx
		first_byte=0b11100000|channel;								
	
		higher_byte=(position>>6)&0b01111111;			//position value from 0-8000 are greater than a byte
		lower_byte=position&0b00111111;					//so need two bytes to send

		//send command to SC08A in 4 byte - Mode with channel, higher byte of position, lower byte of position, and speed
		uart_send(first_byte);								
		uart_send(higher_byte);						//second byte is the higher byte of position 0b0xxxxxxx
		uart_send(lower_byte);						//third byte is the lower byte of position 0b00xxxxxx
		uart_send(speed);							//fourth byte is the speed value from 0-100 


}

//command Enable/disable the servo channel
void on_off_cmd(unsigned char channel, unsigned char on_off)
{
	unsigned char first_byte=0;

	// First byte combined the mode and servo channel
	// ob xxx   xxxxx    (in binary)
	//	  mode channel
	// for servo positioning with speed command, the mode value is 0b110xxxxx
	// channel = 0 meaning all channel
	first_byte=0b11000000|channel;
	
	//send command to SC08A in 2 byte - Mode with channel and ON/Off channel
	uart_send(first_byte);		
	uart_send(on_off);						// 2nd byte: 1 = enable/activate; 0= disable/deactivate
}

//command requesting position of servo channel
unsigned int request_position_cmd(unsigned char channel)
{
	unsigned char first_byte=0, check_id, higher_position, low_position;

	// First byte combined the mode and servo channel
	// ob xxx   xxxxx    (in binary)
	//	  mode channel
	// for servo positioning with speed command, the mode value is 0b101xxxxx
	// the channel value cannot be 0 in this command
	first_byte=0b10100000|channel;	

	//send command to SC08A in 1 byte - Mode with channel; after that SC08A will send back the position value
	uart_send(first_byte);		

	//Received position from SC08A in 2 byte
	higher_position=uart_rec();							//first byte is the higher byte of position 0b0xxxxxxx
	low_position=uart_rec();							//second byte is the higher byte of position 0b00xxxxxx

	//combine the higher byte and lower byte into 16 bits position
	position=higher_position<<6;
	position=position|(low_position&0x3F);

	return position;
}

//command servo starting position for next reseting of SC08A
//this command is useful when you need the servo motor position to start at different position (not at the middle position which is the default starting position for the servo)
//this subroutine is not use at the sample program
void init_servo(unsigned char channel, unsigned int position)
{
	unsigned char first_byte=0, higher_byte=0, lower_byte=0;

		// First byte combined the mode and servo channel
		// ob xxx   xxxxx    (in binary)
		//	  mode channel
		// for servo positioning with speed command, the mode value is 0b100xxxxx
		// channel = 0 meaning all channel
		first_byte=0b10000000|channel;	

		//initial position for the servo start on next time
		higher_byte=(position>>6)&0b01111111; 			//position value from 0-8000 are greater than a byte
		lower_byte=position&0b00111111;					//so need two bytes to send

		//send command to SC08A in 3 byte -  Mode with channel, higher byte of position and lower byte of position
		uart_send(first_byte);					
		uart_send(higher_byte);						//second byte is the higher byte of position 0b0xxxxxxx
		uart_send(lower_byte);						//third byte is the lower byte of position 0b00xxxxxx

		//after that, the SC08A will acknowlegde the MCU with a byte, 0x04
			while (uart_rec()!=0x04)				//wait the 0x04 value from SC08A
			{
			}
	
}

//subroutine for UART
//======================================================================================================
unsigned char uart_rec(void)			//receive uart value
{
	unsigned char rec_data;
	while(RCIF==0);						//wait for data
	rec_data = RCREG;				
	return rec_data;					//return the data received
}

void uart_send(unsigned char data)
{	
	while(TXIF==0);				//only send the new data after 
	TXREG=data;					//the previous data finish sent
}

void uart_str(const char *s)
{
	while(*s)uart_send(*s++);
}
