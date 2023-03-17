// The Thorlabs WFS-20 and -30 data receiver by Oliver Tu, modified from the official code of Thorlabs by Egbert Krause (see original annotation below).
// This code should be complied with provided headers and WFS_64.dll
// This code is editted for triggered mode


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
#include <string.h>
#include <windows.h>


/*===============================================================================================================================
  Defines
===============================================================================================================================*/

#define  DEVICE_OFFSET_WFS10           (0x00100) // device IDs of WFS10 instruments start at 256 decimal
#define  DEVICE_OFFSET_WFS20           (0x00200) // device IDs of WFS20 instruments start at 512 decimal
#define  DEVICE_OFFSET_WFS30           (0x00400) // device IDs of WFS30 instruments start at 1024 decimal
#define  DEVICE_OFFSET_WFS40           (0x00800) // device IDs of WFS40 instruments start at 2048 decimal

// settings for this sample program, you may adapt settings to your preferences
#define  OPTION_OFF                    (0)
#define  OPTION_ON                     (1)

#define  SAMPLE_PIXEL_FORMAT           PIXEL_FORMAT_MONO8   // only 8 bit format is supported
#define  SAMPLE_CAMERA_RESOL_WFS       CAM_RES_768          // 768x768 pixels, see wfs.h for alternative cam resolutions
#define  SAMPLE_CAMERA_RESOL_WFS10     CAM_RES_WFS10_360    // 360x360 pixels
#define  SAMPLE_CAMERA_RESOL_WFS40     CAM_RES_WFS40_512    // 512x512 pixels
#define  SAMPLE_REF_PLANE              WFS_REF_INTERNAL

#define  SAMPLE_IMAGE_READINGS         (10) // trials to read a exposed spotfield image

#define  SAMPLE_OPTION_DYN_NOISE_CUT   OPTION_ON   // use dynamic noise cut features  
#define  SAMPLE_OPTION_CALC_SPOT_DIAS  OPTION_OFF  // don't calculate spot diameters
#define  SAMPLE_OPTION_CANCEL_TILT     OPTION_ON   // cancel average wavefront tip and tilt
#define  SAMPLE_OPTION_LIMIT_TO_PUPIL  OPTION_OFF  // don't limit wavefront calculation to pupil interior

#define  SAMPLE_OPTION_HIGHSPEED       OPTION_ON   // use highspeed mode (only for WFS10 and WFS20 instruments)
#define  SAMPLE_OPTION_HS_ADAPT_CENTR  OPTION_ON   // adapt centroids in highspeed mode to previously measured centroids
#define  SAMPLE_HS_NOISE_LEVEL         (30)        // cut lower 30 digits in highspeed mode
#define  SAMPLE_HS_ALLOW_AUTOEXPOS     (1)         // allow autoexposure in highspeed mode (runs somewhat slower)

#define  SAMPLE_WAVEFRONT_TYPE         WAVEFRONT_MEAS // calculate measured wavefront

#define  SAMPLE_PRINTOUT_SPOTS         (5)  // printout results for first 5 x 5 spots only

/*===============================================================================================================================
  Data type definitions
===============================================================================================================================*/
typedef struct
{
	int               selected_id;
	unsigned long     handle;
	long              status;
	
	char              version_wfs_driver[WFS_BUFFER_SIZE];
	char              version_cam_driver[WFS_BUFFER_SIZE];
	char              manufacturer_name[WFS_BUFFER_SIZE];
	char              instrument_name[WFS_BUFFER_SIZE];
	char              serial_number_wfs[WFS_BUFFER_SIZE];
	char              serial_number_cam[WFS_BUFFER_SIZE];
	
	long              mla_cnt;
	int               selected_mla;
	int               selected_mla_idx;
	char              mla_name[WFS_BUFFER_SIZE];
	double            cam_pitch_um;
	double            lenslet_pitch_um;
	double            center_spot_offset_x;
	double            center_spot_offset_y;
	double            lenslet_f_um;
	double            grd_corr_0;
	double            grd_corr_45;
	
	long              spots_x;
	long              spots_y;

}  instr_t;


