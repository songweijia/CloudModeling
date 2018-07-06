#!/usr/bin/python
from scipy.stats import gaussian_kde
from scipy.signal import argrelextrema
import numpy as np
import sys
import matplotlib.pyplot as plt

if __name__ == "__main__":
  if len(sys.argv) < 3:
    print "%s <data file> <cache level> [bin=0.15]" % sys.argv[0]
    exit(0)
  dat = np.loadtxt(sys.argv[1])
  cl = int(sys.argv[2])
  binsize = 0.15
  if len(sys.argv) >= 4:
    binsize = float(sys.argv[3])
  kernel = gaussian_kde(dat,bw_method=binsize)
  x = np.arange(0,max(dat)+1,0.01)
  density = kernel(x)
  plt.plot(x,density)
  plt.show()
  maxima_x, = argrelextrema(density, np.greater)
  output = [x[i] for i in sorted(maxima_x, key=lambda(x):density[x],reverse=True)[0:(cl+1)] ]
  for thp in sorted(output,reverse=True):
    print "%.3f" % thp
