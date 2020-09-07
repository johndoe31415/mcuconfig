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

import re
import enum
import collections

class PinMapException(Exception): pass
class InvalidPinNameException(PinMapException): pass
class InvalidPinmapDefinitionException(PinMapException): pass

class PinMap():
	def __init__(self, definition_dict):
		self._plausibilize(definition_dict)
		self._defs = [ self._parse_pin_definition(pin_name, definition) for (pin_name, definition) in definition_dict.items() ]
		self._defs.sort(key = lambda definition: definition["pin"])

	def _parse_pin_definition(self, pin_name, pin_definition):
		raise NotImplementedError(self.__class__.__name__)

	def _plausibilize(self, defs):
		seen_names = set()
		for (pin_name, definition) in sorted(defs.items()):
			if "name" not in definition:
				raise InvalidPinmapDefinitionException("No 'name' attribute supplied for '%s'." % (pin_name))
			if "mode" not in definition:
				raise InvalidPinmapDefinitionException("No 'mode' attribute supplied for '%s'." % (pin_name))
			if ("initial" in definition) and (definition["initial"] not in [ "on", "off", "high", "low" ]):
				raise InvalidPinmapDefinitionException("Pin %s (\"%s\") has invalid value for 'initial': %s. Allowed are only on, off, high, low." % (pin_name, definition["name"], definition["initial"]))
			if definition["name"] in seen_names:
				raise InvalidPinmapDefinitionException("Duplicate name '%s' supplied for '%s'." % (definition["name"], pin_name))
			seen_names.add(definition["name"])

	def grouped(self, group_function):
		def _make_sortable(element):
			return tuple(item or 0 for item in element)

		groups = collections.defaultdict(list)
		for pin in self:
			key = group_function(pin)
			groups[key].append(pin)
		for (key, pins) in sorted(groups.items(), key = lambda x: (_make_sortable(x[0]), x[1])):
			yield (key, pins)

	def __iter__(self):
		return iter(self._defs)

	def dump(self):
		for pin in self:
			dictcopy = dict(pin)
			del dictcopy["pin"]
			print("%s: %s" % (pin["pin"], str(dictcopy)))

class PinMapSTMCortexM(PinMap):
	_PortPin = collections.namedtuple("PortPin", [ "port", "pin_no" ])
	_PORTPIN_NAME_RE = re.compile("P(?P<port>[A-Z])(?P<pin_no>\d{1,2})")
	_ALLOWED_DEFINITION_KEYWORDS = set([ "name", "mode", "invert", "af", "speed", "init", "initial" ])

	class PinMode(enum.Enum):
		Analog = "analog"
		InputFloat = "in"
		InputPullup = "in-up"
		InputPulldown = "in-down"
		OutputPushPull = "out"
		OutputOpenDrain = "out-od"
		AlternateFunction = "af"
		AlternateFunctionOpenDrain = "af-od"

		def __lt__(self, other):
			return self.name < other.name

		def stdperiph(self):
			return {
				self.Analog:						"AIN",
				self.InputFloat:					"IN_FLOATING",
				self.InputPullup:					"IPU",
				self.InputPulldown:					"IPD",
				self.OutputPushPull:				"Out_PP",
				self.OutputOpenDrain:				"Out_OD",
				self.AlternateFunction:				"AF_PP",
				self.AlternateFunctionOpenDrain:	"AF_OD",
			}[self]

		@property
		def settable(self):
			return self in (self.OutputPushPull, self.OutputOpenDrain)

	def _parse_pin_name(self, pin_name):
		match = self._PORTPIN_NAME_RE.fullmatch(pin_name)
		if not match:
			raise InvalidPinNameException("Pin name '%s' is invalid." % (pin_name))
		pin_no = int(match["pin_no"])
		if not (0 <= pin_no < 32):
			raise InvalidPinNameException("Pin name '%s' is invalid (pin number must be between 0 and 31)." % (pin_name))
		return self._PortPin(port = match["port"], pin_no = pin_no)

	def _parse_pin_definition(self, pin_name, pin_definition):
		pin = self._parse_pin_name(pin_name)
		invalid_keywords = set(pin_definition) - self._ALLOWED_DEFINITION_KEYWORDS
		if len(invalid_keywords):
			raise InvalidPinmapDefinitionException("Invalid keyword(s) %s specified for pin %s (\"%s\"). Allowed are only %s." % (", ".join(sorted(invalid_keywords)), pin_name, pin_definition["name"], ", ".join(sorted(self._ALLOWED_DEFINITION_KEYWORDS))))
		try:
			pin_definition["mode"] = self.PinMode(pin_definition["mode"])
		except ValueError:
			raise InvalidPinmapDefinitionException("Invalid pin mode specified for pin %s (\"%s\"): %s. Allowed are only %s." % (pin_name, pin_definition["name"], pin_definition["mode"], ", ".join(sorted(mode.value for mode in self.PinMode))))
		pin_definition["pin"] = pin
		return pin_definition

	def used_ports(self):
		return set(definition["pin"].port for definition in self)

	def functional_groups(self):
		return self.grouped(lambda definition: (definition["pin"].port, definition["mode"], definition.get("speed"), definition.get("init", True)))

if __name__ == "__main__":
	pinmap_defs = {
		"PA13": { "name": "foo", "mode": "out", "invert": True },
		"PA12": { "name": "bar", "mode": "out", "invert": True },
		"PA11": { "name": "moo", "mode": "out" },
		"PA10": { "name": "koo", "mode": "out", "init": False },
		"PA9": { "name": "baz", "mode": "analog" },
		"PA31": { "name": "meh", "mode": "analog" },
		"PC1": { "name": "init_low", "mode": "out", "initial": "low" },
		"PC2": { "name": "init_high", "mode": "out", "initial": "high" },
		"PC3": { "name": "init_on", "mode": "out", "initial": "on" },
		"PC4": { "name": "init_off", "mode": "out", "initial": "off" },
	}
	pinmap = PinMapSTMCortexM(pinmap_defs)
	pinmap.dump()
	print(pinmap.used_ports())
	print(list(pinmap.functional_groups()))
