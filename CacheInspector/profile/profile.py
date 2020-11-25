#!/usr/bin/python3
import numpy as np
from scipy.optimize import curve_fit
from scipy.stats import gaussian_kde
from scipy.signal import argrelextrema
import matplotlib.pyplot as plt
import sys

#Parse throughput lines like the followings:
# WRITE @1788928 23.909 GiB/s std 0.008 min 23.180 max 24.076
# ^      ^       ^      ^         ^         ^          ^
# 0      1       2      3         5         7          9
# LATENCY @1719296 9.196 nsecs std 0.142 min 5.968 max 13.194
# ^        ^       ^     ^         ^         ^         ^
# 0        1       2     3         5         7         9
# @param line
# @return buffer_size,avg,std,min,max,unit
# @throws exception on error
def parse_schedule_line(line):
    tks = line.split();
    return int(tks[1][1:]), float(tks[2]), float(tks[5]), float(tks[7]), float(tks[9]), tks[3]
# Parse schedule data file
# @param file_name      the schedule output file
# @return read throughput, write throughput, latency in 2-dimensional numpy array and units(string)
def parse_schedule_file(file_name):
    rthp = []
    wthp = []
    lat  = []
    rthp_unit = ""
    wthp_unit = ""
    lat_unit = ""
    with open(file_name, 'r') as schedule_output:
        for line in schedule_output:
            try:
                sline = line.strip();
                if sline.startswith("READ @"):
                    buffer_size,avg,_std,_min,_max,_unit = parse_schedule_line(sline)
                    rthp.append([buffer_size,avg])
                    rthp_unit = _unit
                elif sline.startswith("WRITE @"):
                    buffer_size,avg,_std,_min,_max,_unit = parse_schedule_line(sline)
                    wthp.append([buffer_size,avg])
                    wthp_unit = _unit
                elif sline.startswith("LATENCY @"):
                    buffer_size,avg,_std,_min,_max,_unit = parse_schedule_line(sline)
                    lat.append([buffer_size,avg])
                    lat_unit = _unit
                else:
                    # skip other lines
                    continue
            except ValueError:
                print ("Warning: skip invalid line:%s",line)
    return np.asarray(rthp),np.asarray(wthp),np.asarray(lat),rthp_unit,wthp_unit,lat_unit

# We model the average read latency to a buffer of size x as following:
# latency(ws) = P_1(x)*L_1 + P_2(x)*L_2 + ... + P_n(x)*L_n + P_mem(x)*L_mem, where P_i is the probability of the access
# served by the i-th cache level(or memory). And L_i is the latency of the i-th cache level(or memory). P_i can be
# modelled as:
#
#           / 0,                    if x <= S_i
#           |
# P_i(x) = <  (x-S_i)/x,            if x>S_i and x<=S_(i+1)
#           |
#           \ (S_(i+1) - S_i)/x,    if x > S_(i+1)
#
# where S_i is the size of the i-th cache level.
#
# get_piecewise_latency_function constructs a piecewise function in this form:
# piecewise_latency(x,L_1,L_2,...,L_n,L_mem,S_1,S_2,...,S_n)
# @param num_level  The number of cache levels
# @return piecewise function for the cache latency
def get_piecewise_latency_function(num_levels):
    # args[0]:      L_1
    # args[1]:      L_2
    # ...
    # args[n-1]:    L_n
    # args[n]:      L_mem
    # args[n+1]:    S_1
    # args[n+2]:    S_2
    # ...
    # args[2n]:     S_n
    def _plfunc(x,*args,n=num_levels):
        cond_arr=[]
        func_arr=[]
        cond_arr.append(x<=args[n+1])
        func_arr.append(lambda x:args[0])
        l = 1
        while l <= n:
            # Here we consider a buffer size between L_l and L_(l+1)
            if l == n:
                cond_arr.append(x>args[n+l])
            else:
                cond_arr.append((x>args[n+l]) & (x<=args[n+l+1]))
            def _func(x,n=n,l=l,args=args):
                sum = 0.0
                for i in range(l):
                    sum = sum + args[n+i+1]*args[i]
                sum = sum + (x-args[n+l])*args[l]
                return sum/x
            func_arr.append(_func);
            l = l+1
        return np.piecewise(x,cond_arr,func_arr)
    return _plfunc

