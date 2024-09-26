from MailingSystem import sendEmailAttachment

class EmailSender:
    def send_email(self, subject, file_path, to_email):
        body = "Please find the attendance report attached."
        sendEmailAttachment(to_email, file_path, body)
