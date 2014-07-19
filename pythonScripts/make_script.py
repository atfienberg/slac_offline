import sys

if len(sys.argv) < 3:
    print "Usage: python make_script.py [start run] [end run]"
    sys.exit(0)

try:
    int(sys.argv[1])
    int(sys.argv[2])
except:
    print "Arguments must be integers"
    sys.exit(0)

if sys.argv[2] < sys.argv[1]:
    print "end run must be greater than start run"
    sys.exit(0)

with open("../fit_script.sh", "w") as file:
    for i in xrange(int(sys.argv[1]), int(sys.argv[2])):
        file.write("./slacAnalyzer datafiles/run_%05i.root  runJsons/setup7-18.json crunchedFiles/run_%05i_crunched.root\n" % (i, i))

