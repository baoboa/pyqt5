This is EAK v1.0 

Electronic Automative Kit abbreviated as EAK v1.0 is designed for analysing the devices connected to Microcontroller or Development Boards on Computer Screen.

In demo, Kit is connected with Arduino Uno Development Board ( Microcontroller Atmega328P) which is sending serial data in the form shown below
"<Reading of Device 1>  <Reading of Device 2>  <Reading of Device 3>  <Reading of Device 4>  <Reading of Device 5>"
because 5 devices are connected to it . These devices are sending data to Arduino Uno (either Analog or Digital). Arduino is sending this data sepreated by spaces to Serial Monitor or (sending data Serially through USB). We have designed a dummy arduino code in which we have defined 5 variables and sending their values serially everytime incrementing their values. INO file is enclosed in this repo. 

Software senses the available ports of Computer and List them. User selects the COM Port whose data he wants to take. Software tells the number of Devices connected to that COM Port when clicking Check Devices button. After clicking GET DATA button, Software fetches the real time Data of all the Devices connected to that PORT and display them sequentially.

Limitation :
1. Number of Devices cannot exceed 12. 

Future Prospects:
1. Removal of Bugs
2. Dynamic Allocation of Components on Software Window 

Project is in Progress...
