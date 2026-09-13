# Analog TV emulator

This repo contains a tool that shows images or videos like they are on an old TV screen. NTSC standard is emulated, interlacing is not used.

It's based on a tool called `analogtv-cli` from [XScreensaver](https://www.jwz.org/xscreensaver/) stripped to the minimum.
The original code is written by [Trevor Blackwell](https://tlb.org/), [Jamie Zawinski](https://jwz.org/) and the team.

It imitates old TV so well that I always wanted to have this as a filter.

OpenCV is used for video I/O, image loading and memory management.
Qt6 or Qt5 is used for GUI.
nlohmann_json is used for JSON loading.

### How to build
* Get OpenCV 5, Qt6 or Qt5, nlohmann_json and CMake
* Run CMake with the flags:
  - `-DOpenCV_DIR=<path_to_OpenCV_installation>/lib/cmake/opencv5`
  - `-DQt6_DIR=<path_to_Qt_installation>/<version>/gcc_64/lib/cmake/Qt6` or `-DQt5_DIR=<path_to_Qt_installation>/lib/cmake/Qt5`,
  - `-Dnlohmann_json_DIR=<path_to_nlohmann_json_installation>/share/cmake/nlohmann_json/`
* Build it

### How to run
* Find several videos, images or cameras as signal sources
* Provide inputs, outputs and other parameters in command line like this:
  ```
  analogtv-cli --control :random:duration=60:powerup --size 1280 1024 --in image.png :bars:logo.png :cam:0 sintel.avi --out video.mp4 :highgui
  ```
* Every output gets the same video frames, can be used to write video and control it with GUI
* Alternatively, GUI with prepared JSON settings can be used:
  ```
  analogtv-gui example.json
  ```
* For more details, see command line help

### TODO
* keep desired FPS
* transform this code to a platform-independent shader-like filter

### Copyright notice

xscreensaver, Copyright (c) 1992-2014 Jamie Zawinski <jwz@jwz.org>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.  No representations are made about the suitability of this
software for any purpose.  It is provided "as is" without express or 
implied warranty.