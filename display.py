import time
import datetime
import math
import pygame
from Adafruit_LED_Backpack import SevenSegment

#initialize display

display = SevenSegment.SevenSegment(address=0x70)
display.begin()

#debug
print datetime.datetime.now()

sober = datetime.date(2016, 8, 18)
daycount = datetime.date.today() - sober

print daycount.days

#-----------display-------------------

#totalDisplay.clear()

unclosedDisplay.clear()

unclosedDisplay.set_digit(3, int(daycount) % 10) #ones
unclosedDisplay.set_digit(2, int(int(daycount)/10) % 10) #tens
unclosedDisplay.set_digit(1, int(int(daycount)/100) % 10) #hundreds
unclosedDisplay.set_digit(0, int(int(daycount)/1000) % 10) #thousands
unclosedDisplay.write_display()
