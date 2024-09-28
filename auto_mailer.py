import time
import json
from datetime import datetime, timedelta
from MailingSystem import sendEmailAttachment

class RoutineChecker:
    def __init__(self, teacher_db, class_routine):
        self.teacher_db = teacher_db
        self.class_routine = class_routine

    def check_and_send_reminders(self, simulated_now=None):
        # Get the current time or use the simulated time
        now = simulated_now if simulated_now else datetime.now()
        day_of_week = now.strftime('%a').lower()
        current_time = [now.hour, now.minute]

        if day_of_week in self.class_routine:
            # Check each class in today's routine
            for class_info in self.class_routine[day_of_week]["classes"]:
                start_time = class_info["time"]["start"]
                start_datetime = now.replace(hour=start_time[0], minute=start_time[1], second=0, microsecond=0)
                reminder_time = start_datetime - timedelta(minutes=10)

                if now >= reminder_time and now < start_datetime:
                    instructor = class_info["instructor"]
                    teacher_data = self.find_teacher_by_initial(instructor)
                    if teacher_data:
                        self.send_reminder_email(teacher_data, class_info)

    def find_teacher_by_initial(self, instructor_initial):
        # Search for the teacher's information using the instructor initial
        for teacher_id, teacher_info in self.teacher_db.items():
            if teacher_info.get("name", "").startswith(instructor_initial):
                return teacher_info
        return None

    def send_reminder_email(self, teacher_data, class_info):
        subject = f"Reminder: Upcoming Class in 10 Minutes"
        body = (f"Dear {teacher_data['name']},\n\n"
                f"This is a reminder for your upcoming class:\n"
                f"Course: {class_info['course'][0]} {class_info['course'][1]}\n"
                f"Room: {class_info['room']}\n"
                f"Time: {class_info['time']['start'][0]:02}:{class_info['time']['start'][1]:02} - "
                f"{class_info['time']['stop'][0]:02}:{class_info['time']['stop'][1]:02}.\n\n"
                f"Best Regards,\n"
                f"RUET Smart Attedance System (SAS)")
        
        # self.email_sender.send_email(subject, body)
        sendEmailAttachment(teacher_data['email'], None, body)


def main():
    # Load teacher database and class routine from files

    with open('database/records/teachers.json') as f:
        teacher_db = json.load(f)
    with open('database/routines/22-CSE-B.json') as f:
        class_routine = json.load(f)

    checker = RoutineChecker(teacher_db, class_routine)

    # Simulated time for testing (Example: "Sat 9:40 AM")
    simulated_time = datetime.now().replace(hour=9, minute=40)

    while True:
        # Pass the simulated time for testing
        checker.check_and_send_reminders(simulated_now=simulated_time)
        time.sleep(60)  # Check every minute
        simulated_time += timedelta(minutes=1)  # Simulate time progression

if __name__ == '__main__':
    main()

