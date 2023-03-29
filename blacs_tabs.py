#####################################################################
#                                                                   #
# /labscript_devices/ThorlabsWaveFrontSensor/blacs_tab.py           #
#                                                                   #
# Copyright 2021, Philip Starkey; edited by Oliver Tu               #
#                                                                   #
# This file is part of labscript_devices, in the labscript suite    #
# (see http://labscriptsuite.org), and is licensed under the        #
# Simplified BSD License. See the license.txt file in the root of   #
# the project for the full license.                                 #
#                                                                   #
#####################################################################
from blacs.device_base_class import (
    DeviceTab,
    define_state,
    MODE_BUFFERED,
    MODE_MANUAL,
    MODE_TRANSITION_TO_BUFFERED,
    MODE_TRANSITION_TO_MANUAL,
)
import labscript_utils.properties
from qtutils.qt import QtWidgets

class ThorlabsWaveFrontSensorTab(DeviceTab):
    def initialise_GUI(self):        
        connection_object = self.settings['connection_table'].find_by_name(self.device_name)
        connection_table_properties = connection_object.properties

        wfsProp = {}
        wfsProp['Resolution Index'] = {
            'base_unit': '',
            'min': 0,
            'max': 11,
            'step': 1,
            'decimals': 0,
        }
        wfsProp['Pupil Center X'] = { # TODO: Determine a reasonable position range
            'base_unit': 'mm',
            'min': -4,
            'max': 4,
            'step': 0.001,
            'decimals': 3,
        }
        wfsProp['Pupil Center Y'] = {
            'base_unit': 'mm',
            'min': -4,
            'max': 4,
            'step': 0.001,
            'decimals': 3,
        }
        wfsProp['Pupil Diameter X'] = {
            'base_unit': 'mm',
            'min': 0,
            'max': 4,
            'step': 0.001,
            'decimals': 3,
        }
        wfsProp['Pupil Diameter Y'] = {
            'base_unit': 'mm',
            'min': 0,
            'max': 4,
            'step': 0.001,
            'decimals': 3,
        }
        wfsProp['Limited to Pupil?'] = {
            'base_unit': 'Bool',
            'min': 0,
            'max': 1,
            'step': 1,
            'decimals': 0,
        }
        wfsProp['Highest Zernike Order'] = {
            'base_unit': '',
            'min': 2,
            'max': 10,
            'step': 1,
            'decimals': 0,
        }
        wfsProp['Fourier Order'] = {
            'base_unit': '',
            'min': 2,
            'max': 6,
            'step': 2,
            'decimals': 0,
        }
        
        # Create DDS Output objects: we only need frequency for the carrier and frequency and phase for the mod
        self.create_analog_outputs(wfsProp)

        # Create widgets for outputs defined so far (i.e. analog outputs only)
        _, AO_widgets, _ = self.auto_create_widgets()  
        widget_list = [("Analog outputs", AO_widgets)]
        self.auto_place_widgets(*widget_list)
        # Connect signals for buttons
        # Add icons
        
        self.serialNum = connection_table_properties.get('serialNum', None)
        self.orientation = connection_table_properties.get('orientation', None)

        # Create and set the primary worker
        self.create_worker("main_worker",
                            'labscript_devices.ThorlabsWaveFrontSensor.blacs_workers.ThorlabsWaveFrontSensorWorker',
                            {'serialNum':self.serialNum,
                             'orientation': self.orientation})
        
        self.primary_worker = "main_worker"

        # Set the capabilities of this device
        self.supports_remote_value_check(False)
        self.supports_smart_programming(False) 