/*===============================================================================================================================
  Function Prototypes
===============================================================================================================================*/
void handle_errors (int);
int select_instrument (int *selection, ViChar resourceName[]);
int select_mla (int *selection);

// int CVIFUNC    KeyHit (void);
// int CVIFUNC    GetKey (void);


/*===============================================================================================================================
  Global Variables
===============================================================================================================================*/
const int   cam_wfs_xpixel[] = { 1280, 1024, 768, 512, 320 }; // WFS150/300
const int   cam_wfs_ypixel[] = { 1024, 1024, 768, 512, 320 };
const int   cam_wfs10_xpixel[] = {  640,  480, 360, 260, 180 };
const int   cam_wfs10_ypixel[] = {  480,  480, 360, 260, 180 };
const int   cam_wfs20_xpixel[] = {  1440, 1080, 768, 512, 360,  720, 540, 384, 256, 180 };
const int   cam_wfs20_ypixel[] = {  1080, 1080, 768, 512, 360,  540, 540, 384, 256, 180 };
const int   cam_wfs30_xpixel[] = {  1936, 1216, 1024, 768, 512, 360, 968, 608, 512, 384, 256, 180 };
const int   cam_wfs30_ypixel[] = {  1216, 1216, 1024, 768, 512, 360, 608, 608, 512, 384, 256, 180 };
const int   cam_wfs40_xpixel[] = {  2048, 1536, 1024, 768, 512, 360, 1024, 768, 512, 384, 256, 180 }; 
const int   cam_wfs40_ypixel[] = {  2048, 1536, 1024, 768, 512, 360, 1024, 768, 512, 384, 256, 180 }; 

const int   zernike_modes[] = { 1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 66 }; // converts Zernike order to Zernike modes

instr_t     instr = { 0 };    // all instrument related data are stored in this structure

long        hs_win_count_x,hs_win_count_y,hs_win_size_x,hs_win_size_y; // highspeed windows data
long        hs_win_start_x[MAX_SPOTS_X],hs_win_start_y[MAX_SPOTS_Y];


