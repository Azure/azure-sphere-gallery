# Balancing Robot Electronics

This folder contains the design files for the electronics used to power the balancing robot. There are two versions of the design files: The v1.1 files were used to manufacture and test the design, and v1.2 which corrects a minor hardware bug, but which have not yet been used for manufacture. Within each of these two folders are the Altium Designer design files, along with manufacturing files for the project. 

## Contents

The electronics files are organised into the following folders:

| File/folder | File/subfolder | Description |
|-------------|-------------|------------|
| `v1.x/Altium`      |  | Altium Designer schematics, PCB layout, and supporting project files |
| `v1.x/Manufacturing Files`   |  `Gerber Files`|Folder containing the PCB gerber files|
||`NC Drill Files`|Folder containing the PCB drill files|
||`Pick and Place Files`|Folder containing PCB pick and place files for automated assembly|
||`Balancing_Robot_BoM_1_x.xlsx`  | Excel spreadsheet with the bill of materials for the electronics
||`BoM Template.xltx`|Excel template used by Altium Designer to format the bill of materials |
||`PCB Layer Stack.png`|PNG image file showing the PCB layer stack|
||`PCB Render - Front v1.x.png`|3D render of front of the PCB|
||`PCB Render - Back v1.x.png`|3D render of back of the PCB|
| `v1.x/Balancing_Robot_Schematics_PCB_v1.x.PDF`        | | PDF version of the schematics and PCB layout |
| `Top Level BoM.xlsx` || Excel spreadsheet containing the bill of materials for other electronic/electrical parts not attached to the PCB |
| `README.md` | |This README file. |
| `LICENSE`   || The license for the project. |

## Tools

The electronics design files were created using [Altium Designer](https://www.altium.com/). 

## Project expectations

The v1.1 version of the project has been manufactured in small quantities to help verify the design. As part of this work, it has undergone EMC pre-compliance testing and laser safety compliance testing. However, it is important to note that if you wish to build and distribute your own version of the balancing robot, it is your responsibility to ensure all compliance requirements for your geographic location are met; the compliance testing that we have undertaken does not carry over to other manufacturers.

Late in the development process, a minor hardware bug was discovered which has been corrected in the v1.2 version of the hardware design, however, this version has not been manufactured or tested.

## License

Creative Commons Attribution 4.0 International. For details on license, see the [LICENSE.txt](../LICENSE.txt) file.