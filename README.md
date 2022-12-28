# TEF6686 [ESP32] - Remastered!

An improvement of PE5PVB's TEF6686 firmware based on ESP32.  
Original firmware: https://github.com/PE5PVB/TEF6686_ESP32

[<img alt="Join the TEF6686 Discord community!" src="https://i.imgur.com/BYqhuLI.png">](https://discord.gg/ZAVNdS74mC)  


## List of changes since v1.15:
### v1.21
- RDS RadioText now scrolls, so you can see all 64 characters
- Graphical glitches while scrolling in menus have been fixed
- Added Wi-Fi signal in dBm to the new menu
- XDR-GTK's signal graph is much smoother now (similar to F1HD) 

### v1.20
- Fixed the rotary button so it doesn't activate twice
- New theme engine
- Added a second menu (hold blue button to activate)
- Added battery voltage/percentage info
- Readded XDR-GTK support for chinese tuners
- Added XDR-GTK support over wifi
- New dBf/dBuV switch
- Changed the bandwidth refresh timer
- Changed the default FM step to 100 kHz
- New switch for RDS info (the user can now choose if the info should disappear on low signal)
- Changed the PTY of "Variable" to "Varied"
- A new switch for screen shutdown on XDR-GTK

## Installation

### Windows: 🪟
1) Press **"WIN+R"** and open **devmgmt.msc**
2) Under the section **"COM & LPT"**, you will see your tuner - mark down the COM port, you will need it later
3) Edit the **flash.bat** file and edit the **"COM"** port to the number you see in devmgmt.msc
4) Hold the **BOOT button** on your TEF6686 ESP32 board while starting the tuner, that will put it into the bootloader mode
5) Open **flash.bat** and wait until the proccess finishes
6) Reboot the tuner - that's it!

## Contributors
[PE5PVB](https://github.com/PE5PVB/TEF6686_ESP32) - ESP32 TEF6686 Project  
[Hyper DX](https://github.com/HyperDX) - RDS Autoclear function + Bandwidth update limiter  
[Koishii](https://github.com/Koishii) - Code ideas, community moderation  
[Konrad Kosmatka](https://github.com/kkonradpl) - XDR-GTK support improvements
  
A big THANK YOU to all the contributors! 💕

## License
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
