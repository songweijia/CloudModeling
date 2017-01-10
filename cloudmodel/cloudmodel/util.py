#!/usr/bin/python
import logging
import paramiko

logger = logging.getLogger(__name__)

def remote_exec(ssh_client, get_pty=False, commands=[], verifiers=[]):
  """
  execute remote commands
  """
  # open a client
  ssh = paramiko.SSHClient()
  # execute commands
  results = []
  for i in xrange(len(commands)):
    # run command
    logger.debug("running remote command: %s" % commands[i])
    stdin,stdout,stderr = ssh_client.exec_command(commands[i],get_pty=get_pty)
    # verify
    logger.debug("verifying...")
    output = stdout.readlines()
    error = stderr.readlines()
    logger.debug("stdout=%s" % output)
    logger.debug("stderr=%s" % error)
    if verifiers[i] != None:
      results.append(verifiers[i](output,error))
    else:
      results.append([output,error])
  return results

def detect_pkgmgr(ssh_client):
  """
  detect the packet manager: apt-get or yum
  """
  def v(output,error):
    if len(error) == 0:
      return True
    else:
      return False
  pkgmgrs = ['apt','apt-get','yum']
  vs = [v,v,v]
  results = remote_exec(ssh_client,commands=pkgmgrs,verifiers=vs)
  for i in xrange(len(pkgmgrs)):
    if results[i]:
      return pkgmgrs[i]
  return None

def install_pkgs(ssh_client,pkg_list=[]):
  """
  install required package list
  """
  pkgmgr = detect_pkgmgr(ssh_client);
  if pkgmgr == None:
    logger.error("Cannot decide packet manager.")
    raise NameError('Unknown packet manager.')
  cmds = []
  vs = []
  for pkg in pkg_list:
    if pkgmgr in ['apt','apt-get','yum']:
      cmds.append('sudo %s install %s -y' % (pkgmgr,pkg[pkgmgr]))
    vs.append(None)
  remote_exec(ssh_client,get_pty=True,commands=cmds,verifiers=vs)

def upload(ssh_client,local,remote):
  """
  upload a file
  """
  sftp_client = ssh_client.open_sftp()
  logger.debug("uploading local file(%s) to remote(%s)" % (local,remote))
  sftp_client.put(local,remote)
  logger.debug("upload succeed.")
  sftp_client.close()

def download(ssh_client,remote,local):
  """
  download a file
  """
  sftp_client = ssh_client.open_sftp()
  logger.debug("downloading remote file(%s) to local(%s)" % (remote,local))
  sftp_client.get(remote,local)
  logger.debug("download succeed.")
  sftp_client.close()
