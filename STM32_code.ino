/*Information
 * The code is written by Curious Scientist
 * https://curiousscientist.tech
 * 
 * Playlist for more ADS1256-related videos
 * https://www.youtube.com/playlist?list=PLaeIi4Gbl1T-RpVNM8uKdiV1G_3t5jCIu
 * 
 * If you use the code, please subscribe to my channel
 * https://www.youtube.com/c/CuriousScientist?sub_confirmation=1
 * 
 * I also accept donations
 * https://www.paypal.com/donate/?hosted_button_id=53CNKJLRHAYGQ
 * 
 * The code belongs to the following video
 * https://youtu.be/rsi9o5PQzwM
 * 
 */
//--------------------------------------------------------------------------------
/*
The green board has a RESET pin and the blue does not.
Therefore, for the blue board, the reset function is disabled.

BUFEN should be enabled for precise measurement, but in that case, the max input voltage
cannot be higher than AVDD-2 V. 
*/
//--------------------------------------------------------------------------------
// Pin configuration - STM32F103 [Arduino]: Pins in square brackets [xx] are for Arduino Uno or Nano.
/*
SPI default pins:
MOSI  - PA7[11] // DIN
MISO  - PA6[12] // DOUT
SCK	  - PA5[13] // SCLK
SS	  -	PA4[10] // CS
--------------------
--------------------
MOSI: Master OUT Slave IN -> DIN
MISO: Master IN Slave OUT -> DOUT
--------------------
--------------------
Other pins - You can assign them to any pins
RST	  -	PA3 
DRDY  - PA2 // this is an interrupt pin
PDWN  - +3.3 V
PDWN - PA1 (Alternatively, if you want to switch it)
*/
//--------------------------------------------------------------------------------
//Clock rate
/*
	f_CLKIN = 7.68 MHz
	tau = 130.2 ns
*/
//--------------------------------------------------------------------------------
//REGISTERS
/*
REG   VAL     USE
0     54      Status Register, Everything Is Default, Except ACAL and BUFEN
1     1       Multiplexer Register, AIN0 POS, AIN1 NEG
2     0       ADCON, Everything is OFF, PGA = 1
3     99      DataRate = 50 SPS
4     225     GPIO, Everything Is Default
*/
//--------------------------------------------------------------------------------
#include <SPI.h> //SPI communication
#include <stm32f1xx.h>
//--------------------------------------------------------------------------------
// The setup() function runs once each time the micro-controller starts
void setup()
{
	Serial.begin(1000000);  //We will need high datarate, so it should be a high baud rate
  delay(1000);
  
	Serial.println("*ADS1256 Initialization...."); //Some message
	initialize_ADS1256(); //run the initialization function 
 
  delay(1000);
	Serial.println("*Initialization finished!"); //Confirmation message
 
	reset_ADS1256(); //Reset the ADS1256
	userDefaultRegisters(); //Set up the default registers
	printInstructions(); //Print the instructions for the commands used in the code
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
  SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX 
  SPI.transfer(B00000001); //AIN0+AIN1
 
}

//--------------------------------------------------------------------------------
//Variables
double VREF = 2.50; //Value of V_ref. In case of internal V_ref, it is 2.5 V
double voltage = 0; //Converted RAW bits. 
int CS_Value; //we use this to store the value of the bool, since we don't want to directly modify the CS_pin

//Pins
const byte CS_pin = PA4;	//goes to CS on ADS1256
const byte DRDY_pin = PA2;  //goes to DRDY on ADS1256
const byte RESET_pin = PA3; //goes to RST on ADS1256 
//const byte PDWN_PIN = PA1; //Goes to the PDWN/SYNC/RESET pin (notice, that some boards have different pins!)
//The above pins are described for STM32. For Arduino, you have to use a different pin definition ("PA" is not used in "PA4)

//Values for registers
uint8_t registerAddress; //address of the register, both for reading and writing - selects the register
uint8_t registerValueR; //this is used to READ a register
uint8_t registerValueW; //this is used to WRTIE a register
int32_t registerData; //this is used to store the data read from the register (for the AD-conversion)
uint8_t directCommand; //this is used to store the direct command for sending a command to the ADS1256
String ConversionResults; //Stores the result of the AD conversion
String PrintMessage; //this is used to concatenate stuff into before printing it out. 

//--------------------------------------------------------------------------------

