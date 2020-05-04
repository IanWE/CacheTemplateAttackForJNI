import os

with open("libs1.txt") as f:
    files = f.read().split('\n')

for i in files:
  if i[-2:]=='so':
    print "Download ",i
    os.popen("adb pull "+i.split(' ')[-1])

