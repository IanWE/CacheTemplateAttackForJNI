import os

with open("libs.txt") as f:
    files = f.read().split('\n')
if not os.path.exists('so/'):
  os.makedirs('so')
os.chdir('so')
for i in files:
  i = i.split('  ')[-1]
  if i[-2:]=='so':
    print "Download ",i
    os.popen("adb pull "+i.split(' ')[-1])
    


