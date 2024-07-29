#include <msp430.h> 


/**
 * main.c - John Dolph (Final Project) 8/5/2020
 */

//char packet[] = {start address, sec, min, hour, day, weekday, month, year}
char packetOut[] = {0x03, 0b000000, 0b1000100, 0b010010, 0b000101, 0b011, 0b01000, 0b00100000}; // Initial time
char packetIn[] = {'0','0','0','0','0','0','0'}; // Updated time
char weekday[]="          ";

unsigned int timeDateData_Cnt[] = {2,1,0}; // Array for accessing specific time OR Date elements

unsigned int uartMode = 0; // Default Receive Mode
unsigned int Data_Cnt = 0; // Counter for interrupt data
unsigned int setTime = 1; // Allows time to be set initially
unsigned int i;
unsigned int updateEnable = 0; //Disable Time update in main while loop(default)


int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
//-------------- Setup Serial Peripherals (eUSCI_A1 and eUSCI_B0) ----------------------------------

	//-- Put Serial Peripherals (eUSCI_A1 and eUSCI_B0) into SW Reset
	UCA1CTLW0 |= UCSWRST;
    UCB0CTLW0 |= UCSWRST;

    //-- Configure eUSCI_A1 UART (115200 Baud)
    UCA1CTLW0 |= UCSSEL__SMCLK; //BRCLK = SMCLK
    UCA1BRW |= 8; //Prescaler
    UCA1MCTLW |= 0xD600; // Modulation

    //-- Configure eUSCI_B0 I2C
   UCB0CTLW0 |= UCSSEL_3; // Choose BRCLK=SMCLK=1MHz
   UCB0BRW = 10; // Divide BRCLK by 10 for SCL=100KHz

   UCB0CTLW0 |= UCMODE_3; //Put into I2C mode
   UCB0CTLW0 |= UCMST; // Put into master mode
   UCB0CTLW0 |= UCTR; // Put into Tx mode
   UCB0I2CSA = 0x68; // Slave address = 0x68

   UCB0TBCNT = sizeof(packetOut); // Send packet size (bytes)
   UCB0CTLW1 |= UCASTP_2; // Auto STOP when UCB0TBCNT reached

    //-- Configure Ports
    P1DIR |= BIT0;              // LED1 Output
    P1OUT &= ~BIT0;             // Clear LED1

    P6DIR |= BIT6;              // LED2 Output
    P6OUT &= ~BIT6;             // Clear LED1

    P4SEL1 &= ~BIT3; // Configure P4.3 to use UCA1TXD
    P4SEL0 |= BIT3;

    P4SEL1 &= ~BIT2; // Configure P4.2 to use UCA1RXD
    P4SEL0 |= BIT2;

    P1DIR |= BIT4;              // P1.4 Output (Servo PWM)
    P1OUT |= BIT4;             // Set P1.4 to HIGH

    P1SEL1 &= ~BIT3; //  P1.3 = SCL
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2; //  P1.2 = SDA
    P1SEL0 |= BIT2;

    P4DIR &= ~BIT1; // Setup S1
    P4REN |= BIT1;
    P4OUT |= BIT1;
    P4IES |= BIT1;

    P2DIR &= ~BIT3; // Setup S2
    P2REN |= BIT3;
    P2OUT |= BIT3;
    P2IES |= BIT3;

    PM5CTL0 &= ~LOCKLPM5; // Enable I/O

    //-- Take Serial Peripherals out of software Reset
    UCA1CTLW0 &= ~UCSWRST;
    UCB0CTLW0 &= ~UCSWRST;

    //-- Enable eUSCI IRQs
    UCA1IE |= UCRXIE; // Enable UCRXIE IRQ
    UCA1IFG &= ~UCRXIFG; // Clear UCRXIFG flag

    UCB0IE |= UCTXIE0; // Enable I2C Tx0 IRQ (Indicates when Tx buffer is empty)
    UCB0IE |= UCRXIE0; // Enable I2C Rx0 IRQ


