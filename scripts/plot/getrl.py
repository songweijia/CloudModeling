#!/usr/bin/python
import sys
import glob

def dumpbins(bins,bfsz,fn):
  ofd = open(fn, 'a')
  cdf = 0.0
  for k in sorted(bins.keys()):
    pdf = float(bins[k])/100000;
    cdf = cdf + pdf;
    ofd.write("%d %f %f %f\n" % (bfsz,0.3*float(k),pdf,cdf));
  # an empty line for gnuplot
  ofd.write("\n")
  ofd.close()

def dumpbins_percentile(bins,bfsz,fn):
  print "bfsz %d \n" % bfsz
  ofd = open(fn, 'a')
  cdf = 0.0
  p2l = {}
  keys = [0.05,0.5,0.95]
  idx = 0
  for k in sorted(bins.keys()):
    pdf = float(bins[k])/100000;
    cdf = cdf + pdf;
    if cdf >= keys[idx] and keys[idx] not in p2l.keys():
      p2l[keys[idx]] = 0.3*float(k)
      idx = idx + 1
      if idx == 3:
        break
  ofd.write("%d" % bfsz)
  for p in keys:
    ofd.write(" %.1f" % p2l[p])
  ofd.write("\n")
  ofd.close()

def getdata(path,prefix):
  flst = sorted(glob.glob('%s/%s.*' % (path,prefix)))
  basetime = 0L
  for f in flst:
    fd = open(f,'r')
    ct = long(fd.name.split('.')[-1])
    if basetime == 0L:
      basetime = ct
    T = float(ct-basetime)/3600
    bfsz = 0
    bins = {}
    for line in fd:
      dat = line.split()
      cur_bfsz = int(dat[0])
      lat = float(dat[2])
      
      if bfsz != cur_bfsz:
        # output
        if bfsz != 0:
          # dumpbins(bins,bfsz,"rl.%.1f.dat" % T)
          dumpbins_percentile(bins,bfsz,"rlp.%.1f.dat" % T)
        # reset bins
        bfsz = cur_bfsz
        i = 0
        while i <= 3333:
          bins[i] = 0
          i = i + 1 # bin size is about one CPU cycle. 0.3ns
      else:
        # here we just through the points with latency higher than 1000ns...which is seldom.
        latbin = round(lat/0.3)
        if latbin in bins.keys():
          bins[latbin] = bins[latbin] + 1
    if len(bins) > 0:
      # dumpbins(bins,bfsz,"rl.%.1f.dat" % T)
      dumpbins_percentile(bins,bfsz,"rlp.%.1f.dat" % T)
  fd.close()
    

if __name__ == '__main__':
  if len(sys.argv) == 3:
    getdata(path=sys.argv[1],prefix=sys.argv[2])
  else:
    print "USAGE: %s <folder> <prefix>" % sys.argv[0]
