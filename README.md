PN532-cardio_NFC_MPR121_simplifie

PN532 eAmusement/Aime/Nesica/BANAPASSPORT NFC USB HID card reader (cardio) with integrated MPR121 keypad. Supports ISO14443 and FeliCa.

This is a mod of my PN5180-cardio project made to work with a PN532 module instead, over its I2C communication mode.

# Acknowledgments

This work is based on zyp's cardio (obviously).

PN532 code is taken from [elechouse](https://github.com/elechouse/PN532) library.

# Supported devices

USBHID code has been tested on Arduino Due, Leonardo, and Pro Micro.

SPICEAPI code has been tested on Arduino UNO.
It should support any SPI-capable arduino without native USB HID capabilities but might require fine-tuning
`SPICEAPI_WRAPPER_BUFFER_SIZE` and `SPICEAPI_WRAPPER_BUFFER_SIZE_STR` parameters in `PN532-cardio.ino`.

# Pinout
For more details about Électronics see : http://burogu.makotoworkshop.org/index.php?post/2022/05/26/sdvx7

**Note:** The PN532 has to be put in I2C mode (usually there are two dipswitches, the first one has to be set to ON and the second to OFF).
