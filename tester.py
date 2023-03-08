import ctypes as ct
import numpy as np
wfs = ct.windll.WFS_64

# Functions and ids declaration
byref = ct.byref
count = ct.c_int32() 
deviceID  = ct.c_int32()  
instrumentListIndex  = ct.c_int32() 
inUse = ct.c_int32() 
instrumentName = ct.create_string_buffer("", 20)
instrumentSN = ct.create_string_buffer("", 20)
resourceName = ct.create_string_buffer("", 30)
IDQuery = ct.c_bool()
resetDevice = ct.c_bool()
instrumentHandle = ct.c_ulong() # This is where the device lives

# Variable declarations
pupilCenterXMm = ct.c_double()
pupilCenterYMm = ct.c_double()
pupilDiameterXMm = ct.c_double()
pupilDiameterYMm = ct.c_double()
exposureTimeAct = ct.c_double()
masterGainAct = ct.c_double()
dynamicNoiseCut = ct.c_int32() 
calculateDiameters = ct.c_int32() 
cancelWavefrontTilt = ct.c_int32() 
triggerMode = ct.c_int32() 
refInternal = ct.c_int32() 
errorMessage = ct.create_string_buffer("", 512)
errorCode = ct.c_int32()
pixelFormat = ct.c_int32()
pixelFormat.value = 0 #currently 8 bit only
camResolIndex = ct.c_int32()
spotsX = ct.c_int32()
spotsY = ct.c_int32()
wavefrontType = ct.c_int32() 
limitToPupil = ct.c_int32() 
beam_centroid_x = ct.c_double() 
beam_centroid_y = ct.c_double() 
beam_diameter_x = ct.c_double() 
beam_diameter_y = ct.c_double() 
deviation_x = ct.c_double() 
deviation_y = ct.c_double() 
wavefront_min = ct.c_double() 
wavefront_max = ct.c_double() 
wavefront_diff = ct.c_double() 
wavefront_mean = ct.c_double() 
wavefront_rms = ct.c_double() 
wavefront_weighted_rms = ct.c_double() 
zernikeOrder = ct.c_int32()
fourierOrder = ct.c_int32()
fourierM = ct.c_double()
fourierJ0 = ct.c_double()
fourierJ45 = ct.c_double()
optoSphere = ct.c_double()
optoCylinder = ct.c_double()
optoAxisDeg = ct.c_double()
radiusOfCurvature = ct.c_double()
doSphericalReference = ct.c_int32()
fitErrMean = ct.c_double()
fitErrStdev = ct.c_double()

# Parameter settings
triggerMode.value = 2 # 0 for continuous mode, 1 for active low trigger, 2 for active high trigger, 3 for software control mode
zernikeOrder.value = 4 # The highest order Zernike coefficient will be fitted; 
                        #   should be between 2 and 10
ZernikeOrderCount = [0,0,6,10,15,21,28,36,45,55,66]
arrayWavefront = np.zeros((35,47),dtype = np.float32)
arrayZernikes = np.zeros(ZernikeOrderCount[zernikeOrder.value],dtype = np.float32)
arrayZernikeRMS = np.zeros(zernikeOrder.value,dtype = np.float32)
fourierOrder.value = 2 # Used for optometric calulations; should be chosen from 2, 4, 6 and no larger than zernikeOrder
doSphericalReference.value = 0 # Not sure what it indicates so I put 0 here assuming it means no reference
instrumentListIndex.value = 0 # 0,1,2,, if multiple instruments connected
camResolIndex.value = 1
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
pupilCenterXMm.value = 0 #mm
pupilCenterYMm.value = 0 #mm
pupilDiameterXMm.value = 4.5 #mm
pupilDiameterYMm.value = 4.5 #mm
dynamicNoiseCut.value = 1
calculateDiameters.value = 0
cancelWavefrontTilt.value = 1
refInternal.value = 1


wavefrontType.value = 0
# Valid settings for wavefrontType: 
# 0   Measured Wavefront 
# 1   Reconstructed Wavefront based on Zernike coefficients 
# 2   Difference between measured and reconstructed Wavefront 
# Note: Function WFS_CalcReconstrDeviations needs to be called prior to this function in case of Wavefront type 1 and 2.
limitToPupil.value = 1
# This parameter defines if the Wavefront should be calculated based on all detected spots or only within the defined pupil. 
# Valid settings: 
# 0   Calculate Wavefront for all spots 
# 1   Limit Wavefront to pupil interior

wfs.WFS_GetInstrumentListLen(None,byref(count))
devStatus = wfs.WFS_GetInstrumentListInfo(None,instrumentListIndex, byref(deviceID), byref(inUse),
                             instrumentName, instrumentSN, resourceName) # Should return 0 if succeeds

wfs.WFS_GetInstrumentListInfo(None,instrumentListIndex, byref(deviceID), byref(inUse),
                             instrumentName, instrumentSN, resourceName)
