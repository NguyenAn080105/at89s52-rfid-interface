#include <REGX51.H>
#include <intrins.h>
#include <Thu_Vien_LCD_I2C.h>

unsigned char slave_add;

sbit sda=P1^2;
sbit scl=P1^3;

void i2c_start(void)
{
	sda=1;_nop_();_nop_();
	scl=1;_nop_();_nop_();
	sda=0;_nop_();_nop_();
}

void i2c_stop(void)
{
	scl=0;
	sda=0;
	scl=1;
	sda=1;
}

void LCD_Slave(unsigned char slave)
{
    slave_add = (slave << 1);
}

void i2c_ACK(void)
{
	scl=0;
	sda=1;
	scl=1;
	while(sda);
}

void i2c_write(unsigned char dat)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
	scl=0;
	sda=(dat&(0x80)>>i)?1:0;
	scl=1;
	}
}

void lcd_send_cmd(unsigned char cmd)
{
	unsigned char cmd_l,cmd_u;
	
	cmd_l=(cmd<<4)&0xf0;
	cmd_u=(cmd &0xf0);
	
	i2c_start();			 //BL EN RW RS 1 1 0 0
	i2c_write(slave_add);
	i2c_ACK();
	i2c_write(cmd_u|0x0C);
	i2c_ACK();
	delay_ms(1);
	i2c_write(cmd_u|0x08); // 1 0 0 0
	i2c_ACK();
	delay_ms(1);
	i2c_write(cmd_l|0x0C);	 // 1 1 0 0
	i2c_ACK();
	delay_ms(1);
	i2c_write(cmd_l|0x08);
	i2c_ACK();
	delay_ms(1);
	i2c_stop();
}

void LCD_Send_Data(unsigned char dataw)// 1 1 0 1
{
	unsigned char dataw_l,dataw_u;
	dataw_l=(dataw<<4)&0xf0;
	dataw_u=(dataw &0xf0);
	i2c_start();
	i2c_write(slave_add);
	i2c_ACK();
	i2c_write(dataw_u|0x0D);//BL EN RW RS   1 1 0 1
	i2c_ACK();
	delay_ms(1);
	i2c_write(dataw_u|0x09);// 1 0 0 1
	i2c_ACK();
	delay_ms(1);
	i2c_write(dataw_l|0x0D);
	i2c_ACK();
	delay_ms(1);
	i2c_write(dataw_l|0x09);
	i2c_ACK();
	delay_ms(1);
	i2c_stop();
}

void LCD_Send_String(unsigned char *p)
{
	while(*p != '\0')
	LCD_Send_Data(*p++);
}


void delay_ms(unsigned int n)
{
	unsigned int m;
	for(n;n>0;n--)
	{
		for(m=121;m>0;m--);
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
	}
}

void LCD_Init()
{
	lcd_send_cmd(0x02);	// Return home
	lcd_send_cmd(0x28);	// 4 bit mode
	lcd_send_cmd(0x0C);	// Display On , cursor off
	lcd_send_cmd(0x06);	// Increment Cursor (shift cursor to right)
	lcd_send_cmd(0x01); // clear display
}

void LCD_Gotoxy (int col, int row)
{
	int pos_Addr;
	if(row == 1) 
	{
		pos_Addr = 0x80 + row - 2 + col;
	}
	else
	{
		pos_Addr = 0x80 | (0x40 + col - 1);
	}
	lcd_send_cmd(pos_Addr);
}

void LCD_Send_Number(int number)
{
	if(number <= 9)
	{
		LCD_Send_Data(number + 48);
	}
	else if(number > 9 && number < 100)
	{
		LCD_Send_Data(number/10 + 48);
		LCD_Send_Data(number%10 + 48);
	}
	else if(number > 99 && number < 1000)
	{
		LCD_Send_Data(number/100 + 48);
		LCD_Send_Data(number/10%10 + 48);
		LCD_Send_Data(number%10 + 48);
	}
	else if(number > 999 && number < 10000)
	{
		LCD_Send_Data(number/1000 + 48);
		LCD_Send_Data(number/100%10 + 48);
		LCD_Send_Data(number/10%10 + 48);
		LCD_Send_Data(number%10 + 48);
	}
}

void LCD_Clear()
{
	LCD_Gotoxy(1,1);
	LCD_Send_String("                ");
	LCD_Gotoxy(1,2);
	LCD_Send_String("                ");
}


