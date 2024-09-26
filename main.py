from sas import SmartAttendanceSystem
from logs import setupLoggingSystem

# setup logger
setupLoggingSystem(
    debug_mode_enabled = True
)

if __name__ == "__main__":
    sas = SmartAttendanceSystem()
    sas.run()