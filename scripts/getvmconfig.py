#!/usr/bin/python
import os
import sys

vmcfg={\
      'amz':{\
             't2.nano':{\
                        'cpu':1,\
                        'ram':0.5,\
                        'disk':'ebs',\
                        'price':0.0065,\
                       },\
             't2.micro':{\
                         'cpu':1,\
                         'ram':1.0,\
                         'disk':'ebs',\
                         'price':0.013,\
                        },\
             't2.small':{\
                         'cpu':1,\
                         'ram':2.0,\
                         'disk':'ebs',\
                         'price':0.026,\
                        },\
             't2.medium':{\
                          'cpu':2,\
                          'ram':4.0,\
                          'disk':'ebs',\
                          'price':0.052,\
                         },\
             't2.large':{\
                         'cpu':2,\
                         'ram':8.0,\
                         'disk':'ebs',\
                         'price':0.104,\
                        },\
             'm4.large':{\
                         'cpu':2,\
                         'ram':8.0,\
                         'disk':'ebs',\
                         'price':0.12,\
                        },\
             'm4.xlarge':{\
                          'cpu':4,\
                          'ram':16.0,\
                          'disk':'ebs',\
                          'price':0.239,\
                         },\
             'm4.2xlarge':{\
                           'cpu':8,\
                           'ram':32.0,\
                           'disk':'ebs',\
                           'price':0.479,\
                          },\
             'm4.4xlarge':{\
                           'cpu':16,\
                           'ram':64.0,\
                           'disk':'ebs',\
                           'price':0.958,\
                          },\
             'm4.10xlarge':{\
                            'cpu':40,\
                            'ram':160.0,\
                            'disk':'ebs',\
                            'price':2.394,\
                           },\
             'm3.medium':{\
                          'cpu':1,\
                          'ram':3.75,\
                          'disk':'ssd',\
                          'price':0.067,\
                         },\
             'm3.large':{\
                         'cpu':2,\
                         'ram':7.5,\
                         'disk':'ssd',\
                         'price':0.133,\
                        },\
             'm3.xlarge':{\
                          'cpu':4,\
                          'ram':15,\
                          'disk':'ssd',\
                          'price':0.266,\
                         },\
             'm3.2xlarge':{\
                          'cpu':8,\
                          'ram':30,\
                          'disk':'ssd',\
                          'price':0.532,\
                         },\
             'c4.large':{\
                         'cpu':2,\
                         'ram':3.75,\
                         'disk':'ebs',\
                         'price':0.105,\
                        },\
             'c4.xlarge':{\
                         'cpu':4,\
                         'ram':7.5,\
                         'disk':'ebs',\
                         'price':0.209,\
                        },\
             'c4.2xlarge':{\
                          'cpu':8,\
                          'ram':15,\
                          'disk':'ebs',\
                          'price':0.419,\
                         },\
             'c4.4xlarge':{\
                          'cpu':16,\
                          'ram':30,\
                          'disk':'ebs',\
                          'price':0.838,\
                         },\
             'c4.8xlarge':{\
                          'cpu':36,\
                          'ram':60,\
                          'disk':'ebs',\
                          'price':1.675,\
                         },\
             'c3.large':{\
                         'cpu':2,\
                         'ram':3.75,\
                         'disk':'ssd',\
                         'price':0.105,\
                        },\
             'c3.xlarge':{\
                         'cpu':4,\
                         'ram':7.5,\
                         'disk':'ssd',\
                         'price':0.209,\
                        },\
             'c3.2xlarge':{\
                          'cpu':8,\
                          'ram':15,\
                          'disk':'ssd',\
                          'price':0.419,\
                         },\
             'c3.4xlarge':{\
                          'cpu':16,\
                          'ram':30,\
                          'disk':'ssd',\
                          'price':0.838,\
                         },\
             'c3.8xlarge':{\
                          'cpu':32,\
                          'ram':60,\
                          'disk':'ssd',\
                          'price':1.675,\
                         },\
            },\
      }

def getvmconfig(provider,vmtype,dim=None):
  if provider in vmcfg.keys():
    if vmtype in vmcfg[provider].keys():
      if dim is None:
        print str(vmcfg[provider][vmtype])
      else:
        if dim in vmcfg[provider][vmtype].keys():
          print str(vmcfg[provider][vmtype][dim])
        else:
          print "Dimension:%s does not exist, please choose from %s." % (dim, str(vmcfg[provider][vmtype].keys))
          exit(3)
    else:
      print "VM type: %s does not exist, please choose from %s." % (vmtype, str(vmcfg[provider].keys()))
      exit(2)
  else:
    print "Cloud provider:%s does not exist, please choose from %s." % ( provider, str(vmcfg.keys()))
    exit(3)
    

if __name__ == "__main__":
  if len(sys.argv) < 3:
    print "Usage: %s <amz|ggl|maz> <vmtype> [dimension]" % sys.argv[0]
    exit(1)
  if len(sys.argv) == 4:
    getvmconfig(sys.argv[1],sys.argv[2],sys.argv[3])
  else:
    getvmconfig(sys.argv[1],sys.argv[2])