# detect cache latency performance
# @param dat        the latency data
# @param num_levels the number of cache levels
# @param size_hint  the hint for the size of each cache level in a list. For example, if the number of cache levels is
#                   3, you give [32768,262144,16384*1024]
# @param lat_hint   the hint for the performance of each cache level. For example, if the number of cache levels is 3,
#                   you give [1,4,12,100]. The last member is for memory.
# @return a list with length num_levels+1, with the first num_levels for each cache level and the last for memory.
def detect_cache_latency(dat,num_levels,size_hint,lat_hint,draw_graph=False,unit='nsec'):
    p0 = lat_hint + size_hint
    pl = get_piecewise_latency_function(num_levels)
    popt,pcov = curve_fit(pl,dat[:,0],dat[:,1],p0);
    if (draw_graph):
        fit_label = "fit ("
        for i in range(num_levels):
            fit_label = fit_label+"L%d:%.2f%s; "%(i+1,popt[i],unit)
        fit_label = fit_label + "MEM:%.2f%s"%(popt[num_levels],unit)
        fit_label = fit_label + ")"
        plt.scatter(dat[:,0],dat[:,1],c='red',s=3,alpha=0.4,label='observed')
        plt.plot(dat[:,0],pl(dat[:,0],*popt),'k--',lw=0.8,label=fit_label)
        plt.xscale("log",basex=2)
        plt.xlabel("Buffer Size (Bytes)")
        plt.ylabel("Latency (%s)" % unit)
        plt.legend()
        plt.tight_layout()
        plt.savefig("latency.pdf")
        plt.close()
    print (str(popt))
    return popt[:num_levels+1]

# We model the average throughput accessing a buffer of size x as following:
#
# throughput(x) =  T_i       if x > S_(i-1) and x <= S_i
# where T_i is the throughput of the i-th cache level, S_i is the size of the i-th cache level. Define
# 1) S_0 = 0,
# 2) T_(i+1) = T_mem
# 3) S_(i+1) = S_mem = inifinity
#
# get_piecewise_throughput_function constructs a piecewise function in this form:
# piecewise_latency(x,T_1,T_2,...,T_n,T_mem,S_1,S_2,...,S_n)
# @param num_level  The number of cache levels
# @return piecewise function for the cache latency
def get_piecewise_throughput_function(num_levels):
    # args[0]:      T_1
    # args[1]:      T_2
    # ...
    # args[n-1]:    T_n
    # args[n]:      T_mem
    # args[n+1]:    S_1
    # args[n+2]:    S_2
    # ...
    # args[2n]:     S_n
    def _plfunc(x,*args,n=num_levels):
        cond_arr=[]
        func_arr=[]
        cond_arr.append(x<=args[n+1])
        func_arr.append(lambda x:args[0])
        l = 0;
        while l <= n:
            # Here we consider a buffer size between L_l and L_(l+1)
            if l == 0:
                cond_arr.append(x<=args[n+1])
            elif l == n:
                cond_arr.append(x>args[n+l])
            else:
                cond_arr.append((x>args[n+l]) & (x<=args[n+l+1]))
            func_arr.append(lambda x,l=l:args[l]);
            l = l+1
        return np.piecewise(x,cond_arr,func_arr)
    return _plfunc

