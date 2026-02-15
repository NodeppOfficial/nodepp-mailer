# NODEPP-MAILER
Run a simple **SMTP Client** in Nodepp

## Dependencies & Cmake Integration
```bash
include(FetchContent)

FetchContent_Declare(
	nodepp
	GIT_REPOSITORY   https://github.com/NodeppOfficial/nodepp
	GIT_TAG          origin/main
	GIT_PROGRESS     ON
)
FetchContent_MakeAvailable(nodepp)

FetchContent_Declare(
	nodepp-mailer
	GIT_REPOSITORY   https://github.com/NodeppOfficial/nodepp-mailer
	GIT_TAG          origin/main
	GIT_PROGRESS     ON
)
FetchContent_MakeAvailable(nodepp-mailer)

#[...]

target_link_libraries( #[...]
	PUBLIC nodepp nodepp-mailer #[...]
)
```

## Example
```cpp
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
```

## Compilation
`g++ -o main main.cpp -I ./include -lssl -lcrypto ; ./main`

## License
**Nodepp-Mailer** is distributed under the MIT License. See the LICENSE file for more details.