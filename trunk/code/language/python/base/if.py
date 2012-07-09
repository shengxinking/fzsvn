#!/usr/bin/python
#   filename: if.py

number = 23
guess = int(raw_input('enter an integer:'))

if guess == number:
        print 'Congratulations, you guessed it.'
        print "(but you not win any prizes!')"
elif guess < number:
        print 'No, it is a little higher than that'
else:
        print 'No, it is a little lower than that'

print 'Done'
