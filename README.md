# **FPGA-Based WAV Music Player 🎵**

## **Overview**
This project is an **embedded audio player** built for an **Altera FPGA**, written in **C**. It supports **WAV file playback** from an SD card using the **FAT filesystem (FATFS)** and interacts with an **Altera Avalon Audio Interface** for real-time audio processing. Users can control playback through **hardware buttons and switches**, with an **LCD display** providing status updates.

## **Features** 🚀
- ✅ **WAV file playback** from SD card (FAT32).
- ✅ **Real-time audio processing** using the Altera UP Avalon Audio Interface.
- ✅ **Playback speed control** (normal, half, double speed, mono).
- ✅ **Interrupt-driven track control** (play, pause, stop, next, previous).
- ✅ **LCD display UI** for real-time track information.
- ✅ **Efficient audio buffering** with FIFO management.

## **Hardware Requirements** 🔧
- **Altera FPGA Development Board** (Cyclone IV, DE2-115, or similar).
- **SD card reader (for FAT32 storage access).**
- **Altera Avalon Audio Interface**.
- **LCD Display (optional, for status updates).**
- **Push buttons & switches** (for playback control).
- **Speakers/headphones (connected via audio out).**

## **Software Requirements** 💻
- **Quartus Prime / NIOS II Embedded Design Suite**.
- **C Compiler for NIOS II (GCC-based toolchain)**.
- **Altera HAL (Hardware Abstraction Layer) libraries**.
- **FATFS Library (for file system operations)**.