if not inUse.value:
    devStatus = wfs.WFS_init(resourceName, IDQuery, resetDevice, byref(instrumentHandle))
    if(devStatus != 0):
        errorCode.value = devStatus
        wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
        print('error in WFS_init():' + str(errorMessage.value))
    else:
        print('WFS has been initialized. Instrument handle: ' +str(instrumentHandle.value))
else:
    print('WFS already in use')

devStatus = wfs.WFS_ConfigureCam(instrumentHandle, 
                                 pixelFormat, camResolIndex, byref(spotsX), byref(spotsY))
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_ConfigureCam():' + str(errorMessage.value))
else:
    print('WFS camera configured')
    print('SpotsX:' + str(spotsX.value))
    print('SpotsY:' + str(spotsY.value))

devStatus = wfs.WFS_SetTriggerMode(instrumentHandle, refInternal)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_SetReferencePlane():' + str(errorMessage.value))
else:
    print('WFS internal reference plane set')

devStatus = wfs.WFS_SetReferencePlane(instrumentHandle, refInternal)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_SetReferencePlane():' + str(errorMessage.value))
else:
    print('WFS internal reference plane set')


devStatus = wfs.WFS_SetPupil(instrumentHandle,
                             pupilCenterXMm, pupilCenterYMm, pupilDiameterXMm, pupilDiameterYMm)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_SetPupil():' + str(errorMessage.value))
else:
    print('WFS pupil set')

devStatus = wfs.WFS_TakeSpotfieldImageAutoExpos(instrumentHandle,
                                                byref(exposureTimeAct), byref(masterGainAct))
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_TakeSpotfieldImageAutoExpos():' + str(errorMessage.value))
else:
    print('Took spotfield image, auto exposure')
    print('exposureTimeAct, ms: ' + str(exposureTimeAct.value))
    print('masterGainAct: ' + str(masterGainAct.value))


# Following for spot field mode
# devStatus = wfs.WFS_CalcSpotsCentrDiaIntens(instrumentHandle, dynamicNoiseCut, calculateDiameters)
# if(devStatus != 0):
#     errorCode.value = devStatus
#     wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
#     print('error in WFS_CalcSpotsCentrDiaIntens():' + str(errorMessage.value))
# else:
#     print('WFS spot centroids calculated')

devStatus = wfs.WFS_CalcBeamCentroidDia(instrumentHandle, beam_centroid_x, beam_centroid_y, beam_diameter_x, beam_diameter_y)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_CalcBeamCentroidDia():' + str(errorMessage.value))
else:
    print('WFS beam centeroid and diameter calculated')

devStatus = wfs.WFS_CalcSpotToReferenceDeviations(instrumentHandle, cancelWavefrontTilt)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_CalcSpotToReferenceDeviations():' + str(errorMessage.value))
else:
    print('WFS spot to ref deviations calculated')

devStatus = wfs.WFS_GetSpotDeviations(instrumentHandle, deviation_x,deviation_y)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_GetSpotDeviations():' + str(errorMessage.value))
else:
    print('WFS spot to ref deviations got')

devStatus = wfs.WFS_CalcWavefront(instrumentHandle, 
                                  wavefrontType, limitToPupil, arrayWavefront.ctypes.data)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_CalcWavefront():' + str(errorMessage.value))
else:
    print('WFS wavefront calculated')

devStatus = wfs.WFS_CalcWavefrontStatistics(instrumentHandle, wavefront_min, wavefront_max, 
                    wavefront_diff, wavefront_mean, wavefront_rms, wavefront_weighted_rms)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_CalcWavefrontStatistics():' + str(errorMessage.value))
else:
    print('WFS wavefront stats calculated')

devStatus = wfs.WFS_CalcFourierOptometric (instrumentHandle, zernikeOrder, fourierOrder, fourierM, fourierJ0,
                                             fourierJ45, optoSphere, optoCylinder, optoAxisDeg)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in CalcFourierOptometric():' + str(errorMessage.value))
else:
    print('WFS Fourier optometrics calculated')

devStatus = wfs.WFS_ZernikeLsf(instrumentHandle, zernikeOrder, arrayZernikes.ctypes.data, 
                    arrayZernikeRMS, radiusOfCurvature)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_ZernikeLsf():' + str(errorMessage.value))
else:
    print('WFS Zernike coefficients calculated')

devStatus = wfs.WFS_CalcReconstrDeviations(instrumentHandle, zernikeOrder, arrayZernikes.ctypes.data,
                                              doSphericalReference, fitErrMean, fitErrStdev)
if(devStatus != 0):
    errorCode.value = devStatus
    wfs.WFS_error_message(instrumentHandle,errorCode,errorMessage)
    print('error in WFS_ZernikeLsf():' + str(errorMessage.value))
else:
    print('WFS Zernike coefficients calculated')

wfs.WFS_close(instrumentHandle)