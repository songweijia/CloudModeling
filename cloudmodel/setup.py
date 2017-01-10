#!/usr/bin/python

from setuptools import setup

setup(name='cloudmodel',
  version='0.1',
  description='The cloudmodel project',
  url='',
  author='Weijia Song',
  author_email='songweijia@gmail.com',
  license='NEW BSD LICENSE',
  packages=['cloudmodel'],
  package_dir={'cloudmodel':'cloudmodel'},
  package_data={'cloudmodel': ['res/cpumem/benchmark.tgz']},
  install_requires=[
    'paramiko',
    'numpy',
    'scipy',
    'matplotlib',
  ],
  zip_safe=False)
