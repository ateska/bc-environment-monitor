#!/usr/bin/env python3
import sys, serial, datetime, urllib.request

INFLUX_URL="http://influx.host/"
INFLUX_DB="environment"
SERIAL_PORT="/dev/cu.usbmodem1411"
LOCATION="mylocation"

def upload(line):
	url = "{}/write?db={}".format(INFLUX_URL, INFLUX_DB)
	if url is None or url == '':
		print("No influx URL is given")
		return;

	try:
		data = line.encode('utf-8')
		req = urllib.request.Request(url=url, data=data, method='POST')
		with urllib.request.urlopen(req) as f:
			pass

		print("Submited at {}".format(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")))

	except:
		print("Unexpected error when networking:", sys.exc_info()[0])


abvrmap = {
	't': 'temperature',
	'h': 'humidity',
	'l': 'luminosity',
	'a': 'altitude',
	'p': 'pressure',
}

def process(block):
	dt = datetime.datetime.utcnow()
	res = dict()
	for x in block:
		k,v = x.split(':')
		k = abvrmap.get(k, k)
		v = float(v)
		res[k] = v
	
	line = "environment,location={} {} {timestamp:.0f}\n".format(
		LOCATION,
		','.join(["{}={}".format(k,v) for k, v in res.items()]),
		timestamp=1e9 * dt.replace(tzinfo=datetime.timezone.utc).timestamp()
	)

	upload(line)


def main():
	with serial.Serial(SERIAL_PORT) as ser:
		latch = []
		while True:
			line = ser.readline()
			if line == b'---\r\n':
				latch = []
			elif line == b'===\r\n':
				process(latch)
				latch = []
			else:
				latch.append(line.strip().decode('ascii'))

if __name__ == '__main__':
	main()
