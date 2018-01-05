'''
USAGE: First start up the debug server in MATLAB. Please see debug_server.m for instructions
Then call `request_var` and pass in the name of the variable you want to auto-magically
transfer from MATLAB to Python. This is intended to be called with both Python and MATLAB
at a breakpoint to easily compare the values of variables for equality.
'''
import scipy.io
import os
import time
import traceback
import numpy as np

# MATLAB_DUMP_DIR = "/Users/ys/work/freesurfer/matlab_dumps"
# MATLAB_DUMP_DIR = '/home/willy/work/cm/my_tests/matlab_dump_dir'
MATLAB_DUMP_DIR = os.getenv('MATLAB_DUMP_DIR')


def request_var(varname):
    mat_filename = os.path.join(MATLAB_DUMP_DIR, varname + '.mat')
    if os.path.exists(mat_filename):
        os.remove(mat_filename)
    request_file = os.path.join(MATLAB_DUMP_DIR, varname + '.request')
    open(request_file, 'w')
    for i in range(200):
        if not os.path.exists(request_file):
            var = scipy.io.loadmat(mat_filename, struct_as_record=False, squeeze_me=True)[varname]
            os.remove(mat_filename)
            return var
        else:
            time.sleep(0.1)
    else:
        raise Exception("Timed out waiting for matlab. Are you at a breakpoint and have the debug server running?")


def compare_vars(varname):
    '''
    Assumes Python is at a breakpoint with a REPL shell. Also assumes MATLAB is at a breakpoint with debug_server
    running. This function returns values for both variable names so they can be compared.
    '''
    import inspect
    frame = inspect.currentframe()
    try:
        return {'python': frame.f_back.f_locals[varname],
                'matlab': request_var(varname)}
    finally:
        del frame

def measure_closeness(ref, item, name=''):

    def show(message):
        print("{0}: {1}".format(name, message))
    if not hasattr(ref, 'shape'):
        show("ref has no shape")
        return
    ref_shape = ref.shape
    if not hasattr(item, 'shape'):
        show('ref.shape={0} but item has no shape'.format(ref_shape))
        return
    item_shape = item.shape
    if item.shape != ref.shape:
        show('shapes differ ref={0} item={1}'.format(ref_shape, item_shape))
        return
    try:
        total_size = np.prod(ref_shape)
        ref_max = np.max(ref)
        ref_min = np.min(ref)
        item_max = np.max(item)
        item_min = np.min(item)
        difference = np.double(item) - np.double(ref)
        max_difference = np.max(difference)
        max_location = np.argmax(difference)
        min_difference = np.min(difference)
        min_location = np.argmin(difference)
        total_difference = np.sum(np.abs(difference))
        average_difference = total_difference / total_size
        ref_total = np.sum(np.abs(ref))
        relative_difference = total_difference / ref_total
        show('ref_max={0} ref_min={1} item_max={2} item_min={3}'.format(ref_max, ref_min, item_max, item_min))
        show('max_diff={0} min_diff={1}'.format(max_difference, min_difference))
        show('max_location={0} min_location={1}'.format(max_location, min_location))
        show('average difference={0} relative_difference={1}'.format(average_difference, relative_difference))
    except Exception as flaw:
        show('flaw = {0}'.format(str(flaw)))
        traceback.print_exc()


class CheckpointManager:
    def __init__(self, matlab_dump_dir=None, python_dump_dir=None):
        if matlab_dump_dir is None:
            matlab_dump_dir = MATLAB_DUMP_DIR
        if python_dump_dir is None:
            python_dump_dir = matlab_dump_dir
        self.matlab_dump_dir = matlab_dump_dir
        self.python_dump_dir = python_dump_dir
        self.counts = {}

    def increment(self, checkpoint_name):
        self.counts.setdefault(checkpoint_name, 0)
        self.counts[checkpoint_name] += 1

    def file_name_for_checkpoint(self, dump_dir, checkpoint_name, checkpoint_number=None):
        if checkpoint_number is None and checkpoint_name not in self.counts:
            raise Exception('You must either specify the checkpoint number or call '
                            'increment in the same logical location as in the MATLAB code.')
        checkpoint_number = checkpoint_number or self.counts[checkpoint_name]
        return os.path.join(dump_dir, '{}_{}.mat'.format(checkpoint_name, checkpoint_number))

    def load(self, checkpoint_name, checkpoint_number=None):
        mat_path = self.file_name_for_checkpoint(self.matlab_dump_dir, checkpoint_name, checkpoint_number)
        return scipy.io.loadmat(mat_path, struct_as_record=False, squeeze_me=True)

    def save(self, value_dict, checkpoint_name, checkpoint_number=None):
        mat_path = self.file_name_for_checkpoint(self.python_dump_dir, checkpoint_name, checkpoint_number)
        return scipy.io.savemat(mat_path, value_dict, long_field_names=True)


