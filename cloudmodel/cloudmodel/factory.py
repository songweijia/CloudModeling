#!/usr/bin/python

from pack import Pack,PackConfig
from cpumem import CPUMemPack

class PackFactory:
  """
  Pack is the abstract class for all cloud model packs, including CPUMemPack (CPU/Memory performance measurement), NetPack (Network performance measurement), IOPack (disk/filesystem io performance measurement), etc.
  Factory class
  """
  def __init__(self):
    self._packs = { \
      'cpumem': CPUMemPack, \
      'net': None, \
      'io' : None, \
    }

  def create_pack(self, conf):
    return self._packs[conf._pack_name](conf)

if __name__=="__main__":
  pf = PackFactory()
  print pf._packs;
  conf = PackConfig('cpumem',None,None);
  p = pf.createPack(conf);
  p.doRun();