void loop()
{
	if (Serial.available() > 0) 
	{
		char commandCharacter = Serial.read(); //we use characters (letters) for controlling the switch-case	

		switch (commandCharacter) //based on the command character, we decide what to do
		{
		case 'r': //this case is used to READ the value of a register
      //Relevant info on registers: https://youtu.be/wUEx6pEHi2c
			//Ask the user to pick a register //Register map: Table 23 in the Datasheet
			Serial.println("*Which register to read?"); //I put the "*" in front of every text message, so my processing software ignores them
      registerAddress = Serial.parseInt(); //we parse the entered number as an integer and pass it to the registerAddress variable
			//Wait for the input; Example command: "r 1". This will read the register 1 which is the MUX register (they start at 0!)
			while (!Serial.available());
			//Text before the print - string concatenation
      PrintMessage = "*Value of register " + String(registerAddress) + " is " + String(readRegister(registerAddress));
			Serial.println(PrintMessage); //printing the confirmation message
      PrintMessage = ""; //reset the value of the variable     
			break;

		case 'w': //this case is used to WRITE the value of a register
			//Ask the user to pick a register
			Serial.println("*Which Register to write?");
			//Wait for the input
			while (!Serial.available());
			registerAddress = Serial.parseInt(); //Store the register in serialData
			//Ask for the value we want to write
			Serial.println("*Which Value to write?");
			//wait for the input; 
      //Example command: "w1 1". This will write the value 1 in the MUX register. This will set the AIN0(+) + AIN1(-) as the input.
			while (!Serial.available());
			registerValueW = Serial.parseInt();

			//Write the serialData register with the recent input value (Serial.parseInt())
			writeRegister(registerAddress, registerValueW); //The writeRegister() function expects 2 arguments
      delay(500); //wait      
      PrintMessage = "*The value of the register now is: " + String(readRegister(registerAddress));    			
			Serial.println(PrintMessage); //printing a confirmation message
			PrintMessage = "";
			break;
		
		case 't': //this case is used to print a message to the serial terminal. Just to test the connection.

			Serial.println("*Test message triggered by serial command");
			break;

		case 'O': //this case is used to read a single value from the AD converter

			readSingle(); //The readSingle() function returns the result of a single conversion.

			break;

		case 'R': //this does a RESET on the ADS1256

			reset_ADS1256(); //the reset_ADS1256() function resets all the register values

			break;

		case 's': //SDATAC - Stop Read Data Continously

			SPI.transfer(B00001111); //Sending a direct code for SDATAC // Figure 33 in datasheet
			break;

      case 'p': //Printing the instructions

      printInstructions(); //this function prints the commands used to control the ADS1256
      break;

      

		case 'C': //Single-ended mode cycling
   
			cycleSingleEnded(); //the cycleSingleEnded() function cycles through ALL the 8 single ended channels
      //single ended channels: A0+COM, A1+COM, A2+COM...A7+COM

			break;

		case 'D': //differential mode cycling			

			cycleDifferential(); //the cycleDifferential() function cycles through ALL the 4 differential channels
      //differential channels: A0+A1, A2+A3, A4+A5, A6+A7
			break;

    case 'H': //"high-speed" differential mode cycling      

      cycleDifferential_HS(); //the cycleDifferential_HS() function cycles through ALL the 4 differential channels
      //differential channels: A0+A1, A2+A3, A4+A5, A6+A7
      break;

		case 'd': //direct command - See Table 24 in datasheet

			while (!Serial.available());

			directCommand = Serial.parseInt();

			sendDirectCommand(directCommand); //This function sends a standalone command

			Serial.println("*Direct command performed!");

			break;


		case 'A': //Single channel continous reading - MUX is manual, can be single and differential too

			readSingleContinuous(); //the readSingleContinuous() continuously returns a single-ended channel's conversion

			break;
    
    case 'U'://Set everything back to default

      userDefaultRegisters(); //the userDefaultRegisters() function writes the default values
      //The default values are determined by the user, therefore it has to be done in this code
      //This is more like a shortcut for quickly resetting the values to those that you usually use
      break;
		}
	}
}

//--------------------------------------------------------------------------------
//Functions

//Reading registers
//DIN should receive 2 command bytes
//1st command byte: address of the register (for example MUX: 1 / 01h / 0000 0001)
//2nd command byte: number of bytes to read (0000 0001 - 1 byte)
//delay between the end of the command and the beginning of the shifting out of the data on DOUT is t6 = 50 * tau_CLKIN
// 50 * 130.2 ns = 6510 ns = 6.51 us
//Seems like, according to the SPI analyzer, 5 us delay is sufficient. 
//When writing the register, the time between the last clock for RREG command an the first clock of the answer is t = 6.5 us.

