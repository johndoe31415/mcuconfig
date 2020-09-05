#	mcuconfig - Generation of microcontroller build setups.
#	Copyright (C) 2019-2020 Johannes Bauer
#
#	This file is part of mcuconfig.
#
#	mcuconfig is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; this program is ONLY licensed under
#	version 3 of the License, later versions are explicitly excluded.
#
#	mcuconfig is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with mcuconfig; if not, write to the Free Software
#	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#	Johannes Bauer <JohannesBauer@gmx.de>

class VectorTable():
	def __init__(self, filename):
		self._step = 4
		(self._min_offset, self._max_offset, self._vectors) = self._parse(filename)

	def _parse(self, filename):
		vectors = { }
		offset = 0
		min_offset = None
		max_offset = None
		with open(filename) as f:
			for line in f:
				line = line.rstrip("\r\n")
				if (line == "") or line.startswith("#"):
					continue
				elif line.startswith("@"):
					offset = int(line[1 : ], 16)
				else:
					vector_name = line.strip()
					if vector_name != "-":
						vectors[offset] = vector_name
					if max_offset is None:
						max_offset = offset
					else:
						max_offset = max(offset, max_offset)
					if min_offset is None:
						min_offset = offset
					else:
						min_offset = min(offset, min_offset)
					offset += self._step
		return (min_offset, max_offset, vectors)

	def __iter__(self):
		for offset in range(self._min_offset, self._max_offset + self._step, self._step):
			yield (offset, self._vectors.get(offset))

if __name__ == "__main__":
	vectors = VectorTable("system/stm32f103c8/vectors.txt")
	for vector in vectors:
		print(vector)
