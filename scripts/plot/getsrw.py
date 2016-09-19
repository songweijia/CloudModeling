#!/usr/bin/python
import sys
import glob

def getdata(path,prefix):
  flst = sorted(glob.glob('%s/%s.*' % (path,prefix)))
  basetime = 0L
  for f in flst:
    fd = open(f,'r')
    ct = long(fd.name.split('.')[-1])
    if basetime == 0L:
      basetime = ct
    T = float(ct-basetime)/3600
    ofd = open("%s.%.1f" % (prefix,T), 'w')
    for line in fd:
      dat = line.split()
      bfsz = int(dat[0])
      thp = float(dat[1])
      if thp < 0.001:
        lat = 8000
      else:
        lat = 8.0 / thp
      ofd.write("%.1f %d %.2f\n" % (T,bfsz,thp))
    ofd.close()
  fd.close()
    

if __name__ == '__main__':
  if len(sys.argv) == 3:
    getdata(path=sys.argv[1],prefix=sys.argv[2])
  else:
    print "USAGE: %s <folder> <prefix>" % sys.argv[0]
