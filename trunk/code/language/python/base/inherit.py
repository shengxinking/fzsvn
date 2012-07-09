#!/usr/bin/python

# filename: inherit.py

class school_member:
	'''represents any school member.'''
	def __init__(self, name, age):
		self.name = name
		self.age = age
		print '(init) school_member name: %s' % self.name

	def detail(self):
		'''print the detail of school_member'''
		print 'name: %s, age %s' %(self.name, self.age)

class teacher(school_member):
	'''represents a teacher'''
	def __init__(self, name, age, salary):
		school_member.__init__(self, name, age)
		self.salary = salary
		print '(init) teacher %s' %self.name

	def detail(self):
		school_member.detail(self)
		print 'salary: %d' % self.salary

class student(school_member):
	'''represents a student.'''
	def __init__(self, name, age, marks):
		school_member.__init__(self, name, age)
		self.marks = marks
		print '(init) student: %s' %self.name

	def detail(self):
		school_member.detail(self)
		print 'marks: %d' %self.marks

t = teacher('Mr. Forrest', 30, 3000)
s = student('jerry', 22, 75)
print

members = [t, s]
for member in members:
	member.detail()
