#!/usr/bin/python
import sys
import glob

def getdata(path,prefix):
  flst = sorted(glob.glob('%s/%s.*' % (path,prefix)))
  basetime = 0L
  for f in flst:
    fd = open(f)
    line = fd.readline()
    if len(line) == 0:
      continue
    t = long(fd.name.split('.')[-1])
    if basetime == 0L:
      basetime = t
    v = float(line.split()[0])
    print "%.1f %.2f" % (float(t-basetime)/3600,v)

if __name__ == '__main__':
  if len(sys.argv) == 3:
    getdata(path=sys.argv[1],prefix=sys.argv[2])
  else:
    print "USAGE: %s <folder> <prefix>" % sys.argv[0]