//-------------- Setup PWM Servo Control Timer ----------------------------------

    //-- Setup Timer B0
    TB0CTL |= TBCLR;            // Clear Timer and Dividers
    TB0CTL |= TBSSEL__SMCLK;     // Source = SMCLK
    TB0CTL |= MC__UP;           // Mode = UP
    TB0CTL &= ~TBIE; // Disable timer overflow tracking

    //-- Setup Timer CCR0, CCR1 Compare IRQ
    TB0CCTL0 |= CCIE;   // Enable TB0 CCR0 Overflow IRQ
    TB0CCTL0 &= ~CCIFG; // Clear CCR0 Flag
    TB0CCTL1 |= CCIE;   // Enable TB0 CCR1 Overflow IRQ
    TB0CCTL1 &= ~CCIFG; // Clear CCR1 Flag

    TB0CCR0 = 20000;            // PWM Period (20 ms)
    TB0CCR1 = 1500;             //Initial Duty Cycle (7.5%) - (1.5ms)

    __enable_interrupt(); // Global Maskable Interrupt enable

    //-- Set the initial time
    UCB0CTLW0 |= UCTXSTT; // Generate a START condition (Set Time)
    while((UCB0IFG & UCSTPIFG) == 0); // Wait for STOP
    UCB0IFG &= ~UCSTPIFG; //Clear STOP Flag
    setTime=0;

    P4IFG &= ~BIT1; // Enable S1 IRQ
    P4IE |= BIT1;

    P2IFG &= ~BIT3; // Enable S2 IRQ
    P2IE |= BIT3;

    while(1)
    {

        if(updateEnable == 1) // Program waiting for S1 or S2 press prior to entering
        {

           //Update the time from RTC prior to sending over UART

           UCB0TBCNT = 1; // 1 byte
           //-- Transmit Register Address with Write Message
           UCB0CTLW0 |= UCTR; // Put into Tx Mode
           UCB0CTLW0 |= UCTXSTT; // Generate a START Condition

            while((UCB0IFG & UCSTPIFG) == 0); // Wait for STOP
           UCB0IFG &= ~UCSTPIFG; //Clear STOP Flag

           UCB0TBCNT = sizeof(packetOut)-1; // Packet information size
           //-- Receive Data from Rx
           UCB0CTLW0 &= ~UCTR; // Put into Rx Mode
           UCB0CTLW0 |= UCTXSTT; // Generate START Condition

           while((UCB0IFG & UCSTPIFG)==0); //  Wait for STOP
           UCB0IFG &= ~UCSTPIFG; // Clear STOP Flag

           switch(packetIn[4]) // Determine the weekday and assign associated string
            {
            case 0x00:
                strcpy(weekday, "Sunday  ");
                break;
            case 0x01:
                strcpy(weekday, "Monday  ");
                break;
            case 0x02:
                strcpy(weekday, "Tuesday ");
                break;
            case 0x03:
                strcpy(weekday, "Wednesday");
                break;
            case 0x04:
                strcpy(weekday, "Thursday");
                break;
            case 0x05:
                strcpy(weekday, "Friday  ");
                break;
            case 0x06:
                strcpy(weekday, "Saturday");
            }


           UCA1IE &= ~UCRXIE; // Disable UCRXIE IRQ
           UCA1IE |= UCTXCPTIE; // Enable UCTXCPTIE IRQ
           UCA1IFG &= ~UCTXCPTIFG; // Clear UCTXCPTIE flag

           Data_Cnt = 0;
           i=0;
           UCA1TXBUF = '\n';

           __bis_SR_register(LPM0_bits); // Put CPU to sleep

       }
    }

    return 0;

}

