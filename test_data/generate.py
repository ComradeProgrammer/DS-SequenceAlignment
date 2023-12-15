import random

l = 1000
res = ""
for i in range(0, l):
    tmp = random.randint(0, 3)
    if tmp == 0:
        res += "A"
    elif tmp == 1:
        res += "G"
    elif tmp == 2:
        res += "C"
    else:
        res += "T"
with open("out.txt","w") as f:
    f.write(res)