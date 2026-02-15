#include <nodepp/nodepp.h>
#include <mailer/mailer.h>

using namespace nodepp;

void onMain() {

    auto mail = mailer::add( "smtp://smtp.gmail.com:587" );

    mail_auth_t auth = {
        .user = "user@gmail.com",
        .pass = "password",
        .type = MAILER::MAIL_AUTH_OAUTH
    };

    mail.emit( auth, "subject@gmail.com", "subject", "Hello World!" )

    .then([=]( string_t msg ){
        console::log( "done" );
    })

    .fail([=]( except_t err ){
        console::error( "->", err.what() );
    });

}