# coding: utf-8
fm = ["select col1, col2, col3, col4"]
fm.append(fm[0] + " where col1=%d")
fm.append(fm[1] + ", col2=%d")
fm.append(fm[2] + ", col3=%d")
fm.append(fm[3] + ", col4=%d")
from itertools import product
b = [0, 1]
repeat(b, 4)
#repeat(b, 4)
from itertools import repeat
repeat(b, 4)
list(repeat(b, 4))
l = []
for i in range(5):
    args = list(product(*list(repeat(b, i))))
    print(args)
    
for i in range(5):
    args = list(product(*list(repeat(b, i))))
    for arg in args:
        l += fm[:i] % args
        
    
for i in range(5):
    args = list(product(*list(repeat(b, i))))
    for arg in args:
        l += fm[i] % args
        
        
    
for i in range(5):
    args = list(product(*list(repeat(b, i))))
    for arg in args:
        l += fm[i] % arg
        
        
        
    
l
for i in range(5):
    args = list(product(*list(repeat(b, i))))
    for arg in args:
        l += [fm[i] % arg]
        
l
l = []
for i in range(5):
    args = list(product(*list(repeat(b, i))))
    for arg in args:
        l += [fm[i] % arg]
        
l
#repeat(b, 4)
get_ipython().magic('h save')
h(save)
\help save
get_ipython().magic('pinfo save')
get_ipython().magic('save ~/ses.txt 1-28')
get_ipython().magic('save ses.txt')
get_ipython().magic('save ./ses.txt')
get_ipython().magic('save filename 1-32')
