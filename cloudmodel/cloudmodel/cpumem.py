#/usr/bin/python
import threading,logging,os,pkg_resources,time,bisect,glob
import paramiko
import numpy
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from scipy.cluster.vq import whiten,kmeans
from itertools import groupby
import util
from pack import Pack,PackConfig

logger = logging.getLogger(__name__);

class CPUMemPack(Pack):
  """
  Cpu/Mem pack
  """
  def __init__(self, conf):
    self._conf = conf
    self._packages = []
    self._packages.append({'apt':'gcc','apt-get':'gcc','yum':'gcc'})
    self._packages.append({'apt':'make','apt-get':'make','yum':'make'})
    self._data_path = 'data'
    self._figs_path = 'figs'

  def upload_benchmark(self, ssh_client, remote_path):
    """
    upload cpumem benchmark through ssh_client
    """
    # STEP 1 find local resources
    resource_package = __name__
    resource_path = '/'.join(('res','cpumem','benchmark.tgz'))
    local_file = pkg_resources.resource_filename(__name__,resource_path)
    remote_file = "%s/benchmark.tgz" % remote_path
    # STEP 2 upload local resources
    util.upload(ssh_client,local_file,remote_file);
    logger.debug("%s is uploaded to %s" % (local_file, remote_file) )
    # STEP 3 unpack
    util.remote_exec(ssh_client, commands=['cd %s; tar -xf benchmark.tgz' % remote_path], verifiers=[None])
    logger.debug("benchmark.tgz unpacked")

  def run_sequential_read_write(self, ssh_client, host_alias, remote_path, work_path, data_path):
    """
    run sequential_read_write benchmark
    test name: sequential_read_write
    test id: srw
    - ssh_client: ssh_client
    - host_alias: alias
    - remote_path: remote path
    - work_path: local path
    - data_path: data path
    return: timestamp.
    """
    # run experiments
    util.remote_exec(ssh_client, commands=['cd %s;./srw.sh' % remote_path], verifiers=[None])
    logger.debug("sequential_read_write test finished.")
    # collect results
    timestamp = int(round(time.time()))
    util.download(ssh_client,"%s/data/srw.result" % remote_path, "%s/srw.%s.%d" % (data_path,host_alias,timestamp) )
    logger.debug("sequential_read_write data collected.")
    # clean remote garbage
    util.remote_exec(ssh_client, commands=['cd %s;rm -rf data/*' % remote_path], verifiers=[None])
    logger.debug("sequential_read_write data remote cleaned up.")
    return timestamp

  def run_random_read(self, ssh_client, host, sec, remote_path, work_path, data_path):
    """
    run randome read benchmark to evaluate the cache/mem latencies
    output: rr[1/2/3/4].[host].[sec]
    """
    # get test sizes
    csdat = numpy.loadtxt('/'.join((data_path,"srw.%s.%d.cs" % (host,sec))));
    bfsz = [csdat[0][0]/2, \
      (csdat[0][1] + csdat[1][0])/2, \
      (csdat[1][1] + csdat[2][0])/2, \
      csdat[2][1]*10]
    util.remote_exec(ssh_client, commands=['cd %s;./rr.sh %d %d %d %d' % (remote_path, bfsz[0], bfsz[1], bfsz[2], bfsz[3])], verifiers=[None])
    logger.debug("random_read test finished.")
    for i in xrange(4):
      util.download(ssh_client, "%s/data/rr%d.result" % (remote_path,i), "%s/rr%d.%s.%d" % (data_path,i,host,sec))
    logger.debug("randome_read data collected.")
    util.remote_exec(ssh_client, commands=['cd %s;rm -rf data/*' % remote_path], verifiers=[None])
    logger.debug("sequential_read_write data remote cleaned up.")

  def analyze_sequential_read_write_preprocess(self,data_path,host,sec):
    """
    convert raw file to data file:
    <buffer size in kb> <read> <write> <stride read> <stride write> 
    there are two files to be generated here:
    srw.<host>.<sec>.dat is the file with all raw data
    srw.<host>.<sec>.max has the file for only the maximum value for same tests
    """
    logger.debug("begin preprocessing srw data.")
    #for h in host_dict.keys():
    #  for s in host_dict[h]:
    # prepare raw data file.
    finn = '/'.join((data_path,"srw.%s.%d" % (host,sec)))
    fin = open(finn,'r')
    raw = []
    while True:
      # read a line
      line = fin.readline()
      if line == '':
        break;
      tks = line.split()
      raw.append((float(tks[1]),float(tks[4]),float(tks[7]),float(tks[10]),float(tks[13])))
    fin.close()
    raw = numpy.array(raw)
    raw = raw[raw[:,0].argsort()] # sort
    numpy.savetxt('/'.join((data_path,"srw.%s.%d.dat" % (host,sec))), raw, fmt='%.4f')
    # prepare max data file for kmeans. 'max' means that, for each value, we pick the maximum throughput in repeated experiments
    mraw = [] # max rows after group
    for (k,g) in groupby(raw,lambda x:x[0]):
      vec = numpy.array(list(g))
      mraw.append(numpy.amax(vec,axis=0))
    mraw = numpy.array(mraw)
    numpy.savetxt('/'.join((data_path,"srw.%s.%d.max" % (host,sec))), mraw, fmt='%.4f')
    logger.debug("finished preprocessing srw data.")

  def analyze_sequential_read_write_kmeans_thp(self,data_path,host,sec):
    """
    get key means throughput
    return centroids, and write to file: srw.host.sec.thp
    """
    logger.debug("begin k-means thp for %s@%d." % (host,sec))
    # STEP 1 load data
    mraw = numpy.loadtxt('/'.join((data_path,"srw.%s.%d.max" % (host,sec))))
    dat = numpy.delete(mraw,0,1) # get rid of the buffer size
    # STEP 2 k-means
    wdat = whiten(dat) # whitening
    wcentroids,distortion = kmeans(wdat,4)
    logger.debug("k-means distortion=%f." % distortion)
    centroids = wcentroids * numpy.std(dat,0) # unwhitening
    centroids = centroids[(-centroids[:,0]).argsort()] # sorting
    # STEP 3 write to file
    numpy.savetxt('/'.join((data_path,"srw.%s.%d.thp" % (host,sec))),centroids,fmt='%.4f')
    logger.debug("finished k-means thp for %s@%d." % (host,sec))
    return centroids

  def analyze_sequential_read_write_cache_size(self,data_path,host,sec):
    """
    calculate cache size using stride read [:2]
    output: srw.<host>.<sec>.cs
    """
    logger.debug("begin calculating cache size for %s@%d." % (host,sec))
    mraw = numpy.loadtxt('/'.join((data_path,"srw.%s.%d.max" % (host,sec))))
    stride_read_max = mraw[:,(0,3)] # use only stride read data(3).
    thp = numpy.loadtxt('/'.join((data_path,"srw.%s.%d.thp" % (host,sec))))
    stride_read_thp = thp[:,2] # the third column(idx:2) is the stride read column
    cache_sizes = []
    range_factor = 0.1 # TODO: is 0.1 good?
    for idx in xrange(len(stride_read_thp)-1):
      lower = max([k for (k,v) in stride_read_max if v > (stride_read_thp[idx] - (stride_read_thp[idx]-stride_read_thp[idx+1]) * range_factor )])
      upper = min([k for (k,v) in stride_read_max if v < (stride_read_thp[idx+1] + (stride_read_thp[idx]-stride_read_thp[idx+1]) * range_factor )])
      cache_sizes.append((lower,upper))
    numpy.savetxt('/'.join((data_path,"srw.%s.%d.cs" % (host,sec))),numpy.array(cache_sizes),fmt="%.0f")
    logger.debug("finished calculating cache size for %s@%d." % (host,sec))

  def analyze_sequential_read_write(self,data_path,host,sec):
    """
    analyze result from sequential_read_write. we will get the estimated cache throughput and size
    data files are put in [work_path]/srw.[hostname].[secs], 
    PARAMETERS:
    ===========
    - data_path the path that contains the data directory
    - host_dict the metadata including all the raw data files like: {'host1':[t1,t2,t3,...],'host2':[t1,t2,t3,...]}
    OUTPUT:
    =======
    - srw.[hostname].[secs].thp
    - srw.[hostname].[secs].cs
    - srw.[hostname].[secs].thp.eps
    - srw.[hostname].l1/l2/l3size.eps
    PROCEDURES:
    ===========
    1. srw.[hostname].[secs] ==> srw.[hostname].[secs].thp
    2. srw.[hostname].[secs] + srw.[hostname].[secs].thp ==> srw.[hostname].size.[secs]
    3. srw.[hostname].[secs] ==> srw.[hostname].[secs].thp.eps
    4. srw.[hostname].size.[secs] ==> srw.[hostname].l1/2/3size.eps
    FORMATS:
    ========
    In src.[hostname].[secs].thp, we have four lines for L1,L2,L3, and memory throughput. each line has four values in GByte/s
    <read> <write> <stride_read> <stride_write>
    In src.[hostname].size.[secs], we have one line for the size of L1/2/3 cache in KByte
    <L1> <L2> <L3>
    """
    # STEP 1 preprocessing
    self.analyze_sequential_read_write_preprocess(data_path,host,sec)
    # STEP 2 get keams throughputs and cache size
    self.analyze_sequential_read_write_kmeans_thp(data_path,host,sec)
    self.analyze_sequential_read_write_cache_size(data_path,host,sec)

  def analyze_random_read(self,data_path,host,sec):
    """
    input: rr[1/2/3/4].[host].[sec]
    analyze random read result.
    OUTPUT: rr.[host].[sec].lat, for L1/2/3 cache and memory(4)
    """
    lats = []
    for i in xrange(4):
      # load file
      f = open('/'.join((data_path,'rr%d.%s.%d' % (i,host,sec))))
      minlat = 9999999999.9
      for line in f.readlines():
        tks = line.split()
        val = float(tks[2])
        if minlat > val:
          minlat = val
      lats.append(minlat)
    numpy.savetxt('/'.join((data_path, 'rr.%s.%d.lat' % (host,sec))),numpy.array(lats),fmt='%.4f')

  def get_test_metadata(self,data_path):
    """
    check the data file in data_path and make a metadata for all the tests like:
    {
      'srw':{'localhost':[1483383227,1483383228,1483383229,1483383230,...],...},...
    }
    """
    files = os.listdir(data_path)
    test_meta = {}
    for fn in files:
      tks = fn.split('.')
      if len(tks) != 3:
        continue
      # get time
      try:
        secs = int(tks[2])
      except ValueError:
        continue
      # put into test_meta
      try:
        test_dict = test_meta[tks[0]]
      except KeyError:
        test_dict = {}
        test_meta[tks[0]] = test_dict
      try:
        host_arr = test_dict[tks[1]]
      except KeyError:
        host_arr = []
        test_dict[tks[1]] = host_arr
      bisect.insort(host_arr,secs)
    return test_meta

  def worker(self, host_cred, remote_path, work_path=None):
    """
    run cpumem benchmarking. The benchmarking has multiple tests. Each test has
    - test name: sequential_read_write
    - test runner: run_sequential_read_write
    the output file format: [test_id].[host_alias].[time_sec]
    - srw.localhost.483380495
    """
    # STEP 0: some initialization
    hostname = host_cred['hostname']
    if host_cred['alias'] != None:
      hostname = host_cred['alias']
    if work_path == None:
      work_path = os.getcwd()
    data_path = '/'.join((work_path,self._data_path))
    if os.path.exists(data_path):
      if not os.path.isdir(data_path):
        os.remove(data_path)
        os.mkdir(data_path)
    else:
      os.mkdir(data_path)
    logger.debug("initialized:target=%s,cwd=%s." % (hostname,os.getcwd()))
    # STEP 1: connect to remote server
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.client.AutoAddPolicy())
    ssh.connect(host_cred['hostname'],username=host_cred['username'],password=host_cred['password'],key_filename='/'.join((work_path,host_cred['key_filename'])))
    logger.debug("connected to %s." % hostname)
    # STEP 2: test prerequisite
    util.install_pkgs(ssh,self._packages)
    logger.debug("packages installed.")
    # STEP 3: build environment
    util.remote_exec(ssh, commands=['rm -rf %s' % remote_path, 'mkdir %s' % remote_path], verifiers=[None,None])
    self.upload_benchmark(ssh, remote_path)
    logger.debug("benchmark installed.")
    # STEP 4: run experiments
    ## 4.1 Sequential Read and Write
    timestamp = self.run_sequential_read_write(ssh, hostname, remote_path, work_path, data_path)
    ## 4.2 analyze SRW
    self.analyze_sequential_read_write(data_path, hostname, timestamp)
    ## 4.3 Random Read
    self.run_random_read(ssh, hostname, timestamp, remote_path, work_path, data_path)
    ## 4.4 analyze RR
    self.analyze_random_read(data_path, hostname, timestamp)
    # STEP 5: clean up
    ssh.close()

  def visualize_thp_snapshot(self,data_path,figs_path,host_alias,timestamp):
    """
    visualize the throughput snapshots
    """
    # STEP 1: load data
    raw = numpy.loadtxt('/'.join((data_path,"srw.%s.%d.dat" % (host_alias,timestamp))))
    mraw = numpy.loadtxt('/'.join((data_path,"srw.%s.%d.max" % (host_alias,timestamp))))
    # STEP 2: draw it
    (d_r,d_w,d_sr,d_sw,m_r,m_w,m_sr,m_sw) = plt.plot( \
      raw[:,0],raw[:,1],'r.', \
      raw[:,0],raw[:,2],'g.', \
      raw[:,0],raw[:,3],'b+', \
      raw[:,0],raw[:,4],'y+', \
      mraw[:,0],mraw[:,1],'r', \
      mraw[:,0],mraw[:,2],'g', \
      mraw[:,0],mraw[:,3],'b', \
      mraw[:,0],mraw[:,4],'y')
    ax = plt.gca()
    ax.set_xscale('log')
    ax.set_xlim(8,130000)
    ax.set_ylim(0,80)
    ax.set_xlabel('Buffer Size (KBytes)')
    ax.set_ylabel('Sequential Access Throughput (GBytes/Sec)')
    plt.legend([m_r,m_w,m_sr,m_sw], \
      ['read','write','stride read','stride write'], \
      ncol=2 )
    plt.savefig('/'.join((figs_path,"srw.%s.%d.thp.eps" % (host_alias,timestamp))));
    plt.close()

  def visualize_lat_snapshot(self,data_path,figs_path,host_name,timestamp):
    """
    visualize the 
    """
    raise NameError("%s not implemented yet" % self.visualize_lat_snapshot.__name__)

  def visualize_cache_size_ts(self,data_path,figs_path,host_alias,tstart=0,tend=0):
    """
    visualize the cache timeseries
    """
    # STEP 1 LOAD data
    fl = glob.glob('/'.join((data_path,'srw.%s.*.cs' % host_alias)))
    raw = []
    for f in fl:
      # filtering
      t = int(f.split('.')[-2])
      if tstart != 0 and tend != 0:
        if t < tstart or t > tend:
          continue
      row = numpy.array(numpy.loadtxt(f).flat)
      raw.append(numpy.concatenate(([t],row)))
    if len(raw) == 0:
      return
    raw = numpy.array(raw)
    sraw = raw[raw[:,0].argsort()]
    # STEP 2 draw figure
    for i in xrange(3):
      plt.fill_between(sraw[:,0],0,sraw[:,2*i+1],facecolor="dodgerblue",linewidth=0.0)
      plt.fill_between(sraw[:,0],sraw[:,2*i+1],sraw[:,2*i+2],facecolor="skyblue",linewidth=0.0)
      ax = plt.gca()
      ax.set_xlim(sraw[:,0][0],sraw[:,0][-1])
      ax.set_xlabel('Time (second)')
      ax.set_ylabel('Observed L%d Cache Size(KB)' % (i+1))
      plt.savefig('/'.join((figs_path,"srw.%s.c%ds.eps" % (host_alias,i+1))));
      plt.close()

  def visualize_throughput_ts(self,data_path,figs_path,host_alias,tstart=0,tend=0):
    """
    visualize the cache timeseries
    """
    # STEP 1 LOAD data
    fl = glob.glob('/'.join((data_path,'srw.%s.*.thp' % host_alias)))
    raw = []
    for f in fl:
      # filtering
      t = int(f.split('.')[-2])
      if tstart != 0 and tend != 0:
        if t < tstart or t > tend:
          continue
      row = numpy.array(numpy.loadtxt(f).flat)
      raw.append(numpy.concatenate(([t],row)))
    if len(raw) == 0:
      return
    raw = numpy.array(raw)
    sraw = raw[raw[:,0].argsort()]
    # STEP 2 draw figure
    (c1r,c1w,c2r,c2w,c3r,c3w,mr,mw) = plt.plot( \
      sraw[:,0],sraw[:,1],"r-", \
      sraw[:,0],sraw[:,2],"r:", \
      sraw[:,0],sraw[:,5],"g-", \
      sraw[:,0],sraw[:,6],"g:", \
      sraw[:,0],sraw[:,9],"b-", \
      sraw[:,0],sraw[:,10],"b:", \
      sraw[:,0],sraw[:,13],"k-", \
      sraw[:,0],sraw[:,14],"k:")
    plt.legend([c1r,c1w,c2r,c2w,c3r,c3w,mr,mw],['L1R','L1W','L2R','L2W','L3R','L3W','MR','MW'],ncol=4)
    ax = plt.gca()
    ax.set_xlim(sraw[:,0][0],sraw[:,0][-1])
    ax.set_ylim(0,80)
    ax.set_xlabel('Time (second)')
    ax.set_ylabel('Sequential Access Throughput(GB/s)')
    plt.savefig('/'.join((figs_path,"srw.%s.thp.eps" % host_alias)));
    plt.close()

  def visualize_latency_ts(self,data_path,figs_path,host_alias,tstart=0,tend=0):
    """
    visualize the latency timeseries
    """
    # STEP 1 LOAD data
    fl = glob.glob('/'.join((data_path,'rr.%s.*.lat' % host_alias)))
    raw = []
    for f in fl:
      # filtering
      t = int(f.split('.')[-2])
      if tstart != 0 and tend != 0:
        if t < tstart or t > tend:
          continue
      row = numpy.array(numpy.loadtxt(f).flat)
      raw.append(numpy.concatenate(([t],row)))
    if len(raw) == 0:
      return
    raw = numpy.array(raw)
    sraw = raw[raw[:,0].argsort()]
    # STEP 2 draw figure
    (l1c,l2c,l3c,mem) = plt.plot( \
      sraw[:,0],sraw[:,1],"k-", \
      sraw[:,0],sraw[:,2],"r--", \
      sraw[:,0],sraw[:,3],"g:", \
      sraw[:,0],sraw[:,4],"b-.")
    plt.legend([l1c,l2c,l3c,mem],['L1 Cache','L2 Cache','L3 Cache','SDRAM'],ncol=2)
    ax = plt.gca()
    ax.set_xlim(sraw[:,0][0],sraw[:,0][-1])
    ax.set_ylim(0,numpy.max(sraw[:,1:5])*1.5)
    ax.set_xlabel('Time (second)')
    ax.set_ylabel('Read Latency (ns)')
    plt.savefig('/'.join((figs_path,"rr.%s.lat.eps" % host_alias)));
    plt.close()

  def run(self):
    """
    do CPU Mem Pack measurement.
    """
    # STEP 1: create threads
    tarr = []
    for cfg in self._conf._host_creds:
      tarr.append(threading.Thread(target=self.worker,args=[cfg,self._conf._remote_path,self._conf._work_path]))
      tarr[-1].start()
      logger.debug("created thread for cfg:%s" % cfg)
    for t in tarr:
      t.join()
    logger.debug("finished testing.")

  def visualize(self, work_path=None, tstart=0, tend=0, snapshots=False):
    """
    visualize arrange of data
    tstart - start time point by seconds since epoch
    tend - end time point by seconds since epoch
    snapshot - if visualize snapshot, it takes a long time. Use with caution.
    """
    # load data
    if work_path == None:
      work_path = os.getcwd()
    data_path = '/'.join((self._conf._work_path,self._data_path))
    test_meta = self.get_test_metadata(data_path)
    # create figure folders
    figs_path = '/'.join((self._conf._work_path,self._figs_path))
    if os.path.exists(figs_path):
      if not os.path.isdir(figs_path):
        raise NameError( 'figs_path:%s exists and it is not a directory.' % figs_path )
    else:
      os.mkdir(figs_path)
    # visualize it.
    for h in test_meta['srw']:
      # draw snapshots:
      if snapshots:
        for t in test_meta['srw'][h]:
          if (tstart == 0 and tend == 0) or (tstart <= t and t <= tend):
            # draw snapshots
            self.visualize_thp_snapshot(data_path,figs_path,h,t)
      # draw timeseries for cache size/throughput
      self.visualize_cache_size_ts(data_path,figs_path,h)
      self.visualize_throughput_ts(data_path,figs_path,h)
      self.visualize_latency_ts(data_path,figs_path,h)
