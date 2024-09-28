import time
import json
import datetime
import traceback

from logs import logging
from codes import SASCode
from email_sender import EmailSender
from database import loadAllDatabases
from serial_handler import SerialHandler
from attendance_record import AttendanceRecord
from automated_classroom import ClassroomAutomatinator


class SmartAttendanceSystem:
    def __init__(self):
        
        self.email_sender = EmailSender()
        self.serial_handler = SerialHandler()
        self.attendance_record = AttendanceRecord()
        
        # all databases
        self.students_db = None
        self.teachers_db = None
        self.routine_db = None
        
        # load those databases
        self.load_databases()
        
        self.automatinator = ClassroomAutomatinator(self.routine_db, advance_time=30)

    def load_databases(self):
        try:
            (
                self.students_db,
                self.teachers_db,
                self.routine_db
            ) = loadAllDatabases().values()
        
        except FileNotFoundError as e:
            logging.error(f"Database file not found: {e}")
            raise
        
        except json.JSONDecodeError as e:
            logging.error(f"Error decoding JSON in database: {e}")
            raise

    def process_uid(self, uid_data):
        try:
            uid = uid_data['UID']
            room = uid_data['room']
            dept = uid_data['dept']
        
        except KeyError as e:
            logging.error(f"Missing key in UID data: {e}")
            return {'code': SASCode.ServerError, 'msg': 'Server Error!'}
        
        leaving_student = self.attendance_record.is_student_leaving(uid)
        if leaving_student:
            return self.handle_leaving_student(leaving_student)

        if uid in self.teachers_db:
            return self.handle_teacher(uid, room, dept)
        
        elif uid in self.students_db:
            return self.handle_student(uid, room, dept)
        
        else:
            return {'code': SASCode.InvalidID}

    def handle_teacher(self, uid, room, dept):
        return self.attendance_record.process_teacher(uid, self.teachers_db[uid], self.finalize_class)

    def handle_student(self, uid, room, dept):
        return self.attendance_record.process_student(uid, room, self.students_db[uid], self.routine_db)

    def handle_leaving_student(self, name):
        return self.attendance_record.process_leaving_student(self.routine_db, name)
    
    def get_day_of_week(self) -> str:
        day = datetime.datetime.now().strftime('%a').lower()
        return day
    
    def get_current_time(self):
        # ctime = datetime.datetime.now().time()
        ctime = datetime.time(10, 29, 00)
        return ctime
    
    def run(self):
        while True:
            try:
                
                led_data = self.automatinator.auto_check()
                
                if led_data:
                    for ldata in led_data:
                        self.serial_handler.send_serial_data(ldata)
                
                uid_data = self.serial_handler.read_serial_data()
                
                if type(uid_data) == type([]):
                    logging.debug(f"Received debug info: {uid_data}. Ignoring...")
                    continue
                
                if uid_data:
                    logging.info(f"{uid_data = }")
                    
                    response = self.process_uid(uid_data)
                    response['to'] = uid_data['room']
                    logging.info(f"{response = }")
                    
                    self.serial_handler.send_serial_data(response)

                    if response['code'] == SASCode.AttendanceAccepted and 'course' in response:
                        logging.info(f"Attendance recorded for {uid_data['UID']} in {response['course']}")
                    
                    elif response['code'] == SASCode.TeacherLeft:
                        logging.info(f"Teacher checked out: {response['name']}")
                        self.finalize_class()
                        logging.info('class finalized done')

                # Check if class is over based on time
                current_time = self.get_current_time()
                day_of_week = self.get_day_of_week()
                
                if (
                    day_of_week in self.routine_db and
                    "ager-raat-chadraat-chilo" not in self.routine_db[day_of_week] and
                    not self.attendance_record.is_class_finalized()
                ):
                    current_class = self.get_current_class(current_time, day_of_week)
                    
                    if current_class:
                        class_end = datetime.time(*current_class['time']['stop'])
                        if current_time >= class_end:
                            self.finalize_class()
                            logging.info("Class finalized based on time")
                    else:
                        # If there's no current class and teacher has checked in, finalize the class
                        if self.attendance_record.is_teacher_checked_in():
                            self.finalize_class()
                            logging.info("Class finalized as no more classes scheduled for today")

                # Reset for the next day
                if current_time == datetime.time(0, 0):  # Midnight
                    self.attendance_record.reset_for_new_day()
                    logging.info("Reset attendance record for new day")

            except Exception as e:
                logging.error(f"Unexpected error in main loop: {e}\n{traceback.format_exc()}")
                time.sleep(1)  # Prevent tight looping in case of persistent errors

    def get_current_class(self, current_time, day_of_week):
        if day_of_week not in self.routine_db:
            return None

        for class_info in self.routine_db[day_of_week]['classes']:
            class_start = datetime.time(*class_info['time']['start'])
            class_end = datetime.time(*class_info['time']['stop'])
            
            if class_start <= current_time <= class_end:
                return class_info

        return None
    
    def finalize_class(self):
        logging.debug(f'from "sas.py": {self.attendance_record.is_class_finalized() = }')
        if not self.attendance_record.is_class_finalized():
            csv_file = self.attendance_record.save_attendance_record()
            if csv_file and self.attendance_record.get_teacher_email():
                try:
                    self.email_sender.send_email("Attendance Report", csv_file, self.attendance_record.get_teacher_email())
                    logging.info(f"Attendance report (CSV) sent to {self.attendance_record.get_teacher_email()}")
                except Exception as e:
                    logging.error(f"Failed to send email: {e}")
            else:
                if not csv_file:
                    logging.error("Failed to save attendance record as CSV")
                if not self.attendance_record.get_teacher_email():
                    logging.error("Teacher email not available")
            
            self.attendance_record.finalize_class()
            logging.info("Class finalized")
