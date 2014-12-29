#!/usr/bin/python

# filename: objvar.py

class person:
	'''represents a person'''

	population = 0

	def __init__(self, name):
		self.name = name
		print '(init %s)' % self.name
		person.population += 1

	def __del__(self):
		'''i am dying'''
		print '%s says bye.' % self.name

		person.population -= 1

		if person.population == 0:
			print "I'm the last one"
		else:
			print 'there are still %d people left.' % person.population

	def sayHi(self):
		'''greeting by the person.population

		really, that's all it does.'''
		print 'Hi, my name is %s.' % self.name

	def howMany(self):
		'''prints the current population.'''
		if person.population == 1:
			print "I'm the only person here."
		else:
			print "we have %d persons here." % person.population

swaroop = person('swaroop')
swaroop.sayHi()
swaroop.howMany()
kalam = person('kalam')
kalam.sayHi()
kalam.howMany()

del kalam

swaroop.sayHi()
swaroop.howMany()

