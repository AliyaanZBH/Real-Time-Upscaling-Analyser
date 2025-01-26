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
