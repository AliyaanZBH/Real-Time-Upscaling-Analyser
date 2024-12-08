This tool was created as part of my dissertation for my Master's in Computer Science for Games. It allows you to test several cutting edge upscaling techniques within the context of the same rendering engine and on the exact same scenes!

This repo was forked from Microsoft's DX12 Samples repository, as I did not have enough time to construct an entire renderng engine from scratch. Please find their README at their repo below:

https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/README.md

# Setup

The repo uses submodules so remember to clone using --recurse-submodules

## Assets
I cannot use Git LFS due to being a public fork of Microsoft's repo, so please find the assets on GDrive! 

 - Amazon Bistro
    - https://drive.google.com/drive/folders/1Pt_xRMfKjZso96NdROuHLoR_yLE3i-NP?usp=sharing

# Build

For the time being, you need to manually copy the nvgx-dlss.dll from the lib folder and place it next to the MiniEngine exe in RTUA. This will soon be improved to be completed within a post-build step.


# Third Party

I use the Amazon Bistro scene provided by ORCA, the NVIDIA research group. The original files were provided in FBX format but these have been converted to GLTF, and the original files are not included in the repo directly. Find the files in the [Assets section](#Assets) above.

# Assets

