from enum import Enum


class SASCode:
    """Code for Smart Attendance System (SAS) that will be used
    by both the server and the ESP32 & ESP8266-NodeMCU systems.
    """
    
    TooEarly = 1
    TooLate = -1
    
    AttendanceAccepted = 0
    StudentLeft = 4
    
    StudentWrongRoom = 2
    TeacherWrongRoom = 3
    
    TeacherEntered = 5
    TeacherLeft = -5
    
    ServerError = 400
    InvalidID = 404
    
    NoClasses = 500
    Holiday = 501
