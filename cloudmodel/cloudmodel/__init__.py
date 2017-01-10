#!/usr/bin/python
import logging
# prepare logger
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
fh = logging.FileHandler("%s.log" % __name__)
formatter = logging.Formatter('%(asctime)s - %(thread)d - %(name)s - %(levelname)s - %(message)s')
fh.setFormatter(formatter)
fh.setLevel(logging.DEBUG)
logger.addHandler(fh)
logger.info("cloudmodule imported.")
