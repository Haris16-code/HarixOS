print ===================================
print Multi-Pin LED Pattern Generator
print ===================================
print
print This app demonstrates complex
print GPIO sequencing with multiple pins
print
print Initializing GPIO pins...
gpio 2 mode output
gpio 4 mode output
print GPIO2 and GPIO4 configured
print
print Starting pattern sequences...
print
print Pattern 1: Sequential Pulse
print
gpio 2 on
delay 200
print - GPIO2 HIGH
gpio 2 off
delay 100
gpio 2 on
delay 200
print - GPIO2 HIGH again
gpio 2 off
delay 200
print
print Pattern 2: Alternating Pins
print
gpio 2 on
delay 150
print - GPIO2 HIGH
gpio 4 on
delay 150
print - GPIO4 HIGH (both on)
gpio 2 off
delay 150
print - GPIO2 OFF
gpio 4 off
delay 150
print - GPIO4 OFF
print
print Pattern 3: Rapid Blink Sequence
print
gpio 2 on
delay 100
gpio 2 off
delay 100
gpio 2 on
delay 100
gpio 2 off
delay 100
print - 4 rapid blinks complete
print
print All patterns executed successfully!
print
