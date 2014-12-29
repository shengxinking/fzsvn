#!/usr/bin/python

# filename: class_init.py

class person:
	def __init__(self, name):
		self.name = name
	def sayHi(self):
		print 'hello, my name is', self.name

p = person("forrest")

p.sayHi()