# multithreaded-blur

A parallel implementation of Gaussian blur using pthreads to process the image
in 4 concurrent segments. Supports *24-bit* BMP files.

In a school project I was tasked with implementing multithreading to complete a specific task. Using pthreads I was able to multi-thread the process to apply a gaussian blur to a BMP image.


![gif showing the process on how to blur an image](assets/blur.gif)


## Usage

### Compiling

>I don't believe that pthreads is supported on Windows, so i would be aware it may only run if you are on MacOS/Linux

Just run the following:
```
make
```

### Running

```
./blur <file_name>.bmp <blur_radius>
```

The resulting blurred image should be in `output.bmp`

## Blur Process

In this implementation I used a typical gaussian blur filter for the image.

$$
G(x) =\frac{1}{2\pi\sigma^2}e^{-\frac{x^2 + y^2}{2\sigma^2}}
$$

I precomputed the kernel to apply and then split the image into 4 regions for which the threads would then apply the blur.

There are better and faster ways to get the nice natural look of a gaussian blur. Many of them being related to downscaling and upscaling such as the Kawase blur.

For simplicity sake, I did not implement that :)
For more information on Kawase blur check out this link:
- [**Intel** - An investigation of fast real-time GPU-based image blur algorithms](https://www.intel.com/content/www/us/en/developer/articles/technical/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms.html)


If you would like to learn more about blur filters in general here are some helpful resources:
- [**Nvidia** - Incremental Computation of the Gaussian](https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-40-incremental-computation-gaussian)
- [**3Blue1Brown** video on convolutions](https://www.youtube.com/watch?v=KuXjwB4LzSA)
- [**Computerphile** video on gaussian blur](https://www.youtube.com/watch?v=C_zFhWdM4ic)


## Final Thoughts

Doing this project made me realize how much I enjoy image processing. I may add onto this project or create a whole suite of tools for image processing someday.
