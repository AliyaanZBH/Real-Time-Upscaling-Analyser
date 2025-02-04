This tool was created as part of my dissertation for my Master's in Computer Science for Games. It allows you to test several cutting edge upscaling techniques within the context of the same rendering engine and on the exact same scenes!

This repo was forked from Microsoft's DX12 Samples repository, as I did not have enough time to construct an entire renderng engine from scratch. Please find their README at their repo below:

https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/README.md

# Setup

This repository uses Git Submodules so it is **vital** that you clone the repo using the **--recurse-submodules** suffix. The project will fail to build if you miss this step, but it can technically be recovered albeit with alot of extra effort. I instead recommend that you delete the local repository and clone it from fresh.

To ensure minimum friction, copy the below command into your terminal to clone the main branch of the repository with all submodules. If you would like to see a specific branch, please add the -b prefix.

```git
git clone https://github.com/AliyaanZBH/Real-Time-Upscaling-Analyser.git --recurse-submodules
```

Please visit the [official documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules) for more information on submodules.

# Build

Simply navigate to MiniEngine/RTUA and open the .sln file and build! All DLLS should be automatically copied across in a post build step now.

# Known Bugs and Troubleshooting

For some reason, **GPU Tweak III** (and possibly older versions, tests needed here) causes an issue with the application, specifcally causing the swapchain to lose sync and cause a run-time exception. 
This has been added to the list of exceptions to ignore, but there may be more applications that cause a similar issue, especially those with **OSD** or **On-Screen Display**. Therefore, **please ensure all applications are exited before running the application!**

Additionally, the University computers cause a few bugs that do not occur anywhere else I have tested the application:
* The application will infrequently open in windowed mode, when it *must* be opened in full-screen in order for all aspects of graphics manipulation to work. So, if you notice the application opening in windowed mode **you must close it and re-open it**.
  * Originally the DLSS implementation worked in both windowed and full-screen mode, but changes were made as it was refined and I decided that rarely are people going to be playing in windowed mode AND running DLSS.
  * If you would like to help solve this bug, see the Windows message pump and elements of the GUI namespace. The way the application has been written, it *should* always start in fullscreen due to the way the swapchain is manipulated in these places.  
* The built application (i.e. the .exe) will sporadically fail to run on certain University machines. If this occurs on your machine, **the solution is to open command line and run the application from there**.
  * Not sure why this happens, and it doesn't even happen consistently. For example, the exact same machine will be able to run the .exe normally on one day, but on the following day it will fail and require you to use the terminal.
  * Leading theories lie within the nature of University machines and the way their drives are networked. When testing the exe on my home computer, laptop, and my peers machines, the .exe has always run fine.
