/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_MAIL
#define NODEPP_MAIL

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/expected.h>
#include <nodepp/encoder.h>
#include <nodepp/promise.h>
#include <nodepp/tcp.h>
#include <nodepp/tls.h>
#include <nodepp/url.h>

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp {
    
    struct MAILER { enum TYPE {
        MAIL_AUTH_LOGIN, MAIL_AUTH_PLAIN, MAIL_AUTH_OAUTH
    };};

    struct mail_auth_t {
        string_t user; string_t pass;
        int type = MAILER::MAIL_AUTH_PLAIN;
    }; 

}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class mail_t : public generator_t {
protected:

    enum STATE {
         MAILER_STATE_UNKNOWN = 0b00000000,
         MAILER_STATE_OPEN    = 0b00000001,
         MAILER_STATE_TLS     = 0b00000010,
         MAILER_STATE_EXTEND  = 0b00000100,
         MAILER_STATE_CLOSE   = 0b10000000
    };

    struct mail_header_t {
        string_t message;
        int      status;
    };

    struct NODE {
        ssl_t ssl, ctx; string_t hostname;
        generator::file::write _write;
        generator::file::read  _read ;
        socket_t    fd; int state=0; 
    };  ptr_t<NODE> obj;
    
    /*─······································································─*/

    mail_header_t read_header() const noexcept {
        mail_header_t result; auto data = read();
        console::log( data.size(), data );
        result.status = string::to_int( data.slice(0,3) );
        result.message= data.slice_view(4); return result;
    }
    
    /*─······································································─*/

    int push( string_t message ) const noexcept { return write( message + "\n" ); }

    void init() const {
        auto header = read_header(); 
        
        if( header.status==0 ){
            throw except_t("something went wrong");
        } elif( header.status >= 400 ) { 
            throw except_t("Can't connect to the server");
        }
    }
    
    /*─······································································─*/

    void handshake() const { mail_header_t header;

        push("EHLO nodepp-mail"); header = read_header(); 
        
        if( header.status <  400 )
          { obj->state|=STATE::MAILER_STATE_EXTEND; return; } 
        if( header.status >= 500 ){ 
            throw except_t("Can't connect to the server");
        }

        push("HELO nodepp-mail"); header = read_header(); 
        
        if( header.status <  400 ){ return; } 
        if( header.status >= 500 ){ 
            throw except_t("Can't connect to the server");
        }

    }
    
    /*─······································································─*/

    void tls() const {
        if( obj->ctx.get_ctx() == nullptr ){ return; }
        push("STARTTLS"); auto header = read_header();

        if( header.status >= 500 ){
            throw except_t("SSL connection not allowed");
        } elif( header.status >= 400 ) { return; }

        obj->ssl = ssl_t( obj->ctx, obj->fd.get_fd() ); 
        obj->ssl.set_hostname( obj->hostname );
        
        obj->state |= STATE::MAILER_STATE_TLS; handshake();
    }
    
    /*─······································································─*/

    void auth_login( mail_auth_t& auth ) const {
        push( "AUTH LOGIN" ); 
        push( encoder::base64::get(auth.user) );
        push( encoder::base64::get(auth.pass) );
        auto header = read_header(); if( header.status >= 400 ){
             throw except_t("auth pass not accepted");
        }
    }

    void auth_plain( mail_auth_t& auth ) const {
        string_t pass = encoder::base64::get( regex::format(
            "${0}${2}${1}${2}", auth.user, auth.pass,
             type::cast<char>( 0x00 )
        ));  push( "AUTH PLAIN "+ pass );
        auto header = read_header(); if( header.status >= 400 ){
             throw except_t("auth pass not accepted");
        }
    }

    void auth_oauth( mail_auth_t& auth ) const {
        string_t pass = encoder::base64::get( regex::format(
            "user=${0}${2}auth=Bearer ${1}${2}${2}",
             auth.user, auth.pass, type::cast<char>(0x01)
        ));  push( string::format("AUTH XOAUTH2 %s", pass.get() ) );
        auto header = read_header(); if( header.status >= 400 ){
             throw except_t("auth pass not accepted");
        }
    }
    
    /*─······································································─*/

    void mail_from( string_t email ) const {
        push( string::format("MAIL FROM: <%s>", email.get() ) );
        auto header = read_header(); if( header.status >= 400 ){
             throw except_t("auth pass not accepted");
        }
    }

    void mail_to( string_t email ) const {
        push( string::format("RCPT TO: <%s>", email.get() ) );
        auto header = read_header(); if( header.status >= 400 ){
             throw except_t("auth pass not accepted");
        }
    }

    void send_msg( string_t message ) const { push( "DATA" );
        auto header = read_header(); if( header.status >= 400 ){
             throw except_t("auth pass not accepted");
        }    write( message ); push("<CR><LF>.<CR><LF>");
    }
    
    /*─······································································─*/

    string_t read() const noexcept {
        if( is_closed() ){ return nullptr; }
        
        if( obj->state & STATE::MAILER_STATE_TLS ){
            socket_t fd = obj->fd; /*--------------*/
            while( obj->_read( &fd )==1 ){ /*unused*/ }
        } else {
            ssocket_t fd = obj->fd; fd.ssl = obj->ssl;
            while( obj->_read( &fd )==1 ){ /*unused*/ }
        }
        
        return obj->_read.data;
    }
    
    /*─······································································─*/

    int write( string_t message ) const noexcept {
        if( is_closed() || message.empty() ){ return 0; } 
        
        if( obj->state & STATE::MAILER_STATE_TLS ){
            socket_t fd = obj->fd; /*------------------------*/
            while( obj->_write( &fd, message )==1 ){ /*unused*/ }
        } else {
            ssocket_t fd = obj->fd; fd.ssl = obj->ssl; /*----*/
            while( obj->_write( &fd, message )==1 ){ /*unused*/ }
        }
        
    return obj->_write.state; }
    
    /*─······································································─*/

    int send( mail_auth_t auth, string_t email, string_t subject, string_t msg ) {
    coBegin ; init(); handshake(); 
        
        if( obj->state & STATE::MAILER_STATE_EXTEND ){ tls(); } 
        
        switch ( auth.type ) {
            case MAILER::MAIL_AUTH_LOGIN: auth_login( auth ); break;
            case MAILER::MAIL_AUTH_PLAIN: auth_plain( auth ); break;
            case MAILER::MAIL_AUTH_OAUTH: auth_oauth( auth ); break;
            default:    throw except_t("AUTH NOT SUPPORTED"); break; 
        }   
        
        coSet(1); goto NEXT; coYield(1); NEXT:;
        mail_from( auth.user ); mail_to( email ); send_msg( msg );
    
    coGoto(1); coFinish }

public:
    
    /*─······································································─*/

    mail_t () noexcept : obj( new NODE ){ obj->state = STATE::MAILER_STATE_CLOSE; }

   ~mail_t () noexcept { if( obj.count()>1 ){ return; } free(); }

    mail_t ( socket_t sk, ssl_t* ssl=nullptr ) : obj( new NODE ){

        if( !sk.is_closed() ){ throw except_t( "Invalid socket" ); }  
        obj->ssl = ssl==nullptr? ssl_t(): *ssl;
        obj->state |= STATE::MAILER_STATE_OPEN;

    }
    
    /*─······································································─*/

    ptr_t<string_t,except_t> emit ( mail_auth_t auth, string_t email, string_t subject, string_t msg ){
           auto self = type::bind( this ); 
    return ptr_t<string_t,except_t> ([=]( res_t<string_t> res, rej_t<except_t> rej ){
        try  { self->send();/**/res( msg ); } 
        catch( except_t err ) { rej( err ); }
    });
    
    /*─······································································─*/

    bool is_closed() const noexcept { 
         return obj->fd.is_closed() || (obj->state & STATE::MAILER_STATE_CLOSE); 
    }

    void close() const noexcept {
        if( !obj->fd.is_available() ){ return; }
        if( is_closed() ) /*------*/ { return; }
            push( "QUIT" ); free();
    }

    void free() const noexcept {
        if( is_closed() ){ return; }
        obj->state = STATE::MAILER_STATE_CLOSE; 
        obj->fd.close();
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace mailer {

    promise_t<mail_t,except_t> connect( const string_t& uri ) {
    return promise_t<mail_t,except_t> ([=]( function_t<void,mail_t> res, function_t<void,except_t> rej ){
        if( !url::is_valid( uri ) ){ rej( except_t("Invalid SMTP URL") ); return; }

        auto host = url::hostname( uri );
        auto port = url::port( uri );

        auto client= tcp_t ([=]( socket_t cli ){
        auto mail  = mail_t(); res(mail); return; });

        client.onError([=]( except_t error ){ rej(error); });
        client.connect( host, port );

    }); }

    /*─······································································─*/

    template<class...T> expected_t<mail_t,except_t> 
    add( const T&... args ) { return connect( args... ).await(); }

} }

/*────────────────────────────────────────────────────────────────────────────*/

#endif

/*────────────────────────────────────────────────────────────────────────────*/