unsigned long readRegister(uint8_t registerAddress) //Function for READING a selected register
{
  //Relevant video: https://youtu.be/KQ0nWjM-MtI
  while (digitalRead(DRDY_pin)) {} //we "stuck" here until the DRDY changes its state
	
	SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
	//SPI_MODE1 = output edge: rising, data capture: falling; clock polarity: 0, clock phase: 1.

	//CS must stay LOW during the entire sequence [Ref: P34, T24]
 
	digitalWrite(CS_pin, LOW); //CS_pin goes LOW
	
	SPI.transfer(0x10 | registerAddress); //0x10 = RREG

	SPI.transfer(0x00);

	delayMicroseconds(5); //see t6 in the datasheet

	registerValueR = SPI.transfer(0xFF);	//0xFF is sent to the ADS1256 which returns us the register value

	digitalWrite(CS_pin, HIGH); //CS_pin goes HIGH
	SPI.endTransaction();

	return registerValueR; //return the registers value
}

void writeRegister(uint8_t registerAddress, uint8_t registerValueW)
{	
  //Relevant video: https://youtu.be/KQ0nWjM-MtI
   while (digitalRead(DRDY_pin)) {} //we "stuck" here until the DRDY changes its state  
  
	SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
	//SPI_MODE1 = output edge: rising, data capture: falling; clock polarity: 0, clock phase: 1.  

	//CS must stay LOW during the entire sequence [Ref: P34, T24]

  digitalWrite(CS_pin, LOW); //CS_pin goes LOW
  
  delayMicroseconds(5); //see t6 in the datasheet
  
	SPI.transfer(0x50 | registerAddress); // 0x50 = WREG

	SPI.transfer(0x00);	

	SPI.transfer(registerValueW); //we write the value to the above selected register
	
	digitalWrite(CS_pin, HIGH); //CS_pin goes HIGH
	SPI.endTransaction();
}

void reset_ADS1256()
{
	SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1)); // initialize SPI with  clock, MSB first, SPI Mode1

	digitalWrite(CS_pin, LOW); //CS_pin goes LOW

	delayMicroseconds(10); //wait

	SPI.transfer(0xFE); //Reset

	delay(2); //Minimum 0.6 ms required for Reset to finish.

	SPI.transfer(0x0F); //Issue SDATAC

	delayMicroseconds(100);

	digitalWrite(CS_pin, HIGH); //CS_pin goes HIGH

	SPI.endTransaction();

 Serial.println("*Reset DONE!"); //confirmation message
}

void initialize_ADS1256()	//starting up the chip by making the necessary steps. This is in the setup() of the Arduino code.
{
	//Setting up the pins first
	//Chip select
	pinMode(CS_pin, OUTPUT); //Chip select is an output
	digitalWrite(CS_pin, LOW); //Chip select LOW

	SPI.begin(); //start SPI (Arduino/STM32 - ADS1256 communication protocol)
  //The STM32-ADS1256 development board uses a different SPI channel (SPI_2)
  //For more info: https://youtu.be/3Rlr0FCffr0

	CS_Value = CS_pin; //We store the value of the CS_pin in a variable

	//DRDY
	pinMode(DRDY_pin, INPUT); //DRDY is an input
	pinMode(RESET_pin, OUTPUT); //RESET pin is an output
	digitalWrite(RESET_pin, LOW); //RESET is set to low 

	delay(500); // Wait

	digitalWrite(RESET_pin, HIGH); //RESET is set to high

	delay(500); // Wait

}

void readSingle()
{
  //Relevant video: https://youtu.be/4-A8aJ5BzIs
	//Reading a single value	
	registerData = 0; // every time we call this function, this should be 0 in the beginning!
	
	//Wait for DRDY to go LOW
	while (digitalRead(DRDY_pin)) {} 

	SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
	digitalWrite(CS_pin, LOW); //REF: P34: "CS must stay low during the entire command sequence"
		
	//Issue RDATA (0000 0001) command
	SPI.transfer(B00000001);

	//Wait t6 time (~6.51 us) REF: P34, FIG:30.
  delayMicroseconds(10); 
  //according to the SPI analyser, the time between DIN and MSB = ~6.833 us
  //SPI.transfer(0x0F) probably also takes some time, therefore less delay is enough.

	//step out the data: MSB | mid-byte | LSB,
	//registerData is ZERO
	registerData |= SPI.transfer(0x0F); //MSB comes in, first 8 bit is updated // '|=' compound bitwise OR operator
	registerData <<= 8;					//MSB gets shifted LEFT by 8 bits
	registerData |= SPI.transfer(0x0F); //MSB | Mid-byte
	registerData <<= 8;					//MSB | Mid-byte gets shifted LEFT by 8 bits
	registerData |= SPI.transfer(0x0F); //(MSB | Mid-byte) | LSB - final result
	//After this, DRDY should go HIGH automatically 	

   Serial.println(registerData); //printing the RAW data 
   convertToVoltage(registerData); // converting and printing the raw data as well (unit is Volts)     
	
	digitalWrite(CS_pin, HIGH); //We finished the command sequence, so we switch it back to HIGH
	SPI.endTransaction();
}

