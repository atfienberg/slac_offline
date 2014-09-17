import subprocess

for i in xrange(3):
    for j in xrange(3):
        if(i == j == 0):
            continue
        with open("../configs/sipm36.json", "r") as in_file:
             with open("../configs/sipm%i%i.json" % (i+3, j+6), "w") as out_file:
                 for line in in_file:
                     if "sipm36" in line:
                         out_file.write(line.replace("sipm36", "sipm%i%i" % (i+3,j+6)))
                     else:
                         out_file.write(line)



