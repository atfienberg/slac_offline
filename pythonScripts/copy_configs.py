#cp all sipm configs to match sipm22 config

import subprocess

for i in xrange(4):
    for j in xrange(7):
        if(i == j == 0):
            continue
        with open("../configs/sipm22.json", "r") as in_file:
             with open("../configs/sipm%i%i.json" % (i+2, j+2), "w") as out_file:
                 for line in in_file:
                     if "sipm52" in line:
                         out_file.write(line.replace("sipm52", "sipm%i%i" % (i+2,j+2)))
                     else:
                         out_file.write(line)



