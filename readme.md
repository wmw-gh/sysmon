# SysMon - System Monitor

## Overview
System Monitor is a command line program for Windows, used to read and display in real time various system information: CPU, GPU, RAM and networking data. It was created as an alternative to tools native to Windows, but lightweight and easy to modify. I used C mainly due to my previous experience, and for builing it utilises plain makefile + msvc, as I'm not a fan of Visual Studio and all the bloat it brings. It was created as a hobby project and as such I was only able to test this on my local PCs equipped with AMD CPUs (one laptop with integrated graphics) and NVIDIA GPU.
## Build
requires:
* Visual Studio Build Tools (tested with 2026 version)
* GNU Make for Windows
* (optional) Windows Terminal for nicer display than plain command line

To build, start "x64 Native Tools Command Prompt for VS 2022", navigate to repo and simply use make. Requires admin privileges to run.

## Libraries
SysMon uses:
* PawnIO to read CPU temperature
* Mini-Regex for extracting data from command line output
* NVIDIA API to read load and temperature of GPU

## Other acknowledgements
LibreHardwareMonitor was a huge inspiration in creating this project.

## To Do
* better error handling
* unit tests
* read and display motherboard information
* control CPU and GPU fans from Windows
* add command line parameters to customise displayed values
* monitor and display current network connections, similar to resmon
