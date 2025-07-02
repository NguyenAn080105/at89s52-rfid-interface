#include <REGX51.H>
#include "Thu_Vien_LCD_I2C.h"
#include "mfrc522.h"
#include <stdio.h>
#include <intrins.h>
#include <string.h>
#define NUM_VALID_UIDS 4

unsigned char slave = 0x27;

void delayms(unsigned int ms) {
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 123; j++);
}

const char* validUIDs[] = {
    "2CF5FA032",
    "F45679712",
    "E31ABA1B2",
    "B3096F2D2"
};

unsigned char checkUID(const char* uid) {
    unsigned char i;
    for(i = 0; i < NUM_VALID_UIDS; i++) {
        if(strcmp(uid, validUIDs[i]) == 0) {
            return 1; // UID h?p l?
        }
    }
    return 0; // UID không h?p l?
}

void display()
{
    LCD_Gotoxy(3, 1);
    LCD_Send_String("RFID Scanner");
		LCD_Gotoxy(7, 2);
    LCD_Send_String("...");
}

void main() {
    unsigned char status, uid[4], uid_str[9];
    
	  LCD_Slave(slave);
    LCD_Init();
    mfrc522_init();
	
    while(1) {
				display();
        status = mfrc522_request(PICC_REQALL, uid);
        if(status == MI_OK) {
            status = mfrc522_anticoll(uid);
            if(status == MI_OK) {
                sprintf(uid_str, "%02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3]);
								LCD_Clear();
								LCD_Gotoxy(1, 2);
								LCD_Send_String("UID:");
								LCD_Send_String(uid_str);
								if(checkUID(uid_str)){
									LCD_Gotoxy(4, 1);
									LCD_Send_String("UID hop le");
								} else {
									LCD_Gotoxy(1, 1);
									LCD_Send_String("UID khong hop le");
								}
								delayms(1500);
								LCD_Clear();
            }
        }
    }
}