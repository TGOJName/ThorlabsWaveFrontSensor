// The Thorlabs WFS-20 and -30 data receiver by Oliver Tu, modified from the official code of Thorlabs by Egbert Krause (see original annotation below).
// This code should be complied with provided headers and WFS_64.dll
// This code is editted for checking indexes of connected WFS accompanied with labscript implementation


/*===============================================================================================================================

	Thorlabs Wavefront Sensor sample application

	This sample program for WFS Wavefront Sensor instruments connects to a selected instrument,
	configures it, takes some measurment and displays the results.
	Finally it closes the connection.

	Source file 'sample.c'

	Date:          Jun-12-2018
	Software-Nr:   N/A
	Version:       1.4
	Copyright:     Copyright(c) 2018, Thorlabs GmbH (www.thorlabs.com)
	Author:        Egbert Krause (ekrause@thorlabs.com)

	Changelog:     Dec-04-2009 -> V1.0
						Nov-30-2010 -> V1.1 extended to WFS10 series instruments, highspeed mode enabled
						Dec-19-2013 -> V1.2 added loop with data output to file
						Sep-08-2014 -> V1.3 added WFS20 support
						Jun-12-2018 -> V1.4 added WFS30/WFS40 support
						
	Disclaimer:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

===============================================================================================================================*/


/*===============================================================================================================================
  Include Files

  Note: You may need to set your compilers include search path to the VXIPNP include directory.
		  This is typically 'C:\Program Files (x86)\IVI Foundation\VISA\WinNT\WFS'.

===============================================================================================================================*/

#include "headers\wfs.h" // Wavefront Sensor driver's header file
#include <stdlib.h>
#include <stdio.h>


/*===============================================================================================================================
  Defines
===============================================================================================================================*/

#define  DEVICE_OFFSET_WFS20           (0x00200) // device IDs of WFS20 instruments start at 512 decimal
#define  DEVICE_OFFSET_WFS30           (0x00400) // device IDs of WFS30 instruments start at 1024 decimal

/*===============================================================================================================================
  Data type definitions
===============================================================================================================================*/
typedef struct
{
	int               selected_id;
	unsigned long     handle;
	
	char              version_wfs_driver[WFS_BUFFER_SIZE];
	char              version_cam_driver[WFS_BUFFER_SIZE];
	char              manufacturer_name[WFS_BUFFER_SIZE];
	char              instrument_name[WFS_BUFFER_SIZE];
	char              serial_number_wfs[WFS_BUFFER_SIZE];
	char              serial_number_cam[WFS_BUFFER_SIZE];
	
	long              mla_cnt;
	char              mla_name[WFS_BUFFER_SIZE];
	double            cam_pitch_um;
	double            lenslet_pitch_um;
	double            center_spot_offset_x;
	double            center_spot_offset_y;
	double            lenslet_f_um;
	double            grd_corr_0;
	double            grd_corr_45;

}  instr_t;


/*===============================================================================================================================
  Function Prototypes
===============================================================================================================================*/
void handle_errors (int);
int select_instrument (int *selection, ViChar resourceName[]);
void check_mla ();


/*===============================================================================================================================
  Global Variables
===============================================================================================================================*/
// const int   cam_wfs_xpixel[] = { 1280, 1024, 768, 512, 320 }; // WFS150/300
// const int   cam_wfs_ypixel[] = { 1024, 1024, 768, 512, 320 };
// const int   cam_wfs10_xpixel[] = {  640,  480, 360, 260, 180 };
// const int   cam_wfs10_ypixel[] = {  480,  480, 360, 260, 180 };
const int   cam_wfs20_xpixel[] = {  1440, 1080, 768, 512, 360,  720, 540, 384, 256, 180 };
const int   cam_wfs20_ypixel[] = {  1080, 1080, 768, 512, 360,  540, 540, 384, 256, 180 };
const int   cam_wfs30_xpixel[] = {  1936, 1216, 1024, 768, 512, 360, 968, 608, 512, 384, 256, 180 };
const int   cam_wfs30_ypixel[] = {  1216, 1216, 1024, 768, 512, 360, 608, 608, 512, 384, 256, 180 };
// const int   cam_wfs40_xpixel[] = {  2048, 1536, 1024, 768, 512, 360, 1024, 768, 512, 384, 256, 180 }; 
// const int   cam_wfs40_ypixel[] = {  2048, 1536, 1024, 768, 512, 360, 1024, 768, 512, 384, 256, 180 }; 

