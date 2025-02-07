#include <nodepp/nodepp.h>
#include <nodepp/ssl.h>
#include <mailer/mailer.h>

using namespace nodepp;

void onMain() {

    ssl_t ssl;//( "./ssl/cert.key", "./ssl/cert.crt" );
    mail_t mail ( "smtp://smtp.gmail.com:587", &ssl );

    mail_auth_t auth = {
        .user = "user@gmail.com",
        .pass = "password",
        .type = MAIL_AUTH_OAUTH
    };

    mail.send( auth, "subject@gmail.com", "subject", "Hello World!" );

}