#####################################################################
#                                                                   #
# /labscript_devices/ThorlabsWaveFrontSensor/blacs_worker.py        #
#                                                                   #
# Thanks to Nikita Vladimirov for his python driver of WFS as a     #
#   reference (https://github.com/nvladimus/WFS)                    #
#                                                                   #
# This file is part of labscript_devices                            #
#                                                                   #
#####################################################################
from asyncio.log import logger
import logging
from formatter import NullFormatter
import labscript_utils.h5_lock
import h5py
from blacs.tab_base_classes import Worker
import labscript_utils.properties as properties
import ctypes as ct
import numpy as np

class ThorlabsWaveFrontSensorWorker(Worker):
    def init(self):
        # logging.basicConfig(level=logging.DEBUG) # Use this line to debugging 
        global h5py; import labscript_utils.h5_lock, h5py 

        self.wfs = ct.WinDLL(r'C:\Program Files\IVI Foundation\VISA\Win64\Bin\WFS_64.dll')

        # Functions and ids declaration
        self.byref = ct.byref
        self.count = ct.c_int32() 
        self.deviceID  = ct.c_int32()  
        self.instrumentListIndex  = ct.c_int32() 
        self.inUse = ct.c_int32() 
        self.instrumentName = ct.create_string_buffer(20)
        self.instrumentSN = ct.create_string_buffer(20)
        self.resourceName = ct.create_string_buffer(30)
        self.IDQuery = ct.c_bool()
        self.resetDevice = ct.c_bool()
        self.instrumentHandle = ct.c_ulong() # This is where the device lives

        # Variable declarations
        self.pupilCenterXMm = ct.c_double()
        self.pupilCenterYMm = ct.c_double()
        self.pupilDiameterXMm = ct.c_double()
        self.pupilDiameterYMm = ct.c_double()
        self.exposureTimeAct = ct.c_double()
        self.masterGainAct = ct.c_double()
        self.dynamicNoiseCut = ct.c_int32() 
        self.calculateDiameters = ct.c_int32() 
        self.cancelWavefrontTilt = ct.c_int32() 
        self.triggerMode = ct.c_int32() 
        self.refInternal = ct.c_int32() 
        self.errorMessage = ct.create_string_buffer(512)
        self.errorCode = ct.c_int32()
        self.pixelFormat = ct.c_int32()
        self.pixelFormat.value = 0 #currently 8 bit only
        self.camResolIndex = ct.c_int32()
        self.spotsX = ct.c_int32()
        self.spotsY = ct.c_int32()
        self.wavefrontType = ct.c_int32() 
        self.limitToPupil = ct.c_int32() 
        self.beam_centroid_x = ct.c_double() 
        self.beam_centroid_y = ct.c_double() 
        self.beam_diameter_x = ct.c_double() 
        self.beam_diameter_y = ct.c_double() 
        self.deviation_x = ct.c_double() 
        self.deviation_y = ct.c_double() 
        self.wavefront_min = ct.c_double() 
        self.wavefront_max = ct.c_double() 
        self.wavefront_diff = ct.c_double() 
        self.wavefront_mean = ct.c_double() 
        self.wavefront_rms = ct.c_double() 
        self.wavefront_weighted_rms = ct.c_double() 
        self.zernikeOrder = ct.c_int32()
        self.fourierOrder = ct.c_int32()
        self.fourierM = ct.c_double()
        self.fourierJ0 = ct.c_double()
        self.fourierJ45 = ct.c_double()
        self.optoSphere = ct.c_double()
        self.optoCylinder = ct.c_double()
        self.optoAxisDeg = ct.c_double()
        self.radiusOfCurvature = ct.c_double()
        self.doSphericalReference = ct.c_int32()
        self.fitErrMean = ct.c_double()
        self.fitErrStdev = ct.c_double()

        # Parameter settings
        self.triggerMode.value = 2 # 0 for continuous mode, 1 for active low trigger, 2 for active high trigger, 3 for software control mode
        self.zernikeOrder.value = 4 # The highest order Zernike coefficient will be fitted; 
                            #   should be between 2 and 10
        self.ZernikeOrderCount = [0,0,6,10,15,21,28,36,45,55,66]
        self.arrayWavefront = np.zeros((35,47),dtype = np.float32)
        self.arrayZernikes = np.zeros(self.ZernikeOrderCount[self.zernikeOrder.value],dtype = np.float32)
        self.arrayZernikeRMS = np.zeros(self.zernikeOrder.value,dtype = np.float32)
        self.fourierOrder.value = 2 # Used for optometric calulations; should be chosen from 2, 4, 6 and no larger than zernikeOrder
        self.doSphericalReference.value = 0 # Not sure what it indicates so I put 0 here assuming it means no reference
        self.instrumentListIndex.value = self.sensorIndex # 0,1,2,, if multiple instruments connected
        self.camResolIndex.value = 1
        # For WFS20 instruments: 
        # Index  Resolution 
        # 0    1440x1080             
        # 1    1080x1080             
        # 2     768x768               
        # 3     512x512               
        # 4     360x360               
        # 5     720x540, bin2 
        # 6     540x540, bin2 
        # 7     384x384, bin2 
        # 8     256x256, bin2 
        # 9     180x180, bin2
        # For WFS30 instruments: 
        # Index  Resolution 
        # 0    1936x1216            
        # 1    1216x1216             
        # 2    1024x1024
        # 3     768x768               
        # 4     512x512               
        # 5     360x360
        # 6     968x608, sub2                
        # 7     608x608, sub2 
        # 8     512x512, sub2 
        # 9     384x384, sub2 
        # 10    256x256, sub2 
        # 11    180x180, sub2
        self.pupilCenterXMm.value = 0 #mm
        self.pupilCenterYMm.value = 0 #mm
        self.pupilDiameterXMm.value = 4.5 #mm
        self.pupilDiameterYMm.value = 4.5 #mm
        self.dynamicNoiseCut.value = 1
        self.calculateDiameters.value = 0
        self.cancelWavefrontTilt.value = 1
        self.refInternal.value = 1

        self.wavefrontType.value = 0
        # Valid settings for wavefrontType: 
        # 0   Measured Wavefront 
        # 1   Reconstructed Wavefront based on Zernike coefficients 
        # 2   Difference between measured and reconstructed Wavefront 
        # Note: Function WFS_CalcReconstrDeviations needs to be called prior to this function in case of Wavefront type 1 and 2.
        self.limitToPupil.value = 1
        # This parameter defines if the Wavefront should be calculated based on all detected spots or only within the defined pupil. 
        # Valid settings: 
        # 0   Calculate Wavefront for all spots 
        # 1   Limit Wavefront to pupil interior

        self.wfs.WFS_GetInstrumentListLen(None,self.byref(self.count))
        devStatus = self.wfs.WFS_GetInstrumentListInfo(None,self.instrumentListIndex, self.byref(self.deviceID), self.byref(self.inUse),
                                    self.instrumentName, self.instrumentSN, self.resourceName) # Should return 0 if succeeds
        if not self.inUse.value:
            devStatus = self.wfs.WFS_init(self.resourceName, self.IDQuery, self.resetDevice, self.byref(self.instrumentHandle))
            if(devStatus != 0):
                self.errorCode.value = devStatus
                self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
                print('error in WFS_init():' + str(self.errorMessage.value))
            else:
                print('WFS has been initialized. Instrument handle: ' +str(self.instrumentHandle.value))
        else:
            print('WFS already in use')



    def program_manual(self,front_panel_values):
        self.camResolIndex.value = int(front_panel_values['Resolution Index'])
        self.pupilCenterXMm.value = front_panel_values['Pupil Center X']
        self.pupilCenterYMm.value = front_panel_values['Pupil Center Y']
        self.pupilDiameterXMm.value = front_panel_values['Pupil Diameter X']
        self.pupilDiameterYMm.value = front_panel_values['Pupil Diameter Y']
        self.zernikeOrder.value = int(front_panel_values['Highest Zernike Order'])
        self.fourierOrder.value = int(front_panel_values['Fourier Order'])
        self.limitToPupil.value = int(front_panel_values['Limited to Pupil?'])

        devStatus = self.wfs.WFS_ConfigureCam(self.instrumentHandle, 
                                        self.pixelFormat, self.camResolIndex, self.byref(self.spotsX), self.byref(self.spotsY))
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_ConfigureCam():' + str(self.errorMessage.value))
        else:
            print('WFS camera configured')
            print('SpotsX:' + str(self.spotsX.value))
            print('SpotsY:' + str(self.spotsY.value))

        devStatus = self.wfs.WFS_SetTriggerMode(self.instrumentHandle, self.refInternal)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_SetReferencePlane():' + str(self.errorMessage.value))
        else:
            print('WFS internal reference plane set')

        devStatus = self.wfs.WFS_SetReferencePlane(self.instrumentHandle, self.refInternal)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_SetReferencePlane():' + str(self.errorMessage.value))
        else:
            print('WFS internal reference plane set')

        devStatus = self.wfs.WFS_SetPupil(self.instrumentHandle,
                                    self.pupilCenterXMm, self.pupilCenterYMm, self.pupilDiameterXMm, self.pupilDiameterYMm)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_SetPupil():' + str(self.errorMessage.value))
        else:
            print('WFS pupil set')

        return front_panel_values
    
    def data_write(self,path = r'C:\Users\Public\Desktop\WFS_data'):
        f=open("WFSdata.txt", "a")
        storedData = {
            'Beam Center X':self.beam_centroid_x.value,
            'Beam Center Y':self.beam_centroid_y.value,
            'Beam Diameter X':self.beam_diameter_x.value,
            'Beam Diameter Y':self.beam_diameter_y.value, 
            'Beam Deviation X':self.deviation_x.value,
            'Beam Deviation Y':self.deviation_y.value,
            'Wavefront Min':self.wavefront_min.value, 
            'Wavefront Max':self.wavefront_max.value, 
            'Wavefront Peak-Valley':self.wavefront_diff.value,
            'Wavefront Mean':self.wavefront_mean.value, 
            'Wavefront RMS':self.wavefront_rms.value, 
            'Wavefront Weighted RMS':self.wavefront_weighted_rms.value,
            'Fourier M':self.fourierM.value,
            'Fourier J0':self.fourierJ0.value,
            'Fourier J45':self.fourierJ45.value,
            'Optometric Sphere':self.optoSphere.value,
            'Optometric Cylinder':self.optoCylinder.value,
            'Optometric Axis Angle':self.optoAxisDeg.value,
            'Radius of Curvature':self.radiusOfCurvature.value,
            'Fit Error Mean':self.fitErrMean.value,
            'Fit Error Std':self.fitErrStdev.value,
            'Zernikes Coefficients':self.arrayZernikes,
            'Zernikes RMS':self.arrayZernikeRMS
        }
        f.write(str(storedData),'\r')
        f.close()
        

    def transition_to_buffered(self,device_name,h5file,initial_values,fresh):
        return initial_values
    
    def abort_transition_to_buffered(self):
        return self.transition_to_manual(True)
        
    def abort_buffered(self):
        return self.transition_to_manual(True)
    
    def transition_to_manual(self,abort = False):
        devStatus = self.wfs.WFS_CalcBeamCentroidDia(self.instrumentHandle, self.beam_centroid_x, self.beam_centroid_y, self.beam_diameter_x, self.beam_diameter_y)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_CalcBeamCentroidDia():' + str(self.errorMessage.value))
        else:
            print('WFS beam centeroid and diameter calculated')

        devStatus = self.wfs.WFS_CalcSpotToReferenceDeviations(self.instrumentHandle, self.cancelWavefrontTilt)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_CalcSpotToReferenceDeviations():' + str(self.errorMessage.value))
        else:
            print('WFS spot to ref deviations calculated')

        devStatus = self.wfs.WFS_GetSpotDeviations(self.instrumentHandle, self.deviation_x,self.deviation_y)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_GetSpotDeviations():' + str(self.errorMessage.value))
        else:
            print('WFS spot to ref deviations got')

        devStatus = self.wfs.WFS_CalcWavefront(self.instrumentHandle, 
                                        self.wavefrontType, self.limitToPupil, self.arrayWavefront.ctypes.data)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_CalcWavefront():' + str(self.errorMessage.value))
        else:
            print('WFS wavefront calculated')

        devStatus = self.wfs.WFS_CalcWavefrontStatistics(self.instrumentHandle, self.wavefront_min, self.wavefront_max, 
                            self.wavefront_diff, self.wavefront_mean, self.wavefront_rms, self.wavefront_weighted_rms)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_CalcWavefrontStatistics():' + str(self.errorMessage.value))
        else:
            print('WFS wavefront stats calculated')

        devStatus = self.wfs.WFS_CalcFourierOptometric (self.instrumentHandle, self.zernikeOrder, self.fourierOrder, self.fourierM, self.fourierJ0,
                                                    self.fourierJ45, self.optoSphere, self.optoCylinder, self.optoAxisDeg)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in CalcFourierOptometric():' + str(self.errorMessage.value))
        else:
            print('WFS Fourier optometrics calculated')

        devStatus = self.wfs.WFS_ZernikeLsf(self.instrumentHandle, self.zernikeOrder, self.arrayZernikes.ctypes.data, 
                            self.arrayZernikeRMS, self.radiusOfCurvature)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_ZernikeLsf():' + str(self.errorMessage.value))
        else:
            print('WFS Zernike coefficients calculated')

        devStatus = self.wfs.WFS_CalcReconstrDeviations(self.instrumentHandle, self.zernikeOrder, self.arrayZernikes.ctypes.data,
                                                    self.doSphericalReference, self.fitErrMean, self.fitErrStdev)
        if(devStatus != 0):
            self.errorCode.value = devStatus
            self.wfs.WFS_error_message(self.instrumentHandle,self.errorCode,self.errorMessage)
            print('error in WFS_ZernikeLsf():' + str(self.errorMessage.value))
        else:
            print('WFS Zernike coefficients calculated')
        
        self.data_write()
        return True

    def shutdown(self):
        self.wfs.WFS_close(self.instrumentHandle)
        pass
