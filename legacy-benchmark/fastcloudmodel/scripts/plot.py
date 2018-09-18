#!/usr/bin/python
import sys
import numpy as np
from datetime import datetime
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def parse_time (tstr):
  return datetime.strptime(tstr.strip(), "%a %b %d %H:%M:%S %Z %Y").strftime('%s')

def parse_file (datfile):
  f = open(datfile,"r")
  ts = []
  cs = None
  rawdat = []
  for line in f:
    if line.startswith("[",0) and line.startswith("]",2):
      cs.append(int(line.split()[1][:-2]))
    else:
      if cs is not None:
        rawdat.append(cs)
      cs = []
      ts.append(long(parse_time(line)))
  while len(ts) > len(rawdat):
    ts = ts[:-1]
  f.close()
  return np.array(ts),np.array(rawdat)

def draw_fig(x,cs,fn):
  nseries = np.shape(cs)[-1];
  delta = 0xB0 / (nseries - 1)
  for i in range(nseries - 1):
    cstr = "#00%02x%02x" % ( (nseries - i - 1) * delta, (i + 1) * delta )
    plt.fill_between(range(len(x)),cs[:,i],cs[:,i+1],color=cstr)
  plt.plot(range(len(x)),cs[:,4],color="#ff0000",label="measured L3 cache size")
  plt.plot(range(len(x)),[20480]*len(x),'k:',label="hardware L3 cache size")
  plt.legend()
  plt.ylabel("L3 Cache Size (KB)")
  plt.xlabel("Time (min)")
  ax = plt.gca()
  ax.set_xlim((0,len(x)));
  plt.savefig(fn)

if __name__ == '__main__':
  if len(sys.argv) != 2:
    print "Usage: %s <data-file>" % sys.argv[0]
    exit(-1)
  ts,cs = parse_file(sys.argv[1])
  np.savetxt("ts.dat",ts,fmt='%d');
  np.savetxt("cs.dat",cs,fmt='%d');
  draw_fig(ts,cs,"cs.pdf");
