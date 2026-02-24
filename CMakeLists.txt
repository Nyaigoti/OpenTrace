cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(security_seal)

target_sources(app PRIVATE src/main.c)

# Add other source directories as we populate them
target_sources(app PRIVATE src/drivers/veml6035.c)
target_sources(app PRIVATE src/drivers/npm1300.c)
target_sources(app PRIVATE src/power/power_mgr.c)
target_sources(app PRIVATE src/app/payload.c)
target_sources(app PRIVATE src/app/storage.c)
target_sources(app PRIVATE src/app/fsm.c)
target_sources(app PRIVATE src/app/watchdog_mgr.c)

