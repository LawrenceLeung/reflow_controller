#! /usr/bin/python

steps = 30;

print "{",
for j in range(steps):
    print "0,",
print "}, // 0"

for i in range(steps):
    print "{",
    space = steps/(i+1.0)
    idx = space
    cnt = 0
    for j in range(steps):
        if(int(idx) == (j+1)):
            idx += space
            cnt += 1
            print "1,",
        else:
            print "0,",
    print "}, // %d" % cnt

