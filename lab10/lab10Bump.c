/*
 * Jacob Bachle and Rafael Cano
 * Spring 2026
 * Section 001
 * March 10th, 2026
 * Using the sensors, the robot follows a line, and turns corners. The LEDs change states accordingly.
 *
 * Certification code: 1775003944
 *  Time completed in: 59:50
 */

/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

//100 ms, read delay value
#define DELAY 300000
//Timer value definitions
#define PERIOD 800
#define DUTY 35 //Range 0 to
uint32_t DutyCycle = DUTY*PERIOD/125;
#define CLOCKDIVIDER 48
#define LEFTCHANNEL TIMER_A_CAPTURECOMPARE_REGISTER_4
#define RIGHTCHANNEL TIMER_A_CAPTURECOMPARE_REGISTER_3

#define SLIGHTTURNVALUE 1.55

//Global Variables
typedef enum {motorOFF,motorForward,motorReverse,motorTest} motorStates;
motorStates leftMotorState, rightMotorState;

typedef enum {buttonON,buttonOFF} buttonStates;
buttonStates b0ButtonState, bnButtonState;

volatile uint8_t lineSensorData = 0x00;
enum {LOST, TRACKING} currentState;
enum {TL, HL, SL, S, SR, HR, TR} turnRate;


//Function Prototypes
void config432IO(void);
void configRobotIO(void);
void robotLEDState(void);
void bumperSwitchesHandler(void);
void motorMotion(void);
void configPWMTimer(uint16_t clockPeriod, uint16_t clockDivider, uint16_t duty, uint16_t channel);
void readLineSensors(void);
void processLineData(void);
void LED2State(void);
void followLine(void);

//Timer variable
Timer_A_PWMConfig timerPWMConfig;

int main(void)
{
    // Halting the Watchdog
    MAP_WDT_A_holdTimer();
    //Configure LEDs
    config432IO();
    //Configure Robot Bumpers, Robot LEDs, and RSLK Motor Connections
    configRobotIO();


    //Initialize Left Wheel in Coast
    //nSLPL
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
    //ELB
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5,GPIO_PIN2);

    //Initialize Right Wheel in Coast
    //nSLPR
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
    //ERB
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5,GPIO_PIN0);

    //Configure PWM Timer for Left and Right wheels
    MAP_Timer_A_stopTimer(TIMER_A0_BASE);
    configPWMTimer(PERIOD,CLOCKDIVIDER,DUTY,LEFTCHANNEL);
    configPWMTimer(PERIOD,CLOCKDIVIDER,DUTY,RIGHTCHANNEL);
    //MAP_Timer_A_stopTimer(TIMER_A0_BASE);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);


    //LED Flash
        //LED1
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN0);
        //RGB LED2
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN0|GPIO_PIN1|GPIO_PIN2);
        //RSLK LEDs
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8,GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
        //2 second delay
        __delay_cycles(DELAY*20);
        //Turn LEDs off
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN0);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN0|GPIO_PIN1|GPIO_PIN2);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8,GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    //Initialize button OFF
    b0ButtonState = buttonOFF;

    //Enable global interrupts
    MAP_Interrupt_enableMaster();

    while(1)
    {
        readLineSensors();
        LED2State();
        processLineData();
        robotLEDState();
        __delay_cycles(DELAY/10);
    }
}

/*
 * config432IO
 * Configures the Launchpad LEDs
 * Input: none
 * Return: none
 * Author: Nathan Gibbs and Daniel Hickson
 */

//LED config
void config432IO(void)
{

//Set LED1 and LED2 (R,G,B) as outputs
MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

//Initialize LEDs LOW
MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

}

/*
 * configRobotIO
 * Configures the bumper switches, LEDs, and motors on the robot
 * Input: none
 * Return: none
 * Author: Nathan Gibbs and Daniel Hickson
 */

