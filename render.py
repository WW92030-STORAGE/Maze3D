# A program to render the output of Scene::buffer_data() to an image

from PIL import Image
# Important constants

INPUT = "maze3d.out" # Where to read the buffer from
OUTPUT = "RENDER.PNG"

"""

The buffer is in the following format:
[WIDTH, HEIGHT]
<color>,<color>,<color> ... <color>,
<color>,<color>...
...
...

line 1 is the dimensions, brackets and commas requried
The next (HEIGHT) lines is a comma separated list of (WIDTH) integers, each representing a color hex code. 
Each line must have a trailing comma.

"""

def render():
	file = open(INPUT, 'r')
	bufdat = file.read()
	
	ket = bufdat.find("]")
	dims = [int(i) for i in bufdat[1:ket].split(",")]

	W = dims[0]
	H = dims[1]
	
	im = Image.new(mode = "RGB", size = (W, H))

	restofdat = "".join(bufdat[ket+1:].splitlines())
	restofdat = [int(i) for i in restofdat.split(",")[:-1]]

	index = 0
	for y in range(H):
		for x in range(W):
			c = restofdat[index]
			r = (c>>24) % (1<<8)
			g = (c>>16) % (1<<8)
			b = (c>>8) % (1<<8)
			a = c % (1<<8)
			im.putpixel((x, y), (r, g, b, 1))
			index += 1

	im.save(OUTPUT)

	file.close()


if __name__ == "__main__":
	render()