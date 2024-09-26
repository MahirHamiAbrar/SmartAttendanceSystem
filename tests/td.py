from datetime import datetime, timedelta

def check_attendance(ctime, xtime):
    # Convert the [hour, minute] format to datetime objects
    current_time = datetime.combine(datetime.today(), datetime.min.time()) + timedelta(hours=ctime[0], minutes=ctime[1])
    class_time = datetime.combine(datetime.today(), datetime.min.time()) + timedelta(hours=xtime[0], minutes=xtime[1])

    # Define the allowed time window: 20 minutes before and after class_time
    early_allowed_time = class_time - timedelta(minutes=20)
    late_allowed_time = class_time + timedelta(minutes=20)

    # Check if the current time is within the allowed time range
    if early_allowed_time <= current_time <= late_allowed_time:
        return "Student gets attendance."
    elif current_time < early_allowed_time:
        return "Student is too early. No attendance."
    else:
        return "Student is too late. No attendance."

# Example usage
ctime = [10, 10]  # Current time (when student arrives)
xtime = [10, 30]  # Class start time

result = check_attendance(ctime, xtime)
print(result)