//Robot Config (LEDs, Bumpers, Motors)
void configRobotIO(void)
{
    //Set bumpers as inputs with pull-up resistors
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    //Enable interrupts
    MAP_GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    //Set High to Low transition
    MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7, GPIO_HIGH_TO_LOW_TRANSITION);
    //Clear interrupt
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    //callback bumper handler function when there is an interrupt request
    MAP_GPIO_registerInterrupt(GPIO_PORT_P4, bumperSwitchesHandler);

    //Robot LED Outputs (Front Left, Front Right, Back Left, Back Right)
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P8,GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);

    //Robot CNTL Even (5.3) and Odd (9.2) outputs
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5,GPIO_PIN3);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P9,GPIO_PIN2);

    //Set Motor Connections as Outputs and Low
    //Configure PWML (P2.7) using PWM timer
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,GPIO_PIN7,GPIO_PRIMARY_MODULE_FUNCTION);
    //Configure PWMR (P2.6) using PWM timer
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,GPIO_PIN6,GPIO_PRIMARY_MODULE_FUNCTION);

    //Sleep Outputs (L - 3.7, R - 3.6)
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P3,GPIO_PIN6 | GPIO_PIN7);

    //Direction Outputs (L - 5.4, R - 5.5)
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5,GPIO_PIN4 | GPIO_PIN5);

    //Enable Outputs (L - 5.2, R - 5.0)
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5,GPIO_PIN0 | GPIO_PIN2);

    //Initialize all outputs low
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN6 | GPIO_PIN7);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6 | GPIO_PIN7);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5,GPIO_PIN4 | GPIO_PIN5);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5,GPIO_PIN0 | GPIO_PIN2);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8,GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
}

void readLineSensors(void)
{
    // 1. Turn ON IR LEDs
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P9, GPIO_PIN2);

    // 2. Charge Capacitors
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    __delay_cycles(30); // 10us

    // 3. Switch to Input
    MAP_GPIO_setAsInputPin(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    __delay_cycles(4620); // 1.54 ms (Calibrated Value)

    // 4. Read Pins
    lineSensorData = P7->IN;

    // 5. Turn OFF IR LEDs
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P9, GPIO_PIN2);
}

void processLineData(void)
{
    static bool ballerina;

    //Check line sensor data for tracking the line or not
    if(lineSensorData == 0)
    {
        currentState = LOST;
    }
    else
    {
        currentState = TRACKING;
        ballerina = false;
    }
    //Set RED1 LED according to currentState
    if(b0ButtonState == buttonOFF)
    {
        MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1,GPIO_PIN0);
        leftMotorState = motorOFF;
        rightMotorState = motorOFF;
        motorMotion();
        __delay_cycles(DELAY*4); //create 1 Hz total toggle time
    }
    else
    {
        if(currentState == TRACKING)
        {
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN0);
            followLine();
        }
        else //Lost!
        {
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN0);
            //Spin 180 degrees

            if(ballerina == false)
            {
                MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4,DutyCycle);
                MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3,DutyCycle);
                leftMotorState = motorReverse;
                rightMotorState = motorForward;
//                Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
                motorMotion();
                //__delay_cycles(3000000*0.75); //0.75 sec
                __delay_cycles(3000000*0.90); //0.75 sec
            }
            ballerina = true; //do not spin again
            //Move until Tracking
            MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4,DutyCycle);
            MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3,DutyCycle);
            leftMotorState = motorForward;
            rightMotorState = motorForward;
//            Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
            motorMotion();
            __delay_cycles(3000000*0.25); //0.25 sec
        }

    }
}

