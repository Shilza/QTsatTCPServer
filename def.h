#ifndef DEF_H
#define DEF_H

#define DATABASE_USER "shilza"
#define DATABASE_PASSWORD "192.168.39.26"
#define EMAIL_USER "service.qtsat@gmail.com"
#define EMAIL_PASSWORD "192.168.39.26a"

#define MAXIMUM_NUM_OF_BANS_TO_SHOW 1000

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

#define PORT 40000

#endif // DEF_H