instr_t     instr = { 0 };    // all instrument related data are stored in this structure

/*===============================================================================================================================
  Code
===============================================================================================================================*/
int main (void)
{
	int               err;
	int               cnt;
	int               selection;
	ViChar            resourceName[256];


	printf("This executable is used to specifies the sensorIndex put in the connection table as well as the MLA index used in blacs_worker.py for labscript implementation.\n");
	printf("Do not run Blacs while running this software otherwise one of them would not funciton properly.\n");
	printf("Press <ENTER> to continue.\n\n");
	fflush(stdin);
	getchar();


	// Get the driver revision
	if(err = WFS_revision_query (0, instr.version_wfs_driver, instr.version_cam_driver)) // pass NULL because handle is not yet initialized
		handle_errors(err);
	
	printf("WFS instrument driver version : %s\n\n", instr.version_wfs_driver);
	
	
	// Show all and select one WFS instrument
	if(select_instrument(&instr.selected_id, resourceName) == 0)
	{
		printf("\nNo instrument selected. Press <ENTER> to exit.\n");
		fflush(stdin);
		getchar();
		return 0; // program ends here if no instrument selected
	}
	
	// print out the resource name
	printf("\nResource name of selected WFS: %s\n", resourceName);
	
	
	// Open the Wavefront Sensor instrument
	if(err = WFS_init (resourceName, VI_FALSE, VI_FALSE, &instr.handle)) 
		handle_errors(err);

	// Get instrument information
	if(err = WFS_GetInstrumentInfo (instr.handle, instr.manufacturer_name, instr.instrument_name, instr.serial_number_wfs, instr.serial_number_cam))
		handle_errors(err);
	
	printf("\n");
	printf("Opened Instrument:\n");
	printf("Manufacturer           : %s\n", instr.manufacturer_name);
	printf("Instrument Name        : %s\n", instr.instrument_name);
	printf("Serial Number WFS      : %s\n", instr.serial_number_wfs);
	
	
	// Select a microlens array (MLA)
	check_mla();

	
	// Configure WFS camera, use a pre-defined camera resolution
	// if((instr.selected_id & DEVICE_OFFSET_WFS10) == 0 && (instr.selected_id & DEVICE_OFFSET_WFS20) == 0 && (instr.selected_id & DEVICE_OFFSET_WFS30) == 0 && (instr.selected_id & DEVICE_OFFSET_WFS40) == 0) // WFS150/300 instrument
	// {   
	// 	printf("\n\nConfigure WFS camera with resolution index %d (%d x %d pixels).\n", SAMPLE_CAMERA_RESOL_WFS, cam_wfs_xpixel[SAMPLE_CAMERA_RESOL_WFS], cam_wfs_ypixel[SAMPLE_CAMERA_RESOL_WFS]);
	// }
	
	// if(instr.selected_id & DEVICE_OFFSET_WFS10) // WFS10 instrument
	// {
	// 	printf("\n\nConfigure WFS10 camera with resolution index %d (%d x %d pixels).\n", SAMPLE_CAMERA_RESOL_WFS10, cam_wfs10_xpixel[SAMPLE_CAMERA_RESOL_WFS10], cam_wfs10_ypixel[SAMPLE_CAMERA_RESOL_WFS10]);
	// }
	
	if(instr.selected_id & DEVICE_OFFSET_WFS20) // WFS20 instrument
	{
		printf("\n\nResolution index for WFS20 camera:\n");
		for(cnt=0;cnt<10;cnt++){
			printf("%d  %dx%d\n",cnt,cam_wfs20_xpixel[cnt],cam_wfs20_ypixel[cnt]);
		}
	}
	
	if(instr.selected_id & DEVICE_OFFSET_WFS30) // WFS30 instrument
	{
		printf("\n\nResolution index for WFS30 camera:\n");
		for(cnt=0;cnt<12;cnt++){
			printf("%d  %dx%d\n",cnt,cam_wfs30_xpixel[cnt],cam_wfs30_ypixel[cnt]);
		}
	}
	
	// if(instr.selected_id & DEVICE_OFFSET_WFS40) // WFS40 instrument
	// {
	// 	printf("\n\nConfigure WFS40 camera with resolution index %d (%d x %d pixels).\n", SAMPLE_CAMERA_RESOL_WFS40, cam_wfs40_xpixel[SAMPLE_CAMERA_RESOL_WFS40], cam_wfs40_ypixel[SAMPLE_CAMERA_RESOL_WFS40]);
	// }
	
	printf("Press <ENTER> to exit.");
	WFS_close(instr.handle);
	fflush(stdin);
	getchar();
	return 0;
}