void LED2State(void)
{
    // count left and right sensors
    int leftCount = 0;
    int rightCount = 0;
    int i = 0;
    for (i = 0; i < 8; i++)
    {
        if ((lineSensorData >> i) & 1)
        {
            if (i < 4) rightCount++;
            else leftCount++;
        }
    }
    if(leftCount == 0 && rightCount == 0) //if none, set LED2 WHITE
    {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN0|GPIO_PIN1|GPIO_PIN2);
    }
    else if(leftCount >= 3) //If too many left, Hard Left, Magenta
    {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN0|GPIO_PIN2);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN1);
        //Hard Left
        turnRate = TL;
    }
    else if(rightCount >= 3) //If too many right, Hard Right, Magenta
    {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN0|GPIO_PIN2);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN1);
        //Hard Right
        turnRate = TR;
    }
    else if(leftCount > rightCount) //if more left than right, set LED2 YELLOW
    {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN0|GPIO_PIN1);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN2);
        //Slight Left
        turnRate = SL;
    }
    else if(rightCount > leftCount) //if more right than left, set LED2 CYAN
    {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN1|GPIO_PIN2);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN0);
        //Slight Right
        turnRate = SR;
    }
    else if(leftCount >= 1 && rightCount == 0) //if at least one left, and no right, set LED2 RED
    {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN0);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN1|GPIO_PIN2);
        //Hard Left
        turnRate = HL;
    }
    else if(rightCount >= 1 && leftCount == 0) //if at least one right, and no left, set LED2 BLUE
    {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN2);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN0|GPIO_PIN1);
        //Hard Right
        turnRate = HR;
    }
    else if(leftCount == rightCount) //if left and right are equal, set LED2 GREEN
    {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN1);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN0|GPIO_PIN2);
        //Straight
        turnRate = S;
    }
}


/*
 * robotLEDState
 * Using the button and motor states, change the LEDs
 * Input: none
 * Return: none
 * Author: Nathan Gibbs and Daniel Hickson
 */

//LED States
void robotLEDState(void)
{
    switch(leftMotorState)
    {
        case motorOFF: //Left Front and Back LEDs OFF
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN6);
            break;
        case motorForward: //Left Front ON, Left Back OFF
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN6);
            break;
        case motorReverse: //Left Front OFF, Left Back ON
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN6);
            break;
        default: //Assume motorTest, LEDs ON
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN6);
    }
    switch(rightMotorState)
    {
        case motorOFF: //Right Front and Back LEDs OFF
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN5|GPIO_PIN7);
            break;
        case motorForward: //Right Front ON, Right Back OFF
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN5);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN7);
            break;
        case motorReverse: //Right Front OFF, Right Back ON
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN5);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN7);
            break;
        default: //Assume motorTest, LEDs ON
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN5);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN7);
    }
}

/*
 * Bumper Switches Handler
 * Once an interrupt has occurred, read the interrupt and react accordingly
 * Input: none
 * Return: none
 * Author: Nathan Gibbs and Daniel Hickson
 */

//Bumper ISR
void bumperSwitchesHandler(void)
{
    //debounce (10 ms)
    __delay_cycles(DELAY/25);

    switch(MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P4))
    {
        case GPIO_PIN0: //BS0
            if(b0ButtonState == buttonON)
            {
                b0ButtonState = buttonOFF;
            }
            else
            {
                b0ButtonState = buttonON;
            }
            break;
        case GPIO_PIN2: //BS1 - BS6
        case GPIO_PIN3:
        case GPIO_PIN4:
        case GPIO_PIN5:
        case GPIO_PIN6:
        case GPIO_PIN7:
            if(b0ButtonState == buttonON)
            {
                b0ButtonState = buttonOFF;
            }
            break;
    }
}

/*
 * followLine
 * After reading the line sensors, use the data to drive hard left, slight left, straight, slight right, or hard right.
 * Input: none
 * Return: none
 * Author: Nathan Gibbs and Daniel Hickson
 */
