import os
import csv
import json
import datetime

from logs import logging
from codes import SASCode
from typing import Callable


class AttendanceRecord:
    ATT_DIR = "logs/attendance_reports"
    
    def __init__(self):
        self.reset()

    def reset(self):
        self.json_data_records = {
            "teacher": {
                'name': None,
                'dept': None,
                'email': None
            },
            "students": []
        }
        self.teacher_checked_in = False
        self.class_finalized = False

    def is_student_leaving(self, uid):
        for student in self.json_data_records['students']:
            if student['id'] == uid:
                return student['name']
        
        return False

    def process_teacher(self, uid, teacher_info, on_teacher_leave_task: Callable | None = None):
        if not self.teacher_checked_in:
            self.json_data_records['teacher'] = {
                'name': teacher_info['name'],
                'dept': teacher_info['dept'],
                'email': teacher_info['email']
            }
        
            self.teacher_checked_in = True
            return {'code': SASCode.TeacherEntered, 'name': teacher_info['name']}
        
        else:
            # Teacher is checking out
            if not self.class_finalized:
                logging.debug('from "attendance_record.py": Finalizing the class')
                
                if callable(on_teacher_leave_task):
                    logging.debug(f'from "attendance_record.py": Calling {on_teacher_leave_task}')
                    on_teacher_leave_task()
                
                self.class_finalized = True
            
            logging.debug(f'from "attendance_record.py": {self.class_finalized = }')
            return {'code': SASCode.TeacherLeft, 'name': teacher_info['name']}

    def process_student(self, uid, room, student_info, routine_db):
        current_time = datetime.time(11, 10, 00)  # Simulated current time
        day_of_week = self.get_day_of_week()
        
        # 'Amar Meeting Ache' Ma'am

        # Check if it's a holiday or there are no classes scheduled for the day
        if (
            day_of_week not in routine_db or 
            "ager-raat-chadraat-chilo" in routine_db[day_of_week]
        ):
            return {'code': SASCode.Holiday}

        # Process each class in the routine for the given day
        for idx, class_info in enumerate(routine_db[day_of_week]['classes']):
            try:
                class_start = datetime.time(*class_info['time']['start'])
                class_end = datetime.time(*class_info['time']['stop'])

                # Convert times to timedelta for easy comparison
                c_timedelta = datetime.timedelta(hours=current_time.hour, minutes=current_time.minute)
                start_timedelta = datetime.timedelta(hours=class_start.hour, minutes=class_start.minute)

                # Calculate the time difference from class start time
                time_diff_start = c_timedelta - start_timedelta

                logging.debug(f"\n\t{class_start = }\n\t{class_end = }")
                logging.debug(f"\n\t{c_timedelta = }\n\t{start_timedelta = }\n\t{time_diff_start = }")

                # Check if the student arrived within the 20-minute window around class start time
                if datetime.timedelta(minutes=-20) <= time_diff_start <= datetime.timedelta(minutes=20):
                    if room != class_info['room']:
                        return {'code': SASCode.StudentWrongRoom, 'room': class_info['room'], 'name': student_info['name']}
                    
                    # Record student's attendance
                    self.json_data_records['students'].append({
                        'name': student_info['name'],
                        'id': uid
                    })
                    
                    return {'code': SASCode.AttendanceAccepted, 'course': f"{class_info['course'][0]}-{class_info['course'][1]}", 'name': student_info['name']}

                # Handle early and late arrivals for current class
                if time_diff_start < datetime.timedelta(minutes=-20):
                    return {'code': SASCode.TooEarly, 'name': student_info['name']}  # Too early
                
                if time_diff_start > datetime.timedelta(minutes=20):
                    logging.debug("Student is too late for the current class.")

                    # Check if there is a next class in the routine
                    if idx + 1 < len(routine_db[day_of_week]['classes']):
                        next_class_info = routine_db[day_of_week]['classes'][idx + 1]
                        next_class_start = datetime.time(*next_class_info['time']['start'])
                        next_class_start_timedelta = datetime.timedelta(hours=next_class_start.hour, minutes=next_class_start.minute)

                        # Calculate the time difference for the next class
                        next_time_diff_start = c_timedelta - next_class_start_timedelta

                        logging.debug(f"\nChecking next class:\n\t{next_class_start = }\n\t{next_time_diff_start = }")

                        # Check if the student is within the 20-minute window of the next class
                        if datetime.timedelta(minutes=-20) <= next_time_diff_start <= datetime.timedelta(minutes=20):
                            if room != next_class_info['room']:
                                return {'code': SASCode.StudentWrongRoom, 'room': next_class_info['room'], 'name': student_info['name']}
                            
                            # Record attendance for the next class
                            self.json_data_records['students'].append({
                                'name': student_info['name'],
                                'id': uid
                            })
                            
                            return {'code': SASCode.AttendanceAccepted, 'course': f"{next_class_info['course'][0]}-{next_class_info['course'][1]}", 'name': student_info['name']}
                        else:
                            logging.debug("Student is too late or too early for the next class as well.")
                            return {'code': SASCode.TooLate, 'name': student_info['name']}  # Too late for the next class as well

                    # If there is no next class
                    return {'code': SASCode.TooLate, 'name': student_info['name']}  # Too late for the current class and no next class available
            
            except KeyError as e:
                logging.error(f"Missing key in class info: {e}")
                continue

        return {'code': SASCode.NoClasses}  # No matching class found

    def process_leaving_student(self, routine_db, name):
        current_time = datetime.datetime.now().time()
        day_of_week = self.get_day_of_week()
        
        if (
            day_of_week not in routine_db or 
            "ager-raat-chadraat-chilo" in routine_db[day_of_week]
        ):
            return {'code': SASCode.Holiday}
        
        classes_done = 0

        if day_of_week in routine_db:
            
            for class_info in routine_db[day_of_week]['classes']:
                try:
                    class_end = datetime.time(*class_info['time']['stop'])
                    
                    if current_time > class_end:
                        classes_done += 1
                
                except KeyError as e:
                    logging.error(f"Missing key in class info: {e}")
                    continue

        return {'code': SASCode.StudentLeft, 'cls_done': classes_done, 'name': name}

    def save_attendance_record(self):
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        
        dir_path = os.path.join(self.ATT_DIR, f"{timestamp}")
        os.mkdir(dir_path)
        
        json_filename = os.path.join(dir_path, f"attendance_record_{timestamp}.json")
        csv_filename = os.path.join(dir_path, f"attendance_record_{timestamp}.csv")
        
        try:
            # Save JSON file
            with open(json_filename, 'w') as f:
                json.dump(self.json_data_records, f, indent=2)
            
            # Convert to CSV and save
            self.json_to_csv(json_filename, csv_filename)
            
            return csv_filename
        
        except IOError as e:
            logging.error(f"Failed to save attendance record: {e}")
            return None

    def json_to_csv(self, json_filename, csv_filename):
        try:
            with open(json_filename, 'r') as json_file:
                data = json.load(json_file)
            
            with open(csv_filename, 'w', newline='') as csv_file:
                writer = csv.writer(csv_file)
                
                # Write header
                writer.writerow(['Teacher Name', 'Teacher Department', 'Teacher Email'])
                writer.writerow([
                    data['teacher']['name'],
                    data['teacher']['dept'],
                    data['teacher']['email']
                ])
                
                # Empty row for separation
                writer.writerow([])
                
                # Student data header
                writer.writerow(['Student Name', 'Student ID'])
                
                # Write student data
                for student in data['students']:
                    writer.writerow([student['name'], student['id']])
            
            logging.info(f"Successfully converted {json_filename} to {csv_filename}")
        
        except Exception as e:
            logging.error(f"Error converting JSON to CSV: {e}")
            raise

    def get_teacher_email(self):
        return self.json_data_records['teacher']['email']

    def is_class_finalized(self):
        return self.class_finalized

    def reset_class_finalized(self):
        self.class_finalized = False
    
    def is_teacher_checked_in(self):
        return self.teacher_checked_in

    def finalize_class(self):
        self.class_finalized = True

    def reset_for_new_day(self):
        self.reset()
        self.class_finalized = False
    
    def get_day_of_week(self) -> str:
        # day = datetime.datetime.now().strftime('%a').lower()
        day = 'sun'
        return day