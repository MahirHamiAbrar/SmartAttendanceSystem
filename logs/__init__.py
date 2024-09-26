import os
import logging


LOG_DIR = 'logs/log-files'
LAST_LOG_NO_FILE = 'logs/last-log-no'


def getNewLogFilePath() -> str:
    global LOG_DIR, LAST_LOG_NO_FILE
    
    log_no = -1
    
    with open(LAST_LOG_NO_FILE, 'r') as file:
        log_no = int(file.read().strip())
    
    log_no += 1
    
    with open(LAST_LOG_NO_FILE, 'w') as file:
        file.write(str(log_no))
    
    return os.path.join(LOG_DIR, f"log-{log_no}.log")


def setupLoggingSystem(debug_mode_enabled: bool = False) -> None:
    # Setup logging config
    
    if debug_mode_enabled:
        import sys
    
    logging.basicConfig(
        level = logging.DEBUG if debug_mode_enabled else logging.INFO,
        format = '%(asctime)s - %(levelname)s - %(message)s',
        handlers = [
            logging.FileHandler( getNewLogFilePath() ),
            logging.StreamHandler(sys.stdout if debug_mode_enabled else None)
        ]
    )
    
