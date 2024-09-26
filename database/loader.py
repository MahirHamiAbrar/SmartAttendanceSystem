import json

# database paths
STUDENT_DB_PATH = 'database/records/students.json'
TEACHER_DB_PATH = 'database/records/teachers.json'
ROUTINE_DB_PATH = 'database/routines/22-CSE-B.json'


def loaddb(fp: str) -> dict:
    with open(fp, 'r') as file:
        return json.load(file)


def load_all_databases():
    return {
        'student': loaddb(STUDENT_DB_PATH),
        'teacher': loaddb(TEACHER_DB_PATH),
        'routine': loaddb(ROUTINE_DB_PATH)
    }
