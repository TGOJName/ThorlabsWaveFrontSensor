#####################################################################
#                                                                   #
# /labscript_devices/ThorlabsWaveFrontSensor/labscript_devices.py   #
#                                                                   #
# Copyright 2021, Philip Starkey; Edited by Oliver Tu               #
#                                                                   #
# This file is part of the module labscript_devices, in the         #
# labscript suite (see http://labscriptsuite.org), and is           #
# licensed under the Simplified BSD License. See the license.txt    #
# file in the root of the project for the full license.             #
#                                                                   #
#####################################################################


from labscript_devices import runviewer_parser

from labscript import config, set_passed_properties,TriggerableDevice
import labscript_utils.properties
import numpy as np

__author__ = ['Oliver Tu']

class ThorlabsWaveFrontSensor(TriggerableDevice):
    """
    This class is initilzed with the key word argument
    """
    description = 'ThorlabsWaveFrontSensor'
    allowed_children = []

    @set_passed_properties(
        property_names={
            'connection_table_properties': [
            ]
        }
    )
    def __init__(
        self,
        name,
        parent_device,
        **kwargs
    ):
        TriggerableDevice.__init__(self, name, parent_device, **kwargs)

    def expose(self, t, trigger_duration = 150e-6):
        """Request an exposure at the given time. A trigger will be produced by the
        parent trigger object, with duration trigger_duration, or if not specified, of
        self.trigger_duration. The frame should have a `name, and optionally a
        `frametype`, both strings. These determine where the image will be stored in the
        hdf5 file. `name` should be a description of the image being taken, such as
        "insitu_absorption" or "fluorescence" or similar. `frametype` is optional and is
        the type of frame being acquired, for imaging methods that involve multiple
        frames. For example an absorption image of atoms might have three frames:
        'probe', 'atoms' and 'background'. For this one might call expose three times
        with the same name, but three different frametypes.
        """
        if not trigger_duration > 0:
            msg = "trigger_duration must be > 0, not %s" % str(trigger_duration)
            raise ValueError(msg)
        self.trigger(t, trigger_duration)
        return trigger_duration

    def generate_code(self, hdf5_file):

        # dtypes = {'names':['motor1','motor2','motor3','motor4'],'formats':[np.uint32,np.uint32,np.uint32,np.uint32]}

        # out_table = np.zeros(1,dtype=dtypes)# create the table data for the hdf5 file.
        # out_table['motor1'].fill(self.target_position[0])
        # out_table['motor2'].fill(self.target_position[1])
        # out_table['motor3'].fill(self.target_position[2])
        # out_table['motor4'].fill(self.target_position[3])
        # grp = self.init_device_group(hdf5_file)
        # grp.create_dataset('DATA',compression=config.compression,data={}) 

        pass