void readSingleContinuous() //Continuously reading 1 single-ended channel (i.e. A0+COM)
{
  //Relevant video: https://youtu.be/4-A8aJ5BzIs
  //Some commands should only be initiated in the beginning of this type of acquisition (RDATAC)
  //Therefore, we run them outside the while() loop.
  
  registerData = 0; // every time we call this function, this should be 0 in the beginning! 
  
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));                                   
	digitalWrite(CS_pin, LOW); //REF: P34: "CS must stay low during the entire command sequence"
  
  //Issue RDATAC (0000 0011) command
  SPI.transfer(B00000011);
  //Wait t6 time (~6.51 us) REF: P34, FIG:30.
  delayMicroseconds(5); 
  
	while (Serial.read() != 's') //while the reading is not interrupted by a command from the serial terminal
	{    
		//Reading a single input continuously using the RDATAC
		while (digitalRead(DRDY_pin)) {} //Wait for DRDY to go LOW        
		delayMicroseconds(5); 
		
    //step out the data: MSB | mid-byte | LSB,
    
		//registerData is ZERO
    //Previous, we used 0x0F, here we use 0 for the SPI.transfer() argument;
		registerData |= SPI.transfer(0); //MSB comes in, first 8 bit is updated // '|=' compound bitwise OR operator
		registerData <<= 8;					//MSB gets shifted LEFT by 8 bits
		registerData |= SPI.transfer(0); //MSB | Mid-byte
		registerData <<= 8;					//MSB | Mid-byte gets shifted LEFT by 8 bits
		registerData |= SPI.transfer(0); //(MSB | Mid-byte) | LSB - final result
		//After this, DRDY should go HIGH automatically 	    
   
    Serial.println(registerData); //RAW data
    //Temporary
    //convertToVoltage(registerData); //Converted data (units is Volts)
    
    registerData = 0; // every time we call this function, this should be 0 in the beginning!      
	}
 
	digitalWrite(CS_pin, HIGH); //We finished the command sequence, so we switch it back to HIGH
	SPI.endTransaction();	
}

