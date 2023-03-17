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
import labscript_utils.h5_lock
import h5py
from blacs.tab_base_classes import Worker
import labscript_utils.properties as properties

class ThorlabsWaveFrontSensorWorker(Worker):
    def init(self):
        # All functions are transferred to the WFS_receiver.exe (the C version of the code) due to memory management bugs.
        pass

    def program_manual(self,front_panel_values):
        return front_panel_values
        
    def transition_to_buffered(self,device_name,h5file,initial_values,fresh):
        return initial_values
    
    def abort_transition_to_buffered(self):
        return self.transition_to_manual(True)
        
    def abort_buffered(self):
        return self.transition_to_manual(True)
    
    def transition_to_manual(self,abort = False):
        return True

    def shutdown(self):
        pass
