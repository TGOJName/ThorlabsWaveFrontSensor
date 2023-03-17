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

        wfsProp = {}
        # Create DDS Output objects: we only need frequency for the carrier and frequency and phase for the mod
        self.create_analog_outputs(wfsProp)

        # Create widgets for outputs defined so far (i.e. analog outputs only)
        _, AO_widgets, _ = self.auto_create_widgets()  
        widget_list = [("Make sure the WFS_receiver.exe is opened", AO_widgets)]
        self.auto_place_widgets(*widget_list)
        # Connect signals for buttons
        # Add icons

        # Create and set the primary worker
        self.create_worker("main_worker",
                            'labscript_devices.ThorlabsWaveFrontSensor.blacs_workers.ThorlabsWaveFrontSensorWorker',)
        
        self.primary_worker = "main_worker"

        # Set the capabilities of this device
        self.supports_remote_value_check(False)
        self.supports_smart_programming(False) 

