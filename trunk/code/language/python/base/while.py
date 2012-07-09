#!/usr/bin/python

number = 23
running = True

while running:
	guess = int(raw_input('enter an integer:'))

	if guess == 23:
		print 'Congratulations, you guessed it.'
		print '(But no prizes!)'
		running = False
	elif guess < number:
		print 'No, it is a little higher than that.'
	else:
		print 'No, it is a little lower than that'
else:
	print 'The while loop is over'

print 'done'
