import os
import base64

from email import encoders
from email.mime.text import MIMEText
from email.mime.base import MIMEBase
from email.mime.multipart import MIMEMultipart

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError


SCOPES = ['https://www.googleapis.com/auth/gmail.compose']

TOKEN_FILE = 'MailingSystem/credentials/token.json'
CREDENTIALS_FILE = 'MailingSystem/credentials/credentials.json'


def get_gmail_service():
    creds = None
    
    if os.path.exists(TOKEN_FILE):
        creds = Credentials.from_authorized_user_file(TOKEN_FILE, SCOPES)
    
    if not creds or not creds.valid:
        
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file(
                CREDENTIALS_FILE, SCOPES
            )
            creds = flow.run_local_server(port=0)
        
        with open(TOKEN_FILE, 'w') as token:
            token.write(creds.to_json())
    
    return build('gmail', 'v1', credentials=creds)


def create_message_with_attachment(sender, to, subject, message_text, file_path):
    message = MIMEMultipart()
    message['to'] = to
    message['from'] = sender
    message['subject'] = subject

    msg = MIMEText(message_text)
    message.attach(msg)

    filename = os.path.basename(file_path)
    with open(file_path, 'rb') as file:
        part = MIMEBase('application', 'octet-stream')
        part.set_payload(file.read())
    encoders.encode_base64(part)
    part.add_header('Content-Disposition', f'attachment; filename= {filename}')
    message.attach(part)

    raw_message = base64.urlsafe_b64encode(message.as_bytes()).decode('utf-8')
    return {'raw': raw_message}


def send_message(service, user_id, message):
    try:
        message = service.users().messages().send(userId=user_id, body=message).execute()
        print(f"Message Id: {message['id']}")
        return message
    except HttpError as error:
        print(f"An error occurred: {error}")


def send_email_with_attachment(to: str, file_path: str, body: str) -> None:
    service = get_gmail_service()
    sender = "iotattendancesystem.g3@gmail.com"
    
    if file_path:
        message = create_message_with_attachment(sender, to, "Attendance Report", body, file_path)
    else:
        message = MIMEMultipart()
        message['to'] = to
        message['from'] = sender
        message['subject'] = "Class Reminder"
        
        msg = MIMEText(body)
        message.attach(msg)
        
        message = {'raw': base64.urlsafe_b64encode(message.as_bytes()).decode('utf-8')}
        
    send_message(service, "me", message)


def _test_function():
    tos = ["mhabrarmts@gmail.com", "riduanislam425@gmail.com", 'rafiulovi11@gmail.com']
    tos = ['mhabrarmts@gmail.com']
    
    # subject = "Mara Kha"
    # message_text = "Tui Mara Kha"
    
    csv_file_path = [
        "/home/mhabrar/Documents/attendance.csv",
        "/home/mhabrar/Documents/2203077.pdf"
    ]
    
    for to in tos:
        send_email_with_attachment(to, csv_file_path[0], 'Attendance Report')


if __name__ == '__main__':
    _test_function()
    