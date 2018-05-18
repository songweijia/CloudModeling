palette=['#0F2043','#79CEDC','#D5A458'] # DARK BLUE, LIGHT BLUE, BEIGE

def get_gradient_palette(n):
  return [ "#%02x%02x%02x" % (0x0F+(0xF0*x/n),0x20+(0xDF*x/n),0x43+(0xBC*x/n)) for x in range(n)]

