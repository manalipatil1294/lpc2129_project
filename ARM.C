#include <LPC214x.h>
#include <stdio.h>
#include <string.h>

// RS -> P0.8
// RW -> GND
// EN -> P0.10
// D4-D7 -> P1.20 - P1.23

#define RS (1<<8)
#define EN (1<<10)

#define LED1 (1<<16)
#define LED2 (1<<17)
#define LED3 (1<<18)
#define LED4 (1<<19)


#define SWITCH (1<<14)

#define THRESHOLD 300

void delay_ms(unsigned int ms);

void LCD_Init(void);
void LCD_Cmd(unsigned char cmd);
void LCD_Char(unsigned char data);
void LCD_String(char *str);
void LCD_Clear(void);

void RTC_Init(void);
void RTC_GetTime(void);

void ADC_Init(void);
unsigned int ADC_Read(void);

void Keypad_Init(void);
char Keypad_Scan(void);


void EINT0_Init(void);
void EINT0_ISR(void)__irq;

void LED_ON(void);
void LED_OFF(void);

void EINT0_Init(void);

void RTC_Menu(void);
void RTC_Edit(void);

//void UART0_Init(void);
unsigned int hour, min, sec;
unsigned int date, month, year, day;

char lcd_buffer[32];

volatile int interrupt_flag = 0;


void delay_ms(unsigned int ms)
{
    unsigned int i,j;

    for(i=0;i<ms;i++)
    {
        for(j=0;j<6000;j++);
    }
}

void LCD_Enable(void)
{
    IO0SET = EN;
    delay_ms(2);

    IO0CLR = EN;
    delay_ms(2);
}

void LCD_Send4Bit(unsigned char data)
{
    IO0CLR = 0x000F0000;

    IO0SET = ((data & 0x0F) << 16);

    LCD_Enable();
}

void LCD_Cmd(unsigned char cmd)
{
    IO0CLR = RS;
	  LCD_Send4Bit(cmd>>4);
	  LCD_Send4Bit(cmd);
	  delay_ms(2);
	  
}

void LCD_Char(unsigned char data)
{
    IO0SET = RS;
    LCD_Send4Bit(data >> 4);
	  LCD_Send4Bit(data);
	  delay_ms(2);
	  
}

void LCD_String(char *str)
{
    while(*str)
    {
        LCD_Char(*str++);
    }
}

void LCD_Clear(void)
{
    LCD_Cmd(0x01);
    delay_ms(5);
}

void LCD_Init(void)
{
    IO0DIR |= RS | EN | 0x000F0000;

    delay_ms(20);

    LCD_Send4Bit(0x03);
    LCD_Send4Bit(0x03);
    LCD_Send4Bit(0x03);
	  LCD_Send4Bit(0x02);
	  
    LCD_Cmd(0x28);
    LCD_Cmd(0x0C);
    LCD_Cmd(0x06);
    LCD_Cmd(0x01);
  	
}

void RTC_Init(void)
{
    CCR = 0x02;     // Reset RTC
    CCR = 0x01;     // Enable RTC

    SEC   = 0;
    MIN   = 0;
    HOUR  = 12;
   
  	DOM   = 1;
    MONTH = 1;
    YEAR  = 2026;
}

void RTC_GetTime(void)
{
    sec   = SEC;
    min   = MIN;
    hour  = HOUR;

    date  = DOM;
    month = MONTH;
    year  = YEAR;
    day   = DOW;
}

void ADC_Init(void)
{
    PINSEL1 |= (1<<24);      // P0.28 as AD0.1

    AD0CR = (1<<1) |         // Select channel 1
            (4<<8) |         // CLKDIV
            (1<<21);         // ADC Enable
}

unsigned int ADC_Read(void)
{
    unsigned int adc_data;

    AD0CR |= (1<<24);        // Start conversion

    while(!(AD0GDR & (1UL<<31)));

    adc_data = (AD0GDR >> 6) & 0x3FF;

    return adc_data;
}

void LED_ON(void)
{
    IO0SET = LED1 | LED2 | LED3 | LED4;
}
							
void LED_OFF(void)
{
    IO0CLR = LED1 | LED2 | LED3 | LED4;
}



/*
ROWS:
R1 -> P1.24
R2 -> P1.25
R3 -> P1.26
R4 -> P1.27

COLUMNS:
C1 -> P1.28
C2 -> P1.29
C3 -> P1.30
C4 -> P1.31
*/