void cycleSingleEnded() //Cycling through all (8) single ended channels
{
  //Relevant video: https://youtu.be/GBWJdyjRIdM
  
  int cycle = 1;  
  registerData = 0;
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));

  while (Serial.read() != 's')
  {
    for (cycle = 1; cycle < 9; cycle++)
    {
      //we cycle through all the 8 single-ended channels with the RDATAC
      //INFO:
      //RDATAC = B00000011
      //SYNC = B11111100
      //WAKEUP = B11111111     
      //---------------------------------------------------------------------------------------------
      /*Some comments regarding the cycling:
      When we start the ADS1256, the preconfiguration already sets the MUX to [AIN0+AINCOM].
      When we start the RDATAC (this function), the default MUX ([AIN0+AINCOM]) will be included in the
      cycling which means that the first readout will be the [AIN0+AINCOM]. But, before we read the data
      from the [AIN0+AINCOM], we have to switch to the next register already, then start RDATA. This is
      demonstrated in Figure 19 on Page 21 of the datasheet. 

      Therefore, in order to get the 8 channels nicely read and formatted, we have to start the cycle
      with the 2nd input of the ADS1256 ([AIN1+AINCOM]) and finish with the first ([AIN0+AINCOM]).

         \ CH1 | CH2 CH3 CH4 CH5 CH6 CH7 CH8 \ CH1 | CH2 CH3 ...

      The switch-case is between the  two '|' characters
      The output (one line of values) is between the two '\' characters.
      *///-------------------------------------------------------------------------------------------
      //Steps are on Page21
      //Step 1. - Updating MUX       
      while (digitalRead(DRDY_pin)) {} //waiting for DRDY
      
      switch (cycle) 
      {
        //Channels are written manually, so we save time on switching the SPI.beginTransaction on and off.
        case 1: //Channel 2          
            digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
            SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
            SPI.transfer(0x00); 
            SPI.transfer(B00011000);  //AIN1+AINCOM           
          break;

        case 2: //Channel 3
            digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
            SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
            SPI.transfer(0x00); 
            SPI.transfer(B00101000);  //AIN2+AINCOM            
          break;

        case 3: //Channel 4
            digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
            SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
            SPI.transfer(0x00); 
            SPI.transfer(B00111000);  //AIN3+AINCOM            
          break;

        case 4: //Channel 5
            digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
            SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
            SPI.transfer(0x00); 
            SPI.transfer(B01001000);  //AIN4+AINCOM 
          break;

        case 5: //Channel 6
            digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
            SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
            SPI.transfer(0x00); 
            SPI.transfer(B01011000);  //AIN5+AINCOM            
          break;

        case 6: //Channel 7
            digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
            SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
            SPI.transfer(0x00); 
            SPI.transfer(B01101000);  //AIN6+AINCOM            
          break;

        case 7: //Channel 8
            digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
            SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
            SPI.transfer(0x00); 
            SPI.transfer(B01111000);  //AIN7+AINCOM            
          break;

        case 8: //Channel 1
            digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
            SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
            SPI.transfer(0x00); 
            SPI.transfer(B00001000); //AIN0+AINCOM              
          break;
      }

      //Step 2.     

      //Issue RDATA (0000 0001) command
      SPI.transfer(B11111100); //SYNC

      delayMicroseconds(4); //t11 delay 24*tau = 3.125 us //delay should be larger, so we delay by 4 us
      
      SPI.transfer(B11111111); //WAKEUP

      //Step 3. 
      //Issue RDATA (0000 0001) command
      SPI.transfer(B00000001);

      //Wait t6 time (~6.51 us) REF: P34, FIG:30.
      delayMicroseconds(5);

      //step out the data: MSB | mid-byte | LSB,

      //registerData is ZERO
      registerData |= SPI.transfer(0x0F); //MSB comes in, first 8 bit is updated // '|=' compound bitwise OR operator
      registerData <<= 8;         //MSB gets shifted LEFT by 8 bits
      registerData |= SPI.transfer(0x0F); //MSB | Mid-byte
      registerData <<= 8;         //MSB | Mid-byte gets shifted LEFT by 8 bits
      registerData |= SPI.transfer(0x0F); //(MSB | Mid-byte) | LSB - final result
      //After this, DRDY should go HIGH automatically   

      //Constructing an output  
      ConversionResults = ConversionResults + registerData;
      ConversionResults = ConversionResults + "\t";     
      //---------------------      
      registerData = 0;

      digitalWrite(CS_pin, HIGH); //We finished the command sequence, so we switch it back to HIGH
          
      //Expected output when using a resistor ladder of 1k resistors and the ~+5V output of the Arduino:
      //Formatting  Channel 1 Channel 2 Channel 3 Channel 4 Channel 5 Channel 6 Channel 7 Channel 8
      /*
      12:41:40.280 -> 4.78714609  4.16558074  3.55143761  2.96154289  2.37305951  1.78396224  1.19539093  0.60204453
      12:41:40.450 -> 4.78708410  4.16603088  3.55298733  2.96177434  2.37242603  1.78440055  1.19551980  0.60218434
      12:41:40.620 -> 4.78826045  4.16563510  3.55332374  2.96192693  2.37245225  1.78419756  1.19552350  0.60213699
      */                    
    }
    Serial.println(ConversionResults);   
    ConversionResults="";
  } 
  SPI.endTransaction(); 
}

