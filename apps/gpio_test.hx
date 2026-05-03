print =====================================
print GPIO Test App
print =====================================
print
print Listing available GPIO pins:
print
gpio list
print
print Testing GPIO2 (available on ESP-01)
print
print Setting GPIO2 to OUTPUT mode...
gpio 2 mode output
print GPIO2 mode set
print
print Setting GPIO2 HIGH...
gpio 2 on
print
print Reading GPIO2 value...
gpio 2 read
print
print Setting GPIO2 LOW...
gpio 2 off
print
print Reading GPIO2 again...
gpio 2 read
print
print Toggling GPIO2 5 times...
gpio 2 pulse 5
print
print GPIO test completed!
