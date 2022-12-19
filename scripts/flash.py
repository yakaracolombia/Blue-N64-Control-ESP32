import sys
import os

idf_path = os.environ['IDF_PATH']  # get value of IDF_PATH from environment
parttool_dir = os.path.join(idf_path, 'components', 'partition_table')  # parttool.py lives in $IDF_PATH/components/partition_table

sys.path.append(parttool_dir)  # this enables Python to find parttool module
from parttool import *  # import all names inside parttool module

print('Connecting to device...')
target = ParttoolTarget('COM6')
print('OK!')

print('Flashing from n64-switch...')
# Write to partition 'switch_mode' the contents from n64-switch
target.write_partition(PartitionName("switch_mode"), os.path.abspath(os.path.join('.', 'n64-switch', 'build', 'n64-control-switch.bin')))
print('OK!')

print('Flashing from n64-blueretro')
# Write to partition 'blueretro_mode' the contents from n64-blueretro
target.write_partition(PartitionName("blueretro_mode"), os.path.abspath(os.path.join('.', 'n64-blueretro', 'build', 'n64-control-blueretro.bin')))
print('OK!')

print('All ready!')