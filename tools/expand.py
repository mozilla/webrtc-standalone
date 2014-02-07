import sys


def expand(file):
  with open(file) as f:
    for line in f:
      val = line.split()
      if val[0] == 'LIBS':
        for item in val[2:]:
          expand(item + '.desc')
      elif val[0] == 'OBJS':
        for item in val[2:]:
          sys.stdout.write(' ' + item)

expand(sys.argv[1])
