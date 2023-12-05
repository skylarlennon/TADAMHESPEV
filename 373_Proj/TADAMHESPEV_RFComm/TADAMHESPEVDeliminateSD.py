from datetime import datetime
import argparse
parser = argparse.ArgumentParser(description="TADAMHESPEV Data File Visualizer")
parser.add_argument("-f", "--filename", default = "TADLOG.txt")
args = parser.parse_args()


HEADER = "Acceleration,Temperature,Speed,Voltage,Current\n"

with open(args.filename, 'r') as fread:
    content = fread.read()

now = datetime.now()
session = 0

for datalines in content.split(HEADER):
    if not datalines:
        continue

    fn = f"TADAMHSPEVLOG-{session}-{str(now).replace(' ', '_').replace(':', '-')}.csv"
    with open(fn, 'w') as fwrite:
        fwrite.write(HEADER + datalines)
    print(fn)
    session+=1


