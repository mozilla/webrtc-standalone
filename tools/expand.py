import sys
import os.path

def dump(str):
  sys.stdout.write(' ' + str)
  #sys.stderr.write('\n' + str)


def expand(file):
  with open(file) as f:
    for line in f:
      val = line.split()
      if len(val) > 2:
        if val[0] == 'LIBS':
          for item in val[2:]:
            desc = item + '.desc'
            if os.path.isfile(desc):
              expand(desc)
            elif os.path.isfile(item):
              dump(item)
              sys.stderr.write('\n' + item)
            else:
              sys.stderr.write('FAILED to file item: ' + item)
              sys.exit(-1)
        elif val[0] == 'OBJS':
          for item in val[2:]:
            dump(item)

for lib in sys.argv[1:]:
  expand(lib)
