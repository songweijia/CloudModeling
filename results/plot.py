#!/usr/bin/python

import sys
import numpy as np
import matplotlib.pyplot as plt
import imp

palette = imp.load_source('palette','palette.py')
intro_data="intro_exp.dat"
inacc_data="inaccuracy.dat"
sensi_data="sensitivity.dat"
apps = ['kmeans','SGD','SVD','pagerank','canopy','PCA','ALS','RF','naive bayes','logistic regression']

def get_intro_data():
  f = open(intro_data, "r")
  rawdata = []
  for line in f:
    tks = line.strip().split();
    rawdata.append([float(tks[-3]),float(tks[-2]),float(tks[-1])])
  f.close()
  return np.array(rawdata)

def get_accuracy_data():
  return np.loadtxt(inacc_data)

def draw_intro():
  ind = np.arange(len(apps))
  width = 0.25
  dat = get_intro_data()
  inacc_dat = get_accuracy_data()
  # y1
  unlimit = plt.bar(ind-width,dat[:,0],width,label="unlimited cache",color=palette.palette[0])
  limit   = plt.bar(ind      ,dat[:,1],width,label="limited cache",color=palette.palette[1],hatch='/')
  limitcm = plt.bar(ind+width,dat[:,2],width,label="limited cache with CloudModel",color=palette.palette[2],hatch='x')
  ax1 = plt.gca()
  ax1.set_xticks(ind)
  ax1.set_xticklabels(apps)
  plt.xticks(rotation=45)
  # y2
  ax2 = ax1.twinx()
  inacc = ax2.plot(ind+width*1.5,inacc_dat,ls='none',label='quality loss',marker='D',color='#ffffff',markersize=5);
  # legend
  ax1.legend(framealpha=0.5,frameon=False,loc=2,numpoints=1)
  ax2.legend(framealpha=0.5,frameon=False,loc=1,numpoints=1)
  # axis
  ax1.set_xlim(-1,len(apps))
  ax1.set_ylim(0,80)
  ax1.set_ylabel("Execution Time (min)")
  ax2.set_ylim(0,4)
  ax2.set_ylabel("Loss of Output Quality (%)")
  # set figure size
  fig = plt.gcf()
  fig.set_size_inches(7, 4.5)
  plt.tight_layout()
  plt.savefig("intro.pdf")
  plt.close()

def nsec2str(nsec):
  if nsec < 60:
    return "%ds" % nsec
  else:
    return "%dm" % (nsec/60)

def draw_sensi():
  dat = np.loadtxt(sensi_data)
  mks = ['o','x','v','+','s','*','D','^','|','3']
  ls = ['|',',','--','-',':']
  gp = palette.get_gradient_palette(len(apps))
  nser = 1
  X = np.array(range(len(dat[:,0])))
  X_ticklabels = [nsec2str(x) for x in dat[:,0]]
  for app in apps:
    plt.plot(X,dat[:,nser],label=apps[nser-1],ls=ls[nser%len(ls)],color=gp[nser],marker=mks[nser-1])
    nser = nser + 1
  ax = plt.gca()
  ax2 = ax.twinx()
  ax2.bar(X - 0.15,(1.0*100)/dat[:,0],0.3,color="white",label="CloudModel Overhead");
  ax2.legend(framealpha=0.5,frameon=False,loc="lower right",numpoints=1)
  # lgd = ax.legend(framealpha=0.5,frameon=False,loc="upper center",ncol=3,numpoints=1,bbox_to_anchor=(0,1.02,1,0.4))
  lgd = ax.legend(framealpha=0.5,frameon=False,loc="upper center",ncol=3,numpoints=1)
  ax.set_xlim(-1,len(X))
  ax.set_xlabel("Sampling Interval")
  ax.set_xticks(X)
  ax.set_xticklabels(X_ticklabels)
  ax.set_ylim(0,90)
  ax.set_ylabel("Execution Time (Min)")
  ax.arrow(4,10,0,-5,head_width=0.2,head_length=1.5,zorder=10)
  ax.text(4.2,8,"best interval",zorder=10)
  ax2.set_ylim(0,100)
  ax2.set_ylabel("CloudModel Overhead(%)")
  # set figure size
  fig = plt.gcf()
  fig.set_size_inches(7.5, 5)
  fig.tight_layout()
  plt.savefig('sensi.pdf',bbox_extra_artists=(lgd,), bbox_inches='tight')
  plt.show()

if __name__ == '__main__':
  draw_intro()
  draw_sensi()

