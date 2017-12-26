#ifndef DEF_H
#define DEF_H

#define DATABASE_USER "shilza"
#define DATABASE_PASSWORD "192.168.39.26"
#define EMAIL_USER "service.qtsat@gmail.com"
#define EMAIL_PASSWORD "192.168.39.26a"

#define EMAIL_REGISTRATION_SUBJECT "Registration in QTsat"
const QString EMAIL_REGISTRATION_BODY = "Welcome to QTsat!\n"
                                        "Your nickname: %1\n"
                                        "Your confirmation code: %2\n";

#define EMAIL_RECOVERY_SUBJECT "Password recovery"
const QString EMAIL_RECOVERY_BODY = "Recovery password in QTsat.\n"
                                        "Your nickname: %1\n"
                                        "Your confirmation code: %2\n";

#define EMAIL_PASSWORD_CHANGED_SUBJECT "Password has been changed"
const QString EMAIL_PASSWORD_CHANGED_BODY = "Dear %1, your password in QTsat has been changed!";

#define MAX_GLOBAL_MESSAGE_SIZE 140

#define HANDSHAKE 0
#define REGISTRATION 1
#define REGISTRATION_CODE 2
#define RECOVERY 3
#define RECOVERY_CODE 4
#define RECOVERY_NEW_PASS 5
#define DOES_EXIST_NICKNAME 6
#define DOES_EXIST_EMAIL 7

#define ERROR_AUTH 8
#define NICKNAME_EXIST 9
#define NICKNAME_NOT_EXIST 10
#define EMAIL_EXIST 11
#define EMAIL_NOT_EXIST 12
#define RECOVERY_FOUND 13
#define RECOVERY_NOT_FOUND 14
#define RIGHT_CODE 15
#define INVALID_CODE 16
#define SUCCESS_RECOVERY 17
#define REGISTRATION_SUCCESSFUL 18

#define ACTIVITY 19

#define OUT_PORT 49000
#define IN_PORT 49001
#define SYSTEM_OUT_PORT 49002
#define SYSTEM_IN_PORT 49003


#endif // DEF_H
