#change all sipm configs to draw mode

for i in xrange(4):
    for j in xrange(7):
        out_lines = []
        with open("../configs/sipm%i%i.json" % (i+2, j+2), "r") as in_file:
            for line in in_file:
                if "draw" in line:
                    out_lines.append(line.replace("true","false"))
                else:
                    out_lines.append(line)
        with open("../configs/sipm%i%i.json" % (i+2, j+2), "w") as out_file:
            for line in out_lines:
                out_file.write(line)