void followLine(void)
{
    switch(turnRate)
    {
    case SL:
        //Slow Left Wheel, maintain Right Wheel
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4,DutyCycle/SLIGHTTURNVALUE);
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3,DutyCycle);
        leftMotorState = motorForward;
        rightMotorState = motorForward;
        motorMotion();
        break;
    case HL:
        //Reverse Left wheel, Maintain Right Wheel
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4,DutyCycle);
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3,DutyCycle);
        leftMotorState = motorReverse;
        rightMotorState = motorForward;
        motorMotion();
        break;
    case S:
        //Maintain both wheels
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4,DutyCycle);
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3,DutyCycle);
        leftMotorState = motorForward;
        rightMotorState = motorForward;
        motorMotion();
        break;
    case HR:
        //Reverse Left Wheel, Maintain Right Wheel
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4,DutyCycle);
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3,DutyCycle);
        leftMotorState = motorForward;
        rightMotorState = motorReverse;
        motorMotion();
        break;
    case SR:
        //Stop Right Wheel, maintain Left Wheel
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4,DutyCycle);
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3,DutyCycle/SLIGHTTURNVALUE);
        leftMotorState = motorForward;
        rightMotorState = motorForward;
        motorMotion();
        break;
    case TL:
        //Maintain Left wheel, Reverse Right Wheel, Slight Delay
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4,DutyCycle);
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3,DutyCycle);
        leftMotorState = motorReverse;
        rightMotorState = motorForward;
        motorMotion();
        __delay_cycles(DELAY*2); //200 ms
        break;
    case TR:
        //Reverse Left Wheel, Maintain Right Wheel, Slight Delay
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4,DutyCycle);
        MAP_Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3,DutyCycle);
        leftMotorState = motorForward;
        rightMotorState = motorReverse;
        motorMotion();
        __delay_cycles(DELAY*2); //200 ms
        break;
    }
//    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
}

/*
 * motorMotion
 * Using the motor states, move the robot accordingly
 * Input: none
 * Return: none
 * Author: Nathan Gibbs and Daniel Hickson
 */

//Motor Motion Function
void motorMotion(void)
{
    /* Left Motor */
    switch(leftMotorState)
    {
        case motorOFF: //Set motor in Coast (nSLPL (P3.7) = 0)
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
            break;
        case motorForward: //ELB (P5.2) - 1, DIRL (P5.4) - 0, nSLPL (P3.7) - 1
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5,GPIO_PIN2);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5,GPIO_PIN4);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN7);
            break;
        case motorReverse: //ELB (P5.2) - 1, DIRL (P5.4) - 1, nSLPL (P3.7) - 1
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5,GPIO_PIN2);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5,GPIO_PIN4);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN7);
            break;
        default: //assume motorTest, motor off/coast
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
            break;
    }
    /* Right Motor */
    switch(rightMotorState)
    {
        case motorOFF: //Set motor in Coast (nSLPL (P3.6) = 0)
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
            break;
        case motorForward: //ERB (P5.0) - 1, DIRR (P5.5) - 0, nSLPR (P3.6) - 1
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5,GPIO_PIN0);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5,GPIO_PIN5);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN6);
            break;
        case motorReverse: //ERB (P5.0) - 1, DIRR (P5.5) - 1, nSLPR (P3.6) - 1
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5,GPIO_PIN0);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5,GPIO_PIN5);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN6);
            break;
        default: //assume motorTest, motor off/coast
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
            break;
    }
}

/*
 * configPWMTimer
 * Configures the PWM timers for the motors
 * Input: none
 * Return: none
 * Author: Nathan Gibbs and Daniel Hickson
 */

//Configure PWM Timer
void configPWMTimer(uint16_t clockPeriod, uint16_t clockDivider, uint16_t duty, uint16_t channel)
{
    const uint32_t TIMER=TIMER_A0_BASE;
    uint16_t dutyCycle = duty*clockPeriod/100;
    timerPWMConfig.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    timerPWMConfig.clockSourceDivider = clockDivider;
    timerPWMConfig.timerPeriod = clockPeriod;
    timerPWMConfig.compareOutputMode = TIMER_A_OUTPUTMODE_TOGGLE_SET;
    timerPWMConfig.compareRegister = channel;
    timerPWMConfig.dutyCycle = dutyCycle;
    MAP_Timer_A_generatePWM(TIMER, &timerPWMConfig);
//    MAP_Timer_A_stopTimer(TIMER); Insert after motor initialization
}