void cycleDifferential() 
{
  //Relevant viodeo: https://youtu.be/GBWJdyjRIdM
  
  int cycle = 1;  

  //outside while() loop, we have to switch to the first differential channel ([AIN0+AIN1]) 
  //writeRegister(1, 1); //B00000001 = 1;  [AIN0+AIN1]
  
  registerData = 0;
  
  //SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1)); //We start this SPI.beginTransaction once.
  
  // digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
  // SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
  // SPI.transfer(0x00); 
  // SPI.transfer(B00000001);  //AIN0+AIN1
  // digitalWrite(CS_pin, HIGH);
  // SPI.endTransaction();
  

  while (Serial.read() != 's')
  {  
    //long int t1 = micros(); 
    for (cycle = 4; cycle < 5; cycle++)
    {
      //we cycle through all the 4 differential channels with the RDATA

      //RDATA = B00000001
      //SYNC = B11111100
      //WAKEUP = B11111111

      //Steps are on Page21
      //Step 1. - Updating MUX 
      
      
      digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
      //SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
      //SPI.transfer(0x00); 
      //SPI.transfer(B00000001); //AIN0+AIN1
      
      while (digitalRead(DRDY_pin)) {} 

      // switch (cycle)
      // {
      // case 1: //Channel 2        
      //   digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
      //   SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
      //   SPI.transfer(0x00); 
      //   SPI.transfer(B00100011);  //AIN2+AIN3
      //   break;

      // case 2: //Channel 3        
      //   digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
      //   SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
      //   SPI.transfer(0x00); 
      //   SPI.transfer(B01000101); //AIN4+AIN5
      //   break;

      // case 3: //Channel 4
      //   digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
      //   SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
      //   SPI.transfer(0x00); 
      //   SPI.transfer(B01100111); //AIN6+AIN7          
      //   break;      

      // case 4: //Channel 1       
      //   digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
      //   SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
      //   SPI.transfer(0x00); 
      //   SPI.transfer(B00000001); //AIN0+AIN1
      //   break;
      // }            
      
      SPI.transfer(B11111100); //SYNC mitooni comment koni
      //SPI.transfer(B11111100);     

      //delayMicroseconds(4); //t11 delay 24*tau = 3.125 us //delay should be larger, so we delay by 4 us
      //SPI.transfer(B11111101); //Standy
      SPI.transfer(B11111111); //WAKEUP mitooni comment koni
      //delayMicroseconds(100);
      //Step 3. 
      //Issue RDATA (0000 0001) command      
      SPI.transfer(B00000001);

      //Wait t6 time (~6.51 us) REF: P34, FIG:30.
      delayMicroseconds(7);

      //step out the data: MSB | mid-byte | LSB,

      //registerData is ZERO
      registerData |= SPI.transfer(0x0F); //MSB comes in, first 8 bit is updated // '|=' compound bitwise OR operator
      registerData <<= 8;         //MSB gets shifted LEFT by 8 bits
      registerData |= SPI.transfer(0x0F); //MSB | Mid-byte
      registerData <<= 8;         //MSB | Mid-byte gets shifted LEFT by 8 bits
      registerData |= SPI.transfer(0x0F); //(MSB | Mid-byte) | LSB - final result
      //After this, DRDY should go HIGH automatically   
      
      //Constructing an output  
      //ConversionResults = ConversionResults + registerData;
      //ConversionResults = ConversionResults + "\t";  
      convertToVoltage(registerData); 
      //if (long minus = registerData >> 23 == 1) //if the 24th bit (sign) is 1, the number is negative
      //{
       // registerData = registerData - 16777216;  //conversion for the negative sign
      //"mirroring" around zero
      //}

      //voltage = ((2*VREF) / 8388608)*registerData;  
      
      //---------------------      
      registerData = 0;
      digitalWrite(CS_pin, HIGH); //We finished the command sequence, so we switch it back to HIGH                  
      //SPI.transfer(B11111101);
      // if (long minus = registerData >> 23 == 1) //if the 24th bit (sign) is 1, the number is negative
      // {
      //   registerData = registerData - 16777216;  //conversion for the negative sign
      // //"mirroring" around zero
      // }

      // voltage = ((2*VREF) / 8388608)*registerData; //2.5 = Vref; 8388608 = 2^{23} - 1
      // //ConversionResults = voltage*1000 + '\n';
      // ConversionResults2 = voltage*1000;
      // ConversionResults = ConversionResults + ConversionResults2 + '\n';
      // if (count==10000){
      //   Serial.println(ConversionResults);
      //   count=0;
      //   ConversionResults=' ';
      // } 
      //Expected output when using a resistor ladder of 1k resistors and the ~+5V output of the Arduino:
      //Formatting  Channel 1 Channel 2 Channel 3 Channel 4 
      /*
      16:14:23.066 -> 4.79074764  4.16625738  3.55839943  2.96235866  
      16:14:23.136 -> 4.79277801  4.16681241  3.55990862  2.96264190  
      16:14:23.238 -> 4.79327344  4.16698741  3.55968427  2.96277694  
      */

    }
    //Serial.println(ConversionResults);
    //ConversionResults = "";
    //long int t2 = micros();
    //Serial.println(t2-t1);
    //t2=0;
    //t1=0;
  }
  //SPI.endTransaction();
}

