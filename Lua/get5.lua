#!/usr/bin/env lua

local function format_headers(hdrs)
  local res = ""
  for name,value in pairs(hdrs) do
    res = res .. name .. ": " .. value .. "\r\n"
  end
  return res
end

local host   = arg[1] or "www.google.com"
local tls    = require "org.flummux.tls"
local config = tls.config()
local ctx    = tls.client()

config:protocols "all"
ctx:configure(config)

if not ctx:connect(host,"https") then
  print(ctx:error())
  os.exit(1)
end

ctx:handshake()

io.stderr:write("Version:    " , ctx:conn_version() or "","\n")
io.stderr:write("Subject:    " , ctx:peer_cert_subject(),"\n")
io.stderr:write("Issuer:     " , ctx:peer_cert_issuer(),"\n")
io.stderr:write("Cipher:     " , ctx:conn_cipher(),"\n")
io.stderr:write("ALPN:       " , ctx:conn_alpn_selected() or "","\n")
io.stderr:write("Server:     " , ctx:conn_servername(),"\n")
io.stderr:write("Not-Before: " , os.date("%c",ctx:peer_cert_notbefore()),"\n")
io.stderr:write("Not-After:  " , os.date("%c",ctx:peer_cert_notafter()),"\n")
io.stderr:write("Hash:       " , ctx:peer_cert_hash(),"\n")

ctx:write(
	"GET / HTTP/1.1\r\n" ..
	format_headers {
		['Host']       = host,
		['User-Agent'] = "TLSTester/1.0 (TLS Testing Program Lua)",
		['Connection'] = 'close',
		['Accept']     = "*/*",
	} ..
	"\r\n"
)

repeat
  local bytes = ctx:read(tls.BUFFERSIZE)
  io.stdout:write(bytes)
until bytes == ""