/*===============================================================================================================================
  Handle Errors
  This function retrieves the appropriate text to the given error number and closes the connection in case of an error
===============================================================================================================================*/
void handle_errors (int err)
{
	char buf[WFS_ERR_DESCR_BUFFER_SIZE];

	if(!err) return;

	// Get error string
	WFS_error_message (instr.handle, err, buf);

	if(err < 0) // errors
	{
		printf("\nWavefront Sensor Error: %s\n", buf);

		// close instrument after an error has occured
		printf("\nProgram will be closed because of the occured error, press <ENTER>.");
		WFS_close(instr.handle); // required to release allocated driver data
		fflush(stdin);
		getchar();
		exit(1);
	}
}



/*===============================================================================================================================
	Select Instrument
===============================================================================================================================*/
int select_instrument (int *selection, ViChar resourceName[])
{
	long           i,err,instr_cnt;
	ViInt32        device_id;
	long           in_use;
	char           instr_name[WFS_BUFFER_SIZE];
	char           serNr[WFS_BUFFER_SIZE];
	char           strg[WFS_BUFFER_SIZE];
	int 		   returned = 0;

	// Find available instruments
	if(err = WFS_GetInstrumentListLen (VI_NULL, &instr_cnt))
		handle_errors(err);
		
	if(instr_cnt == 0)
	{
		printf("No Wavefront Sensor instrument found!\n");
		return 0;
	}

	// List available instruments
	printf("Available Wavefront Sensor instruments:\n\n");
	printf("Device_ID  WFS_name  Serial_num\n");
	
	for(i=0;i<instr_cnt;i++)
	{
		if(err = WFS_GetInstrumentListInfo (VI_NULL, i, &device_id, &in_use, instr_name, serNr, resourceName))
			handle_errors(err);
		
		printf(" %4d   %s    %s    %s\n", device_id, instr_name, serNr, (!in_use) ? "" : "(inUse)");
	}

	// Select instrument
	printf("\nSelect a Wavefront Sensor instrument for its MLA info: ");
	fflush(stdin);
	
	fgets (strg, WFS_BUFFER_SIZE, stdin);
	*selection = atoi(strg);

	// get selected resource name
	for(i=0;i<instr_cnt;i++)
	{   
		if(err = WFS_GetInstrumentListInfo (VI_NULL, i, &device_id, &in_use, instr_name, serNr, resourceName))
		   handle_errors(err);
		
		if(device_id == *selection)
			returned = *selection;
			break; // resourceName fits to device_id
	}
	
	return returned;
}


/*===============================================================================================================================
	MLA Specifier
===============================================================================================================================*/
void check_mla ()
{
	long           i,err,mla_cnt;

	// Read out number of available Microlens Arrays 
	if(err = WFS_GetMlaCount (instr.handle, &instr.mla_cnt))
		handle_errors(err);

	// List available Microlens Arrays
	printf("\nAvailable Microlens Arrays:\n\n");
	for(i=0;i<instr.mla_cnt;i++)
	{   
		if(WFS_GetMlaData (instr.handle, i, instr.mla_name, &instr.cam_pitch_um, &instr.lenslet_pitch_um, &instr.center_spot_offset_x, &instr.center_spot_offset_y, &instr.lenslet_f_um, &instr.grd_corr_0, &instr.grd_corr_45))
			handle_errors(err);   
	
		printf("%2d  %s   CamPitch=%6.3f LensletPitch=%8.3f\n", i, instr.mla_name, instr.cam_pitch_um, instr.lenslet_pitch_um);
	}
}


/*===============================================================================================================================
	End of source file
===============================================================================================================================*/
