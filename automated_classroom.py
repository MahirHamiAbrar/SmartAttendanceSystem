import json
import time

from logs import logging
from datetime import datetime, timedelta


class ClassroomAutomatinator:
    ROOM_INFO_PATH = 'database/rooms/room-info.json'
    
    def __init__(self, schedule_db, advance_time=30):
        self.schedule_db = schedule_db
        self.room_info = self.load_json(self.ROOM_INFO_PATH)
        self.advance_time = timedelta(minutes=advance_time)
        
        self.turned_on = []

    @staticmethod
    def load_json(file_path):
        with open(file_path, 'r') as f:
            return json.load(f)

    def get_current_and_upcoming_classes(self, day, current_time):
        if (
            (day not in self.schedule_db) or
            ("ager-raat-chadraat-chilo" in self.schedule_db[day])
        ):
            return None, None
        
        current_class = None
        upcoming_class = None
        
        for class_info in self.schedule_db[day]['classes']:
            start_time = timedelta(hours=class_info['time']['start'][0], minutes=class_info['time']['start'][1])
            end_time = timedelta(hours=class_info['time']['stop'][0], minutes=class_info['time']['stop'][1])
            
            if start_time <= current_time <= end_time:
                current_class = class_info
            elif start_time - self.advance_time <= current_time < start_time:
                upcoming_class = class_info
                break  # We've found the next upcoming class
            elif current_time < start_time - self.advance_time:
                upcoming_class = class_info
                break  # We've found the next upcoming class
        
        return current_class, upcoming_class

    def get_room_contains(self, room_number):
        for room in self.room_info:
            if room['room'] == str(room_number):
                return room['contains']
        return []

    # @staticmethod
    def generate_led_data(self, contains, is_upcoming=False):
        data = []
        
        for item in contains:
            if item not in self.turned_on:
                self.turned_on.append(item)
                data.append({"led": item, "on": 1})
        
        # return [{"led": item, "on": 1} for item in contains]
        return data
    
        # # For upcoming classes, only turn on lights and fans
        # if is_upcoming:
        #     return [{"led": item, "on": 1} for item in contains]# if item in ['light', 'fan']]
        # else:
        #     return [{"led": item, "on": 1} for item in contains]
    
    def auto_check(self) -> None | dict:
        try:
            now = datetime.now()
            # current_day = now.strftime('%a').lower()
            current_day = 'sun'
            
            if not current_day:
                return None
            
            # current_time = timedelta(hours=now.hour, minutes=now.minute)
            current_time = timedelta(hours=10, minutes=30)

            current_class, upcoming_class = self.get_current_and_upcoming_classes(current_day, current_time)
            
            if (not current_class and not upcoming_class):
                logging.info("No current or upcoming classes at the moment.")
            
            room_number = upcoming_class['room']
            
            contains = self.get_room_contains(room_number)
            led_data = self.generate_led_data(contains, is_upcoming=(upcoming_class is not None))
            
            if upcoming_class:
                start_time = timedelta(hours=upcoming_class['time']['start'][0], minutes=upcoming_class['time']['start'][1])
                minutes_until_start = (start_time - current_time).total_seconds() / 60

                logging.info(f"Preparing room {room_number} for upcoming class in {minutes_until_start:.0f} minutes:")
            else:
                logging.info(f"Current class in room {room_number}:")
                
            return led_data
        
        except Exception as e:
            logging.error(f"Error in 'automated_classroom.py': {e}")

    def run(self):
        while True:
            data = self.auto_check()
            
            if data:
                logging.info(json.dumps(data, indent=4))
                
            time.sleep(2)      # 2s delay


if __name__ == "__main__":
    schedule_path = 'database/routines/22-CSE-B.json'
    automation = ClassroomAutomatinator(ClassroomAutomatinator.load_json(schedule_path))
    automation.run()