void cycleDifferential_HS() //"High-speed" version of the cycleDifferential() function
{  
  //Relevant viodeo: https://youtu.be/GBWJdyjRIdM
  
  int cycle = 1;  
  registerData = 0;
  
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1)); //We start this SPI.beginTransaction once.

  //Setting up the input channel
  digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
  SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
  SPI.transfer(0x00); 
  SPI.transfer(B00000001);  //AIN0+AIN1
  digitalWrite(CS_pin, HIGH);
  
  SPI.endTransaction();
  

  while (Serial.read() != 's')
  {   
    for(int package = 0; package < 10; package++) //We will collect 10 "packages"
    {      
      for (cycle = 4; cycle < 5; cycle++)
      {
        //we cycle through all the 4 differential channels with the RDATA
  
        //RDATA = B00000001
        //SYNC = B11111100
        //WAKEUP = B11111111
  
        //Steps are on Page21
        //Step 1. - Updating MUX        
        long int t1 = millis();
        while (digitalRead(DRDY_pin)) {} //waiting for DRDY
        
        switch (cycle)
        {
        case 1: //Channel 2        
          digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00); 
          SPI.transfer(B00100011);  //AIN2+AIN3
          break;
  
        case 2: //Channel 3        
          digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00); 
          SPI.transfer(B01000101); //AIN4+AIN5
          break;
  
        case 3: //Channel 4
          digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00); 
          SPI.transfer(B01100111); //AIN6+AIN7          
          break;      
  
        case 4: //Channel 1       
          digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00); 
          SPI.transfer(B00000001); //AIN0+AIN1
          break;
        }            
        
        //SPI.transfer(B11111100); //SYNC      
  
        //delayMicroseconds(4); //t11 delay 24*tau = 3.125 us //delay should be larger, so we delay by 4 us
  
        //SPI.transfer(B11111111); //WAKEUP
  
        //Step 3. 
        //Issue RDATA (0000 0001) command      
        SPI.transfer(B00000001);
  
        //Wait t6 time (~6.51 us) REF: P34, FIG:30.
        delayMicroseconds(5);
  
        //step out the data: MSB | mid-byte | LSB,
  
        //registerData is ZERO
        registerData |= SPI.transfer(0x0F); //MSB comes in, first 8 bit is updated // '|=' compound bitwise OR operator
        registerData <<= 8;         //MSB gets shifted LEFT by 8 bits
        registerData |= SPI.transfer(0x0F); //MSB | Mid-byte
        registerData <<= 8;         //MSB | Mid-byte gets shifted LEFT by 8 bits
        registerData |= SPI.transfer(0x0F); //(MSB | Mid-byte) | LSB - final result
        //After this, DRDY should go HIGH automatically   
        
        //Constructing an output  
        ConversionResults = ConversionResults + registerData;
        if(cycle < 4)
        {      
        ConversionResults = ConversionResults + "\t";     
        }
        else
        {
        ConversionResults = ConversionResults;
        }
        //---------------------      
        registerData = 0;
        digitalWrite(CS_pin, HIGH); //We finished the command sequence, so we switch it back to HIGH                  
  
        //Expected output when using a resistor ladder of 1k resistors and the ~+5V output of the Arduino:
        //Formatting  Channel 1 Channel 2 Channel 3 Channel 4 
        /*
        16:14:23.066 -> 4.79074764  4.16625738  3.55839943  2.96235866  
        16:14:23.136 -> 4.79277801  4.16681241  3.55990862  2.96264190  
        16:14:23.238 -> 4.79327344  4.16698741  3.55968427  2.96277694        */
  
      }
    ConversionResults = ConversionResults + '\n'; //Add a linebreak after a line of data (4 columns)
    }
    Serial.print(ConversionResults); //print everything after 10 packages
    ConversionResults = "";
  }
  SPI.endTransaction();
}

void sendDirectCommand(uint8_t directCommand)
{
	//Direct commands can be found in the datasheet Page 34, Table 24. 
  //Use binary, hex or dec format. 
	//Here, we want to use everything EXCEPT: RDATA, RDATAC, SDATAC, RREG, WREG
	//We don't want to involve DRDY here. We just write, but don't read anything.

	//Start SPI
	SPI.beginTransaction(SPISettings(1700000, MSBFIRST, SPI_MODE1));

	digitalWrite(CS_pin, LOW); //REF: P34: "CS must stay low during the entire command sequence"

	delayMicroseconds(5); //t6 - maybe not necessary

	SPI.transfer(directCommand); //Send Command

	delayMicroseconds(5); //t6 - maybe not necessary

	digitalWrite(CS_pin, HIGH); //REF: P34: "CS must stay low during the entire command sequence"

	SPI.endTransaction();

}

