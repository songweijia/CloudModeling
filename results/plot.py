#!/usr/bin/python

import sys
import numpy as np
import matplotlib.pyplot as plt

intro_data="intro_exp.dat"
inacc_data="inaccuracy.dat"
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
  unlimit = plt.bar(ind-width,dat[:,0],width,label="unlimited cache",color="#CCCC00")
  limit   = plt.bar(ind      ,dat[:,1],width,label="limited cache",color="#CC0000",hatch='/')
  limitcm = plt.bar(ind+width,dat[:,2],width,label="limited cache with CloudModel",color="#0000CC",hatch='x')
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
  plt.show()


if __name__ == '__main__':
  draw_intro()