//---------------------- Interrupt Service Routines --------------------------------------
#pragma vector = EUSCI_A1_VECTOR
__interrupt void ISR_EUSCI_A1(void) // UART Tx and Rx
{
    switch(uartMode)
    {
    case 0: // Rx Mode (Servo Duty Cycle Control)
        if (UCA1RXBUF == '1') // Reading UCA1RXBUF clears UCR1IFG flag
        {
            //move servo one step to the left
            TB0CCR1 -= 100;

            if(TB0CCR1 < 1000) // Limit lower duty cycle to 5% (1ms)
            {
                TB0CCR1 = 1000;
            }
            else
            {
                P1OUT ^= BIT0; // Toggle LED1
            }
        }
        else if (UCA1RXBUF == '2') // Reading UCA1RXBUF clears UCR1IFG flag
        {
            //move servo one step to the right
            TB0CCR1 += 100;

            if(TB0CCR1 > 2000) // Limit upper duty cycle to 10% (2ms)
            {
                TB0CCR1 = 2000;
            }
            else
            {
                P6OUT ^= BIT6; // Toggle LED1
            }
        }

    break;

    case 1: // Print Time OR Date to Serial terminal

        UCA1IFG &= ~UCTXCPTIFG; // Clear the UCTXCPTIFG Flag

        switch(i) // Determine proper time / date indices in packetIn[]
        {
        case 0:
        case 1:
            Data_Cnt = timeDateData_Cnt[0];
        break;
        case 3:
        case 4:
            Data_Cnt = timeDateData_Cnt[1];
        break;
        case 6:
        case 7:
            Data_Cnt = timeDateData_Cnt[2];

        }

        switch(i) // Send data to UCA1TXBUF
        {
        case 0:
        case 3:
        case 6:
            UCA1TXBUF = ((packetIn[Data_Cnt] & 0xF0) >> 4) + '0';
        break;
        case 1:
        case 4:
        case 7:
            UCA1TXBUF = ((packetIn[Data_Cnt] & 0x0F)) + '0';
        break;
        case 2:
        case 5:
            if(timeDateData_Cnt[0] == 5) // Date format mode
            {
                UCA1TXBUF = '/';
            }
            else
            {
                UCA1TXBUF = ':';
            }
        break;
        default: //Finished Sending Data
            i=0;
            Data_Cnt=0;
            UCA1IE &= ~UCTXCPTIE; //Disable UCTXCPTIE IRQ
            updateEnable=0;

            uartMode = 0;

            UCA1IE |= UCRXIE; // Enable UCRXIE IRQ
            UCA1IFG &= ~UCRXIFG; // Clear UCRXIFG flag

            __bic_SR_register_on_exit(LPM0_bits); // CPU Wake

        }

        i++;

        break;

    case 2: // Send Day of Week Name to Serial terminal

        UCA1IFG &= ~UCTXCPTIFG; // Clear the UCTXCPTIFG Flag

        if (Data_Cnt == sizeof(weekday)-1) //Check whether all characters have been sent.
        {
            //Send Month Data
            uartMode = 1; // I2C Data Print Mode
            UCA1TXBUF = ' ';
        }
        else
        {
            UCA1TXBUF = weekday[Data_Cnt]; //insert next chracter into Tx buffer
            Data_Cnt++;

        }

    break;
    }

}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void)
{
    P1OUT |= BIT4;      // Set P1.4
    TB0CCTL0 &= ~CCIFG; // Clear CCR0 Flag
}

#pragma vector = TIMER0_B1_VECTOR
__interrupt void ISR_TB0_CCR1(void)
{
    P1OUT &= ~BIT4;     // Clear P1.4
    TB0CCTL1 &= ~CCIFG; // Clear CCR1 Flag
}



#pragma vector = PORT4_VECTOR
__interrupt void ISR_Port4_S1(void)
{

    uartMode = 1; // UART Tx Time
    updateEnable=1;

    // PacketIn indices for time
    timeDateData_Cnt[0] = 2;
    timeDateData_Cnt[1] = 1;
    timeDateData_Cnt[2] = 0;

    P4IFG &= ~BIT1;

    //Once the first byte is shifted out, TXIFG is asserted.
}

#pragma vector = PORT2_VECTOR
__interrupt void ISR_Port3_S2(void)
{

    uartMode = 2; // UART Tx Date
    updateEnable=1;

    // PacketIn indices for date
    timeDateData_Cnt[0] = 5;
    timeDateData_Cnt[1] = 3;
    timeDateData_Cnt[2] = 6;


    P2IFG &= ~BIT3;

    //Once the first byte is shifted out, TXIFG is asserted.
}


#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) // I2C
{
    switch(UCB0IV) // Determines which IRQ flag has triggered
    {
    case 0x16: // RXIFG0 (Receive Time)
        if(Data_Cnt == sizeof(packetIn) - 1)
       {
           packetIn[Data_Cnt] = UCB0RXBUF;
           Data_Cnt = 0;
       }
       else
       {
           packetIn[Data_Cnt] = UCB0RXBUF;
           Data_Cnt++;
       }

    break;

    case 0x18: // TXIFG
        if(setTime==1) // Set Initial Time
        {
            if(Data_Cnt == sizeof(packetOut) - 1)
            {
                UCB0TXBUF = packetOut[Data_Cnt];
                Data_Cnt = 0;
            }
            else
            {
                UCB0TXBUF = packetOut[Data_Cnt];
                Data_Cnt++;
            }
        }
        else // Send register start address
        {
            UCB0TXBUF = packetOut[0]; // Send register address
        }

    }

}
