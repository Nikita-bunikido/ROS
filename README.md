# ROS

![languages count](https://img.shields.io/github/languages/count/Nikita-bunikido/ROS)
![license](https://img.shields.io/github/license/Nikita-bunikido/ROS)
![stars count](https://img.shields.io/github/stars/Nikita-bunikido/ROS?style=social)
![forks count](https://img.shields.io/github/forks/Nikita-bunikido/ROS?style=social)
![yt subs count](https://img.shields.io/youtube/channel/subscribers/UCW3RoBYtEBnrX_dOI3ELlxA?style=social)
![C_lang](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=whit)


[![Readme Card](https://github-readme-stats.vercel.app/api/pin/?username=Nikita-Bunikido&repo=ROS&title_color=0F0F0F&icon_color=0F0F0F&text_color=0F0F0F&bg_color=f9f9f9)](https://github.com/extremecodetv/ExtremeCodeOS)

![ROS demo](pict/demo3.png)

### Introduction

**ROS** (_Rom Operating System_) is a small, DOS-like, AVR-targetting operating system, written specially for my own computing machine _NPAD 5_. 

It operates in text mode, without any _UI_, but applications can still draw _TUI_ using pseudo graphics.

The main idea of the project is to minimize usage of external libraries to reach the highest level of perfomance and efficiency. That's why all modules are beging made completely from scratch, including:

- Hardware drivers
- Custom 6x8 px font
- Custom filesystem

System works with filesystem located directly in _EEPROM chips_.

### Hardware

- **Screen** : ST7735 based 128x160 px TFT display
- **Keyboard** : Custom keyboard which uses _74hc165 shift registers_
- **CPU** : ATmega328P
- ...

### Build

- #### Step 1 - clone the repository

First of all you need to clone repository. The following command to do it:

    git clone https://github.com/Nikita-bunikido/ROS.git

- #### Step2 - compile & link modules

Using Makefile:

    cd ROS
    make

### TODO

- CHIP-8 emulator
- Programs executing from FLASH memory
- Working with RAM
- Command shell
  - System commands
  - Unknown command detection
  - Executing batch files
  - Executing [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) programs
  - File format detection for opening in correct program
    - **.rex** ( _ROS executable_ ) -> program loader
    - **.raw** ( _Raw binary_ ) -> hex editor
    - **.txt** ( _Text document_ ) -> text editor
    - **.rtm** ( _ROS text markup document_ ) -> ros text markup
    - **.rch8** ( _ROS-CHIP8 source file_ ) -> text editor
- Working with ROM
- Custom filesystem
- Documented API
- Builtin applications:
  - Physical memory editor
  - Hex editor
  - Text editor
  - CHIP8 assembler
  - Basic interpreter
- Desktop emulator

### CHANGELOG

- Font 6x8px (ASCII & pseudo graphics)
- SPI driver
- ST7735 driver
- Keyboard driver
- Output stack
- Basic I/O functions
- Full QWERTY support
- Replaced *"reserved"* attribute with *"underline"*
- Text cursor
- Input buffer
- System modes
  - Input mode
  - Busy mode
  - Idle mode
- ROS logo in boot process
- Log system
- Red screen of death
- Graphic timer
- Blinking cursor
- Cursor visibility control
- Flashing threads
- Screen clear question
- Running strings
- ROS-CHIP8 X86 Assembler lexer
- ROS-CHIP8 X86 Assembler parser
- ROS-CHIP8 X86 Assembler core
- ROS-CHIP8 X86 Assembler data segments
- ROS-CHIP8 X86 Assembler define, include support
- ROS-CHIP8 X86 Assembler