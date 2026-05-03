print ========================================
print Device Initialization Sequence
print ========================================
print
print [STAGE 1] System Check
print ----------------------------------------
print Checking system resources...
system info
print
system heap
print
print System check complete!
print
print [STAGE 2] Hardware Initialization
print ----------------------------------------
print Checking available GPIO pins...
gpio list
print
print Configuring GPIO2 as OUTPUT...
gpio 2 mode output
print GPIO2 configured: OUTPUT mode
print
print [STAGE 3] Hardware Self-Test
print ----------------------------------------
print Testing GPIO2 functionality...
print
print - Setting GPIO2 HIGH
gpio 2 on
delay 200
print - Reading GPIO2
gpio 2 read
print - Setting GPIO2 LOW
gpio 2 off
delay 200
print - Reading GPIO2
gpio 2 read
print
print Hardware self-test PASSED
print
print [STAGE 4] Network Configuration
print ----------------------------------------
print Checking WiFi status...
wifi status
print
print Current network info:
wifi ip
print
print [STAGE 5] Startup Complete
print ----------------------------------------
print
print Device fully initialized!
print Ready for operation
print
print Boot sequence finished successfully
print