# detect throughput performance by piecewise function curve_fit
# @param dat        the throughput data
# @param num_levels the number of cache levels
# @param size_hint  the hint for the size of each cache level in a list. For example, if the number of cache levels is
#                   3, you give [32768,262144,16384*1024]
# @param thp_hint   the hint for the throughput performance
def detect_cache_throughput_by_curve_fit(dat,num_levels,size_hint,thp_hint,draw_graph=False,unit='GiB/s'):
    p0 = thp_hint + size_hint
    pl = get_piecewise_throughput_function(num_levels)
    popt,pcov = curve_fit(pl,dat[:,0],dat[:,1],p0);
    if (draw_graph):
        fit_label = "fit ("
        for i in range(num_levels):
            fit_label = fit_label+"L%d:%.2f%s; "%(i+1,popt[i],unit)
        fit_label = fit_label + "MEM:%.2f%s"%(popt[num_levels],unit)
        fit_label = fit_label + ")"
        plt.scatter(dat[:,0],dat[:,1],c='red',s=3,alpha=0.4,label='throughput data')
        plt.plot(dat[:,0],pl(dat[:,0],*popt),'g--',lw=0.8,label=fit_label)
        plt.xscale("log",basex=2)
        plt.xlabel("Buffer Size (Bytes)")
        plt.ylabel("Throughput (%s)" % unit)
        plt.legend()
        plt.tight_layout()
        plt.savefig("throughput.pdf")
        plt.close()
    return popt[:num_levels+1]

# detect throughput performance by kernel density estimation
def detect_cache_throughput_by_kde(dat,num_levels,bin_size=0.02,draw_graph=False,unit='GiB/s',is_write=False):
    thp_dim = dat[:,1]
    kernel = gaussian_kde(thp_dim,bw_method=bin_size)
    x = np.arange(0,max(thp_dim)+1,0.01)
    density = kernel(x)
    maxima_x, = argrelextrema(density, np.greater)
    output = sorted([x[i] for i in sorted(maxima_x, key=lambda x:density[x],reverse=True)[0:(num_levels+1)]],reverse=True)
    lss=["-",":","--","-."]
    if draw_graph:
        X_len = dat.shape[0]
        plt.scatter(dat[:,0],dat[:,1],c='red',s=3,alpha=0.4,label='observed')
        for i in range(len(output) - 1):
            plt.plot(dat[:,0],np.repeat(output[i],X_len),c='k',ls=lss[i%len(lss)],lw=0.8,label='fitted l%d (%.2f %s)'%(i+1,output[i],unit))
        plt.plot(dat[:,0],np.repeat(output[-1],X_len),c='k',ls=lss[(len(output)-1)%len(lss)],lw=0.8,label='mem (%.2f %s)'%(output[-1],unit))
        plt.xscale("log",basex=2)
        plt.xlabel("Buffer Size (Bytes)")
        if is_write:
            plt.ylabel("Write Throughput (%s)" % unit)
        else:
            plt.ylabel("Read Throughput (%s)" % unit)
        plt.legend(loc=1)
        plt.tight_layout()
        if is_write:
            plt.savefig("write_throughput.pdf")
        else:
            plt.savefig("read_throughput.pdf")
        plt.close()
    return output

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print ("Usage %s <schedule output file>" % sys.argv[0])
        quit(-1)
    r,w,l,ru,wu,lu = parse_schedule_file(sys.argv[1]);
    if r.size > 0:
        np.savetxt("read_throughput",r)
        # read_throughput = detect_cache_throughput_by_curve_fit(r,3,[32768,262144,16384*1024],[100,50,30,10],draw_graph=True,unit=ru)
        read_thps = detect_cache_throughput_by_kde(r,3,draw_graph=True,is_write=False)
        print(str(read_thps))
    if w.size > 0:
        np.savetxt("write_throughput",w)
        write_thps = detect_cache_throughput_by_kde(w,3,draw_graph=True,is_write=True)
        print(str(write_thps))
    if l.size > 0:
        np.savetxt("latency",l)
        latencies = detect_cache_latency(l,3,[32768,262144,16384*1024],[1.0,5.0,12.0,200.0],draw_graph=True,unit=lu)
        print(str(latencies))
