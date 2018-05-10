#!/usr/bin/python
from cloudmodel.factory import PackFactory
from cloudmodel.pack import PackConfig

if __name__=="__main__":
  pf = PackFactory()
  # for m in ["1","3","7","f","1f","3f","7f","ff","1ff","3ff","7ff","fff","1fff","3fff","7fff","ffff","1ffff","3ffff","7ffff","fffff"]:
  conf = PackConfig('cpumem',[{'hostname':'localhost', 'alias':'localhost', 'username':'ws393', 'password':None, 'key_filename':'id_rsa'}],'work','tmp',"0xfffff");
  p = pf.create_pack(conf);
  p.run()