void userDefaultRegisters()
{
	// This function is "manually" updating the values of the registers then reads them back.
	// This function should be used in the setup() after performing an initialization-reset process 
  // I use the below listed settings for my "startup configuration"
	/*
		REG   VAL     USE
		0     54      Status Register, Everything Is Default, Except ACAL and BUFEN
		1     1       Multiplexer Register, AIN0 POS, AIN1 POS
		2     0       ADCON, Everything is OFF, PGA = 1
		3     99      DataRate = 50 SPS		
    */	
    
	//We update the 4 registers that we are going to use
  
	delay(500);
  
  writeRegister(0x00, B00110000); //STATUS                      
	delay(200);
	writeRegister(0x01, B00000001); //MUX AIN0+AIN1
	delay(200);
	writeRegister(0x02, B00000111); //ADCON
	delay(200);
	writeRegister(0x03, B11110000); //DRATE - DEC[99] - 50 SPS
	delay(500);
  sendDirectCommand(B11110000);	// SELFCAL
	Serial.println("*Register defaults updated!");
}

void printInstructions()
{
	//This function should be in the setup() and it shows the commands

 PrintMessage = "*Use the following letters to send a command to the device:" + String("\n") 
  + "*r - Read a register. Example: 'r1' - reads the register 1" + String("\n") 
  + "*w - Write a register. Example: 'w1 8' - changes the value of the 1st register to 8." + String("\n") 
  + "*O - Single readout. Example: 'O' - Returns a single value from the ADS1256." + String("\n") 
  + "*A - Single, continuous reading with manual MUX setting." + String("\n") 
  + "*C - Cycling the ADS1256 Input multiplexer in single-ended mode (8 channels). " + String("\n") 
  + "*D - Cycling the ADS1256 Input multiplexer in differential mode (4 channels). " + String("\n")  
  + "*H - Cycling the ADS1256 Input multiplexer in differential mode (4 channels) at high-speed. " + String("\n")  
  + "*R - Reset ADS1256. Example: 'R' - Resets the device, everything is set to default." + String("\n")  
  + "*s - SDATAC: Stop Read Data Continuously." + String("\n")  
  + "*U - User Default Registers."  + String("\n")
  + "*t - triggers a test message."  + String("\n")    
  + "*d - Send direct command.";

  Serial.println(PrintMessage);
  PrintMessage = ""; //Reset (empty) variable.
}

void convertToVoltage(int32_t registerData)
{
  if (long minus = registerData >> 23 == 1) //if the 24th bit (sign) is 1, the number is negative
    {
      registerData = registerData - 16777216;  //conversion for the negative sign
      //"mirroring" around zero
    }

    voltage = ((2*VREF) / 8388608)*registerData; //2.5 = Vref; 8388608 = 2^{23} - 1

    //Basically, dividing the positive range with the resolution and multiplying with the bits   
    
    Serial.println(voltage*1000000,0); //print it on serial, 8 decimals    
    //long a=RCC_GetClocksFreq()
    //Serial.println(a)
    voltage = 0; //reset voltage
}

//--------------------------------------------------------------------------------
//end of code
//--------------------------------------------------------------------------------
//Good to know things
/*
TAU = 1/f = 1/7.68 MHz = 130.2 ns

For a 7.68 MHz ADC clock, the SCLK may not exceed 1.92 MHz.

Binary to decimal is read from right to left
8 bit = 1 byte

0000 0001 = 1 (2^0)
0000 0011 = 3 (2^0 + 2^1)
1000 0001 = 129 (2^0 + 2^7)

PAGE 34 of the datasheet, Table 24: Command definitions
RREG and WREG require a second command byte plus data. 
The ORDER bit in the STATUS register sets the order of the bits within the output data.
CS must stay LOW during the entire sequence.

The operator '|' is the bitwise OR operator. 
Example:
0  0  1  1    operand1
0  1  0  1    operand2
----------
0  1  1  1    (operand1 | operand2) - returned result
//---------------------------------------------------

DRDY goes LOW when new data is available. - When all the 24 bits have been read back, it goes back to HIGH.

The operator '|=' is the compound bitwise operator.

OUTPUT CODES //REF Page23 Table16

1: Vin (AIN_P - AIN_N) > (2Vref)/PGA
	7FFFFFh = 8388607 = 0111 1111 1111 1111 1111 1111

2: Vin (AIN_P - AIN_N) < (-2Vref)/PGA *((2^23)/(2^23 -1))
	800000h = 8388608 = 1000 0000 0000 0000 0000 0000
*/