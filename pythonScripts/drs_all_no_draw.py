#change all sipm configs to draw mode

for i in xrange(16):
    out_lines = []
    with open("../configs/drs%i.json" % i, "r") as in_file:
        for line in in_file:
            if "draw" in line:
                out_lines.append(line.replace("true","false"))
            else:
                out_lines.append(line)
    with open("../configs/drs%i.json" % i, "w") as out_file:
        for line in out_lines:
            out_file.write(line)