/*===============================================================================================================================
  Code
===============================================================================================================================*/
int main (void)
{
	int               err;
	int               i,j,cnt;
	long              rows, cols;   // image height and width, depending on camera resolution
	int               selection;
	unsigned char     *ImageBuffer; // pointer to the camera image buffer
	
	double            expos_act, master_gain_act;
	double            beam_centroid_x, beam_centroid_y;
	double            beam_diameter_x, beam_diameter_y;
	
	float             centroid_x[MAX_SPOTS_Y][MAX_SPOTS_X];
	float             centroid_y[MAX_SPOTS_Y][MAX_SPOTS_X];

	float             deviation_x[MAX_SPOTS_Y][MAX_SPOTS_X];
	float             deviation_y[MAX_SPOTS_Y][MAX_SPOTS_X];

	float             intensity[MAX_SPOTS_Y][MAX_SPOTS_X];

	float             wavefront[MAX_SPOTS_Y][MAX_SPOTS_X];
	
	float             zernike_um[MAX_ZERNIKE_MODES+1];             // index runs from 1 - MAX_ZERNIKE_MODES
	float             zernike_orders_rms_um[MAX_ZERNIKE_ORDERS+1]; // index runs from 1 - MAX_ZERNIKE_MODES
	double            roc_mm;
	
	long              zernike_order;
	
	double            wavefront_min, wavefront_max, wavefront_diff, wavefront_mean, wavefront_rms, wavefront_weighted_rms;
	ViChar            resourceName[256];
	FILE              *fp;
	int               key;

	char			  inputreader[200];
	int 			  selectedi;
	double			  selectedf;
	double			  pupilCen_x;
	double			  pupilCen_y;
	double			  pupilDia_x;
	double			  pupilDia_y;
	int 			  limited_to_pupil;
	char		      path[200];
	int 			  fourier_order;

	double 			  fourierM;
	double 			  fourierJ0;
	double 			  fourierJ45;
	double 			  optoSphere;
	double 			  optoCylinder;
	double 			  optoAxisDeg;
	long		      zernike_reconstruct[MAX_ZERNIKE_MODES+1];
	double			  fitErrMean;
	double			  fitErrDev;

	
	// Get the driver revision
	if(err = WFS_revision_query (0, instr.version_wfs_driver, instr.version_cam_driver)) // pass NULL because handle is not yet initialized
		handle_errors(err);
	
	//printf("Camera USB driver version     : %s\n", instr.version_cam_driver);
	printf("WFS instrument driver version : %s\n\n", instr.version_wfs_driver);
	
	
	// Show all and select one WFS instrument
	if(select_instrument(&instr.selected_id, resourceName) == 0)
	{
		printf("\nNo instrument selected. Press <ENTER> to exit.\n");
		fflush(stdin);
		getchar();
		return 0; // program ends here if no instrument selected
	}
	
	// Get the resource name for this instrument
	//if(err = WFS_GetInstrumentListInfo (VI_NULL, instr.selected_id, VI_NULL, VI_NULL, VI_NULL, VI_NULL, resourceName))
	// handle_errors(err);
	
	
	// print out the resource name
	printf("\nResource name of selected WFS: %s\n", resourceName);
	
	
	// Open the Wavefront Sensor instrument
	//if(err = WFS_init (instr.selected_id, &instr.handle))
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
	if(select_mla(&instr.selected_mla) < 0)
	{
		printf("\nNo MLA selected. Press <ENTER> to exit.\n");
		fflush(stdin);
		getchar();
		return 0;
	}
	
	// Activate desired MLA
	if(err = WFS_SelectMla (instr.handle, instr.selected_mla))
		handle_errors(err);

	
	
	// Configure WFS camera, use a pre-defined camera resolution
	if((instr.selected_id & DEVICE_OFFSET_WFS10) == 0 && (instr.selected_id & DEVICE_OFFSET_WFS20) == 0 && (instr.selected_id & DEVICE_OFFSET_WFS30) == 0 && (instr.selected_id & DEVICE_OFFSET_WFS40) == 0) // WFS150/300 instrument
	{   
		printf("\n\nConfigure WFS camera with resolution index %d (%d x %d pixels).\n", SAMPLE_CAMERA_RESOL_WFS, cam_wfs_xpixel[SAMPLE_CAMERA_RESOL_WFS], cam_wfs_ypixel[SAMPLE_CAMERA_RESOL_WFS]);
		
		if(err = WFS_ConfigureCam (instr.handle, SAMPLE_PIXEL_FORMAT, SAMPLE_CAMERA_RESOL_WFS, &instr.spots_x, &instr.spots_y))
			handle_errors(err);
	}
	
	if(instr.selected_id & DEVICE_OFFSET_WFS10) // WFS10 instrument
	{
		printf("\n\nConfigure WFS10 camera with resolution index %d (%d x %d pixels).\n", SAMPLE_CAMERA_RESOL_WFS10, cam_wfs10_xpixel[SAMPLE_CAMERA_RESOL_WFS10], cam_wfs10_ypixel[SAMPLE_CAMERA_RESOL_WFS10]);
	
		if(err = WFS_ConfigureCam (instr.handle, SAMPLE_PIXEL_FORMAT, SAMPLE_CAMERA_RESOL_WFS10, &instr.spots_x, &instr.spots_y))
			handle_errors(err);
	}
	
	if(instr.selected_id & DEVICE_OFFSET_WFS20) // WFS20 instrument
	{
		printf("\n\nChoose the resolution used for WFS20 camera:\n");
		for(cnt=0;cnt<10;cnt++){
			printf("%d  %dx%d\n",cnt,cam_wfs20_xpixel[cnt],cam_wfs20_ypixel[cnt]);
		}
		fflush(stdin);
		fgets(inputreader,3,stdin);
		selectedi = strtol(inputreader, NULL, 10);

		printf("\n\nConfigure WFS20 camera with resolution index %d (%d x %d pixels).\n", selectedi, cam_wfs20_xpixel[selectedi], cam_wfs20_ypixel[selectedi]);
	
		if(err = WFS_ConfigureCam (instr.handle, SAMPLE_PIXEL_FORMAT, selectedi, &instr.spots_x, &instr.spots_y))
			handle_errors(err);
	}
	
	if(instr.selected_id & DEVICE_OFFSET_WFS30) // WFS30 instrument
	{
		printf("\n\nChoose the resolution used for WFS30 camera (by default 1936x1216):\n");
		for(cnt=0;cnt<12;cnt++){
			printf("%d  %dx%d\n",cnt,cam_wfs30_xpixel[cnt],cam_wfs30_ypixel[cnt]);
		}
		fflush(stdin);
		fgets(inputreader,3,stdin);
		selectedi = strtol(inputreader, NULL, 10);

		printf("\n\nConfigure WFS30 camera with resolution index %d (%d x %d pixels).\n", selectedi, cam_wfs30_xpixel[selectedi], cam_wfs30_ypixel[selectedi]);
	
		if(err = WFS_ConfigureCam (instr.handle, SAMPLE_PIXEL_FORMAT, selectedi, &instr.spots_x, &instr.spots_y))
			handle_errors(err);
	}
	
	if(instr.selected_id & DEVICE_OFFSET_WFS40) // WFS40 instrument
	{
		printf("\n\nConfigure WFS40 camera with resolution index %d (%d x %d pixels).\n", SAMPLE_CAMERA_RESOL_WFS40, cam_wfs40_xpixel[SAMPLE_CAMERA_RESOL_WFS40], cam_wfs40_ypixel[SAMPLE_CAMERA_RESOL_WFS40]);
	
		if(err = WFS_ConfigureCam (instr.handle, SAMPLE_PIXEL_FORMAT, SAMPLE_CAMERA_RESOL_WFS40, &instr.spots_x, &instr.spots_y))
			handle_errors(err);
	}
	

	printf("Camera is configured to detect %d x %d lenslet spots.\n\n", instr.spots_x, instr.spots_y);

	printf("\nSet WFS to internal reference plane.\n");

	// set camera exposure time and gain if you don't want to use auto exposure
	// use functions WFS_GetExposureTimeRange, WFS_SetExposureTime, WFS_GetMasterGainRange, WFS_SetMasterGain
	
	// set WFS internal reference plane
	printf("\nSet WFS to internal reference plane.\n");
	if(err = WFS_SetReferencePlane (instr.handle, SAMPLE_REF_PLANE))
		handle_errors(err);
	
	
	// define pupil
	printf("\nDefine pupil to:\n");
	printf("Centroid_x in mm (by default 0.000): ");
	fflush(stdin);
	fgets(inputreader,6,stdin);
	if(strlen(inputreader)==1){
		pupilCen_x = 0;
	}else{
		pupilCen_x = strtod(inputreader, NULL);
	}
	printf("\nCentroid_y in mm (by default 0.000): ");
	fflush(stdin);
	fgets(inputreader,6,stdin);
	if(strlen(inputreader)==1){
		pupilCen_y = 0;
	}else{
		pupilCen_y = strtod(inputreader, NULL);
	}
	printf("\nDiameter_x in mm (by default 3.000): ");
	fflush(stdin);
	fgets(inputreader,6,stdin);
	if(strlen(inputreader)==1){
		pupilDia_x = 3;
	}else{
		pupilDia_x = strtod(inputreader, NULL);
	}
	printf("\nDiameter_y in mm (by default 3.000): ");
	fflush(stdin);
	fgets(inputreader,6,stdin);
	if(strlen(inputreader)==1){
		pupilDia_y = 3;
	}else{
		pupilDia_y = strtod(inputreader, NULL);
	}
	printf("Centroid_x = %6.3f\n", pupilCen_x);
	printf("Centroid_y = %6.3f\n", pupilCen_y);
	printf("Diameter_x = %6.3f\n", pupilDia_x);
	printf("Diameter_y = %6.3f\n", pupilDia_y);

	if(err = WFS_SetPupil (instr.handle, pupilCen_x, pupilCen_y, pupilDia_x, pupilDia_y))
		handle_errors(err);

	printf("Should the wavefront calculated based on the data limited in the pupil? (1 for Y and 0 for N) (by default 1):\n");
	fflush(stdin);
	fgets(inputreader,2,stdin);
	selectedi = strtod(inputreader,NULL);
	if(selectedi == 0){
		limited_to_pupil = 0;
	}else{
		limited_to_pupil=1;
	}


	printf("Set the highest Zernike Order for fitting; should be between 2 and 10; The fitting will be skipped for invalid values (by default 4):\n");
	fflush(stdin);
	fgets(inputreader,3,stdin);
	selectedi = strtod(inputreader,NULL);
	if(strlen(inputreader)==1){
		zernike_order = 4;
	}
	else if(selectedi < 11 && selectedi > 1){
		zernike_order = selectedi;
	}else{
		zernike_order=0;
	}

	printf("Set the highest Zernike Order for calculating Fourier constants; should be 2, 4, or 6 and no more than the highest Zernike order; The calculation will be skipped for invalid values (by default 4):\n");
	fflush(stdin);
	fgets(inputreader,2,stdin);
	selectedi = strtod(inputreader,NULL);
	if(strlen(inputreader)==1){
		fourier_order = 4;
	}
	else if(selectedi <= zernike_order && (selectedi == 2 || selectedi == 4 || selectedi == 6)){
		fourier_order = selectedi;
	}else{
		fourier_order=0;
	}

	printf("Set the path where the data is saved (by default at the dekstop):\n");
	fflush(stdin);
	fgets(inputreader,200,stdin);
	if(strlen(inputreader)==1){
		strcpy(path,"C:\\Users\\Public\\Desktop\\WFSdata.txt");
	}else if(inputreader[strlen(inputreader)-2]=='\\'){
		strcpy(path,inputreader);
		strcat(path,"WFSdata.txt");
	}else{
		strcpy(path,inputreader);
		strcat(path,"\\WFSdata.txt");
	}

	if(err = WFS_SetTriggerMode (instr.handle, 2)) // Set to active-high trigger mode
		handle_errors(err);	

	do{ // Looping until interrupted by disconnection or kill command

		// Wait for trigger
		cnt = 0;
		do{
			if(err = WFS_GetStatus (instr.handle, &instr.status)){
				handle_errors(err);
				break;
			}
			// if(instr.status & WFS_STATBIT_ATR){
			if(instr.status == 0x00000710){
				if(cnt==100){
					cnt=0;
					printf("Waiting for trigger\n");
				}
				Sleep(1);
				cnt ++;
			}else{
				printf("Status code: 0x%08X\n",instr.status);
				break;
			}
		}while(1);

		// close program if no well exposed image is feasible
		if( (instr.status & WFS_STATBIT_PTH) || (instr.status & WFS_STATBIT_PTL) ||(instr.status & WFS_STATBIT_HAL) )
		{
			printf("\nProgram will be closed because of unusable image quality, press <ENTER>.");
			WFS_close(instr.handle); // required to release allocated driver data
			fflush(stdin);
			getchar();
			exit(1);
		}

		
		// calculate all spot centroid positions using dynamic noise cut option
		if(err = WFS_CalcSpotsCentrDiaIntens (instr.handle, SAMPLE_OPTION_DYN_NOISE_CUT, SAMPLE_OPTION_CALC_SPOT_DIAS))
			handle_errors(err);

		// get centroid result arrays
		if(err = WFS_GetSpotCentroids (instr.handle, *centroid_x, *centroid_y))
			handle_errors(err);

		// get centroid and diameter of the optical beam, you may use this beam data to define a pupil variable in position and size
		// for WFS20: this is based on centroid intensties calculated by WFS_CalcSpotsCentrDiaIntens()
		if(err = WFS_CalcBeamCentroidDia (instr.handle, &beam_centroid_x, &beam_centroid_y, &beam_diameter_x, &beam_diameter_y))
			handle_errors(err);
		
		printf("\nInput beam is measured to:\n");
		printf("Centroid_x = %6.3f mm\n", beam_centroid_x);
		printf("Centroid_y = %6.3f mm\n", beam_centroid_y);
		printf("Diameter_x = %6.3f mm\n", beam_diameter_x);
		printf("Diameter_y = %6.3f mm\n", beam_diameter_y);

		// calculate spot deviations to internal reference
		if(err = WFS_CalcSpotToReferenceDeviations (instr.handle, SAMPLE_OPTION_CANCEL_TILT))
			handle_errors(err);
		
		// get spot deviations
		if(WFS_GetSpotDeviations (instr.handle, *deviation_x, *deviation_y))
			handle_errors(err);

		if(WFS_GetSpotIntensities (instr.handle, *intensity))
			handle_errors(err);
		
		// calculate and printout measured wavefront
		if(err = WFS_CalcWavefront (instr.handle, SAMPLE_WAVEFRONT_TYPE, limited_to_pupil, *wavefront))
			handle_errors(err);
		
		// calculate wavefront statistics within defined pupil
		if(err = WFS_CalcWavefrontStatistics (instr.handle, &wavefront_min, &wavefront_max, &wavefront_diff, &wavefront_mean, &wavefront_rms, &wavefront_weighted_rms))
			handle_errors(err);
		
		printf("\nWavefront Statistics in microns:\n");
		printf("Min          : %8.3f\n", wavefront_min);
		printf("Max          : %8.3f\n", wavefront_max);
		printf("Diff         : %8.3f\n", wavefront_diff);
		printf("Mean         : %8.3f\n", wavefront_mean);
		printf("RMS          : %8.3f\n", wavefront_rms);
		printf("Weigthed RMS : %8.3f\n", wavefront_weighted_rms);
		
		if(fourier_order){
			if(err = WFS_CalcFourierOptometric (instr.handle, zernike_order, fourier_order, &fourierM, &fourierJ0, &fourierJ45, &optoSphere, &optoCylinder, &optoAxisDeg))
				handle_errors(err);
		}
		
		if(zernike_order){
			// calculate Zernike coefficients
			printf("\nZernike fit up to order %d:\n",zernike_order);
			if(err = WFS_ZernikeLsf (instr.handle, &zernike_order, zernike_um, zernike_orders_rms_um, &roc_mm)){ // calculates also deviation from centroid data for wavefront integration
				// handle_errors(err);
				printf("Insufficient\n");}else{

			printf("\nZernike Mode    Coefficient\n");
			for(i=0; i < zernike_modes[zernike_order]; i++)
			{
				printf("  %2d         %9.3f\n",i, zernike_um[i]);
				zernike_reconstruct[i] = 1;
			}

			if(err = WFS_CalcReconstrDeviations (instr.handle, zernike_order, zernike_reconstruct, 0, &fitErrMean, &fitErrDev))
				handle_errors(err);
		}}

		if((fp = fopen (path, "a")) != NULL)
		{   
			fprintf(fp,"{");
			fprintf(fp, "'Beam Center X': %6.3f, ",beam_centroid_x);
			fprintf(fp, "'Beam Center Y': %6.3f, ",beam_centroid_y);
			fprintf(fp, "'Beam Diameter X': %6.3f, ",beam_diameter_x);
			fprintf(fp, "'Beam Diameter Y': %6.3f, ",beam_diameter_y);
			fprintf(fp, "'Wavefront Min': %8.3f, ",wavefront_min);
			fprintf(fp, "'Wavefront Max': %8.3f, ",wavefront_max);
			fprintf(fp, "'Wavefront Mean': %8.3f, ",wavefront_mean);
			fprintf(fp, "'Wavefront Peak-to-valley': %8.3f, ", wavefront_diff);
			fprintf(fp, "'Wavefront RMS': %8.3f, ", wavefront_rms);
			fprintf(fp, "'Wavefront Weigthed RMS': %8.3f, ", wavefront_weighted_rms);
			fprintf(fp, "'FourierM': %8.6f, ", fourierM);
			fprintf(fp, "'FourierJ0': %8.6f, ", fourierJ0);
			fprintf(fp, "'FourierJ45': %8.6f, ", fourierJ45);
			fprintf(fp, "'Optometric Sphere': %8.6f, ", optoSphere);
			fprintf(fp, "'Optometric Cylinder': %8.6f, ", optoCylinder);
			fprintf(fp, "'Optometric Axis Angle': %8.6f, ", optoAxisDeg);
			fprintf(fp, "'Radius of Curvature': %8.6f, ", roc_mm);
			fprintf(fp, "'Fit Error Mean': %8.3f, ", fitErrMean);
			fprintf(fp, "'Fit Error Std': %8.3f, ", fitErrDev);
			fprintf(fp, "'Zernike Amplitudes Array': [");
			for(i=0;i < zernike_modes[zernike_order]; i++)
				fprintf(fp, "%8.6f,", zernike_um[i]);
			fprintf(fp, "], 'Zernike RMS Array': [");
			for(i=0;i < zernike_order; i++)
				fprintf(fp, "%8.6f,", zernike_orders_rms_um[i]);
			fprintf(fp, "], 'Spot Deviation Array': [");
			for(i=0;i<instr.spots_x; i++){
				for(j=0;j<instr.spots_y; j++){
					fprintf(fp, "[%8.3f,%8.3f], ", deviation_x[i][j], deviation_y[i][j]);
				}
			}
			fprintf(fp, "], 'Spot Intensity Array': [");
			for(i=0;i<instr.spots_x; i++){
				for(j=0;j<instr.spots_y; j++){
					fprintf(fp, "%8.3f, ", intensity[i][j]);
				}
			}
			fprintf(fp, "]};\n");
			printf("data stored at %s\n",path);
		}else{printf("file writing error at %s\n",path);}
		fclose(fp);
	}while(1);
	// Close instrument, important to release allocated driver data!
	WFS_close(instr.handle);
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
	
	for(i=0;i<instr_cnt;i++)
	{
		if(err = WFS_GetInstrumentListInfo (VI_NULL, i, &device_id, &in_use, instr_name, serNr, resourceName))
			handle_errors(err);
		
		printf("%4d   %s    %s    %s\n", device_id, instr_name, serNr, (!in_use) ? "" : "(inUse)");
	}

	// Select instrument
	printf("\nSelect a Wavefront Sensor instrument: ");
	fflush(stdin);
	
	fgets (strg, WFS_BUFFER_SIZE, stdin);
	*selection = atoi(strg);

	// get selected resource name
	for(i=0;i<instr_cnt;i++)
	{   
		if(err = WFS_GetInstrumentListInfo (VI_NULL, i, &device_id, &in_use, instr_name, serNr, resourceName))
		   handle_errors(err);
		
		if(device_id == *selection)
			break; // resourceName fits to device_id
	}
	
	return *selection;
}


/*===============================================================================================================================
	Select MLA
===============================================================================================================================*/
int select_mla (int *selection)
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
	
	// Select MLA
	printf("\nSelect a Microlens Array: ");
	fflush(stdin);
	*selection = getchar() - '0';
	if(*selection < -1)
		*selection = -1; // nothing selected

	return *selection;
}


/*===============================================================================================================================
	End of source file
===============================================================================================================================*/
