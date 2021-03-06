# zswalc

Zscheile Web Application Light Chat is an highly insecure,
unreliable, experimental ~~fastcgi~~ HTTP-based chatting app,
which utilizes javascript to update the chat view.

## License

The old code was released under GPL-3+, the new Rust source code
is released under MIT+Apache-2.0.

## expected command line arguments
```
USAGE:
    zswalc [OPTIONS] --database <database> --serv-addr <serv–addr>

FLAGS:
    -h, --help       Prints help information
    -V, --version    Prints version information

OPTIONS:
    -b, --database <database>      sets the file where to store the chat data
    -a, --serv-addr <serv–addr>    sets the server address to bind/listen to
    -r, --vroot <vroot>            sets the HTTP base path of this app (defaults to '')
```

## TODO

* return new messages on `POST` request
* improve frontend
* more sophisticated XSS prevention
* WebSockets support

## lighttpd example
```
###############################################################################
# zswebapp.conf
# include'd by lighttpd.conf.
###############################################################################

$HTTP["url"] =~ "^/zschat($|/)" {
  proxy.server = ( "" => (( "host" => "127.0.0.1", "port" = "9003" )) )
  proxy.forwarded = ( "for" => 1, "proto" => 1, "remote_user" => 1 )
  proxy.header = (
    "upgrade" => "enable"
  )
}

# vim: set ft=conf foldmethod=marker et :
```