void Keypad_Init(void)
{
    /* ROWS OUTPUT */
    IO1DIR |= (1<<24)|(1<<25)|(1<<26)|(1<<27);

    /* COLUMNS INPUT */
    IO1DIR &= ~((1<<28)|(1<<29)|(1<<30)|(1<<31));

    /* ALL ROWS HIGH */
    IO1SET = (1<<24)|(1<<25)|(1<<26)|(1<<27);
}

char Keypad_Scan(void)
{
    unsigned int row;

    char keypad[4][4] =
    {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };

    while(1)
    {
        for(row=0; row<4; row++)
        {
            IO1SET = (1<<24)|(1<<25)|(1<<26)|(1<<27);

            IO1CLR = (1<<(24+row));

            if(!(IO1PIN & (1<<28)))
            {
                delay_ms(20);

                while(!(IO1PIN & (1<<28)));

                return keypad[row][0];
            }

            if(!(IO1PIN & (1<<29)))
            {
                delay_ms(20);

                while(!(IO1PIN & (1<<29)));

                return keypad[row][1];
            }

            if(!(IO1PIN & (1<<30)))
            {
                delay_ms(20);

                while(!(IO1PIN & (1<<30)));

                return keypad[row][2];
            }

            if(!(IO1PIN & (1<<31)))
            {
                delay_ms(20);

                while(!(IO1PIN & (1<<31)));

                return keypad[row][3];
            }
        }
    }
	}
void RTC_Edit(void)
{
    char h1,h2,m1,m2,s1,s2;

    LCD_Clear();
    LCD_String("SET HOUR:");

    LCD_Cmd(0xC0);

    h1 = Keypad_Scan();
    LCD_Char(h1);

    h2 = Keypad_Scan();
    LCD_Char(h2);

    hour = ((h1-'0')*10) + (h2-'0');

    delay_ms(1000);


    LCD_Clear();
    LCD_String("SET MIN:");

    LCD_Cmd(0xC0);

    m1 = Keypad_Scan();
    LCD_Char(m1);

    m2 = Keypad_Scan();
    LCD_Char(m2);

    min = ((m1-'0')*10) + (m2-'0');

    delay_ms(1000);

    LCD_Clear();
    LCD_String("SET SEC:");

    LCD_Cmd(0xC0);

    s1 = Keypad_Scan();
    LCD_Char(s1);

    s2 = Keypad_Scan();
    LCD_Char(s2);

    sec = ((s1-'0')*10) + (s2-'0');

    HOUR = hour;
    MIN  = min;
    SEC  = sec;

    LCD_Clear();
    LCD_String("RTC UPDATED");

    delay_ms(2000);
}


void EINT0_ISR(void)__irq
{
    interrupt_flag = 1;

    EXTINT = 0x01;

    VICVectAddr = 0x00;
}

void EINT0_Init(void)
{
    PINSEL0 |= (1<<29);

    EXTMODE = 0x01;
    EXTPOLAR = 0x00;

    VICIntEnable = (1<<14);

    VICVectCntl0 = 0x20 | 14;
    VICVectAddr0 = (unsigned long)EINT0_ISR;
}

void RTC_Menu(void)
{
    LCD_Clear();

    LCD_String("1.Edit RTC");

    LCD_Cmd(0xC0);

    LCD_String("2.Exit");

    delay_ms(3000);

    RTC_Edit();

    interrupt_flag = 0;
}


int main(void)
{
    unsigned int ldr_value;

    IO0DIR |= LED1 | LED2 | LED3 | LED4 ;

    LCD_Init();

    RTC_Init();

    ADC_Init();

    EINT0_Init();

    LCD_Clear();

    while(1)
    {
        RTC_GetTime();

        LCD_Cmd(0x80);

        sprintf(lcd_buffer,
                "%02d:%02d:%02d",
                hour,min,sec);

        LCD_String(lcd_buffer);

        LCD_Cmd(0xC0);

        sprintf(lcd_buffer,
                "%02d/%02d/%04d",
                date,month,year);

        LCD_String(lcd_buffer);

       
        
        if(hour >= 18 || hour <= 6)
        {
            ldr_value = ADC_Read();

            if(ldr_value < THRESHOLD)
            {
                LED_ON();

                LCD_Clear();
                LCD_String("STREET LIGHT ON");
            }
            else
            {
                LED_OFF();
				
                LCD_Clear();
                LCD_String("LIGHT OFF");
            }
        }
        else
        {
            LED_OFF();
        }

        if(interrupt_flag == 1)
        {
            RTC_Menu();
        }

        delay_ms(500);
    }
	}