#coding:utf-8
import os
import random

count = 0
for i in os.listdir("so/"):
    if i.split('.')[-1]!="so":
        continue;
    print i
    funcs = os.popen("nm -D so/"+i).read().split('\n')
    funcs = filter(lambda x:len(x)>2 and x[1]=="T",map(lambda x:x.split(' '),funcs[:]))
    funcs = map(lambda x:x[0],funcs[:])
    funcs = filter(lambda x:x!='',funcs)
    print funcs
    #random.shuffle(funcs)
    count += len(funcs)
    funcs = list(set(funcs))
    funcs = sorted(funcs)
    with open("txt/"+i,'w') as f:
        for index,j in enumerate(funcs[0:]):
            if index==len(funcs)-1:
                f.write(j)
                f.flush()
                break
            f.write(j+',')
            f.flush()
print count
