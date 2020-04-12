/*
** 
**    This is not part of the firmware. Just for your information! 
**    
**    This is the avrdude log
*/

~/Library/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/bin/avrdude -C~/Library/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf -v -patmega328p -cusbtiny -Uflash:w:~/I2C_ATmega_RelaysMux.ino.hex:i 

avrdude: Version 6.3-20190619
         Copyright (c) 2000-2005 Brian Dean, http://www.bdmicro.com/
         Copyright (c) 2007-2014 Joerg Wunsch

         System wide configuration file is "~/Library/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf"
         User configuration file is "~/.avrduderc"
         User configuration file does not exist or is not a regular file, skipping

         Using Port                    : usb
         Using Programmer              : usbtiny
avrdude: usbdev_open(): Found USBtinyISP, bus:device: 020:005
         AVR Part                      : ATmega328P
         Chip Erase delay              : 9000 us
         PAGEL                         : PD7
         BS2                           : PC2
         RESET disposition             : dedicated
         RETRY pulse                   : SCK
         serial program mode           : yes
         parallel program mode         : yes
         Timeout                       : 200
         StabDelay                     : 100
         CmdexeDelay                   : 25
         SyncLoops                     : 32
         ByteDelay                     : 0
         PollIndex                     : 3
         PollValue                     : 0x53
         Memory Detail                 :

                                  Block Poll               Page                       Polled
           Memory Type Mode Delay Size  Indx Paged  Size   Size #Pages MinW  MaxW   ReadBack
           ----------- ---- ----- ----- ---- ------ ------ ---- ------ ----- ----- ---------
           eeprom        65    20     4    0 no       1024    4      0  3600  3600 0xff 0xff
           flash         65     6   128    0 yes     32768  128    256  4500  4500 0xff 0xff
           lfuse          0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           hfuse          0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           efuse          0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           lock           0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           calibration    0     0     0    0 no          1    0      0     0     0 0x00 0x00
           signature      0     0     0    0 no          3    0      0     0     0 0x00 0x00

         Programmer Type : USBtiny
         Description     : USBtiny simple USB programmer, https://learn.adafruit.com/usbtinyisp
avrdude: programmer operation not supported

avrdude: Using SCK period of 10 usec
avrdude: AVR device initialized and ready to accept instructions

Reading | ################################################## | 100% 0.00s

avrdude: Device signature = 0x1e950f (probably m328p)
avrdude: safemode: lfuse reads as FF
avrdude: safemode: hfuse reads as D6
avrdude: safemode: efuse reads as FF
avrdude: NOTE: "flash" memory has been specified, an erase cycle will be performed
         To disable this feature, specify the -D option.
avrdude: erasing chip
avrdude: Using SCK period of 10 usec
avrdude: reading input file "~/I2C_ATmega_RelaysMux.ino.hex"
avrdude: writing flash (3248 bytes):

Writing | ################################################## | 100% 3.10s

avrdude: 3248 bytes of flash written
avrdude: verifying flash memory against ~/I2C_ATmega_RelaysMux.ino.hex:
avrdude: load data flash data from input file ~/I2C_ATmega_RelaysMux.ino.hex:
avrdude: input file ~/I2C_ATmega_RelaysMux.ino.hex contains 3248 bytes
avrdude: reading on-chip flash data:

Reading | ################################################## | 100% 2.63s

avrdude: verifying ...
avrdude: 3248 bytes of flash verified

avrdude: safemode: lfuse reads as FF
avrdude: safemode: hfuse reads as D6
avrdude: safemode: efuse reads as FF
avrdude: safemode: Fuses OK (E:FF, H:D6, L:FF)

avrdude done.  Thank you.
