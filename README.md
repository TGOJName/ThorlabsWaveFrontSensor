# ThorlabsWaveFrontSensor
This is a project to implement a [labscript_device](https://docs.labscriptsuite.org/projects/labscript-devices/en/latest/index.html) for accessing the Thorlabs [WFS](https://www.thorlabs.com/newgrouppage9.cfm?objectgroup_id=5287) Shack-Hartmann wavefront sensors used in the optical setup of our [RbYb Tweezer Lab](https://porto.jqi.umd.edu/).

**If you are a member of the Lab, please access the project's [OneNote](https://onedrive.live.com/view.aspx?resid=601343494F74D454%215914&id=documents&wd=target%28Setting%20up%20Hardware%20Vol.%202.one%7C4494AEAA-32A1-4BE6-AB27-3721055533CD%2FWavefront%20Sensor%20Computer%20Interface%7C93D8EA3C-5EB2-40D4-88B2-01A67AE4D704%2F%29) page.**

## Author Info:
**Created by:** Juntian "Oliver" Tu  
**E-mail:** [juntian@umd.edu](mailto:juntian@umd.edu)  
**Address:** 2261 Atlantic Building, 4254 Stadium Dr, College Park, MD 20742

## Environment Requirement
The project is developed in `Python 3` and based on the [`WFS`](https://www.thorlabs.com/software_pages/ViewSoftwarePage.cfm?Code=WFS) SDK from Thorlabs under 64-bit Windows environment.

This implementation is a part of the [Labscript suite](https://docs.labscriptsuite.org/en/latest/) in the master computer in our lab and could not run without the Labscript platform. There is a floating `WFS_checker.cpp` that is used for testing principles.

The program is designed to work with wavefront sensors connected through USB.

## Project Structure:
The wavefront sensor is implemented in a similar form of [IMAQdx Camera](https://docs.labscriptsuite.org/projects/labscript-devices/en/latest/devices/#:~:text=the%20IMAQdx%20class.-,IMAQdx%20Cameras,-Overview). However, it will not generate an image but store the measured wavefront data and Zernike fits in thr hdf5 file of the experiment shot.

## Acknowledgements:
Thanks to **P. T. Starkey** and **C. J. Billington** from Monash University for the development and maintenance of Labscript.