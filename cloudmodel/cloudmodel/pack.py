#!/usr/bin/python
class PackConfig:
  """
  PackConfig is the configuration of a pack
  """
  def __init__(self,pack_name,host_creds,work_path,remote_path):
    """
    - pack_name tells the engine which pack to use.
    - host_creds is a list of hosts with their credentials:
    [ {'hostname':"192.168.10.1", 'alias':'azure-ds1', 'username':'username','password':None,'key_filename':'/path/to/key/file'}, ... ]
    - work_path the input and output files are stored in the work_path. the private key file must put here.
    - remote_path is the working space on the remote host
    """
    self._pack_name = pack_name
    self._host_creds = host_creds
    self._work_path = work_path
    self._remote_path = remote_path

class Pack:
  """
  Pack is the abstract class for all cloud model packs, including CPUMemPack (CPU/Memory performance measurement), NetPack (Network performance measurement), IOPack (disk/filesystem io performance measurement), etc.
  """
  def run(self):
    raise NameError('%s Not Implemented.' % self.run.__name__)

  def visualize(self,tstart,tend):
    """
    tstart - start time point by seconds since epoch
    tend - end time point by seconds since epoch
    """
    raise NameError('%s Not Implemented.' % self.visualize.__name__)
