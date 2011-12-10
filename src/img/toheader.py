filehandle = open("re_24.png", 'r')
contents = filehandle.read()
filehandle.close()

row = 0
col = 0

output = ""

for byte in contents:
	if col == 0:
		output += "    "

	output += '0x%02X,' % ord(byte)
	
	col += 1
	
	if col >= 12:
		output += "\n"
		col = 0
	else:
		output += ""
		
print output.lower()
