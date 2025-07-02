#include "mfrc522.h"
#include <intrins.h>

void delay_us(unsigned int us) {
    while (us--) {
        _nop_(); // T?o d? tr? nh?, c?n di?u ch?nh d?a trên t?n s? clock
    }
}

// Hàm SPI bit-banging: G?i và nh?n m?t byte
unsigned char spi_transfer(unsigned char byte) {
    unsigned char i, received = 0;
    for(i = 0; i < 8; i++) {
        if(byte & 0x80) MFRC522_MOSI = 1; else MFRC522_MOSI = 0;
        MFRC522_SCK = 1;
        if(MFRC522_MISO) received |= (1 << (7 - i));
        MFRC522_SCK = 0;
        byte <<= 1;
        delay_us(1);  // Delay nh? d? d?m b?o timing
    }
    return received;
}

// Hàm ghi vào thanh ghi MFRC522
void mfrc522_write(unsigned char reg, unsigned char value) {
    MFRC522_CS = 0;
    spi_transfer((reg << 1) & 0x7E);  // Ð?a ch? ghi, bit 7 = 0
    spi_transfer(value);
    MFRC522_CS = 1;
}

// Hàm d?c t? thanh ghi MFRC522
unsigned char mfrc522_read(unsigned char reg) {
    unsigned char value;
    MFRC522_CS = 0;
    spi_transfer(((reg << 1) & 0x7E) | 0x80);  // Ð?a ch? d?c, bit 7 = 1
    value = spi_transfer(0x00);  // G?i byte dummy d? nh?n d? li?u
    MFRC522_CS = 1;
    return value;
}

// Hàm reset MFRC522
void mfrc522_reset(void) {
    MFRC522_RST = 0;
    delay_us(10);
    MFRC522_RST = 1;
    delay_us(10);
    mfrc522_write(CommandReg, PCD_RESETPHASE);
}

// Hàm kh?i t?o MFRC522
void mfrc522_init(void) {
    MFRC522_CS = 1;
    MFRC522_SCK = 0;
    MFRC522_MOSI = 0;
    mfrc522_reset();
    
    // Thi?t l?p các thanh ghi
    mfrc522_write(TModeReg, 0x8D);
    mfrc522_write(TPrescalerReg, 0x3E);
    mfrc522_write(TReloadRegL, 30);
    mfrc522_write(TReloadRegH, 0);
    mfrc522_write(TxASKReg, 0x40);
    mfrc522_write(ModeReg, 0x3D);
    
    // B?t anten
    mfrc522_write(TxControlReg, mfrc522_read(TxControlReg) | 0x03);
}

// Hàm yêu c?u th?
unsigned char mfrc522_request(unsigned char req_mode, unsigned char *tag_type) {
    unsigned char status;
    unsigned int back_bits;
    mfrc522_write(BitFramingReg, 0x07);
    tag_type[0] = req_mode;
    status = mfrc522_to_card(PCD_TRANSCEIVE, tag_type, 1, tag_type, &back_bits);
    if((status != MI_OK) || (back_bits != 0x10)) {
        status = MI_ERR;
    }
    return status;
}

// Hàm ch?ng va ch?m (anticollision)
unsigned char mfrc522_anticoll(unsigned char *serNum) {
    unsigned char status;
    unsigned char i;
    unsigned char serNumCheck = 0;
    unsigned int unLen;
    mfrc522_write(BitFramingReg, 0x00);
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = mfrc522_to_card(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);
    if(status == MI_OK) {
        for(i = 0; i < 4; i++) {
            serNumCheck ^= serNum[i];
        }
        if(serNumCheck != serNum[4]) {
            status = MI_ERR;
        }
    }
    return status;
}

// Hàm giao ti?p v?i th?
unsigned char mfrc522_to_card(unsigned char cmd, unsigned char *send_data, unsigned char send_len, unsigned char *back_data, unsigned int *back_len) {
    unsigned char status = MI_ERR;
    unsigned char irqEn = 0x00;
    unsigned char waitIRq = 0x00;
    unsigned char lastBits;
    unsigned char n;
    unsigned int i;
    
    switch(cmd) {
        case PCD_TRANSCEIVE:
            irqEn = 0x77;
            waitIRq = 0x30;
            break;
        default:
            break;
    }
    
    mfrc522_write(ComIEnReg, irqEn | 0x80);
    mfrc522_write(ComIrqReg, 0x7F);
    mfrc522_write(CommandReg, PCD_IDLE);
    mfrc522_write(FIFOLevelReg, 0x80);
    
    for(i = 0; i < send_len; i++) {
        mfrc522_write(FIFODataReg, send_data[i]);
    }
    
    mfrc522_write(CommandReg, cmd);
    
    if(cmd == PCD_TRANSCEIVE) {
        mfrc522_write(BitFramingReg, mfrc522_read(BitFramingReg) | 0x80);
    }
    
    i = 2000;
    do {
        n = mfrc522_read(ComIrqReg);
        i--;
    } while((i != 0) && !(n & 0x01) && !(n & waitIRq));
    
    mfrc522_write(BitFramingReg, mfrc522_read(BitFramingReg) & (~0x80));
    
    if(i != 0) {
        if(!(mfrc522_read(ErrorReg) & 0x1B)) {
            status = MI_OK;
            if(n & irqEn & 0x01) {
                status = MI_NOTAGERR;
            }
            if(cmd == PCD_TRANSCEIVE) {
                n = mfrc522_read(FIFOLevelReg);
                lastBits = mfrc522_read(ControlReg) & 0x07;
                if(lastBits) {
                    *back_len = (n - 1) * 8 + lastBits;
                } else {
                    *back_len = n * 8;
                }
                if(n == 0) {
                    n = 1;
                }
                if(n > 16) {
                    n = 16;
                }
                for(i = 0; i < n; i++) {
                    back_data[i] = mfrc522_read(FIFODataReg);
                }
            }
        } else {
            status = MI_ERR;
        }
    }
    
    return status;
}