#change all sipm configs to draw mode

for i in xrange(15):
    if(i == 9):
        continue
    out_lines = []
    with open("../configs/sipm%i.json" % (i+1), "r") as in_file:
        for line in in_file:
            if "draw" in line:
                out_lines.append(line.replace("false","true"))
            else:
                out_lines.append(line)
    with open("../configs/sipm%i.json" % (i+1), "w") as out_file:
        for line in out_lines:
            out_file.write(line)
