# rfDecoder_v1dot1
This is the source code to upload to your arduino which display the results of a decrypted RF signal in the 433.29 MHz range.
The inspiration comes from building a home automation network and needing to extract the 24 bit binary code, pulse length, and 
protocol of each on or off transmission for each outlet. This code when inserted into the appropriate hardware will display
those values to a screen in addition to recording the values to a .csv value allowing you to be un-tethered to your laptop
when retrieving these values to replace your RF remotes. Most of the code is simply taking the RCSwitch Library example code
along with the Elegoo TFT display example and combining them. An example of the hardware this code is applicable to can be 
found under my open-source page at machineandc.tech.
