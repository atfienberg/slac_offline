#cp all sipm configs to match sipm22 config

import subprocess

for i in xrange(15):
    if(i == 0):
        continue
    if(i == 9):
        continue
    with open("../configs/sipm1.json", "r") as in_file:
        with open("../configs/sipm%i.json" % (i+1), "w") as out_file:
            for line in in_file:
                if "sipm1" in line:
                    out_file.write(line.replace("sipm1", "sipm%i" % (i+1)))
                else:
                    out_file.write(line)



